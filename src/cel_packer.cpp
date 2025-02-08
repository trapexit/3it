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


struct PackedDataPacket
{
  uint8_t type;
  uint8_t bpp;
  std::vector<uint32_t> pixels;

  bool is_literal() const { return type == PACK_LITERAL; };
  bool is_packed() const { return type == PACK_PACKED; };
  bool is_transparent() const { return type == PACK_TRANSPARENT; };
  bool is_eol() const { return type == PACK_EOL; };

  uint32_t size_in_bits() const;
  uint32_t raw_literal_size() const;
};

struct PackedDataPacketVec : public std::vector<PackedDataPacket>
{
  uint32_t pixel_count() const;
  uint32_t size_in_bits() const;
};

struct AbstractPackedImage : public std::vector<PackedDataPacketVec>
{
  uint32_t line_width;
  uint32_t size_in_bits() const;
};


uint32_t
PackedDataPacketVec::pixel_count() const
{
  uint32_t c;

  c = 0;
  for(const auto &pdp : *this)
    c += pdp.pixels.size();

  return c;
}

uint32_t
PackedDataPacketVec::size_in_bits() const
{
  uint32_t c;

  c = 0;
  for(const auto &pdp : *this)
    c += pdp.size_in_bits();

  return c;
}

uint32_t
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

uint32_t
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

uint32_t
AbstractPackedImage::size_in_bits() const
{
  uint32_t c;

  c = 0;
  for(const auto &pdpvec : *this)
    c += pdpvec.size_in_bits();

  return c;
}

#define ALPHA 0xFFFFFFFF

static
void
pass0_build_api_from_bitmap(const Bitmap        &b_,
                            const RGBA8888Converter   &pc_,
                            AbstractPackedImage &api_)
{
  api_.line_width = b_.w;
  api_.resize(b_.h);
  for(size_t y = 0; y < b_.h; y++)
    {
      PackedDataPacketVec &pdpvec = api_[y];

      for(size_t x = 0; x < b_.w; x++)
        {
          RGBA8888 p;
          PackedDataPacket pdp;
          uint32_t c;
          
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

static void
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

static
void
api_to_bytevec(const Bitmap              &b_,
               const AbstractPackedImage &api_,
               const RGBA8888Converter   &pc_,
               ByteVec                   &pdat_)
{

  BitStreamWriter bs;
  u64 offset_width;
  u64 next_row_offset;

  offset_width = ::calc_offset_width(pc_.bpp());

  // BitStreamWriter will resize as needed
  pdat_.resize(b_.w * b_.h * BYTES_PER_WORD);
  bs.reset(pdat_);

  for(const auto &pdpvec : api_)
    {
      next_row_offset = bs.tell();
      bs.write(offset_width,0);
      for(auto i = pdpvec.begin(), ei = pdpvec.end(); i != ei; ++i)
        {
          const auto &pdp = *i;

          bs.write(DATA_PACKET_DATA_TYPE_SIZE,pdp.type);
          switch(pdp.type)
            {
            case PACK_PACKED:
              bs.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                       pdp.pixels.size()-1);
              bs.write(pc_.bpp(),
                       pdp.pixels[0]);
              fmt::print("packed: {} {}\n",
                         pdp.pixels.size(),
                         pdp.pixels[0]);
              break;
            case PACK_LITERAL:
              bs.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                       pdp.pixels.size()-1);
              fmt::print("literal: {} ",pdp.pixels.size());
              for(const auto pixel : pdp.pixels)
                {
                  bs.write(pc_.bpp(),pixel);
                  fmt::print("{} ",pixel);
                }
              fmt::print("\n");
              break;
            case PACK_TRANSPARENT:
              bs.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                       pdp.pixels.size()-1);
              fmt::print("transparent: {}\n",
                         pdp.pixels.size());
              
              break;
            case PACK_EOL:
              fmt::print("eol:\n");
              break;
            }
        }

      // Like unpacked CELs the pipelining of the CEL engine requires
      // minus 2 words for the length / offset meaning a minimum of 2
      // words in the CEL data.
      {
        s64 next_row_in_words;

        bs.zero_till_32bit_boundary();
        if((bs.tell() - next_row_offset) < (2 * BITS_PER_WORD))
          bs.write(((2 * BITS_PER_WORD) - (bs.tell() - next_row_offset)),0);

        next_row_in_words = (((bs.tell() - next_row_offset) / BITS_PER_WORD) - 2);

        if(next_row_in_words > 0)
          {
            if(bs.read(bs.tell() - 32,32) == 0)
              fmt::print("ending in 0\n");
          }

        bs.write(next_row_offset,
                 offset_width,
                 next_row_in_words);
      }
    }

  pdat_.resize(bs.tell_bytes());
}

static
void
api_to_bytevec2(const Bitmap              &b_,
                const AbstractPackedImage &api_,
                const RGBA8888Converter   &pc_,
                ByteVec                   &pdat_)
{
  ByteVec row_pdat;  
  std::vector<ByteVec> pdat_vec;
  std::vector<bool> has_eol;
  std::vector<u8> leading_zeros;
  std::vector<u8> trailing_zeros;
  BitStreamWriter bs;
  u64 offset_width;

  offset_width = ::calc_offset_width(pc_.bpp());

  for(const auto &pdpvec : api_)
    {
      bool eol = false;
        
      row_pdat.clear();
      bs.reset(row_pdat);

      // Reserve space for the offset
      bs.write(offset_width,0);
      for(const auto &pdp : pdpvec)
        {
          bs.write(DATA_PACKET_DATA_TYPE_SIZE,pdp.type);
          switch(pdp.type)
            {
            case PACK_PACKED:
              bs.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                       pdp.pixels.size()-1);
              bs.write(pc_.bpp(),
                       pdp.pixels[0]);
              // fmt::print("packed: {} {}\n",
              //            pdp.pixels.size(),
              //            pdp.pixels[0]);
              break;
            case PACK_LITERAL:
              bs.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                       pdp.pixels.size()-1);
              // fmt::print("literal: {} ",pdp.pixels.size());
              for(const auto pixel : pdp.pixels)
                {
                  bs.write(pc_.bpp(),pixel);
                  // fmt::print("{} ",pixel);
                }
              // fmt::print("\n");
              break;
            case PACK_TRANSPARENT:
              bs.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                       pdp.pixels.size()-1);
              // fmt::print("transparent: {}\n",
              //            pdp.pixels.size());
              
              break;
            case PACK_EOL:
              eol = true;
              // fmt::print("eol:\n");
              break;
            }
        }

      // Like unpacked CELs the pipelining of the CEL engine requires
      // minus 2 words for the length / offset meaning a minimum of 2
      // words in the CEL data.
      int excess_bits;

      excess_bits = bs.tell_bits() & (BITS_PER_WORD-1);
      if(bs.read(bs.tell_bits() - excess_bits,excess_bits) != 0)
        excess_bits = 0;
      
      bs.zero_till_32bit_boundary();
      if(bs.tell_u32() < 2)
        bs.write(BITS_PER_WORD,0);

      has_eol.push_back(eol);
      trailing_zeros.push_back(excess_bits);
      {
        int offset;
        int first_word;

        offset = ((row_pdat.size() / BYTES_PER_WORD) - 2);

        bs.write(0,
                 offset_width,
                 offset);
        first_word = bs.read(0,BITS_PER_WORD);
        fmt::print("row_pdat size={}; eol={}; beginning_0_bits={}; excess_0_bits={}\n",
                   row_pdat.size(),
                   eol,
                   ((offset == 0) ?
                    offset_width :
                    __builtin_clz(first_word)),
                   excess_bits);
        leading_zeros.push_back(((offset == 0) ?
                                 offset_width :
                                 __builtin_clz(first_word)));
      }

      pdat_vec.emplace_back(row_pdat);
    }

  for(size_t i = 0; i < pdat_vec.size(); i++)
    {
      u32 first_word;
      if(i + 1 == pdat_vec.size())
        continue;
      if(trailing_zeros[i] == 0)
        continue;

      fmt::print("{} {}\n",trailing_zeros[i],leading_zeros[i+1]);
      if(trailing_zeros[i+0] <= leading_zeros[i+1])
        {
          fmt::print("row {} can save a word\n",i);
          pdat_vec[i].resize(pdat_vec[i].size() - BYTES_PER_WORD);
          bs.reset(&pdat_vec[i]);
          int offset = bs.read(0,offset_width);
          bs.write(0,offset_width,offset-1);
        }
    }

  for(const auto &pdat : pdat_vec)
    pdat_.insert(pdat_.end(),
                 pdat.begin(),
                 pdat.end());
  
  //  pdat_.resize(bs.tell_bytes());
}

static
void
api_to_bytevec3(const Bitmap              &b_,
                const AbstractPackedImage &api_,
                const RGBA8888Converter   &pc_,
                ByteVec                   &pdat_)
{
  std::vector<BitStream> rows_pdat;
  std::vector<bool> has_eol;
  std::vector<u8> leading_zeros;
  std::vector<u8> trailing_zeros;
  u64 offset_width;

  offset_width = ::calc_offset_width(pc_.bpp());

  rows_pdat.resize(api_.size());
  for(size_t i = 0; i < api_.size(); i++)
    {
      const auto &pdpvec   = api_[i];
      auto       &row_pdat = rows_pdat[i];

      bool eol = false;
        
      // Reserve space for the offset
      row_pdat.write(offset_width,0);
      for(const auto &pdp : pdpvec)
        {
          row_pdat.write(DATA_PACKET_DATA_TYPE_SIZE,pdp.type);
          switch(pdp.type)
            {
            case PACK_PACKED:
              row_pdat.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                             pdp.pixels.size()-1);
              row_pdat.write(pc_.bpp(),
                             pdp.pixels[0]);
              // fmt::print("packed: {} {}\n",
              //            pdp.pixels.size(),
              //            pdp.pixels[0]);
              break;
            case PACK_LITERAL:
              row_pdat.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                             pdp.pixels.size()-1);
              // fmt::print("literal: {} ",pdp.pixels.size());
              for(const auto pixel : pdp.pixels)
                {
                  row_pdat.write(pc_.bpp(),pixel);
                  // fmt::print("{} ",pixel);
                }
              // fmt::print("\n");
              break;
            case PACK_TRANSPARENT:
              row_pdat.write(DATA_PACKET_PIXEL_COUNT_SIZE,
                             pdp.pixels.size()-1);
              // fmt::print("transparent: {}\n",
              //            pdp.pixels.size());
              
              break;
            case PACK_EOL:
              eol = true;
              // fmt::print("eol:\n");
              break;
            }
        }
    }

  BitStream &a = rows_pdat[0];
  BitStream &b = rows_pdat[1];  
  
  
  for(auto &row_pdat : rows_pdat)
    {
      row_pdat.shrink();
      pdat_.insert(pdat_.end(),
                   row_pdat.data().begin(),
                   row_pdat.data().end());
    }
}

void
CelPacker::pack(const Bitmap            &b_,
                const RGBA8888Converter &pc_,
                ByteVec                 &pdat_)
{
  AbstractPackedImage api;

  pass0_build_api_from_bitmap(b_,pc_,api);
  pass1_pack_packed(api);
  pass2_mark_transparents(api);
  pass3_combine(api);
  pass4_split_large_packets(api);
  pass5_remove_trailing_transparents(api);
  pass6_remove_trailing_eol(api);

  //  api_to_bytevec(b_,api,pc_,pdat_);
  // api_to_bytevec2(b_,api,pc_,pdat_);
  api_to_bytevec3(b_,api,pc_,pdat_);  
};
