#pragma once

#include "bitmap.hpp"
#include "chunk.hpp"

#include <cstdint>
#include <vector>


class PLUT : public std::vector<uint16_t>
{
public:
  uint64_t max_size() const;
  uint64_t min_size(int bpp) const;

public:
  PLUT& operator=(const Chunk &chunk);

public:
  int lookup(uint16_t const  color,
             bool const      allow_closest = true,
             bool           *closest       = nullptr) const;
  bool has_color(uint16_t const color) const;

public:
  void build(Bitmap const &bitmap);
};
