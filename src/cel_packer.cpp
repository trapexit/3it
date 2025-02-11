/*
  ISC License

  Copyright (c) 2022, Antonio SJ Musumeci <trapexit@spawn.link>

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
  This code is not optimal wrt speed or space but it really doesn't
  need to be and makes the code easier to understand. It is broken
  down into multiple, order dependant passes.

  0. Convert the raw bitmap into an abstract form. Absolutely worse
  'packed' form with every pixel being an individual literal
  packet. Sets alpha == 0 pixels to be RGBA == 0000 to make further
  logic easier.
  1. Walk over rows and turn contiguous lists of pixels of the same
  color into packed packets.
  2. Go over all packets and change those with alpha set to 0 as
  transparent packets.
  3. Find instances of literal packets followed by packed, packed
  followed by literal, or literal after literal and check if it would
  take less space if combined and do so. This will be run till the
  size stops reducing.
  4. Split abstract packets with a counter greater than 64 into
  multiple packets of size 64 or less.
  5. Remove trailing transparent packets and replace with EOL.
  6. Remove unnecessary EOLs
*/

#include "cel_packer.hpp"

#include "bitmap.hpp"
#include "bits_and_bytes.hpp"
#include "bitstream.hpp"
#include "bpp.hpp"
#include "byte_reader.hpp"
#include "bytevec.hpp"
#include "ccb_flags.hpp"
#include "fmt/format-inl.h"
#include "packed.hpp"
#include "pixel_converter.hpp"

#include "fmt.hpp"

#include <bits/floatn-common.h>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

typedef std::vector<BitStream> BitStreamVec;

struct PackedDataPacket
{
  uint8_t type;
  uint8_t bpp;
  std::vector<u32> pixels;

  bool is_literal() const { return type == PACK_LITERAL; };
  bool is_packed() const { return type == PACK_PACKED; };
  bool is_transparent() const { return type == PACK_TRANSPARENT; };
  bool is_eol() const { return type == PACK_EOL; };

  u32 size_in_bits() const;
  u32 raw_literal_size() const;
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


u32
PackedDataPacketVec::pixel_count() const
{
  u32 c;

  c = 0;
  for(const auto &pdp : *this)
    c += pdp.pixels.size();

  return c;
}

u32
PackedDataPacketVec::size_in_bits() const
{
  u32 c;

  c = 0;
  for(const auto &pdp : *this)
    c += pdp.size_in_bits();

  return c;
}

u32
PackedDataPacket::size_in_bits() const
{
  switch(type)
    {
    case PACK_LITERAL:
      return (DATA_PACKET_DATA_TYPE_SIZE +
              DATA_PACKET_PIXEL_COUNT_SIZE +
              (pixels.size() * bpp));
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
PackedDataPacket::raw_literal_size() const
{
  switch(type)
    {
    case PACK_LITERAL:
    case PACK_PACKED:
      return (pixels.size() * bpp);
    case PACK_TRANSPARENT:
    case PACK_EOL:
      return 0;
    }

  return 0;
}

u32
AbstractPackedImage::size_in_bits() const
{
  u32 c;

  c = 0;
  for(const auto &pdpvec : *this)
    c += pdpvec.size_in_bits();

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
pass0_build_api_from_bitmap(const Bitmap            &b_,
                            const RGBA8888Converter &pc_,
                            AbstractPackedImage     &api_)
{
  api_.bpp = pc_.bpp();
  api_.line_width = b_.w;
  api_.offset_width = ::calc_offset_width(pc_.bpp());
  api_.resize(b_.h);
  for(size_t y = 0; y < b_.h; y++)
    {
      PackedDataPacketVec &pdpvec = api_[y];

      for(size_t x = 0; x < b_.w; x++)
        {
          RGBA8888 p;
          PackedDataPacket pdp;
          u32 c;

          // If alpha is 0 then zero out the color to make packing
          // easier later
          p = *b_.xy(x,y);

          if(p.a == 0)
            c = ALPHA;
          else
            c = pc_.convert(&p);

          pdp.type = PACK_LITERAL;
          pdp.bpp  = pc_.bpp();
          pdp.pixels.emplace_back(c);

          pdpvec.emplace_back(pdp);
        }
    }
}

static
void
pass1_pack_packed(AbstractPackedImage &api_)
{
  for(auto &pdpvec : api_)
    {
      PackedDataPacketVec newpdpvec;

      newpdpvec.emplace_back(pdpvec[0]);

      for(size_t i = 1; i < pdpvec.size(); i++)
        {
          PackedDataPacket &curpdp = pdpvec[i];
          PackedDataPacket &newpdp = newpdpvec.back();

          if(curpdp.pixels[0] == newpdp.pixels[0])
            {
              newpdp.type = PACK_PACKED;
              newpdp.pixels.emplace_back(curpdp.pixels[0]);
            }
          else
            {
              newpdpvec.emplace_back(pdpvec[i]);
            }
        }

      pdpvec = newpdpvec;
    }
}

static
void
pass2_mark_transparents(AbstractPackedImage &api_)
{
  for(auto &pdpvec : api_)
    {
      for(auto &pdp : pdpvec)
        {
          if(pdp.pixels[0] != ALPHA)
            continue;

          pdp.type = PACK_TRANSPARENT;
        }
    }
}

static
void
pass3_compress_literal_and_packed(AbstractPackedImage &api_)
{
  for(auto &pdpvec : api_)
    {
      PackedDataPacketVec newpdpvec;

      newpdpvec.emplace_back(pdpvec.front());

      for(size_t i = 1; i < pdpvec.size(); i++)
        {
          auto &pdp    = pdpvec[i];
          auto &newpdp = newpdpvec.back();

          if(newpdp.is_literal() && pdp.is_literal())
            {
              for(auto &p : pdp.pixels)
                newpdp.pixels.emplace_back(p);
              continue;
            }

          if(newpdp.is_literal() && pdp.is_packed())
            {
              if(pdp.size_in_bits() >= pdp.raw_literal_size())
                {
                  for(auto &p : pdp.pixels)
                    newpdp.pixels.emplace_back(p);
                  continue;
                }
            }

          if(newpdp.is_packed() && pdp.is_literal())
            {
              if(newpdp.size_in_bits() >= newpdp.raw_literal_size())
                {
                  newpdp.type = PACK_LITERAL;
                  for(auto &p : pdp.pixels)
                    newpdp.pixels.emplace_back(p);
                  continue;
                }
            }

          newpdpvec.emplace_back(pdp);
        }

      pdpvec = newpdpvec;
    }
}

static
void
pass3_combine(AbstractPackedImage &api_)
{
  size_t prev_size;
  size_t comp_size;

  do
    {
      prev_size = api_.size_in_bits();
      pass3_compress_literal_and_packed(api_);
      comp_size = api_.size_in_bits();
    }
  while(comp_size != prev_size);
}

static
void
pass4_split_large_packets(AbstractPackedImage &api_)
{
  for(auto &pdpvec : api_)
    {
      PackedDataPacketVec newpdpvec;

      for(auto &pdp : pdpvec)
        {
          PackedDataPacket newpdp;

          newpdp.type = pdp.type;
          auto i  = pdp.pixels.begin();
          auto ei = pdp.pixels.end();
          while(i != ei)
            {
              newpdp.pixels.clear();
              for(size_t c = 0; i != ei && c < 64; ++c, ++i)
                newpdp.pixels.emplace_back(*i);
              newpdpvec.emplace_back(newpdp);
            }
        }

      pdpvec = newpdpvec;
    }
}

static
void
pass5_remove_trailing_transparents(AbstractPackedImage &api_)
{
  for(auto &pdpvec : api_)
    {
      size_t orig_size;

      orig_size = pdpvec.size();
      while(!pdpvec.empty() && pdpvec.rbegin()->is_transparent())
        pdpvec.pop_back();

      if(orig_size != pdpvec.size())
        {
          pdpvec.emplace_back();
          pdpvec.rbegin()->type = PACK_EOL;
        }
    }
}

static
void
pass6_remove_trailing_eol(AbstractPackedImage &api_)
{
  for(auto &pdpvec : api_)
    {
      if(pdpvec.pixel_count() < api_.line_width)
        continue;

      while(!pdpvec.empty() && pdpvec.rbegin()->is_eol())
        pdpvec.pop_back();
    }
}

static
void
pass7_api_to_bitstreams(const AbstractPackedImage &api_,
                        BitStreamVec              &rows_)
{
  rows_.clear();
  rows_.resize(api_.size());
  for(size_t i = 0; i < api_.size(); i++)
    {
      const auto &pdpvec = api_[i];
      auto       &row    = rows_[i];

      // Reserve space for the offset
      row.write(api_.offset_width,0);
      for(const auto &pdp : pdpvec)
        {
          row.write(DATA_PACKET_DATA_TYPE_SIZE,pdp.type);
          switch(pdp.type)
            {
            case PACK_PACKED:
              row.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                        pdp.pixels.size()-1);
              row.write(api_.bpp,
                        pdp.pixels[0]);
              break;
            case PACK_LITERAL:
              row.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                        pdp.pixels.size()-1);
              for(const auto pixel : pdp.pixels)
                row.write(api_.bpp,pixel);
              break;
            case PACK_TRANSPARENT:
              row.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                        pdp.pixels.size()-1);
              break;
            case PACK_EOL:
              break;
            }
        }

      // Needs to be done in prep for overlap pass
      {
        int word_offset;

        word_offset = std::max((uint64_t)2,
                               row.tell_32bits_round_up());
        row.write(0,
                  api_.offset_width,
                  (word_offset - 2));
      }
    }
}

// The overlap in theory could be from word 3 to the end of the
// row but in practice this is extremely unlikely. For the moment just
// checking the trailing bits % 32bit word.
static
void
pass8_trim_overlap(const AbstractPackedImage &api_,
                   BitStreamVec              &rows_)
{
  for(size_t i = 0; i < (rows_.size() - 1); i++)
    {
      if(rows_[i].tell_32bits_round_up() <= 2)
        continue;
      
      bool overlap;
      int trailing_bits;
      BitStream &a = rows_[i+0];
      BitStream &b = rows_[i+1];

      trailing_bits = (a.tell_bits() & 31);
      if(trailing_bits == 0)
        trailing_bits = 32;

      overlap = a.cmp(a.tell_bits() - trailing_bits,
                      b,0,
                      trailing_bits);
      if(!overlap)
        continue;

      a.rewind(trailing_bits);
      a.shrink_to_idx();
      a.write(0,
              api_.offset_width,
              (a.tell_32bits_round_up() - 2));
    }
}

static
void
pass9_pad_rows(BitStreamVec &rows_)
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
pass10_bsvec_to_bytevec(const BitStreamVec &rows_,
                        ByteVec            &pdat_)
{
  pdat_.clear();
  for(const auto &row : rows_)
    {
      assert(row.tell_bits() >= (2 * BITS_PER_WORD));
      assert((row.tell_bits() & 31) == 0);
      pdat_.insert(pdat_.end(),
                   row.begin(),
                   row.idx_end());
    }
}

void
CelPacker::pack(const Bitmap            &b_,
                const RGBA8888Converter &pc_,
                ByteVec                 &pdat_)
{
  AbstractPackedImage api;
  BitStreamVec rows;

  pass0_build_api_from_bitmap(b_,pc_,api);
  pass1_pack_packed(api);
  pass2_mark_transparents(api);
  pass3_combine(api);
  pass4_split_large_packets(api);
  pass5_remove_trailing_transparents(api);
  pass6_remove_trailing_eol(api);
  pass7_api_to_bitstreams(api,rows);
  pass8_trim_overlap(api,rows);
  pass9_pad_rows(rows);
  pass10_bsvec_to_bytevec(rows,pdat_);
};
