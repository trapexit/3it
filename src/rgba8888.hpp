/*
  ISC License

  Copyright (c) 2023, Antonio SJ Musumeci <trapexit@spawn.link>

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

#pragma once

#include <cstdint>


struct RGBA8888
{
  RGBA8888() = default;

  RGBA8888(uint8_t r_,
           uint8_t g_,
           uint8_t b_,
           uint8_t a_)
    : r(r_),
      g(g_),
      b(b_),
      a(a_)
  {
  }

  RGBA8888(uint32_t rgba_)
    : r((rgba_ & 0xFF000000) >> 24),
      g((rgba_ & 0x00FF0000) >> 16),
      b((rgba_ & 0x0000FF00) >>  8),
      a((rgba_ & 0x000000FF) >>  0)
  {
  }

  bool
  operator==(const RGBA8888 &rhs_) const
  {
    return ((r == rhs_.r) &&
            (g == rhs_.g) &&
            (b == rhs_.b) &&
            (a == rhs_.a));
  }

  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};
