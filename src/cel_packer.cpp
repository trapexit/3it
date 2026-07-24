/*
  ISC License

  Copyright (c) 2025, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

// Docs:
// https://3dodev.com/documentation/development/opera/pf25/ppgfldr/ggsfldr/gpgfldr/5gpgd
// https://3dodev.com/documentation/development/opera/pf25/ppgfldr/ggsfldr/gpgfldr/3gpga

/*
  This code is intentionally straightforward rather than micro-optimized,
  but the hot path still avoids avoidable work:

  0. Initialize the abstract packed image metadata.
  1. Convert each bitmap row to packed-source pixels and run dynamic
  programming over that row to find the minimum-cost legal encoding.
  2. Encode the abstract packets as 3DO packed row bitstreams.
  3. Pad each row bitstream to at least 2 words and then to a 32-bit
  boundary.
  4. Serialize the row bitstreams into a byte vector for writing to
  disk.
*/

#include "cel_packer.hpp"

#include "bitmap.hpp"
#include "bits_and_bytes.hpp"
#include "bitstream.hpp"
#include "bpp.hpp"
#include "byte_reader.hpp"
#include "bytevec.hpp"
#include "ccb_flags.hpp"
#include "packed.hpp"
#include "pixel_converter.hpp"

#include "fmt.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

typedef std::vector<BitStream> BitStreamVec;

static constexpr u32 MAX_PACKET_PIXELS = (1u << DATA_PACKET_PIXEL_COUNT_SIZE);

struct PackedDataPacket
{
  uint8_t type;
  uint8_t bpp = 0;
  u32 count = 0;
  u32 pixel = 0;
  std::vector<u32> pixels;

  bool is_literal() const { return type == PACK_LITERAL; };
  bool is_packed() const { return type == PACK_PACKED; };
  bool is_transparent() const { return type == PACK_TRANSPARENT; };
  bool is_eol() const { return type == PACK_EOL; };

  u32 pixel_count() const;
  u32 size_in_bits() const;
};

struct PackedDataPacketVec : public std::vector<PackedDataPacket>
{
  u32 pixel_count() const;
  u32 size_in_bits() const;
};

struct AbstractPackedImage : public std::vector<PackedDataPacketVec>
{
  u32 bpp;
  u32 line_width;
  u32 offset_width;
  u32 size_in_bits() const;
};

struct RowChoice
{
  size_t begin;
  size_t end;
  u32 type;
};


u32
PackedDataPacket::pixel_count() const
{
  if(is_literal())
    return pixels.size();

  return count;
}

u32
PackedDataPacketVec::pixel_count() const
{
  u32 c;

  c = 0;
  for(const auto &pdp : *this)
    {
      c += pdp.pixel_count();
    }

  return c;
}

u32
PackedDataPacketVec::size_in_bits() const
{
  u32 c;

  c = 0;
  for(const auto &pdp : *this)
    {
      c += pdp.size_in_bits();
    }

  return c;
}

u32
PackedDataPacket::size_in_bits() const
{
  const u32 packet_pixel_count = pixel_count();

  switch(type)
    {
    case PACK_LITERAL:
      return (DATA_PACKET_DATA_TYPE_SIZE +
              DATA_PACKET_PIXEL_COUNT_SIZE +
              (packet_pixel_count * bpp));
    case PACK_TRANSPARENT:
      return (DATA_PACKET_DATA_TYPE_SIZE +
              DATA_PACKET_PIXEL_COUNT_SIZE);
    case PACK_PACKED:
      return (DATA_PACKET_DATA_TYPE_SIZE +
              DATA_PACKET_PIXEL_COUNT_SIZE +
              bpp);
    case PACK_EOL:
      return (DATA_PACKET_DATA_TYPE_SIZE);
    }

  return 0;
}

u32
AbstractPackedImage::size_in_bits() const
{
  u32 c;

  c = 0;
  for(const auto &pdpvec : *this)
    {
      c += pdpvec.size_in_bits();
    }

  return c;
}

static
std::size_t
calc_offset_width(const std::size_t bpp_)
{
  switch(bpp_)
    {
    case BPP_1:
    case BPP_2:
    case BPP_4:
    case BPP_6:
      return 8;
    case BPP_8:
    case BPP_16:
      return 16;
    }

  throw std::runtime_error("invalid bpp");
}

#define ALPHA 0xFFFFFFFF

static
void
pass0_init_api(const Bitmap            &b_,
               const RGBA8888Converter &pc_,
               AbstractPackedImage     &api_)
{
  api_.bpp = pc_.bpp();
  api_.line_width = b_.w;
  api_.offset_width = ::calc_offset_width(pc_.bpp());
  api_.resize(b_.h);
}

static
std::vector<u32>
build_row_pixels(const Bitmap            &b_,
                 const RGBA8888Converter &pc_,
                 const size_t             row_)
{
  std::vector<u32> pixels;

  pixels.reserve(b_.w);
  for(size_t x = 0; x < b_.w; x++)
    {
      RGBA8888 p;

      // If alpha is 0 then zero out the color to make packing easier later.
      p = *b_.xy(x,row_);
      if(p.a == 0)
        pixels.emplace_back(ALPHA);
      else
        pixels.emplace_back(pc_.convert(&p));
    }

  return pixels;
}

static
PackedDataPacket
build_packet_from_range(const std::vector<u32> &pixels_,
                        const u32               bpp_,
                        const size_t            begin_,
                        const size_t            end_,
                        const u32               type_,
                        const bool              zero_transparency_)
{
  PackedDataPacket pdp;
  const size_t len = (end_ - begin_);

  pdp.bpp = bpp_;
  pdp.count = len;
  pdp.type = type_;
  if(len == 0)
    return pdp;

  if(type_ == PACK_TRANSPARENT)
    return pdp;

  if(type_ == PACK_PACKED)
    {
      if(pixels_[begin_] == ALPHA)
        throw std::runtime_error("cel_packer: transparent packed pixel");

      pdp.pixel = pixels_[begin_];
      return pdp;
    }

  if(type_ != PACK_LITERAL)
    throw std::runtime_error("cel_packer: invalid packet type");

  pdp.pixels.reserve(len);
  for(size_t i = begin_; i < end_; ++i)
    {
      if(pixels_[i] != ALPHA)
        pdp.pixels.emplace_back(pixels_[i]);
      else if(zero_transparency_)
        pdp.pixels.emplace_back(0);
      else
        throw std::runtime_error("cel_packer: ALPHA sentinel found in literal range");
    }

  return pdp;
}

static
u32
packet_size_in_bits(const u32 type_,
                    const u32 count_,
                    const u32 bpp_)
{
  switch(type_)
    {
    case PACK_LITERAL:
      return (DATA_PACKET_DATA_TYPE_SIZE +
              DATA_PACKET_PIXEL_COUNT_SIZE +
              (count_ * bpp_));
    case PACK_TRANSPARENT:
      return (DATA_PACKET_DATA_TYPE_SIZE +
              DATA_PACKET_PIXEL_COUNT_SIZE);
    case PACK_PACKED:
      return (DATA_PACKET_DATA_TYPE_SIZE +
              DATA_PACKET_PIXEL_COUNT_SIZE +
              bpp_);
    case PACK_EOL:
      return DATA_PACKET_DATA_TYPE_SIZE;
    }

  return 0;
}

static
bool
pass1_choice_is_better(const RowChoice &candidate_,
                       const RowChoice &current_,
                       const u32        bpp_)
{
  const auto candidate_count = candidate_.end - candidate_.begin;
  const auto current_count = current_.end - current_.begin;
  const auto candidate_bits = packet_size_in_bits(candidate_.type,candidate_count,bpp_);
  const auto current_bits = packet_size_in_bits(current_.type,current_count,bpp_);

  if(candidate_bits != current_bits)
    return candidate_bits < current_bits;
  if(candidate_count != current_count)
    return candidate_count > current_count;
  return candidate_.type > current_.type;
}

static
PackedDataPacketVec
pass1_optimize_row_encoding(const std::vector<u32> &pixels_,
                            const u32               bpp_,
                            const bool              zero_transparency_)
{
  const size_t n = pixels_.size();
  std::vector<u32> best_cost(n + 1,std::numeric_limits<u32>::max());
  std::vector<RowChoice> best_choice(n + 1);
  std::vector<bool> has_choice(n + 1,false);
  std::vector<bool> transparent_suffix(n + 1,false);

  transparent_suffix[n] = true;
  for(size_t pos = n; pos-- > 0;)
    transparent_suffix[pos] = (pixels_[pos] == ALPHA) && transparent_suffix[pos + 1];

  best_cost[n] = 0;
  has_choice[n] = true;
  for(size_t pos = n; pos-- > 0;)
    {
      auto consider_choice = [&](const size_t end_,
                                 const u32    type_)
      {
        const u32 packet_cost = packet_size_in_bits(type_,
                                                    end_ - pos,
                                                    bpp_);
        const u32 total_cost = packet_cost + best_cost[end_];
        const RowChoice choice = {pos,end_,type_};

        if(!has_choice[pos] ||
           (total_cost < best_cost[pos]) ||
           ((total_cost == best_cost[pos]) &&
            pass1_choice_is_better(choice,best_choice[pos],bpp_)))
          {
            best_cost[pos] = total_cost;
            best_choice[pos] = choice;
            has_choice[pos] = true;
          }
      };

      if(transparent_suffix[pos])
        consider_choice(n,PACK_EOL);

      if(zero_transparency_)
        {
          for(size_t end = pos + 1;
              (end <= n) && ((end - pos) <= MAX_PACKET_PIXELS);
              ++end)
            {
              consider_choice(end,PACK_LITERAL);
            }
        }

      if(pixels_[pos] == ALPHA)
        {
          size_t end = pos;

          while((end < n) &&
                ((end - pos) < MAX_PACKET_PIXELS) &&
                (pixels_[end] == ALPHA))
            {
              ++end;
              if(transparent_suffix[end])
                continue;

              consider_choice(end,PACK_TRANSPARENT);
            }
        }
      else
        {
          size_t packed_end = pos + 1;

          while((packed_end < n) &&
                ((packed_end - pos) < MAX_PACKET_PIXELS) &&
                (pixels_[packed_end] == pixels_[pos]) &&
                (pixels_[packed_end] != ALPHA))
            ++packed_end;

          if(!zero_transparency_)
            {
              for(size_t end = pos + 1;
                  (end <= n) && ((end - pos) <= MAX_PACKET_PIXELS) && (pixels_[end - 1] != ALPHA);
                  ++end)
                {
                  consider_choice(end,PACK_LITERAL);
                }
            }

          for(size_t end = pos + 2; end <= packed_end; ++end)
            {
              consider_choice(end,PACK_PACKED);
            }
        }
    }

  if(!has_choice[0])
    throw std::runtime_error("cel_packer: row candidate enumeration failed");

  PackedDataPacketVec out;
  for(size_t pos = 0; pos < n;)
    {
      const RowChoice &choice = best_choice[pos];
      PackedDataPacket packet;

      if(choice.type == PACK_EOL)
        {
          packet.type = PACK_EOL;
          packet.bpp = bpp_;
          out.emplace_back(std::move(packet));
          break;
        }

      packet = build_packet_from_range(pixels_,
                                       bpp_,
                                       choice.begin,
                                       choice.end,
                                       choice.type,
                                       zero_transparency_);
      out.emplace_back(std::move(packet));
      pos = choice.end;
    }

  return out;
}

static
void
pass1_optimize_rows(const Bitmap            &b_,
                    const RGBA8888Converter &pc_,
                    AbstractPackedImage     &api_,
                    const bool               zero_transparency_)
{
  for(size_t row = 0; row < api_.size(); row++)
    {
      const auto pixels = build_row_pixels(b_,pc_,row);
      api_[row] = pass1_optimize_row_encoding(pixels,
                                              api_.bpp,
                                              zero_transparency_);
    }
}

static
void
pass2_api_to_bitstreams(const AbstractPackedImage &api_,
                        BitStreamVec              &rows_)
{
  rows_.clear();
  rows_.resize(api_.size());
  for(size_t i = 0; i < api_.size(); i++)
    {
      const auto &pdpvec = api_[i];
      auto       &row    = rows_[i];

      // Reserve space for the row offset header.
      row.write(api_.offset_width,0);
      for(const auto &pdp : pdpvec)
        {
          const u32 count = pdp.pixel_count();

          row.write(DATA_PACKET_DATA_TYPE_SIZE,pdp.type);
          switch(pdp.type)
            {
            case PACK_PACKED:
              row.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                        count-1);
              row.write(api_.bpp,
                        pdp.pixel);
              break;
            case PACK_LITERAL:
              row.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                        count-1);
              for(const auto pixel : pdp.pixels)
                row.write(api_.bpp,pixel);
              break;
            case PACK_TRANSPARENT:
              row.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                        count-1);
              break;
            case PACK_EOL:
              break;
            }
        }

      if((pdpvec.pixel_count() < api_.line_width) &&
         (pdpvec.empty() || pdpvec.back().type != PACK_EOL))
        row.write(DATA_PACKET_DATA_TYPE_SIZE,PACK_EOL);

      // Finalize the row offset header before padding.
      {
        u64 word_offset;

        word_offset = std::max((u64)2,
                               row.tell_32bits_round_up());
        row.write(0,
                  api_.offset_width,
                  (word_offset - 2));
      }
    }
}

static
void
pass3_pad_rows(BitStreamVec &rows_)
{
  for(auto &row : rows_)
    {
      if(row.tell_bits() < (2 * BITS_PER_WORD))
        row.zero_till_64bit_boundary();
      else
        row.zero_till_32bit_boundary();
    }
}

static
void
pass4_bitstreams_to_bytevec(const BitStreamVec &rows_,
                            ByteVec            &pdat_)
{
  pdat_.clear();
  for(const auto &row : rows_)
    {
      if(row.tell_bits() < (2 * BITS_PER_WORD))
        throw std::runtime_error("cel_packer: row is shorter than 2 words after padding");
      if(row.tell_bits() & 31)
        throw std::runtime_error("cel_packer: row is not word-aligned after padding");
      pdat_.insert(pdat_.end(),
                   row.begin(),
                   row.idx_end());
    }
}

static
void
pack_with_mode(const Bitmap            &b_,
               const RGBA8888Converter &pc_,
               const bool               zero_transparency_,
               ByteVec                 &pdat_)
{
  AbstractPackedImage api;
  BitStreamVec rows;

  pass0_init_api(b_,pc_,api);
  pass1_optimize_rows(b_,pc_,api,zero_transparency_);
  pass2_api_to_bitstreams(api,rows);
  pass3_pad_rows(rows);
  pass4_bitstreams_to_bytevec(rows,pdat_);
}

static
bool
can_use_zero_transparency(const Bitmap            &b_,
                          const RGBA8888Converter &pc_)
{
  bool has_transparency = false;

  for(size_t y = 0; y < b_.h; ++y)
    {
      for(size_t x = 0; x < b_.w; ++x)
        {
          const RGBA8888 *pixel = b_.xy(x,y);

          if(pixel->a == 0)
            has_transparency = true;
          else if(pc_.convert(pixel) == 0)
            return false;
        }
    }

  return has_transparency;
}

bool
CelPacker::pack(const Bitmap            &b_,
                const RGBA8888Converter &pc_,
                ByteVec                 &pdat_,
                const bool               allow_zero_transparency_)
{
  ByteVec regular_pdat;

  ::pack_with_mode(b_,pc_,false,regular_pdat);
  if(allow_zero_transparency_ && ::can_use_zero_transparency(b_,pc_))
    {
      ByteVec zero_pdat;

      ::pack_with_mode(b_,pc_,true,zero_pdat);
      if(zero_pdat.size() < regular_pdat.size())
        {
          pdat_ = std::move(zero_pdat);
          return true;
        }
    }

  pdat_ = std::move(regular_pdat);
  return false;
};
