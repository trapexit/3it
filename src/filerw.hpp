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

#include "datarw.hpp"

#include <cstdint>
#include <stdio.h>

#include <string>
#include <filesystem>

#include <stdint.h>


class FileRW : public DataRW
{
private:
  FILE *_file;

public:
  FileRW();
  ~FileRW();

public:
  bool eof() const;
  bool error() const;
  uint64_t tell() const;
  void seek(const uint64_t idx);

protected:
  uint64_t _r(void *p, const uint64_t count);
  uint64_t _w(const void *p, const uint64_t count);

public:
  int open(char const *filepath,
           char const *mode);
  int open(std::string const &filepath,
           std::string const &mode);
  int open(std::filesystem::path &filepath,
           std::string const     &mode);
  int open_write_trunc(std::string const &filepath);
  int open_write_trunc(std::filesystem::path const &filepath);

public:
  int close();
};
