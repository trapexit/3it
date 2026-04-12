#pragma once

#include "CLI11.hpp"
#include "bits_and_bytes.hpp"
#include "span.hpp"

#include "types_ints.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <type_traits>


#if defined(__GNUC__) || defined(__clang__)
  #define BSR_HAS_BSWAP 1
  static inline u32 bsr_bswap32(u32 v) { return __builtin_bswap32(v); }
  static inline u64 bsr_bswap64(u64 v) { return __builtin_bswap64(v); }
#elif defined(_MSC_VER)
  #include <stdlib.h>
  #define BSR_HAS_BSWAP 1
  static inline u32 bsr_bswap32(u32 v) { return _byteswap_ulong(v);  }
  static inline u64 bsr_bswap64(u64 v) { return _byteswap_uint64(v); }
#else
  #define BSR_HAS_BSWAP 0
#endif

#if BSR_HAS_BSWAP
  static inline u32 bsr_load32_be(const u8 *p) { u32 v; std::memcpy(&v,p,4); return bsr_bswap32(v); }
  static inline u64 bsr_load64_be(const u8 *p) { u64 v; std::memcpy(&v,p,8); return bsr_bswap64(v); }
#endif


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
    return ((0x20 - (_idx & 0x1F)) & 0x1F);
  }

  bool
  on_64bit_boundary() const
  {
    return !(_idx & 0x3F);
  }

  u8
  bits_to_64bit_boundary() const
  {
    return ((0x40 - (_idx & 0x3F)) & 0x3F);
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
    assert((idx_ + bits_) <= _size);

    if(bits_ == 0)
      return 0;

    const u64 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];
    const u64 mask     = (bits_ == 64) ? ~0ULL : ((1ULL << bits_) - 1);

#if BSR_HAS_BSWAP
    if(bit_off + bits_ <= 64)
      {
        u64 acc = bsr_load64_be(src);
        return (acc >> (64 - bit_off - bits_)) & mask;
      }

    u64 acc = bsr_load64_be(src);
    acc &= (1ULL << (64 - bit_off)) - 1;
    const u8 remaining = bits_ - (64 - bit_off);
    return (acc << remaining) | (src[8] >> (8 - remaining));
#else
    if(!bit_off && !(bits_ & 7))
      {
        u64 val = 0;
        for(u64 i = 0; i < (bits_ >> 3); i++)
          val = (val << 8) | src[i];
        return val;
      }

    if(bit_off + bits_ <= 64)
      {
        u64 acc = 0;
        const u64 n = (bit_off + bits_ + 7) >> 3;
        for(u64 i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (n * 8 - bit_off - bits_)) & mask;
      }

    u64 acc = 0;
    for(u64 i = 0; i < 8; i++)
      acc = (acc << 8) | src[i];
    acc &= (1ULL << (64 - bit_off)) - 1;
    const u8 remaining = bits_ - (64 - bit_off);
    return (acc << remaining) | (src[8] >> (8 - remaining));
#endif
  }

  u64
  read(const u64 bits_)
  {
    u64 v;

    v = read(_idx,bits_);
    _idx += bits_;

    return v;
  }

private:
  template<u64 BITS>
  inline
  typename std::enable_if<(BITS >= 1 && BITS <= 57), u64>::type
  _read_fixed(const u64 idx_)
  {
    static constexpr u64 BYTES = (7 + BITS + 7) >> 3;
    static constexpr u64 MASK  = (1ULL << BITS) - 1;

    const u64 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];

    u64 acc = 0;
    for(u64 i = 0; i < BYTES; i++)
      acc = (acc << 8) | src[i];

    return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;
  }

  template<u64 BITS>
  inline
  typename std::enable_if<(BITS >= 58 && BITS <= 64), u64>::type
  _read_fixed(const u64 idx_)
  {
    static constexpr u64 MASK = (BITS == 64) ? ~0ULL : ((1ULL << BITS) - 1);

    const u64 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];

    if(bit_off + BITS <= 64)
      {
        u64 acc = 0;
        static constexpr u64 N = (BITS + 7) >> 3;
        for(u64 i = 0; i < N; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (N * 8 - bit_off - BITS)) & MASK;
      }

    u64 acc = 0;
    for(u64 i = 0; i < 8; i++)
      acc = (acc << 8) | src[i];
    acc &= (1ULL << (64 - bit_off)) - 1;
    const u8 remaining = BITS - (64 - bit_off);
    return (acc << remaining) | (src[8] >> (8 - remaining));
  }

public:
  template<u64 BITS>
  u64
  read(const u64 idx_)
  {
    assert((idx_ + BITS) <= _size);
    return _read_fixed<BITS>(idx_);
  }

  template<u64 BITS>
  u64
  read()
  {
    u64 v = read<BITS>(_idx);
    _idx += BITS;
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
    return ((0x08 - (_idx & 0x7)) & 0x7);
  }

  bool
  on_16bit_boundary() const
  {
    return !(_idx & 0xF);
  }

  u8
  bits_to_16bit_boundary() const
  {
    return ((0x10 - (_idx & 0xF)) & 0xF);
  }

  bool
  on_32bit_boundary() const
  {
    return !(_idx & 0x1F);
  }

  u8
  bits_to_32bit_boundary() const
  {
    return ((0x20 - (_idx & 0x1F)) & 0x1F);
  }

  bool
  on_64bit_boundary() const
  {
    return !(_idx & 0x3F);
  }

  u8
  bits_to_64bit_boundary() const
  {
    return ((0x40 - (_idx & 0x3F)) & 0x3F);
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

    if(bits_ == 0)
      return;

    u8 *dst      = &(*_data)[idx_ >> 3];
    u8  bit_off  = idx_ & 7;
    u64 remaining = bits_;

    if(bit_off)
      {
        const u8 avail = 8 - bit_off;
        const u8 take  = (remaining < avail) ? (u8)remaining : avail;
        const u8 shift = avail - take;
        const u8 mask  = (u8)(((1U << take) - 1) << shift);
        dst[0] = (dst[0] & ~mask) | (u8)(((val_ >> (remaining - take)) & ((1ULL << take) - 1)) << shift);
        dst++;
        remaining -= take;
      }

    while(remaining >= 8)
      {
        remaining -= 8;
        *dst++ = (u8)((val_ >> remaining) & 0xFF);
      }

    if(remaining)
      {
        const u8 shift = 8 - (u8)remaining;
        const u8 mask  = (u8)(((1U << remaining) - 1) << shift);
        dst[0] = (dst[0] & ~mask) | (u8)((val_ & ((1ULL << remaining) - 1)) << shift);
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
    if(_idx & 7)
      {
        for(const u8 byte : v_)
          write(8,byte);
        return;
      }

    _maybe_resize(_idx + (v_.size() * 8));
    std::memcpy(&(*_data)[_idx >> 3],v_.data(),v_.size());
    _idx += v_.size() * 8;
  }

public:
  u64
  read(const u64 idx_,
       const u64 bits_)
  {
    if(bits_ == 0)
      return 0;

    const u64 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &(*_data)[byte_idx];
    const u64 mask     = (bits_ == 64) ? ~0ULL : ((1ULL << bits_) - 1);

#if BSR_HAS_BSWAP
    if(bit_off + bits_ <= 64)
      {
        u64 acc = bsr_load64_be(src);
        return (acc >> (64 - bit_off - bits_)) & mask;
      }

    u64 acc = bsr_load64_be(src);
    acc &= (1ULL << (64 - bit_off)) - 1;
    const u8 remaining = bits_ - (64 - bit_off);
    return (acc << remaining) | (src[8] >> (8 - remaining));
#else
    if(!bit_off && !(bits_ & 7))
      {
        u64 val = 0;
        for(u64 i = 0; i < (bits_ >> 3); i++)
          val = (val << 8) | src[i];
        return val;
      }

    if(bit_off + bits_ <= 64)
      {
        u64 acc = 0;
        const u64 n = (bit_off + bits_ + 7) >> 3;
        for(u64 i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (n * 8 - bit_off - bits_)) & mask;
      }

    u64 acc = 0;
    for(u64 i = 0; i < 8; i++)
      acc = (acc << 8) | src[i];
    acc &= (1ULL << (64 - bit_off)) - 1;
    const u8 remaining = bits_ - (64 - bit_off);
    return (acc << remaining) | (src[8] >> (8 - remaining));
#endif
  }

private:
  template<u64 BITS>
  inline
  typename std::enable_if<(BITS >= 1 && BITS <= 57), u64>::type
  _read_fixed(const u64 idx_)
  {
    static constexpr u64 BYTES = (7 + BITS + 7) >> 3;
    static constexpr u64 MASK  = (1ULL << BITS) - 1;

    const u64 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &(*_data)[byte_idx];

    u64 acc = 0;
    for(u64 i = 0; i < BYTES; i++)
      acc = (acc << 8) | src[i];

    return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;
  }

  template<u64 BITS>
  inline
  typename std::enable_if<(BITS >= 58 && BITS <= 64), u64>::type
  _read_fixed(const u64 idx_)
  {
    static constexpr u64 MASK = (BITS == 64) ? ~0ULL : ((1ULL << BITS) - 1);

    const u64 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &(*_data)[byte_idx];

    if(bit_off + BITS <= 64)
      {
        u64 acc = 0;
        static constexpr u64 N = (BITS + 7) >> 3;
        for(u64 i = 0; i < N; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (N * 8 - bit_off - BITS)) & MASK;
      }

    u64 acc = 0;
    for(u64 i = 0; i < 8; i++)
      acc = (acc << 8) | src[i];
    acc &= (1ULL << (64 - bit_off)) - 1;
    const u8 remaining = BITS - (64 - bit_off);
    return (acc << remaining) | (src[8] >> (8 - remaining));
  }

public:
  template<u64 BITS>
  u64
  read(const u64 idx_)
  {
    return _read_fixed<BITS>(idx_);
  }
};


class BitStream
{
private:
  u64 _idx;
  u64 _size;
  std::vector<u8> _data;

public:
  BitStream()
    : _idx(0),
      _size(0),
      _data()
  {
  }

private:
  void
  _maybe_resize(const u64 size_in_bits_)
  {
    if(size_in_bits_ > (_data.size() * BITS_PER_BYTE))
      _data.resize((size_in_bits_ + BITS_PER_BYTE - 1) / BITS_PER_BYTE);
  }

public:
  const
  std::vector<u8>&
  data() const
  {
    return _data;
  }

  std::vector<u8>::iterator
  begin()
  {
    return _data.begin();
  }

  std::vector<u8>::const_iterator
  begin() const
  {
    return _data.begin();
  }

  std::vector<u8>::iterator
  end()
  {
    return _data.end();
  }

  std::vector<u8>::const_iterator
  end() const
  {
    return _data.end();
  }

  std::vector<u8>::iterator
  idx_end()
  {
    return (_data.begin() + ((_idx + 7) / 8));
  }

  std::vector<u8>::const_iterator
  idx_end() const
  {
    return (_data.begin() + ((_idx + 7) / 8));
  }

  std::vector<u8>::iterator
  size_end()
  {
    return (_data.begin() + ((_size + 7) / 8));
  }

  std::vector<u8>::const_iterator
  size_end() const
  {
    return (_data.begin() + ((_size + 7) / 8));
  }

public:
  void
  seek(const u64 idx_)
  {
    _maybe_resize(idx_);
    _idx = idx_;
    _size = std::max(_idx,_size);
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
    return ((0x08 - (_idx & 0x7)) & 0x7);
  }

  bool
  on_16bit_boundary() const
  {
    return !(_idx & 0xF);
  }

  u8
  bits_to_16bit_boundary() const
  {
    return ((0x10 - (_idx & 0xF)) & 0xF);
  }

  bool
  on_32bit_boundary() const
  {
    return !(_idx & 0x1F);
  }

  u8
  bits_to_32bit_boundary() const
  {
    return ((0x20 - (_idx & 0x1F)) & 0x1F);
  }

  bool
  on_64bit_boundary() const
  {
    return !(_idx & 0x3F);
  }

  u8
  bits_to_64bit_boundary() const
  {
    return ((0x40 - (_idx & 0x3F)) & 0x3F);
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
  tell_16bits() const
  {
    return (tell_bytes() / sizeof(u16));
  }

  u64
  tell_32bits_round_up() const
  {
    return ((tell_bytes() + (sizeof(u32)-1)) / sizeof(u32));
  }

  u64
  tell_64bits() const
  {
    return (tell_bytes() / sizeof(u64));
  }

public:
  u64
  size_bits() const
  {
    return _size;
  }

  u64
  size_8bits() const
  {
    return ((_size + 7) / 8);
  }

  u64
  size_32bits() const
  {
    return ((_size + 31) / 32);
  }

public:
  void
  shrink_to_idx()
  {
    u64 len_in_bytes;

    len_in_bytes = ((_idx + 7) / 8);

    _data.resize(len_in_bytes);
  }

  void
  shrink_to_size()
  {
    u64 len_in_bytes;

    len_in_bytes = ((_size + 7) / 8);

    _data.resize(len_in_bytes);
  }

  void
  set_size_bits(u64 size_)
  {
    _size = size_;
    _idx = std::min(_idx,_size);
    shrink_to_size();
  }

  void
  set_size_8bits(u64 size_)
  {
    set_size_bits(size_ * 8);
  }

  void
  set_size_32bits(u64 size_)
  {
    set_size_bits(size_ * 32);
  }

public:
  bool
  cmp(u64        idx_,
      BitStream &bs_,
      u64        bs_idx_,
      u64        length_)
  {
    for(u64 i = 0; i < length_; i++)
      {
        if(read_bit(idx_) != bs_.read_bit(bs_idx_))
          return false;

        idx_++;
        bs_idx_++;
      }

    return true;
  }

public:
  void
  write(u64 idx_,
        u64 bits_,
        u64 val_)
  {
    _maybe_resize(idx_ + bits_);

    if(bits_ == 0)
      return;

    u8 *dst      = &_data[idx_ >> 3];
    u8  bit_off  = idx_ & 7;
    u64 remaining = bits_;

    if(bit_off)
      {
        const u8 avail = 8 - bit_off;
        const u8 take  = (remaining < avail) ? (u8)remaining : avail;
        const u8 shift = avail - take;
        const u8 mask  = (u8)(((1U << take) - 1) << shift);
        dst[0] = (dst[0] & ~mask) | (u8)(((val_ >> (remaining - take)) & ((1ULL << take) - 1)) << shift);
        dst++;
        remaining -= take;
      }

    while(remaining >= 8)
      {
        remaining -= 8;
        *dst++ = (u8)((val_ >> remaining) & 0xFF);
      }

    if(remaining)
      {
        const u8 shift = 8 - (u8)remaining;
        const u8 mask  = (u8)(((1U << remaining) - 1) << shift);
        dst[0] = (dst[0] & ~mask) | (u8)((val_ & ((1ULL << remaining) - 1)) << shift);
      }
  }

  void
  write(u64 bits_,
        u64 val_)
  {
    write(_idx,bits_,val_);
    _idx += bits_;
    _size = std::max(_idx,_size);
  }

  void
  write(const std::vector<u8> &v_)
  {
    if(_idx & 7)
      {
        for(const u8 byte : v_)
          write(8,byte);
        return;
      }

    _maybe_resize(_idx + (v_.size() * 8));
    std::memcpy(&_data[_idx >> 3],v_.data(),v_.size());
    _idx += v_.size() * 8;
    _size = std::max(_idx,_size);
  }

public:
  u64
  read_bit(const u64 idx_)
  {
    return ((_data[idx_ >> 3] >> (7 - (idx_ & 7))) & 1);
  }

  u64
  read(const u64 idx_,
       const u64 bits_)
  {
    if(bits_ == 0)
      return 0;

    const u64 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];
    const u64 mask     = (bits_ == 64) ? ~0ULL : ((1ULL << bits_) - 1);

#if BSR_HAS_BSWAP
    if(bit_off + bits_ <= 64)
      {
        u64 acc = bsr_load64_be(src);
        return (acc >> (64 - bit_off - bits_)) & mask;
      }

    u64 acc = bsr_load64_be(src);
    acc &= (1ULL << (64 - bit_off)) - 1;
    const u8 remaining = bits_ - (64 - bit_off);
    return (acc << remaining) | (src[8] >> (8 - remaining));
#else
    if(!bit_off && !(bits_ & 7))
      {
        u64 val = 0;
        for(u64 i = 0; i < (bits_ >> 3); i++)
          val = (val << 8) | src[i];
        return val;
      }

    if(bit_off + bits_ <= 64)
      {
        u64 acc = 0;
        const u64 n = (bit_off + bits_ + 7) >> 3;
        for(u64 i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (n * 8 - bit_off - bits_)) & mask;
      }

    u64 acc = 0;
    for(u64 i = 0; i < 8; i++)
      acc = (acc << 8) | src[i];
    acc &= (1ULL << (64 - bit_off)) - 1;
    const u8 remaining = bits_ - (64 - bit_off);
    return (acc << remaining) | (src[8] >> (8 - remaining));
#endif
  }

  u64
  read(const u64 bits_)
  {
    u64 v;

    v = read(_idx,bits_);
    _idx += bits_;

    return v;
  }

private:
  template<u64 BITS>
  inline
  typename std::enable_if<(BITS >= 1 && BITS <= 57), u64>::type
  _read_fixed(const u64 idx_)
  {
    static constexpr u64 BYTES = (7 + BITS + 7) >> 3;
    static constexpr u64 MASK  = (1ULL << BITS) - 1;

    const u64 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];

    u64 acc = 0;
    for(u64 i = 0; i < BYTES; i++)
      acc = (acc << 8) | src[i];

    return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;
  }

  template<u64 BITS>
  inline
  typename std::enable_if<(BITS >= 58 && BITS <= 64), u64>::type
  _read_fixed(const u64 idx_)
  {
    static constexpr u64 MASK = (BITS == 64) ? ~0ULL : ((1ULL << BITS) - 1);

    const u64 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];

    if(bit_off + BITS <= 64)
      {
        u64 acc = 0;
        static constexpr u64 N = (BITS + 7) >> 3;
        for(u64 i = 0; i < N; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (N * 8 - bit_off - BITS)) & MASK;
      }

    u64 acc = 0;
    for(u64 i = 0; i < 8; i++)
      acc = (acc << 8) | src[i];
    acc &= (1ULL << (64 - bit_off)) - 1;
    const u8 remaining = BITS - (64 - bit_off);
    return (acc << remaining) | (src[8] >> (8 - remaining));
  }

public:
  template<u64 BITS>
  u64
  read(const u64 idx_)
  {
    return _read_fixed<BITS>(idx_);
  }

  template<u64 BITS>
  u64
  read()
  {
    u64 v = read<BITS>(_idx);
    _idx += BITS;
    return v;
  }
};


class BitStreamReader32
{
private:
  const u8 *_data;
  u32       _size;
  u32       _idx;

public:
  BitStreamReader32()
    : _data(NULL),
      _size(0),
      _idx(0)
  {
  }

  BitStreamReader32(const u8 *data_,
                    const u32  size_,
                    const u32  idx_ = 0)
  {
    reset(data_,size_,idx_);
  }

  BitStreamReader32(cspan<u8> &data_,
                    const u32   idx_ = 0)
  {
    reset(data_,idx_);
  }

public:
  void
  reset(const u8  *data_,
        const u32  size_,
        const u32  idx_ = 0)
  {
    _data = data_;
    _size = size_ * BITS_PER_BYTE;
    _idx  = idx_;
  }

  void
  reset(cspan<u8> &data_,
        const u32  idx_ = 0)
  {
    reset(data_.data(),
          (u32)data_.size(),
          idx_);
  }

public:
  void
  seek(const u32 idx_)
  {
    _idx = idx_;
  }

  void
  rewind()
  {
    seek(0);
  }

  void
  rewind(const u32 bits_)
  {
    seek(_idx - bits_);
  }

  void
  skip(const u32 bits_)
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
    return ((0x20 - (_idx & 0x1F)) & 0x1F);
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

  u32
  size() const
  {
    return _size;
  }

  u32
  tell() const
  {
    return _idx;
  }

  u32
  tell_bits() const
  {
    return tell();
  }

  u32
  tell_bytes() const
  {
    return ((_idx + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
  }

public:
  u32
  read(const u32 idx_,
       const u32 bits_)
  {
    assert((idx_ + bits_) <= _size);

    if(bits_ == 0)
      return 0;

    const u32 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];
    const u32 mask     = (bits_ == 32) ? ~0U : ((1U << bits_) - 1);

#if BSR_HAS_BSWAP
    if(bit_off + bits_ <= 32)
      {
        u32 acc = bsr_load32_be(src);
        return (acc >> (32 - bit_off - bits_)) & mask;
      }

    u32 acc = bsr_load32_be(src);
    acc &= (1U << (32 - bit_off)) - 1;
    const u8 remaining = bits_ - (32 - bit_off);
    return (acc << remaining) | (src[4] >> (8 - remaining));
#else
    if(!bit_off && !(bits_ & 7))
      {
        u32 val = 0;
        for(u32 i = 0; i < (bits_ >> 3); i++)
          val = (val << 8) | src[i];
        return val;
      }

    if(bit_off + bits_ <= 32)
      {
        u32 acc = 0;
        const u32 n = (bit_off + bits_ + 7) >> 3;
        for(u32 i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (n * 8 - bit_off - bits_)) & mask;
      }

    u32 acc = 0;
    for(u32 i = 0; i < 4; i++)
      acc = (acc << 8) | src[i];
    acc &= (1U << (32 - bit_off)) - 1;
    const u8 remaining = bits_ - (32 - bit_off);
    return (acc << remaining) | (src[4] >> (8 - remaining));
#endif
  }

  u32
  read(const u32 bits_)
  {
    u32 v;

    v = read(_idx,bits_);
    _idx += bits_;

    return v;
  }

private:
  template<u32 BITS>
  inline
  typename std::enable_if<(BITS >= 1 && BITS <= 25), u32>::type
  _read_fixed(const u32 idx_)
  {
    static constexpr u32 BYTES = (7 + BITS + 7) >> 3;
    static constexpr u32 MASK  = (1U << BITS) - 1;

    const u32 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];

    u32 acc = 0;
    for(u32 i = 0; i < BYTES; i++)
      acc = (acc << 8) | src[i];

    return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;
  }

  template<u32 BITS>
  inline
  typename std::enable_if<(BITS >= 26 && BITS <= 32), u32>::type
  _read_fixed(const u32 idx_)
  {
    static constexpr u32 MASK = (BITS == 32) ? ~0U : ((1U << BITS) - 1);

    const u32 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];

    if(bit_off + BITS <= 32)
      {
        u32 acc = 0;
        static constexpr u32 N = (BITS + 7) >> 3;
        for(u32 i = 0; i < N; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (N * 8 - bit_off - BITS)) & MASK;
      }

    u32 acc = 0;
    for(u32 i = 0; i < 4; i++)
      acc = (acc << 8) | src[i];
    acc &= (1U << (32 - bit_off)) - 1;
    const u8 remaining = BITS - (32 - bit_off);
    return (acc << remaining) | (src[4] >> (8 - remaining));
  }

public:
  template<u32 BITS>
  u32
  read(const u32 idx_)
  {
    assert((idx_ + BITS) <= _size);
    return _read_fixed<BITS>(idx_);
  }

  template<u32 BITS>
  u32
  read()
  {
    u32 v = read<BITS>(_idx);
    _idx += BITS;
    return v;
  }
};


class BitStreamWriter32
{
private:
  u32 _idx;
  std::vector<u8> *_data;

public:
  BitStreamWriter32()
    : _idx(0),
      _data(NULL)
  {
  }

  BitStreamWriter32(std::vector<u8> &data_,
                    const u32        idx_ = 0)
  {
    reset(data_,idx_);
  }

public:
  void
  reset(std::vector<u8> &data_,
        const u32        idx_ = 0)
  {
    _data = &data_;
    _idx  = idx_;
  }

  void
  reset(std::vector<u8> *data_,
        const u32        idx_ = 0)
  {
    reset(*data_,idx_);
  }

private:
  void
  _maybe_resize(const u32 size_in_bits_)
  {
    assert(_data != NULL);

    if(size_in_bits_ > (_data->size() * BITS_PER_BYTE))
      _data->resize((size_in_bits_ + BITS_PER_BYTE - 1) / BITS_PER_BYTE);
  }

public:
  void
  seek(const u32 idx_)
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
  rewind(const u32 bits_)
  {
    seek(_idx - bits_);
  }

  void
  skip(const u32 bits_)
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
    return ((0x08 - (_idx & 0x7)) & 0x7);
  }

  bool
  on_16bit_boundary() const
  {
    return !(_idx & 0xF);
  }

  u8
  bits_to_16bit_boundary() const
  {
    return ((0x10 - (_idx & 0xF)) & 0xF);
  }

  bool
  on_32bit_boundary() const
  {
    return !(_idx & 0x1F);
  }

  u8
  bits_to_32bit_boundary() const
  {
    return ((0x20 - (_idx & 0x1F)) & 0x1F);
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

  u32
  tell() const
  {
    return _idx;
  }

  u32
  tell_bits() const
  {
    return tell();
  }

  u32
  tell_bytes() const
  {
    return ((_idx + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
  }

  u32
  tell_u32() const
  {
    return (tell_bytes() / 4);
  }

public:
  void
  write(u32 idx_,
        u32 bits_,
        u32 val_)
  {
    _maybe_resize(idx_ + bits_);

    if(bits_ == 0)
      return;

    u8 *dst       = &(*_data)[idx_ >> 3];
    u8  bit_off   = idx_ & 7;
    u32 remaining = bits_;

    if(bit_off)
      {
        const u8  avail = 8 - bit_off;
        const u8  take  = (remaining < avail) ? (u8)remaining : avail;
        const u8  shift = avail - take;
        const u8  mask  = (u8)(((1U << take) - 1) << shift);
        dst[0] = (dst[0] & ~mask) | (u8)(((val_ >> (remaining - take)) & ((1U << take) - 1)) << shift);
        dst++;
        remaining -= take;
      }

    while(remaining >= 8)
      {
        remaining -= 8;
        *dst++ = (u8)((val_ >> remaining) & 0xFF);
      }

    if(remaining)
      {
        const u8 shift = 8 - (u8)remaining;
        const u8 mask  = (u8)(((1U << remaining) - 1) << shift);
        dst[0] = (dst[0] & ~mask) | (u8)((val_ & ((1U << remaining) - 1)) << shift);
      }
  }

  void
  write(u32 bits_,
        u32 val_)
  {
    write(_idx,bits_,val_);
    _idx += bits_;
  }

  void
  write(const std::vector<u8> &v_)
  {
    if(_idx & 7)
      {
        for(const u8 byte : v_)
          write((u32)8,(u32)byte);
        return;
      }

    _maybe_resize(_idx + (v_.size() * 8));
    std::memcpy(&(*_data)[_idx >> 3],v_.data(),v_.size());
    _idx += v_.size() * 8;
  }

public:
  u32
  read(const u32 idx_,
       const u32 bits_)
  {
    if(bits_ == 0)
      return 0;

    const u32 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &(*_data)[byte_idx];
    const u32 mask     = (bits_ == 32) ? ~0U : ((1U << bits_) - 1);

#if BSR_HAS_BSWAP
    if(bit_off + bits_ <= 32)
      {
        u32 acc = bsr_load32_be(src);
        return (acc >> (32 - bit_off - bits_)) & mask;
      }

    u32 acc = bsr_load32_be(src);
    acc &= (1U << (32 - bit_off)) - 1;
    const u8 remaining = bits_ - (32 - bit_off);
    return (acc << remaining) | (src[4] >> (8 - remaining));
#else
    if(!bit_off && !(bits_ & 7))
      {
        u32 val = 0;
        for(u32 i = 0; i < (bits_ >> 3); i++)
          val = (val << 8) | src[i];
        return val;
      }

    if(bit_off + bits_ <= 32)
      {
        u32 acc = 0;
        const u32 n = (bit_off + bits_ + 7) >> 3;
        for(u32 i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (n * 8 - bit_off - bits_)) & mask;
      }

    u32 acc = 0;
    for(u32 i = 0; i < 4; i++)
      acc = (acc << 8) | src[i];
    acc &= (1U << (32 - bit_off)) - 1;
    const u8 remaining = bits_ - (32 - bit_off);
    return (acc << remaining) | (src[4] >> (8 - remaining));
#endif
  }

private:
  template<u32 BITS>
  inline
  typename std::enable_if<(BITS >= 1 && BITS <= 25), u32>::type
  _read_fixed(const u32 idx_)
  {
    static constexpr u32 BYTES = (7 + BITS + 7) >> 3;
    static constexpr u32 MASK  = (1U << BITS) - 1;

    const u32 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &(*_data)[byte_idx];

    u32 acc = 0;
    for(u32 i = 0; i < BYTES; i++)
      acc = (acc << 8) | src[i];

    return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;
  }

  template<u32 BITS>
  inline
  typename std::enable_if<(BITS >= 26 && BITS <= 32), u32>::type
  _read_fixed(const u32 idx_)
  {
    static constexpr u32 MASK = (BITS == 32) ? ~0U : ((1U << BITS) - 1);

    const u32 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &(*_data)[byte_idx];

    if(bit_off + BITS <= 32)
      {
        u32 acc = 0;
        static constexpr u32 N = (BITS + 7) >> 3;
        for(u32 i = 0; i < N; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (N * 8 - bit_off - BITS)) & MASK;
      }

    u32 acc = 0;
    for(u32 i = 0; i < 4; i++)
      acc = (acc << 8) | src[i];
    acc &= (1U << (32 - bit_off)) - 1;
    const u8 remaining = BITS - (32 - bit_off);
    return (acc << remaining) | (src[4] >> (8 - remaining));
  }

public:
  template<u32 BITS>
  u32
  read(const u32 idx_)
  {
    return _read_fixed<BITS>(idx_);
  }
};


class BitStream32
{
private:
  u32 _idx;
  u32 _size;
  std::vector<u8> _data;

public:
  BitStream32()
    : _idx(0),
      _size(0),
      _data()
  {
  }

private:
  void
  _maybe_resize(const u32 size_in_bits_)
  {
    if(size_in_bits_ > (_data.size() * BITS_PER_BYTE))
      _data.resize((size_in_bits_ + BITS_PER_BYTE - 1) / BITS_PER_BYTE);
  }

public:
  const
  std::vector<u8>&
  data() const
  {
    return _data;
  }

  std::vector<u8>::iterator
  begin()
  {
    return _data.begin();
  }

  std::vector<u8>::const_iterator
  begin() const
  {
    return _data.begin();
  }

  std::vector<u8>::iterator
  end()
  {
    return _data.end();
  }

  std::vector<u8>::const_iterator
  end() const
  {
    return _data.end();
  }

  std::vector<u8>::iterator
  idx_end()
  {
    return (_data.begin() + ((_idx + 7) / 8));
  }

  std::vector<u8>::const_iterator
  idx_end() const
  {
    return (_data.begin() + ((_idx + 7) / 8));
  }

  std::vector<u8>::iterator
  size_end()
  {
    return (_data.begin() + ((_size + 7) / 8));
  }

  std::vector<u8>::const_iterator
  size_end() const
  {
    return (_data.begin() + ((_size + 7) / 8));
  }

public:
  void
  seek(const u32 idx_)
  {
    _maybe_resize(idx_);
    _idx = idx_;
    _size = std::max(_idx,_size);
  }

  void
  rewind()
  {
    seek(0);
  }

  void
  rewind(const u32 bits_)
  {
    seek(_idx - bits_);
  }

  void
  skip(const u32 bits_)
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
    return ((0x08 - (_idx & 0x7)) & 0x7);
  }

  bool
  on_16bit_boundary() const
  {
    return !(_idx & 0xF);
  }

  u8
  bits_to_16bit_boundary() const
  {
    return ((0x10 - (_idx & 0xF)) & 0xF);
  }

  bool
  on_32bit_boundary() const
  {
    return !(_idx & 0x1F);
  }

  u8
  bits_to_32bit_boundary() const
  {
    return ((0x20 - (_idx & 0x1F)) & 0x1F);
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

  u32
  tell() const
  {
    return _idx;
  }

  u32
  tell_bits() const
  {
    return tell();
  }

  u32
  tell_bytes() const
  {
    return ((_idx + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
  }

  u32
  tell_16bits() const
  {
    return (tell_bytes() / sizeof(u16));
  }

  u32
  tell_32bits_round_up() const
  {
    return ((tell_bytes() + (sizeof(u32)-1)) / sizeof(u32));
  }

public:
  u32
  size_bits() const
  {
    return _size;
  }

  u32
  size_8bits() const
  {
    return ((_size + 7) / 8);
  }

  u32
  size_32bits() const
  {
    return ((_size + 31) / 32);
  }

public:
  void
  shrink_to_idx()
  {
    u32 len_in_bytes;

    len_in_bytes = ((_idx + 7) / 8);

    _data.resize(len_in_bytes);
  }

  void
  shrink_to_size()
  {
    u32 len_in_bytes;

    len_in_bytes = ((_size + 7) / 8);

    _data.resize(len_in_bytes);
  }

  void
  set_size_bits(u32 size_)
  {
    _size = size_;
    _idx = std::min(_idx,_size);
    shrink_to_size();
  }

  void
  set_size_8bits(u32 size_)
  {
    set_size_bits(size_ * 8);
  }

  void
  set_size_32bits(u32 size_)
  {
    set_size_bits(size_ * 32);
  }

public:
  bool
  cmp(u32          idx_,
      BitStream32 &bs_,
      u32          bs_idx_,
      u32          length_)
  {
    for(u32 i = 0; i < length_; i++)
      {
        if(read_bit(idx_) != bs_.read_bit(bs_idx_))
          return false;

        idx_++;
        bs_idx_++;
      }

    return true;
  }

public:
  void
  write(u32 idx_,
        u32 bits_,
        u32 val_)
  {
    _maybe_resize(idx_ + bits_);

    if(bits_ == 0)
      return;

    u8 *dst       = &_data[idx_ >> 3];
    u8  bit_off   = idx_ & 7;
    u32 remaining = bits_;

    if(bit_off)
      {
        const u8  avail = 8 - bit_off;
        const u8  take  = (remaining < avail) ? (u8)remaining : avail;
        const u8  shift = avail - take;
        const u8  mask  = (u8)(((1U << take) - 1) << shift);
        dst[0] = (dst[0] & ~mask) | (u8)(((val_ >> (remaining - take)) & ((1U << take) - 1)) << shift);
        dst++;
        remaining -= take;
      }

    while(remaining >= 8)
      {
        remaining -= 8;
        *dst++ = (u8)((val_ >> remaining) & 0xFF);
      }

    if(remaining)
      {
        const u8 shift = 8 - (u8)remaining;
        const u8 mask  = (u8)(((1U << remaining) - 1) << shift);
        dst[0] = (dst[0] & ~mask) | (u8)((val_ & ((1U << remaining) - 1)) << shift);
      }
  }

  void
  write(u32 bits_,
        u32 val_)
  {
    write(_idx,bits_,val_);
    _idx += bits_;
    _size = std::max(_idx,_size);
  }

  void
  write(const std::vector<u8> &v_)
  {
    if(_idx & 7)
      {
        for(const u8 byte : v_)
          write((u32)8,(u32)byte);
        return;
      }

    _maybe_resize(_idx + (v_.size() * 8));
    std::memcpy(&_data[_idx >> 3],v_.data(),v_.size());
    _idx += v_.size() * 8;
    _size = std::max(_idx,_size);
  }

public:
  u32
  read_bit(const u32 idx_)
  {
    return ((_data[idx_ >> 3] >> (7 - (idx_ & 7))) & 1);
  }

  u32
  read(const u32 idx_,
       const u32 bits_)
  {
    if(bits_ == 0)
      return 0;

    const u32 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];
    const u32 mask     = (bits_ == 32) ? ~0U : ((1U << bits_) - 1);

#if BSR_HAS_BSWAP
    if(bit_off + bits_ <= 32)
      {
        u32 acc = bsr_load32_be(src);
        return (acc >> (32 - bit_off - bits_)) & mask;
      }

    u32 acc = bsr_load32_be(src);
    acc &= (1U << (32 - bit_off)) - 1;
    const u8 remaining = bits_ - (32 - bit_off);
    return (acc << remaining) | (src[4] >> (8 - remaining));
#else
    if(!bit_off && !(bits_ & 7))
      {
        u32 val = 0;
        for(u32 i = 0; i < (bits_ >> 3); i++)
          val = (val << 8) | src[i];
        return val;
      }

    if(bit_off + bits_ <= 32)
      {
        u32 acc = 0;
        const u32 n = (bit_off + bits_ + 7) >> 3;
        for(u32 i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (n * 8 - bit_off - bits_)) & mask;
      }

    u32 acc = 0;
    for(u32 i = 0; i < 4; i++)
      acc = (acc << 8) | src[i];
    acc &= (1U << (32 - bit_off)) - 1;
    const u8 remaining = bits_ - (32 - bit_off);
    return (acc << remaining) | (src[4] >> (8 - remaining));
#endif
  }

  u32
  read(const u32 bits_)
  {
    u32 v;

    v = read(_idx,bits_);
    _idx += bits_;

    return v;
  }

private:
  template<u32 BITS>
  inline
  typename std::enable_if<(BITS >= 1 && BITS <= 25), u32>::type
  _read_fixed(const u32 idx_)
  {
    static constexpr u32 BYTES = (7 + BITS + 7) >> 3;
    static constexpr u32 MASK  = (1U << BITS) - 1;

    const u32 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];

    u32 acc = 0;
    for(u32 i = 0; i < BYTES; i++)
      acc = (acc << 8) | src[i];

    return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;
  }

  template<u32 BITS>
  inline
  typename std::enable_if<(BITS >= 26 && BITS <= 32), u32>::type
  _read_fixed(const u32 idx_)
  {
    static constexpr u32 MASK = (BITS == 32) ? ~0U : ((1U << BITS) - 1);

    const u32 byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_data[byte_idx];

    if(bit_off + BITS <= 32)
      {
        u32 acc = 0;
        static constexpr u32 N = (BITS + 7) >> 3;
        for(u32 i = 0; i < N; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (N * 8 - bit_off - BITS)) & MASK;
      }

    u32 acc = 0;
    for(u32 i = 0; i < 4; i++)
      acc = (acc << 8) | src[i];
    acc &= (1U << (32 - bit_off)) - 1;
    const u8 remaining = BITS - (32 - bit_off);
    return (acc << remaining) | (src[4] >> (8 - remaining));
  }

public:
  template<u32 BITS>
  u32
  read(const u32 idx_)
  {
    return _read_fixed<BITS>(idx_);
  }

  template<u32 BITS>
  u32
  read()
  {
    u32 v = read<BITS>(_idx);
    _idx += BITS;
    return v;
  }
};
