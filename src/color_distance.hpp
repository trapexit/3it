#pragma once

#include <cmath>
#include "types_ints.h"


// https://www.compuphase.com/cmetric.htm
static
inline
double
color_distance(const s64 r1_,
               const s64 g1_,
               const s64 b1_,
               const s64 r2_,
               const s64 g2_,
               const s64 b2_)
{
  s64 r_mean;
  s64 r_diff;
  s64 g_diff;
  s64 b_diff;

  r_mean = ((r1_ + r2_) / 2);
  r_diff = (r1_ - r2_);
  g_diff = (g1_ - g2_);
  b_diff = (b1_ - b2_);

  return sqrt((((512+r_mean)*r_diff*r_diff)>>8) +
              (4*g_diff*g_diff) +
              (((767-r_mean)*b_diff*b_diff)>>8));
}
