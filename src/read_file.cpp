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

#include "read_file.hpp"

#include <cstdint>
#include <exception>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;


void
ReadFile::read(std::istream &is_,
               ByteVec      &data_)
{
  is_.unsetf(std::ios::skipws);
  data_.insert(data_.begin(),
               std::istream_iterator<uint8_t>(is_),
               std::istream_iterator<uint8_t>());
}

void
ReadFile::read(const fs::path &filepath_,
               ByteVec        &data_)
{
  std::ifstream is;

  is.open(filepath_,std::ios::binary|std::ios::in);
  if(!is)
    throw std::system_error(errno,std::system_category(),"failed to open "+filepath_.string());

  ReadFile::read(is,data_);

  is.close();
}
