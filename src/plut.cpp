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
#include "fmt.hpp"
#include "pixel_converter.hpp"

#include <stdexcept>
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
closest(const PLUT     &plut_,
        const uint16_t  color_)
{
  float  closest     = 10000000;
  size_t closest_idx = 0;

  float r = ((color_ >>  0) & 0x001F);
  float g = ((color_ >>  5) & 0x001F);
  float b = ((color_ >> 10) & 0x001F);

  for(size_t i = 0; i < plut_.size(); i++)
    {
      float pr = ((plut_[i] >>  0) & 0x1F);
      float pg = ((plut_[i] >>  5) & 0x1F);
      float pb = ((plut_[i] >> 10) & 0x1F);
      float distance;
      float dr, dg, db;

      dr = ((pr - r) * 0.30);
      dg = ((pg - g) * 0.59);
      db = ((pb - b) * 0.11);

      distance = ((dr * dr) +
                  (dg * dg) +
                  (db * db));

      if(distance > closest)
        continue;

      closest     = distance;
      closest_idx = i;
    }

  return plut_[closest_idx];
}

int
PLUT::lookup(const uint16_t  color_,
             bool const      allow_closest_,
             bool           *closest_) const
{
  for(size_t i = 0; i < size(); i++)
    {
      if(operator[](i) == color_)
        return i;
    }

  if(allow_closest_ == false)
    {
      std::string err;

      err = fmt::format("color {:#06x} not found in PLUT",color_);

      throw std::runtime_error(err);
    }

  if(closest_ != nullptr)
    *closest_ = true;

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
