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

#include "bpp.hpp"
#include "byte_reader.hpp"
#include "fmt.hpp"
#include "pixel_converter.hpp"
#include "color_distance.hpp"

#include <stdexcept>

#include <stdint.h>


uint64_t
PLUT::max_size() const
{
  return 32;
}

uint64_t
PLUT::min_size(const int bpp_) const
{
  switch(bpp_)
    {
    case BPP_1:
      return 2;
    case BPP_2:
      return 4;
    case BPP_4:
      return 16;
    case BPP_6:
    case BPP_8:
    case BPP_16:
      return 32;
    }

  return max_size();
}

PLUT&
PLUT::operator=(const Chunk &chunk_)
{
  uint32_t count;
  ByteReader br;

  br.reset(chunk_.data(),
           chunk_.size());

  clear();

  count = br.u32be();
  for(uint32_t i = 0; i < count; i++)
    push_back(br.u16be());

  return *this;
}

static
int
closest(const PLUT     &plut_,
        const uint16_t  color_)
{
  double closest     = 1000000;
  u64    closest_idx = 0;

  s64 r = ((color_ >> 10) & 0x1F);
  s64 g = ((color_ >>  5) & 0x1F);
  s64 b = ((color_ >>  0) & 0x1F);

  for(uint64_t i = 0; i < plut_.size(); i++)
    {
      double distance;
      s64 pr = ((plut_[i] >> 10) & 0x1F);
      s64 pg = ((plut_[i] >>  5) & 0x1F);
      s64 pb = ((plut_[i] >>  0) & 0x1F);

      distance = color_distance(r,g,b,pr,pg,pb);
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
  for(uint64_t i = 0; i < size(); i++)
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

bool
PLUT::has_color(std::uint16_t const c_) const
{
  for(const auto c : *this)
    {
      if(c == c_)
        return true;
    }

  return false;
}

void
PLUT::build(const Bitmap &bitmap_)
{
  uint16_t color;

  clear();
  for(uint64_t y = 0; y < bitmap_.h; y++)
    {
      for(uint64_t x = 0; x < bitmap_.w; x++)
        {
          const RGBA8888 *p = bitmap_.xy(x,y);

          if(p->a == 0)
            continue;

          color = RGBA8888Converter::to_rgb0555(p);

          if(has_color(color))
            continue;

          push_back(color);

          if(size() > max_size())
            throw std::runtime_error("too many colors for 3DO PLUT");
        }
    }

  // This should only happen if the source is entirely transparent in
  // which case add black (the 'transparent' value if NOBLK flag is
  // set to 1).
  if(empty())
    push_back(0);
}
