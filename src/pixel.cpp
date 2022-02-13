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

#include "pixel.hpp"

uint8_t
pixel::rgba8888_to_rgb332(const uint8_t *p_)
{
  return (((p_[0] & 0xE0) << 0) |
          ((p_[1] & 0xE0) >> 3) |
          ((p_[2] & 0xC0) >> 6));
}

uint16_t
pixel::rgba8888_to_rgb0555(const uint8_t *p_)
{
  return (((p_[0] & 0xF8) << 7) |
          ((p_[1] & 0xF8) << 2) |
          ((p_[2] & 0xF8) >> 3));
}

