#pragma once

#include "span.hpp"

#include <cstddef>
#include <cstdint>


class BitStreamReader
{
private:
  const uint8_t *_data;
  size_t         _size;
  size_t         _idx;

public:
  BitStreamReader()
    : _data(NULL),
      _size(0),
      _idx(0)
  {
  }

  BitStreamReader(const uint8_t *data_,
                  const size_t   size_,
                  const size_t   idx_ = 0)
  {
    reset(data_,size_,idx_);
  }

  BitStreamReader(cspan<uint8_t> &data_,
                  const size_t    idx_ = 0)
  {
    reset(data_,idx_);
  }

public:
  void
  reset(const uint8_t *data_,
        const size_t   size_,
        const size_t   idx_ = 0)
  {
    _data = data_;
    _size = size_;
    _idx  = idx_;
  }

  void
  reset(cspan<uint8_t> &data_,
        const size_t    idx_ = 0)
  {
    reset(data_.data(),
          data_.size(),
          idx_);
  }

public:
  void
  seek(const size_t idx_)
  {
    _idx = idx_;
  }

  void
  rewind()
  {
    seek(0);
  }

  void
  rewind(const size_t bits_)
  {
    seek(_idx - bits_);
  }

  void
  skip(const size_t bits_)
  {
    seek(_idx + bits_);
  }

  void
  skip_to_8bit_boundary()
  {
    if(_idx & 0x7)
      skip(0x8 - (_idx & 0x7));
  }

  void
  skip_to_16bit_boundary()
  {
    if(_idx & 0x0F)
      skip(0x10 - (_idx & 0x0F));
  }

  void
  skip_to_32bit_boundary()
  {
    if(_idx & 0x1F)
      skip(0x20 - (_idx & 0x1F));
  }

  void
  skip_to_64bit_boundary()
  {
    if(_idx & 0x3F)
      skip(0x40 - (_idx & 0x3F));
  }

  size_t
  tell() const
  {
    return _idx;
  }

public:
  uint64_t
  read(const size_t idx_,
       const size_t bits_)
  {
    uint64_t val = 0;

    for(size_t i = idx_; i < (idx_ + bits_); i++)
        val = ((val << 1) | ((_data[i >> 3] >> (7 - (i & 7))) & 1));

    return val;
  }

  uint64_t
  read(const size_t bits_)
  {
    uint32_t v;

    v = read(_idx,bits_);
    _idx += bits_;

    return v;
  }
};


class BitStreamWriter
{
private:
  uint8_t *_data;
  size_t   _size;
  size_t   _idx;

public:
  BitStreamWriter()
    : _data(NULL),
      _size(0),
      _idx(0)
  {
  }

  BitStreamWriter(uint8_t      *data_,
                  const size_t  size_,
                  const size_t  idx_ = 0)
  {
    reset(data_,size_,idx_);
  }

  BitStreamWriter(span<uint8_t> &data_,
                  const size_t   idx_ = 0)
  {
    reset(data_,idx_);
  }

public:
  void
  reset(uint8_t      *data_,
        const size_t  size_,
        const size_t  idx_ = 0)
  {
    _data = data_;
    _size = size_;
    _idx  = idx_;
  }

  void
  reset(span<uint8_t> data_,
        const size_t  idx_ = 0)
  {
    reset(data_.data(),
          data_.size(),
          idx_);
  }

public:
  void
  seek(const size_t idx_)
  {
    _idx = idx_;
  }

  void
  rewind()
  {
    seek(0);
  }

  void
  rewind(const size_t bits_)
  {
    seek(_idx - bits_);
  }

  void
  skip(const size_t bits_)
  {
    seek(_idx + bits_);
  }

  void
  skip_to_8bit_boundary()
  {
    if(_idx & 0x7)
      skip(0x8 - (_idx & 0x7));
  }

  void
  skip_to_16bit_boundary()
  {
    if(_idx & 0x0F)
      skip(0x10 - (_idx & 0x0F));
  }

  void
  skip_to_32bit_boundary()
  {
    if(_idx & 0x1F)
      skip(0x20 - (_idx & 0x1F));
  }

  void
  skip_to_64bit_boundary()
  {
    if(_idx & 0x3F)
      skip(0x40 - (_idx & 0x3F));
  }

  size_t
  tell() const
  {
    return _idx;
  }

public:
  void
  write(size_t   idx_,
        size_t   bits_,
        uint64_t val_)
  {
    for(size_t i = 0; i < bits_; i++)
      {
        uint8_t &d = _data[idx_ >> 3];
        const int shift = (7 - (idx_ & 7));

        d = ((d & ~(1UL << shift)) | (((val_ >> (bits_ - i - 1)) & 1) << shift));
        idx_++;
      }
  }

  void
  write(size_t   bits_,
        uint64_t val_)
  {
    write(_idx,bits_,val_);
    _idx += bits_;
  }
};
