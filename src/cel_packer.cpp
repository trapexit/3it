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
  transparent.
  3. Compress lists of literal packets into a literal packet.
  4. Find instances of literal packets followed by packed (or packed
  followed by literal) and check if it would take less space if
  combined and do so. This will be run multiple times till the size stops reducing.
*/

#include "cel_packer.hpp"

#include "bitmap.hpp"
#include "bitstream.hpp"
#include "bpp.hpp"
#include "byte_reader.hpp"
#include "bytevec.hpp"
#include "ccb_flags.hpp"
#include "pixel_converter.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>


#define BITS_PER_BYTE 8
#define BYTES_PER_WORD 4
#define BITS_PER_WORD (BITS_PER_BYTE * BYTES_PER_WORD)
#define DATA_PACKET_DATA_TYPE_SIZE 2
#define DATA_PACKET_PIXEL_COUNT_SIZE 6


struct PackedDataPacket
{
  uint8_t type;
  uint8_t bpp;
  std::vector<RGBA8888> pixels;

  bool is_literal() const { return type == PACK_LITERAL; };
  bool is_packed() const { return type == PACK_PACKED; };
  bool is_literal_or_packed() const { return is_literal() || is_packed(); }

  size_t size() const;
  size_t raw_literal_size() const;
};

typedef std::vector<std::vector<PackedDataPacket> > AbstractPackedImage;


size_t
PackedDataPacket::size() const
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

size_t
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

static
void
pass0_build_api_from_bitmap(const Bitmap        &b_,
                            uint8_t              bpp_,
                            AbstractPackedImage &api_)
{
  api_.resize(b_.h);
  for(size_t y = 0; y < b_.h; y++)
    {
      std::vector<PackedDataPacket> &pdp_list = api_[y];

      for(size_t x = 0; x < b_.w; x++)
        {
          RGBA8888 p;
          PackedDataPacket pdp;

          // If alpha is 0 then zero out the color to make packing
          // easier later
          p = *b_.xy(x,y);
          if(p.a == 0)
            p = RGBA8888(0);

          pdp.type = PACK_LITERAL;
          pdp.bpp  = bpp_;
          pdp.pixels.emplace_back(p);

          pdp_list.emplace_back(pdp);
        }
    }
}

static
bool
can_pack(PackedDataPacket &curpdp_,
         PackedDataPacket &newpdp_)
{
  if(!(curpdp_.pixels[0] == newpdp_.pixels[0]))
    return false;
  if(newpdp_.pixels.size() < 64)
    return true;
  return false;
}

static
void
pass1_pack_packed(AbstractPackedImage &api_)
{
  for(auto &pdp_list : api_)
    {
      std::vector<PackedDataPacket> newpdp_list;

      newpdp_list.emplace_back(pdp_list[0]);

      for(size_t i = 1; i < pdp_list.size(); i++)
        {
          PackedDataPacket &curpdp = pdp_list[i];
          PackedDataPacket &newpdp = newpdp_list.back();

          if(::can_pack(curpdp,newpdp))
            {
              newpdp.type = PACK_PACKED;
              newpdp.pixels.emplace_back(curpdp.pixels[0]);
            }
          else
            {
              newpdp_list.emplace_back(pdp_list[i]);
            }
        }

      pdp_list = newpdp_list;
    }
}

static
void
pass2_mark_transparents(AbstractPackedImage &api_)
{
  for(auto &pdp_list : api_)
    {
      for(auto &pdp : pdp_list)
        {
          if(pdp.pixels[0].a != 0)
            continue;

          switch(pdp.type)
            {
            case PACK_PACKED:
            case PACK_LITERAL:
              pdp.type = PACK_TRANSPARENT;
              break;
            default:
              break;
            }
        }
    }
}

static
void
pass3_pack_literal(AbstractPackedImage &pi_)
{
  for(auto &pdp_list : pi_)
    {
      std::vector<PackedDataPacket> newpdp_list;

      for(size_t i = 0; i < pdp_list.size();)
        {
          newpdp_list.emplace_back(pdp_list[i]);
          if(pdp_list[i].type != PACK_LITERAL)
            {
              i++;
              continue;
            }

          i++;
          while((i < pdp_list.size()) && newpdp_list.back().pixels.size() < 64)
            {
              if(pdp_list[i].type != PACK_LITERAL)
                break;

              newpdp_list.back().pixels.emplace_back(pdp_list[i].pixels[0]);
              i++;
            }
        }
      pdp_list = newpdp_list;
    }
}

static
size_t
api_row_size(std::vector<PackedDataPacket> const &pdp_list_)
{
  size_t rv;

  rv = 0;
  for(auto const &pdp : pdp_list_)
    rv += pdp.size();

  return rv;
}

static
size_t
api_size(AbstractPackedImage &api_)
{
  size_t rv;

  rv = 0;
  for(auto const &pdp_list : api_)
    rv += api_row_size(pdp_list);

  return rv;
}

static
void
pass4_compress_literal_and_packed(AbstractPackedImage &api_)
{
  for(auto &pdp_list : api_)
    {
      std::vector<PackedDataPacket> newpdp_list;

      for(auto const &pdp : pdp_list)
        {
          switch(pdp.type)
            {
            case PACK_LITERAL:
              {
                if(newpdp_list.empty())
                  {
                    newpdp_list.emplace_back(pdp);
                    break;
                  }

                auto &newpdp = newpdp_list.back();
                if(newpdp.is_literal_or_packed())
                  {
                    if(newpdp.size() >= newpdp.raw_literal_size())
                      {
                        newpdp.type = PACK_LITERAL;
                        for(auto &p : pdp.pixels)
                          newpdp.pixels.emplace_back(p);
                      }
                    else
                      {
                        newpdp_list.emplace_back(pdp);
                      }
                  }
                else
                  {
                    newpdp_list.emplace_back(pdp);
                  }
              }
              break;
            case PACK_TRANSPARENT:
            case PACK_EOL:
              newpdp_list.emplace_back(pdp);
              break;
            case PACK_PACKED:
              {
                if(newpdp_list.empty() || !newpdp_list.back().is_literal())
                  {
                    newpdp_list.emplace_back(pdp);
                    break;
                  }

                auto &newpdp = newpdp_list.back();
                if(pdp.size() > pdp.raw_literal_size())
                  {
                    for(auto &p : pdp.pixels)
                      newpdp.pixels.emplace_back(p);
                  }
                else
                  {
                    newpdp_list.emplace_back(pdp);
                  }
              }
              break;
            }
        }

      pdp_list = newpdp_list;
    }
}

static
void
pass4_combine(AbstractPackedImage &api_)
{
  size_t prev_size;
  size_t comp_size;

  do
    {
      prev_size = api_size(api_);
      pass4_compress_literal_and_packed(api_);
      comp_size = api_size(api_);
    }
  while(comp_size != prev_size);
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

// offset = number of words to next row minus 2
// Meaning a min of 8 bytes per row.
static
void
write_next_row_offset(BitStreamWriter   &bs_,
                      const std::size_t  offset_width_,
                      const std::size_t  next_row_offset_)
{
  std::size_t next_row_in_words;

  next_row_in_words = (((bs_.tell() - next_row_offset_) / BITS_PER_WORD) - 2);
  bs_.write(next_row_offset_,
            offset_width_,
            next_row_in_words);
}

static
void
api_to_bytevec(const Bitmap              &b_,
               const AbstractPackedImage &api_,
               const RGBA8888Converter   &pc_,
               ByteVec                   &pdat_)
{

  BitStreamWriter bs;
  std::size_t offset_width;
  std::size_t next_row_offset;

  offset_width = ::calc_offset_width(pc_.bpp());

  pdat_.resize(b_.w * b_.h * BYTES_PER_WORD);
  bs.reset(pdat_);

  for(size_t i = 0; i < api_.size(); i++)
    {
      const auto &pdplist = api_[i];

      next_row_offset = bs.tell();
      bs.skip(offset_width);
      for(const auto &pdp : pdplist)
        {
          bs.write(DATA_PACKET_DATA_TYPE_SIZE,pdp.type);
          switch(pdp.type)
            {
            case PACK_PACKED:
              uint32_t c;
              bs.write(DATA_PACKET_PIXEL_COUNT_SIZE,pdp.pixels.size()-1);
              c = pc_.convert(&pdp.pixels[0].r);
              bs.write(pc_.bpp(),c);
              break;
            case PACK_LITERAL:
              bs.write(DATA_PACKET_PIXEL_COUNT_SIZE,pdp.pixels.size()-1);
              for(const auto &pixel : pdp.pixels)
                {
                  uint32_t c;
                  c = pc_.convert(&pixel.r);
                  bs.write(pc_.bpp(),c);
                }
              break;
            case PACK_TRANSPARENT:
              bs.write(DATA_PACKET_PIXEL_COUNT_SIZE,pdp.pixels.size()-1);
              break;
            case PACK_EOL:
              break;
            }

        }
      // The offset is minus 2 so need to be at least 2 words
      bs.skip_to_64bit_boundary();

      ::write_next_row_offset(bs,offset_width,next_row_offset);
    }

  pdat_.resize(bs.tell() / BITS_PER_BYTE);
}

void
CelPacker::pack(const Bitmap            &b_,
                const RGBA8888Converter &pc_,
                ByteVec                 &pdat_)
{
  AbstractPackedImage api;

  pass0_build_api_from_bitmap(b_,pc_.bpp(),api);
  pass1_pack_packed(api);
  pass2_mark_transparents(api);
  pass3_pack_literal(api);
  pass4_combine(api);

  api_to_bytevec(b_,api,pc_,pdat_);
};
