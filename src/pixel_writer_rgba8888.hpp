#pragma once

#include "bitmap.hpp"
#include "types_ints.h"

class PixelWriterRGBA8888
{
protected:
  Bitmap *_b;
  u64     _idx;

public:
  void init(Bitmap &b);

public:
  Bitmap& bitmap() const;
  u32 w() const;
  u32 h() const;
  u64 idx() const;
  u32 x() const;
  u32 y() const;
  bool row_filled() const;

public:
  void move_xy(const u32 x, const u32 y);
  void move_y(const u32 y);

public:
  virtual void write_rgba(const u8 r, const u8 g, const u8 b, const u8 a);
};
