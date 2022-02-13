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
  int lookup(const uint16_t color_) const;

public:
  void build(const Bitmap &bitmap);
};
