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

// https://3dodev.com/ext/3DO/Portfolio_2.5/OnLineDoc/DevDocs/ppgfldr/ggsfldr/gpgfldr/3gpga.html#XREF38473

#include "cel_packer.hpp"

#include "bitmap.hpp"
#include "bitstream.hpp"
#include "byte_reader.hpp"
#include "bytevec.hpp"
#include "ccb_flags.hpp"
#include "pixel_converter.hpp"

#include <cstdint>
#include <vector>


struct RGB8888
{
  RGB8888(uint8_t r_,
          uint8_t g_,
          uint8_t b_,
          uint8_t a_)
    : r(r_),
      g(g_),
      b(b_),
      a(a_)
  {
  }

  RGB8888(uint32_t rgba_)
    : r((rgba_ & 0xFF000000) >> 24),
      g((rgba_ & 0x00FF0000) >> 16),
      b((rgba_ & 0x0000FF00) >>  8),
      a((rgba_ & 0x000000FF) >>  0)
  {
  }

  bool
  operator==(const RGB8888 &rhs_) const
  {
    return ((r == rhs_.r) &&
            (g == rhs_.g) &&
            (b == rhs_.b));
  }

  uint8_t r,g,b,a;
};

struct PackedDataPacket
{
  uint8_t type;
  std::vector<RGB8888> pixels;
};

typedef std::vector<std::vector<PackedDataPacket> > AbstractPackedImage;

static
void
pass0(const Bitmap        &b_,
      AbstractPackedImage &pi_)
{
  pi_.resize(b_.h);
  for(size_t y = 0; y < b_.h; y++)
    {
      std::vector<PackedDataPacket> &pdp_list = pi_[y];

      for(size_t x = 0; x < b_.w; x++)
        {
          const uint8_t *p = b_.xy(x,y);
          PackedDataPacket pdp;

          pdp.type = PACK_LITERAL;
          pdp.pixels.emplace_back(p[0],p[1],p[2],p[3]);

          pdp_list.emplace_back(pdp);
        }
    }
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
          if((pdp_list[i].pixels[0] == newpdp_list.back().pixels[0]) &&
             (newpdp_list.back().pixels.size() < 64))
            {
              newpdp_list.back().type = PACK_PACKED;
              newpdp_list.back().pixels.emplace_back(pdp_list[0].pixels[0]);
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
pass2_pack_literal(AbstractPackedImage &pi_)
{
  for(auto &pdp_list : pi_)
    {
      std::vector<PackedDataPacket> newpdp_list;

      for(size_t i = 0; i < pdp_list.size();)
        {
          newpdp_list.emplace_back(pdp_list[i]);
          if(pdp_list[i].type == PACK_PACKED)
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
void
pass3_pack_transparent(AbstractPackedImage &pi_,
                       RGB8888              transparent_)
{
  for(auto &pdp_list : pi_)
    {
      for(auto &pdp : pdp_list)
        {
          if(pdp.pixels[0] == transparent_)
            pdp.type = PACK_TRANSPARENT;
          else if(pdp.pixels[0].a == 0xFF)
            pdp.type = PACK_TRANSPARENT;
        }
    }
}

static
void
api_to_bytevec(const Bitmap              &b_,
               const AbstractPackedImage &api_,
               const RGBA8888Converter   &pc_,
               ByteVec                   &pdat_)
{
  size_t next_row_offset;
  BitStreamWriter bs;

  pdat_.resize(b_.w * b_.h * 4);
  bs.reset(pdat_);

  for(size_t i = 0; i < api_.size(); i++)
    {
      const auto &pdplist = api_[i];

      next_row_offset = bs.tell();
      bs.skip(16);
      for(const auto &pdp : pdplist)
        {
          bs.write(2,pdp.type);
          switch(pdp.type)
            {
            case PACK_PACKED:
              uint32_t c;
              bs.write(6,pdp.pixels.size()-1);
              c = pc_.convert(&pdp.pixels[0].r);
              bs.write(pc_.bpp(),c);
              break;
            case PACK_LITERAL:
              bs.write(6,pdp.pixels.size()-1);
              for(const auto &pixel : pdp.pixels)
                {
                  uint32_t c;
                  c = pc_.convert(&pixel.r);
                  bs.write(pc_.bpp(),c);
                }
              break;
            case PACK_TRANSPARENT:
              bs.write(6,pdp.pixels.size()-1);
              break;
            case PACK_EOL:
              break;
            }

        }
      bs.skip_to_32bit_boundary();

      bs.write(next_row_offset,
               16,
               (((bs.tell() - next_row_offset) / 8 / 4) - 2));
    }

  bs.skip_to_32bit_boundary();

  pdat_.resize(bs.tell() / 8);
}

void
CelPacker::pack(const Bitmap            &b_,
                const RGBA8888Converter &pc_,
                const uint32_t           transparent_color_,
                ByteVec                 &pdat_)
{
  AbstractPackedImage api;
  RGB8888 transparent_color(transparent_color_);


  pass0(b_,api);
  pass1_pack_packed(api);
  pass2_pack_literal(api);
  pass3_pack_transparent(api,transparent_color_);

  api_to_bytevec(b_,api,pc_,pdat_);
};
