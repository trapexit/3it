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

public:
  void build(Bitmap const &bitmap);
};
