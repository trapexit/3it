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

#pragma once

#include "plut.hpp"

#include <cstdint>

class RGBA8888Converter
{
public:
  RGBA8888Converter(const int   bpp,
                    const PLUT &plut);
  RGBA8888Converter(const int bpp);

public:
  static uint8_t  to_rgb332(const uint8_t *p);
  static uint16_t to_rgb0555(const uint8_t *p);

public:
  uint32_t convert(const uint8_t *p) const;

public:
  int bpp() const { return _bpp; }

private:
  int  _bpp;
  bool _coded;
  const PLUT *_plut;
};
