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

#include "pixel_converter.hpp"


RGBA8888Converter::RGBA8888Converter(const int   bpp_,
                                     const PLUT &plut_)
  : _bpp(bpp_),
    _coded(true),
    _plut(&plut_)
{
}

RGBA8888Converter::RGBA8888Converter(const int bpp_)
  : _bpp(bpp_),
    _coded(false),
    _plut(nullptr)
{
}

uint8_t
RGBA8888Converter::to_rgb332(const uint8_t *p_)
{
  return (((p_[0] & 0xE0) << 0) |
          ((p_[1] & 0xE0) >> 3) |
          ((p_[2] & 0xC0) >> 6));
}

uint8_t
RGBA8888Converter::to_rgb332(const RGBA8888 *p_)
{
  return (((p_->r & 0xE0) << 0) |
          ((p_->g & 0xE0) >> 3) |
          ((p_->b & 0xC0) >> 6));
}

uint16_t
RGBA8888Converter::to_rgb0555(const uint8_t *p_)
{
  return (((p_[0] & 0xF8) << 7) |
          ((p_[1] & 0xF8) << 2) |
          ((p_[2] & 0xF8) >> 3));
}

uint16_t
RGBA8888Converter::to_rgb0555(const RGBA8888 *p_)
{
  return (((p_->r & 0xF8) << 7) |
          ((p_->g & 0xF8) << 2) |
          ((p_->b & 0xF8) >> 3));
}

uint32_t
RGBA8888Converter::convert(const uint8_t *p_) const
{
  if(_coded == false)
    {
      switch(_bpp)
        {
        case 8:
          return to_rgb332(p_);
        case 16:
          return to_rgb0555(p_);
        default:
          return 0;
        }
    }
  else
    {
      uint16_t c;

      c = to_rgb0555(p_);

      return _plut->lookup(c);
    }
}

uint32_t
RGBA8888Converter::convert(const RGBA8888 *p_) const
{
  if(_coded == false)
    {
      switch(_bpp)
        {
        case 8:
          return to_rgb332(p_);
        case 16:
          return to_rgb0555(p_);
        default:
          return 0;
        }
    }
  else
    {
      uint16_t c;

      c = to_rgb0555(p_);

      return _plut->lookup(c);
    }
}
