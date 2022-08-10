#pragma once

#include "byteswap.hpp"

#include <cstdint>
#include <vector>


class ByteVec : public std::vector<uint8_t>
{
public:
  void
  push_back_be(uint16_t v_)
  {
    const uint8_t *v = (const uint8_t*)&v_;
    v_ = byteswap_if_little_endian(v_);
    push_back(v[0]);
    push_back(v[1]);
  }
};

typedef std::vector<ByteVec> ByteVecVec;
