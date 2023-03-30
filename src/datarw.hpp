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
#include <vector>

class DataRW
{
public:
  virtual ~DataRW() {};

public:
  virtual bool eof() const = 0;
  virtual size_t tell() const = 0;
  virtual void seek(const size_t idx) = 0;
  virtual size_t read(uint8_t *p, const size_t count) = 0;
  virtual size_t write(uint8_t const * const p, const size_t count) = 0;

public:
  void rewind();
  void rewind(const size_t bytes);
  void skip(const size_t bytes);
  void skip_to_2byte_boundary();
  void skip_to_4byte_boundary();
  void skip_to_8byte_boundary();

public:
  template<typename T, size_t N>
  size_t
  read(std::array<T,N> &array_,
       const size_t     count_ = N)
  {
    return read((uint8_t*)array_.data(),(sizeof(T) * count_));
  }

  template<typename T>
  size_t
  readbe(T &d_)
  {
    size_t rv;

    rv = read((uint8_t*)&d_,sizeof(d_));

    byteswap_if_little_endian(&d_);

    return rv;
  }

  template<typename T>
  size_t
  readle(T &d_)
  {
    size_t rv;

    rv = read((uint8_t*)&d_,sizeof(d_));

    byteswap_if_big_endian(&d_);

    return rv;
  }

public:
  template<typename T>
  size_t
  writebe(T d_)
  {
    size_t rv;

    byteswap_if_little_endian(&d_);

    rv = write((uint8_t const *)&d_,sizeof(d_));

    return rv;
  }

  template<typename T>
  size_t
  writele(T d_)
  {
    size_t rv;

    byteswap_if_big_endian(&d_);

    rv = write((uint8_t const *)&d_,sizeof(d_));

    return rv;
  }

public:
  char     c();
  int8_t   i8();
  uint8_t  u8();
  int16_t  i16be();
  int16_t  i16le();
  uint16_t u16be();
  uint16_t u16le();
  int32_t  i24be();
  int32_t  i24le();
  uint32_t u24be();
  uint32_t u24le();
  int32_t  i32be();
  int32_t  i32le();
  uint32_t u32be();
  uint32_t u32le();
};
