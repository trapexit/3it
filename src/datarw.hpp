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

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>


class DataRW
{
public:
  virtual ~DataRW() {};

public:
  virtual bool   eof() const            = 0;
  virtual bool   error() const          = 0;
  virtual size_t tell() const           = 0;
  virtual void   seek(const size_t idx) = 0;

protected:
  virtual size_t _r(void *p, const size_t count)       = 0;
  virtual size_t _w(const void *p, const size_t count) = 0;

public:
  void
  rewind()
  {
    return seek(0);
  }

  void
  rewind(const size_t bytes_)
  {
    return seek(tell() - bytes_);
  }

  void
  skip(const size_t bytes_)
  {
    return seek(tell() + bytes_);
  }

  void
  skip_to_2byte_boundary()
  {
    return skip(tell() & 1);
  }

  void
  skip_to_4byte_boundary()
  {
    size_t offset;

    offset = tell();
    if(offset & 0x3)
      return skip(4 - (offset & 0x3));
  }

  void
  skip_to_8byte_boundary()
  {
    size_t offset;

    offset = tell();
    if(offset & 0x7)
      return skip(8 - (offset & 0x7));
  }

protected:
  template<typename T>
  T
  rpodbe()
  {
    static_assert(std::is_trivial<T>::value, "T must be POD/trivial");

    T v;

    _r((uint8_t*)&v,sizeof(v));

    byteswap_if_little_endian(&v);

    return v;
  }

  template<typename T>
  size_t
  wpodbe(T v_)
  {
    static_assert(std::is_trivial<T>::value, "T must be POD/trivial");

    byteswap_if_little_endian(&v_);

    return _w(&v_,sizeof(v_));
  }

  template<typename T>
  size_t
  wpodle(T v_)
  {
    static_assert(std::is_trivial<T>::value, "T must be POD/trivial");

    byteswap_if_big_endian(&v_);

    return _w(&v_,sizeof(v_));
  }

public:
  size_t
  w(const char *s_)
  {
    return _w(s_,strlen(s_));
  }

  size_t
  w(std::string const &s_)
  {
    return _w(s_.c_str(),
              s_.size());
  }

  size_t
  w(std::vector<char> const &vec_)
  {
    return _w(vec_.data(),
              vec_.size());
  }

  size_t
  w(std::vector<uint8_t> const &vec_)
  {
    return _w(vec_.data(),
              vec_.size());
  }

public:
  size_t
  u8(uint8_t v_)
  {
    return _w(&v_,sizeof(v_));
  }

  size_t
  u16be(uint16_t v_)
  {
    return wpodbe<uint16_t>(v_);
  }

  size_t
  u16le(uint16_t v_)
  {
    return wpodle<uint16_t>(v_);
  }

  size_t
  i32be(int32_t v_)
  {
    return wpodbe<int32_t>(v_);
  }

  size_t
  u32be(uint32_t v_)
  {
    return wpodbe<uint32_t>(v_);
  }
};
