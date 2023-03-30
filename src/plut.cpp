#/*
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

#include "plut.hpp"

#include "byte_reader.hpp"
#include "pixel_converter.hpp"

#include <unordered_set>


PLUT&
PLUT::operator=(const Chunk &chunk_)
{
  uint32_t count;
  ByteReader br;

  br.reset(chunk_.data(),
           chunk_.size());

  count = br.u32be();
  for(uint32_t i = 0; i < count; i++)
    (*this)[i] = br.u16be();

  return *this;
}

static
int
distance(int a_,
         int b_)
{
  return std::abs(a_ - b_);
}

static
int
closest(const PLUT     &plut_,
        const uint16_t  color_)
{
  int32_t closest = 0;

  for(size_t i = 0; i < plut_.size(); i++)
    {
      if(::distance(closest,color_) > ::distance(plut_[i],color_))
        closest = plut_[i];
    }

  return closest;
}

int
PLUT::lookup(const uint16_t color_) const
{
  for(size_t i = 0; i < size(); i++)
    {
      if(operator[](i) == color_)
        return i;
    }

  return ::closest(*this,color_);
}

void
PLUT::build(const Bitmap &bitmap_)
{
  uint16_t color;
  std::unordered_set<uint16_t> colors;

  for(size_t y = 0; y < bitmap_.h; y++)
    {
      for(size_t x = 0; x < bitmap_.w; x++)
        {
          const uint8_t *p = bitmap_.xy(x,y);

          color = RGBA8888Converter::to_rgb0555(p);

          colors.emplace(color);
        }
    }

  for(auto &c : *this)
    {
      c = *colors.begin();
      colors.erase(c);
      if(colors.empty())
        break;
    }
}
