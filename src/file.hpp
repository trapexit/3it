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

#include "span.hpp"

#include <filesystem>

#include <cstdio>


class File
{
public:
  File();

public:
  void open(const std::filesystem::path &filepath, const char *mode);
  void close();

public:
  void big_endian();
  void little_endian();

public:
  size_t write(const void *ptr, const size_t size);
  size_t read(void *ptr, const size_t size);

public:
  size_t write(uint8_t v);
  size_t write(uint16_t v);
  size_t write(uint32_t v);

public:
  size_t write(cspan<uint8_t> data);

public:
  int seek(long offset, int whence);

public:
  bool eof();
  operator bool();

private:
  FILE *_file;
  bool  _big_endian;
};
