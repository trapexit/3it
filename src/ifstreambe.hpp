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

#include "byteswap.hpp"

#include <fstream>


namespace std
{
  class istreambe : public istream
  {
  public:
    explicit
    istreambe(streambuf *sb_)
      : istream(sb_)
    {
    }

  public:
    template<typename T>
    istreambe&
    read(T &d_)
    {
      this->istream::read((char*)&d_,(std::streamsize)sizeof(T));
      return *this;
    }

    template<typename T>
    istreambe&
    readbe(T &d_)
    {
      this->istream::read((char*)&d_,(std::streamsize)sizeof(T));
      d_ = byteswap_if_little_endian(d_);
      return *this;
    }

    istreambe&
    read(char       *s_,
         streamsize  n_)
    {
      this->istream::read(s_,n_);
      return *this;
    }
  };

  class ifstreambe : public ifstream
  {
  public:
    template<typename T>
    ifstreambe&
    readbe(T &d_)
    {
      this->ifstream::read((char*)&d_,(std::streamsize)sizeof(T));
      d_ = byteswap_if_little_endian(d_);
      return *this;
    }

    ifstreambe&
    read(char       *s_,
         streamsize  n_)
    {
      this->ifstream::read(s_,n_);
      return *this;
    }
  };
}
