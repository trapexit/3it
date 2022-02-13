#pragma once

#include "byteswap.hpp"

#include <string>

#include <cstdint>


class ChunkID
{
public:
  ChunkID()
  {
    u8[0] = '?';
    u8[1] = '?';
    u8[2] = '?';
    u8[3] = '?';
  }

  ChunkID(const uint8_t a_,
          const uint8_t b_,
          const uint8_t c_,
          const uint8_t d_)
  {
    u8[0] = a_;
    u8[1] = b_;
    u8[2] = c_;
    u8[3] = d_;
    u32   = ::byteswap_if_little_endian(u32);
  }

  ChunkID(const std::string &s_)
  {
    u8[0] = s_[0];
    u8[1] = s_[1];
    u8[2] = s_[2];
    u8[3] = s_[3];
    u32   = ::byteswap_if_little_endian(u32);
  }

public:
  ChunkID&
  operator=(const uint32_t v_)
  {
    u32 = v_;
    return *this;
  }

  ChunkID&
  operator=(const std::string &s_)
  {
    u8[0] = s_[0];
    u8[1] = s_[1];
    u8[2] = s_[2];
    u8[3] = s_[3];
    u32   = ::byteswap_if_little_endian(u32);
    return *this;
  }

  ChunkID&
  operator=(const ChunkID &v_)
  {
    u32 = v_.u32;
    return *this;
  }

public:
  bool
  operator==(const ChunkID &v_) const
  {
    return (u32 == v_.u32);
  }

  bool
  operator==(const uint32_t v_) const
  {
    return (u32 == v_);
  }

  bool
  operator==(const std::string &s_) const
  {
    ChunkID tmp(s_);

    return (u32 == tmp.u32);
  }

public:
  const
  uint32_t&
  uint32() const
  {
    return u32;
  }

  const
  uint8_t*
  uint8() const
  {
    return u8;
  }

  std::string
  str() const
  {
    std::string s;
    union { uint8_t u8[4]; uint32_t u32; } id;

    id.u32 = ::byteswap_if_little_endian(u32);

    s.push_back(id.u8[0]);
    s.push_back(id.u8[1]);
    s.push_back(id.u8[2]);
    s.push_back(id.u8[3]);

    return s;
  }

public:
  union { uint8_t u8[4]; uint32_t u32; };
};
