#ifndef WRAPPER_CHUNK_H_INCLUDED
#define WRAPPER_CHUNK_H_INCLUDED

#include "chunk_types.h"

#include <stdint.h>

typedef struct WrapperChunk WrapperChunk;
struct WrapperChunk
{
  int32_t chunk_id;
  int32_t chunk_size;
  ubyte   data[1];
};

#endif
