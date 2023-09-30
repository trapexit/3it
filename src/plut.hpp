#pragma once

#include "bitmap.hpp"
#include "chunk.hpp"

#include <array>
#include <cstdint>


class PLUT : public std::array<uint16_t,32>
{
public:
  PLUT& operator=(const Chunk &chunk);

public:
  int lookup(uint16_t const  color,
             bool const      allow_closest = true,
             bool           *closest       = nullptr) const;
  bool has_color(uint16_t const    color,
                 std::size_t const end_idx = 32);

public:
  void build(Bitmap const &bitmap);
};
