#ifndef RGB555_H_INCLUDED
#define RGB555_H_INCLUDED

#include <stdint.h>

typedef struct RGB555 RGB555;
struct RGB555
{
  uint16_t alpha:1;
  uint16_t red:5;
  uint16_t green:5;
  uint16_t blue:5;
};

#endif
