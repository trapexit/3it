#pragma once

#include "bits_and_bytes.hpp"
#include "span.hpp"

#include "types_ints.h"

#include <cassert>
#include <cstddef>


class BitStreamReader
{
private:
  const u8 *_data;
  u64       _size;
  u64       _idx;

public:
  BitStreamReader()
    : _data(NULL),
      _size(0),
      _idx(0)
  {
  }

  BitStreamReader(const u8 *data_,
                  const u64   size_,
                  const u64   idx_ = 0)
  {
    reset(data_,size_,idx_);
  }

  BitStreamReader(cspan<u8> &data_,
                  const u64    idx_ = 0)
  {
    reset(data_,idx_);
  }

public:
  void
  reset(const u8  *data_,
        const u64  size_,
        const u64  idx_ = 0)
  {
    _data = data_;
    _size = size_ * BITS_PER_BYTE;
    _idx  = idx_;
  }

  void
  reset(cspan<u8> &data_,
        const u64  idx_ = 0)
  {
    reset(data_.data(),
          data_.size(),
          idx_);
  }

public:
  void
  seek(const u64 idx_)
  {
    _idx = idx_;
  }

  void
  rewind()
  {
    seek(0);
  }

  void
  rewind(const u64 bits_)
  {
    seek(_idx - bits_);
  }

  void
  skip(const u64 bits_)
  {
    seek(_idx + bits_);
  }

  bool
  on_32bit_boundary() const
  {
    return !(_idx & 0x1F);
  }

  u8
  bits_to_32bit_boundary() const
  {
    return (0x20 - (_idx & 0x1F));
  }

  bool
  on_64bit_boundary() const
  {
    return !(_idx & 0x3F);
  }

  u8
  bits_to_64bit_boundary() const
  {
    return (0x40 - (_idx & 0x3F));
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

  u64
  size() const
  {
    return _size;
  }

  u64
  tell() const
  {
    return _idx;
  }

  u64
  tell_bits() const
  {
    return tell();
  }

  u64
  tell_bytes() const
  {
    return ((_idx + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
  }

public:
  u64
  read(const u64 idx_,
       const u64 bits_)
  {
    u64 val = 0;

    for(u64 i = idx_; i < (idx_ + bits_); i++)
      val = ((val << 1) | ((_data[i >> 3] >> (7 - (i & 7))) & 1));

    return val;
  }

  u64
  read(const u64 bits_)
  {
    u32 v;

    v = read(_idx,bits_);
    _idx += bits_;

    return v;
  }
};


class BitStreamWriter
{
private:
  u64 _idx;
  std::vector<u8> *_data;

public:
  BitStreamWriter()
    : _idx(0),
      _data(NULL)
  {
  }

  BitStreamWriter(std::vector<u8> &data_,
                  const u64        idx_ = 0)
  {
    reset(data_,idx_);
  }

public:
  void
  reset(std::vector<u8> &data_,
        const u64        idx_ = 0)
  {
    _data = &data_;
    _idx  = idx_;
  }

  void
  reset(std::vector<u8> *data_,
        const u64        idx_ = 0)
  {
    reset(*data_,idx_);
  }

private:
  void
  _maybe_resize(const u64 size_in_bits_)
  {
    assert(_data != NULL);

    if(size_in_bits_ > (_data->size() * BITS_PER_BYTE))
      _data->resize((size_in_bits_ + BITS_PER_BYTE - 1) / BITS_PER_BYTE);
  }

public:
  void
  seek(const u64 idx_)
  {
    _maybe_resize(idx_);
    _idx = idx_;
  }

  void
  rewind()
  {
    seek(0);
  }

  void
  rewind(const u64 bits_)
  {
    seek(_idx - bits_);
  }

  void
  skip(const u64 bits_)
  {
    seek(_idx + bits_);
  }

  bool
  on_8bit_boundary() const
  {
    return !(_idx & 0x7);
  }
  
  u8
  bits_to_8bit_boundary() const
  {
    return (0x08 - (_idx & 0x7));
  }

  bool
  on_16bit_boundary() const
  {
    return !(_idx & 0xF);
  }

  u8
  bits_to_16bit_boundary() const
  {
    return (0x10 - (_idx & 0xF));
  }

  bool
  on_32bit_boundary() const
  {
    return !(_idx & 0x1F);
  }

  u8
  bits_to_32bit_boundary() const
  {
    return (0x20 - (_idx & 0x1F));
  }

  bool
  on_64bit_boundary() const
  {
    return !(_idx & 0x3F);
  }

  u8
  bits_to_64bit_boundary() const
  {
    return (0x40 - (_idx & 0x3F));
  }
  
  void
  skip_to_8bit_boundary()
  {
    if(!on_8bit_boundary())
      skip(bits_to_8bit_boundary());
  }

  void
  zero_till_8bit_boundary()
  {
    if(!on_8bit_boundary())
      write(bits_to_8bit_boundary(),0);
  }

  void
  skip_to_16bit_boundary()
  {
    if(!on_16bit_boundary())
      skip(bits_to_16bit_boundary());
  }

  void
  zero_till_16bit_boundary()
  {
    if(!on_16bit_boundary())
      write(bits_to_16bit_boundary(),0);
  }

  void
  skip_to_32bit_boundary()
  {
    if(!on_32bit_boundary())
      skip(bits_to_32bit_boundary());
  }

  void
  zero_till_32bit_boundary()
  {
    if(!on_32bit_boundary())
      write(bits_to_32bit_boundary(),0);
  }

  void
  skip_to_64bit_boundary()
  {
    if(!on_64bit_boundary())
      skip(bits_to_64bit_boundary());
  }

  void
  zero_till_64bit_boundary()
  {
    if(!on_64bit_boundary())
      write(bits_to_64bit_boundary(),0);
  }

  u64
  tell() const
  {
    return _idx;
  }

  u64
  tell_bits() const
  {
    return tell();
  }

  u64
  tell_bytes() const
  {
    return ((_idx + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
  }

  u64
  tell_u32() const
  {
    return (tell_bytes() / 4);
  }

public:
  void
  write(u64 idx_,
        u64 bits_,
        u64 val_)
  {
    _maybe_resize(idx_ + bits_);

    for(u64 i = 0; i < bits_; i++)
      {
        u8 &d = (*_data)[idx_ >> 3];
        const int shift = (7 - (idx_ & 7));

        d = ((d & ~(1UL << shift)) | (((val_ >> (bits_ - i - 1)) & 1) << shift));
        idx_++;
      }
  }

  void
  write(u64 bits_,
        u64 val_)
  {
    write(_idx,bits_,val_);
    _idx += bits_;
  }

  void
  write(const std::vector<u8> &v_)
  {
    for(const u8 byte : v_)
      write(8,byte);
  }

public:
  u64
  read(const u64 idx_,
       const u64 bits_)
  {
    u64 val = 0;

    for(u64 i = idx_; i < (idx_ + bits_); i++)
      val = ((val << 1) | (((*_data)[i >> 3] >> (7 - (i & 7))) & 1));

    return val;
  }
};


class BitStream
{
private:
  u64 _idx;
  std::vector<u8> _data;

public:
  BitStream()
    : _idx(0),
      _data()
  {
  }

  BitStream(std::vector<u8> &data_,
            const u64        idx_ = 0)
  {
    reset(data_,idx_);
  }

private:
  void
  _maybe_resize(const u64 size_in_bits_)
  {
    if(size_in_bits_ > (_data->size() * BITS_PER_BYTE))
      _data->resize((size_in_bits_ + BITS_PER_BYTE - 1) / BITS_PER_BYTE);
  }

public:
  void
  seek(const u64 idx_)
  {
    _maybe_resize(idx_);
    _idx = idx_;
  }

  void
  rewind()
  {
    seek(0);
  }

  void
  rewind(const u64 bits_)
  {
    seek(_idx - bits_);
  }

  void
  skip(const u64 bits_)
  {
    seek(_idx + bits_);
  }

  bool
  on_8bit_boundary() const
  {
    return !(_idx & 0x7);
  }
  
  u8
  bits_to_8bit_boundary() const
  {
    return (0x08 - (_idx & 0x7));
  }

  bool
  on_16bit_boundary() const
  {
    return !(_idx & 0xF);
  }

  u8
  bits_to_16bit_boundary() const
  {
    return (0x10 - (_idx & 0xF));
  }

  bool
  on_32bit_boundary() const
  {
    return !(_idx & 0x1F);
  }

  u8
  bits_to_32bit_boundary() const
  {
    return (0x20 - (_idx & 0x1F));
  }

  bool
  on_64bit_boundary() const
  {
    return !(_idx & 0x3F);
  }

  u8
  bits_to_64bit_boundary() const
  {
    return (0x40 - (_idx & 0x3F));
  }
  
  void
  skip_to_8bit_boundary()
  {
    if(!on_8bit_boundary())
      skip(bits_to_8bit_boundary());
  }

  void
  zero_till_8bit_boundary()
  {
    if(!on_8bit_boundary())
      write(bits_to_8bit_boundary(),0);
  }

  void
  skip_to_16bit_boundary()
  {
    if(!on_16bit_boundary())
      skip(bits_to_16bit_boundary());
  }

  void
  zero_till_16bit_boundary()
  {
    if(!on_16bit_boundary())
      write(bits_to_16bit_boundary(),0);
  }

  void
  skip_to_32bit_boundary()
  {
    if(!on_32bit_boundary())
      skip(bits_to_32bit_boundary());
  }

  void
  zero_till_32bit_boundary()
  {
    if(!on_32bit_boundary())
      write(bits_to_32bit_boundary(),0);
  }

  void
  skip_to_64bit_boundary()
  {
    if(!on_64bit_boundary())
      skip(bits_to_64bit_boundary());
  }

  void
  zero_till_64bit_boundary()
  {
    if(!on_64bit_boundary())
      write(bits_to_64bit_boundary(),0);
  }

  u64
  tell() const
  {
    return _idx;
  }

  u64
  tell_bits() const
  {
    return tell();
  }

  u64
  tell_bytes() const
  {
    return ((_idx + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
  }

  u64
  tell_u32() const
  {
    return (tell_bytes() / 4);
  }

public:
  void
  write(u64 idx_,
        u64 bits_,
        u64 val_)
  {
    _maybe_resize(idx_ + bits_);

    for(u64 i = 0; i < bits_; i++)
      {
        u8 &d = (*_data)[idx_ >> 3];
        const int shift = (7 - (idx_ & 7));

        d = ((d & ~(1UL << shift)) | (((val_ >> (bits_ - i - 1)) & 1) << shift));
        idx_++;
      }
  }

  void
  write(u64 bits_,
        u64 val_)
  {
    write(_idx,bits_,val_);
    _idx += bits_;
  }

  void
  write(const std::vector<u8> &v_)
  {
    for(const u8 byte : v_)
      write(8,byte);
  }

public:
  u64
  read(const u64 idx_,
       const u64 bits_)
  {
    u64 val = 0;

    for(u64 i = idx_; i < (idx_ + bits_); i++)
      val = ((val << 1) | (((*_data)[i >> 3] >> (7 - (i & 7))) & 1));

    return val;
  }
};
