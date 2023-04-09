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

#include "endian.hpp"

#include <cstdint>
#include <type_traits>


static
inline
int8_t
byteswap(const int8_t v_)
{
  return v_;
}

static
inline
uint8_t
byteswap(const uint8_t v_)
{
  return v_;
}

static
inline
int16_t
byteswap(const int16_t v_)
{
  return (((v_ & INT16_C(0x00FF)) << 8) |
          ((v_ & INT16_C(0xFF00)) >> 8));

}

static
inline
uint16_t
byteswap(const uint16_t v_)
{
  return (((v_ & UINT16_C(0x00FF)) << 8) |
          ((v_ & UINT16_C(0xFF00)) >> 8));

}

static
inline
int32_t
byteswap(const int32_t v_)
{
  return (((v_ & INT32_C(0x000000FF)) << 24) |
          ((v_ & INT32_C(0x0000FF00)) <<  8) |
          ((v_ & INT32_C(0x00FF0000)) >>  8) |
          ((v_ & INT32_C(0xFF000000)) >> 24));
}

static
inline
uint32_t
byteswap(const uint32_t v_)
{
  return (((v_ & UINT32_C(0x000000FF)) << 24) |
          ((v_ & UINT32_C(0x0000FF00)) <<  8) |
          ((v_ & UINT32_C(0x00FF0000)) >>  8) |
          ((v_ & UINT32_C(0xFF000000)) >> 24));
}

template<typename T>
static
inline
typename std::enable_if<std::is_fundamental<T>::value, T>::type
byteswap_if_little_endian(const T v_)
{
  if(is_little_endian())
    return byteswap(v_);
  return v_;
}

template<typename T>
static
inline
typename std::enable_if<std::is_pointer<T>::value, void>::type
byteswap_if_little_endian(const T v_)
{
  *v_ = byteswap_if_little_endian(*v_);
}

template<typename T>
static
inline
typename std::enable_if<std::is_fundamental<T>::value, T>::type
byteswap_if_big_endian(const T v_)
{
  if(is_big_endian())
    return byteswap(v_);
  return v_;
}

template<typename T>
static
inline
typename std::enable_if<std::is_pointer<T>::value, void>::type
byteswap_if_big_endian(const T v_)
{
  *v_ = byteswap_if_big_endian(*v_);
}
