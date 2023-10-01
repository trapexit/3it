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
  std::vector<RGBA8888> pixels;
};

typedef std::vector<std::vector<PackedDataPacket> > AbstractPackedImage;

static
void
pass0_build_api_from_bitmap(const Bitmap        &b_,
                            AbstractPackedImage &pi_)
{
  pi_.resize(b_.h);
  for(size_t y = 0; y < b_.h; y++)
    {
      std::vector<PackedDataPacket> &pdp_list = pi_[y];

      for(size_t x = 0; x < b_.w; x++)
        {
          const RGBA8888 &p = *b_.xy(x,y);
          PackedDataPacket pdp;

          pdp.type = PACK_LITERAL;
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
pass1_pack_packed(AbstractPackedImage &pi_)
{
  for(auto &pdp_list : pi_)
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
pass2_mark_transparents(AbstractPackedImage &pi_)
{
  for(auto &pdp_list : pi_)
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

  pass0_build_api_from_bitmap(b_,api);
  pass1_pack_packed(api);
  pass2_mark_transparents(api);
  pass3_pack_literal(api);

  api_to_bytevec(b_,api,pc_,pdat_);
};
