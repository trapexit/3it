#pragma once

#include "types_ints.h"

#include <cmath>

static
inline
u8
scale_u8_to_u5(const u8 v_)
{
  return (((v_ * 31) + 127) / 255);
}

static
inline
u8
scale_u5_to_u8(const u8 v_)
{
  return ((v_ << 3)|(v_ >> 2));
}

static
inline
u8
scale_u8_to_u3(const u8 v_)
{
  return (((v_ * 7) + 127) / 255);
}

static
inline
u8
scale_u3_to_u8(const u8 v_)
{
  return ((v_ << 5)|(v_ << 2)|(v_ >> 1));
}

static
inline
u8
scale_u8_to_u2(const u8 v_)
{
  return (((v_ * 3) + 127) / 255);
}

static
inline
u8
scale_u2_to_u8(const u8 v_)
{
  return ((v_ << 6)|(v_ << 4)|(v_ << 2)|(v_ << 0));
}
