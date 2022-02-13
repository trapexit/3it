#pragma once

#include "chunk.hpp"
#include "chunkid.hpp"

#include <cstdint>
#include <type_traits>

class ImageControlChunk
{
public:
  ChunkID  id;                  /* 'IMAG' */
  uint32_t chunk_size;          /* size in bytes of chunk including size (28)*/
  int32_t  w;                   /* width in pixels */
  int32_t  h;                   /* height in pixels */
  int32_t  bytesperrow;         /* may include pad bytes at row end for alignment */
  uint8_t  bitsperpixel;        /* 8, 16, 24 */
  uint8_t  numcomponents;       /* 3 => RGB (or YUV) , 1 => color index */
                                /* 3 => RGB (8  16 or 24 bits per pixel) */
                                /* 8 bit is 332 RGB (or YUV) */
                                /* 16 bit is 555 RGB (or YUV) */
                                /* 24 bit is 888 RGB (or YUV) */
                                /* 1 => coded meaning color indexed */
                                /* Coded images Require a Pixel Lookup Table Chunk */
  uint8_t  numplanes;           /* 1 => chunky;  3=> planar */
                                /* although the hardware does not support planar modes */
                                /* it is useful for some compression methods to separate */
                                /* the image into RGB planes or into YCrCb planes */
                                /* numcomponents must be greater than 1 for planar to */
                                /* have any effect */
  uint8_t  colorspace;          /* 0 => RGB, 1 => YCrCb */
  uint8_t  comptype;            /* compression type; 0 => uncompressed */
                                /* 1 = Cel bit packed */
                                /* other compression types will be defined later */
  uint8_t  hvformat;            /* 0 => 0555; 1=> 0554h; 2=> 0554v; 3=> v554h */
  uint8_t  pixelorder;          /* 0 => (0,0), (1,0), (2,0) (x,y) is (row,column) */
                                /* 1 => (0,0), (0,1), (1,0), (1,1) Sherrie LRform */
				/* 2 => (0,1), (0,0), (1,1), (1,0) UGO LRform */
  uint8_t  version;             /* file format version identifier. 0 for now */

// public:
//   ImageControlChunk();

public:
  std::string idstr() const;
  std::string numplanes_str() const;
  std::string colorspace_str() const;
  std::string comptype_str() const;
  std::string hvformat_str() const;
  std::string pixelorder_str() const;

public:
  ImageControlChunk& operator=(const Chunk &chunk);
  operator bool() const;

public:
  void byteswap_if_little_endian();
};

static_assert(sizeof(ImageControlChunk) == 28,"ImageControlChunk not properly packed");
