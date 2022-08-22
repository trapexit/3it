#pragma once

#include "span.hpp"

#include <cstddef>


class ByteReader
{
private:
  const uint8_t *_data;
  size_t         _size;
  size_t         _idx;

public:
  ByteReader&
  reset(const uint8_t *data_,
        const size_t   size_)
  {
    _data = data_;
    _size = size_;
    _idx  = 0;

    return *this;
  }

  ByteReader&
  reset(cspan<uint8_t> data_)
  {
    return reset(data_.data(),
                 data_.size());
  }

  bool
  eof() const
  {
    return (_idx >= _size);
  }

  size_t
  tell() const
  {
    return _idx;
  }

  ByteReader&
  rewind()
  {
    return seek(0);
  }

  ByteReader&
  rewind(const size_t bytes_)
  {
    return seek(_idx - bytes_);
  }

  ByteReader&
  seek(size_t idx_)
  {
    _idx = idx_;

    return *this;
  }

  ByteReader&
  skip(const size_t count_)
  {
    return seek(_idx + count_);
  }

  ByteReader&
  skip_to_2byte_boundary()
  {
    return skip(_idx & 1);
  }

  ByteReader&
  skip_to_4byte_boundary()
  {
    if(_idx & 0x3)
      return skip(4 - (_idx & 0x3));
    return *this;
  }

  ByteReader&
  skip_to_8byte_boundary()
  {
    if(_idx & 0x7)
      return skip(8 - (_idx & 0x7));
    return *this;
  }

public:
  ByteReader&
  read(uint8_t      *d_,
       const size_t  count_)
  {
    for(size_t i = 0; i < count_; i++)
      *d_++ = _data[_idx++];

    return *this;
  }

  ByteReader&
  read(char         *d_,
       const size_t  count_)
  {
    for(size_t i = 0; i < count_; i++)
      *d_++ = _data[_idx++];

    return *this;
  }

  ByteReader&
  readbe(uint8_t &d_)
  {
    d_ = u8();

    return *this;
  }

  ByteReader&
  readbe(uint16_t &d_)
  {
    d_ = u16be();

    return *this;
  }

  ByteReader&
  readbe(uint32_t &d_)
  {
    d_ = u32be();

    return *this;
  }

  ByteReader&
  readle(uint8_t &d_)
  {
    d_ = u8();

    return *this;
  }

  ByteReader&
  readle(uint16_t &d_)
  {
    d_ = u16le();

    return *this;
  }

  ByteReader&
  readle(uint32_t &d_)
  {
    d_ = u32le();

    return *this;
  }

  uint8_t
  u8()
  {
    return _data[_idx++];
  }

  int16_t
  i16be()
  {
    return (int16_t)u16be();
  }

  uint16_t
  u16be()
  {
    uint16_t v;

    v = ((_data[_idx+0] << 8) |
         (_data[_idx+1] << 0));

    _idx += 2;

    return v;
  }

  uint16_t
  u16le()
  {
    uint16_t v;

    v = ((_data[_idx+1] << 8) |
         (_data[_idx+0] << 0));

    _idx += 2;

    return v;
  }

  int32_t
  i24be()
  {
    int32_t v;

    v = (((int8_t)_data[_idx+0] << 16) |
         (_data[_idx+1] << 8) |
         (_data[_idx+2] << 0));

    _idx += 3;

    return v;
  }

  uint32_t
  u24be()
  {
    uint32_t v;

    v = ((_data[_idx+0] << 16) |
         (_data[_idx+1] <<  8) |
         (_data[_idx+2] <<  0));

    _idx += 3;

    return v;
  }

  uint32_t
  u32be()
  {
    uint32_t v;

    v = ((_data[_idx+0] << 24) |
         (_data[_idx+1] << 16) |
         (_data[_idx+2] <<  8) |
         (_data[_idx+3] <<  0));

    _idx += 4;

    return v;
  }

  uint32_t
  u32le()
  {
    uint32_t v;

    v = ((_data[_idx+3] << 24) |
         (_data[_idx+2] << 16) |
         (_data[_idx+1] <<  8) |
         (_data[_idx+0] <<  0));

    _idx += 4;

    return v;
  }

public:
  operator cspan<uint8_t>() const
  {
    return cspan<uint8_t>(_data,_size,_idx);
  }

  cspan<uint8_t>
  span() const
  {
    return cspan<uint8_t>(_data,_size,_idx);
  }
};
