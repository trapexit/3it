#pragma once

#include "coord.h"
#include "chunk.hpp"
#include "chunkid.hpp"

#include <cstdint>
#include <type_traits>


// https://3dodev.com/documentation/file_formats/media/container/3do
class CelControlChunk
{
public:
  ChunkID  id;                  /* `CCB ` Identifies pixel data */
  uint32_t chunk_size;          /* size including chunk_header */
  uint32_t ccb_version;         /* version number of struct. 0 now*/
  uint32_t ccb_Flags;           /* 32 bits of CCB flags */
  uint32_t ccb_NextPtr;
  uint32_t ccb_CelData;
  uint32_t ccb_PLUTPtr;
  Coord    ccb_X;
  Coord    ccb_Y;
  int32_t  ccb_hdx;
  int32_t  ccb_hdy;
  int32_t  ccb_vdx;
  int32_t  ccb_vdy;
  int32_t  ccb_ddx;
  int32_t  ccb_ddy;
  uint32_t ccb_PPMPC;
  uint32_t ccb_PRE0;            /* Cel Preamble Word 0 */
  uint32_t ccb_PRE1;            /* Cel Preamble Word 1 */
  int32_t  ccb_Width;
  int32_t  ccb_Height;

public:
  CelControlChunk();

public:
  CelControlChunk& operator=(const Chunk &chunk);
  operator bool() const;

public:
  bool     coded() const;
  bool     packed() const;
  bool     lrform() const;
  int      bpp() const;
  bool     rep8() const;
  uint8_t  pluta() const;
  uint32_t type() const;
  uint32_t pdv() const;

public:
  void bpp(const uint32_t bpp);

public:
  void byteswap_if_little_endian();
};


static_assert(sizeof(CelControlChunk) == 80,"CelControlChunk not properly packed");
