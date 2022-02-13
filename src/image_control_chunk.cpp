

#include "image_control_chunk.hpp"

#include "byteswap.hpp"
#include "chunk_ids.hpp"


std::string
ImageControlChunk::numplanes_str() const
{
  switch(numplanes)
    {
    case 1:
      return "chunky";
    case 3:
      return "planar";
    default:
      return "unknown";
    }
}

std::string
ImageControlChunk::colorspace_str() const
{
  switch(colorspace)
    {
    case 0:
      return "RGB";
    case 1:
      return "YCrCb";
    default:
      return "unknown";
    }
}

std::string
ImageControlChunk::comptype_str() const
{
  switch(comptype)
    {
    case 0:
      return "uncompressed";
    case 1:
      return "CEL bit packed";
    default:
      return "unknown";
    }
}

std::string
ImageControlChunk::hvformat_str() const
{
  switch(hvformat)
    {
    case 0:
      return "0555";
    case 1:
      return "0544h";
    case 2:
      return "0554v";
    case 3:
      return "v554h";
    default:
      return "unknown";
    }
}

std::string
ImageControlChunk::pixelorder_str() const
{
  switch(pixelorder)
    {
    case 0:
      return "linear";
    case 1:
      return "Sherrie LRForm";
    case 2:
      return "UGO LRForm";
    default:
      return "unknown";
    }
}

ImageControlChunk&
ImageControlChunk::operator=(const Chunk &chunk_)
{
  const ImageControlChunk *imag;

  imag = (const ImageControlChunk*)chunk_.base();
  *this = *imag;
  byteswap_if_little_endian();

  return *this;
}

ImageControlChunk::operator bool() const
{
  return ((id         == CHUNK_IMAG) &&
          (chunk_size == sizeof(ImageControlChunk)));
}

void
ImageControlChunk::byteswap_if_little_endian()
{
  ::byteswap_if_little_endian(&id.u32);
  ::byteswap_if_little_endian(&chunk_size);
  ::byteswap_if_little_endian(&w);
  ::byteswap_if_little_endian(&h);
  ::byteswap_if_little_endian(&bytesperrow);
}
