#include "datarw.hpp"
#include "byteswap.hpp"

void
DataRW::rewind()
{
  return seek(0);
}

void
DataRW::rewind(const size_t bytes_)
{
  return seek(tell() - bytes_);
}

void
DataRW::skip(const size_t bytes_)
{
  return seek(tell() + bytes_);
}

void
DataRW::skip_to_2byte_boundary()
{
  return skip(tell() & 1);
}

void
DataRW::skip_to_4byte_boundary()
{
  size_t offset;

  offset = tell();
  if(offset & 0x3)
    return skip(4 - (offset & 0x3));
}

void
DataRW::skip_to_8byte_boundary()
{
  size_t offset;

  offset = tell();
  if(offset & 0x7)
    return skip(8 - (offset & 0x7));
}

char
DataRW::c()
{
  char c;

  read((uint8_t*)&c,sizeof(c));

  return c;
}

int8_t
DataRW::i8()
{
  return (int8_t)c();
}

uint8_t
DataRW::u8()
{
  return (uint8_t)c();
}

int16_t
DataRW::i16be()
{
  int16_t i;

  read((uint8_t*)&i,sizeof(i));

  return byteswap_if_little_endian(i);
}

int16_t
DataRW::i16le()
{
  int16_t i;

  read((uint8_t*)&i,sizeof(i));

  return byteswap_if_big_endian(i);
}
