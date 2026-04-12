#pragma once

#include "bits_and_bytes.hpp"
#include "types_ints.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <vector>


namespace bitstream_detail_11 {

/* void_t backport (with CWG 1558 workaround) */
template<typename...> struct void_t_helper { typedef void type; };
template<typename... Ts> using void_t = typename void_t_helper<Ts...>::type;

/* detect .resize(n) */
template<typename T, typename = void>
struct is_resizable : std::false_type {};

template<typename T>
struct is_resizable<T, void_t<decltype(std::declval<T&>().resize(std::declval<size_t>()))> >
  : std::true_type {};

/* storage traits */
template<typename Storage>
struct storage_traits
{
  typedef decltype(std::declval<Storage&>().data()) data_ptr_type;
  typedef typename std::remove_cv<
            typename std::remove_pointer<data_ptr_type>::type>::type element_type;
  static const size_t elem_size = sizeof(element_type);
};

/* bswap */
#if defined(__GNUC__) || defined(__clang__)
  static inline u32 bswap32(u32 v) { return __builtin_bswap32(v); }
  static inline u64 bswap64(u64 v) { return __builtin_bswap64(v); }
  #define BITSTREAM_T11_HAS_BSWAP 1
#elif defined(_MSC_VER)
  #include <stdlib.h>
  static inline u32 bswap32(u32 v) { return _byteswap_ulong(v);  }
  static inline u64 bswap64(u64 v) { return _byteswap_uint64(v); }
  #define BITSTREAM_T11_HAS_BSWAP 1
#else
  #define BITSTREAM_T11_HAS_BSWAP 0
#endif

#if BITSTREAM_T11_HAS_BSWAP
  static inline u32 load32_be(const u8 *p) { u32 v; std::memcpy(&v,p,4); return bswap32(v); }
  static inline u64 load64_be(const u8 *p) { u64 v; std::memcpy(&v,p,8); return bswap64(v); }
#endif

/* tag dispatch for idx width */
template<size_t N> struct idx_size_tag {};

} // namespace bitstream_detail_11


template<typename T = u8>
struct BitStreamSpan
{
  T  *_data;
  u64 _size;

  BitStreamSpan() : _data(NULL), _size(0) {}
  BitStreamSpan(T *d, u64 n) : _data(d), _size(n) {}

  T*       data()       { return _data; }
  const T* data() const { return _data; }
  u64      size() const { return _size; }
};


template<typename T = u8>
struct BitStreamConstSpan
{
  const T *_data;
  u64      _size;

  BitStreamConstSpan() : _data(NULL), _size(0) {}
  BitStreamConstSpan(const T *d, u64 n) : _data(d), _size(n) {}

  const T* data() const { return _data; }
  u64      size() const { return _size; }
};


template<typename Storage, typename Idx = u64>
class BitStreamT
{
  typedef bitstream_detail_11::storage_traits<Storage> traits;

  static const Idx IDX_BITS = sizeof(Idx) * 8;

private:
  Storage _storage;
  Idx     _idx;
  Idx     _size;

  const u8*
  _rptr() const
  {
    return reinterpret_cast<const u8*>(_storage.data());
  }

  /*
   * _wptr: get writable byte pointer.
   *
   * For types where data() returns non-const (vector, spans), use data().
   * For types where data() returns const (C++11 std::string), use &[0].
   *
   * Only instantiated when write methods are called.
   */
  template<typename S>
  static u8*
  _wptr_from(S &s, typename std::enable_if<
    !std::is_const<typename std::remove_pointer<
       decltype(std::declval<S&>().data())>::type>::value>::type* = 0)
  {
    return reinterpret_cast<u8*>(s.data());
  }

  template<typename S>
  static u8*
  _wptr_from(S &s, typename std::enable_if<
    std::is_const<typename std::remove_pointer<
       decltype(std::declval<S&>().data())>::type>::value>::type* = 0)
  {
    return reinterpret_cast<u8*>(&s[0]);
  }

  u8*
  _wptr()
  {
    return _wptr_from(_storage);
  }

  Idx
  _byte_capacity() const
  {
    return (Idx)(_storage.size() * traits::elem_size);
  }

  /* resize dispatch: resizable storage */
  template<typename S = Storage>
  typename std::enable_if<bitstream_detail_11::is_resizable<S>::value>::type
  _do_resize(Idx size_in_bits)
  {
    Idx bytes_needed = (size_in_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    if(bytes_needed > _byte_capacity())
      {
        size_t elems = (size_t)((bytes_needed + traits::elem_size - 1) / traits::elem_size);
        _storage.resize(elems);
      }
  }

  /* resize dispatch: non-resizable storage */
  template<typename S = Storage>
  typename std::enable_if<!bitstream_detail_11::is_resizable<S>::value>::type
  _do_resize(Idx size_in_bits)
  {
    Idx bytes_needed = (size_in_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    (void)bytes_needed;
    assert(bytes_needed <= _byte_capacity() && "BitStream: buffer overflow on non-resizable storage");
  }

  /* conditional resize: resizable */
  template<typename S = Storage>
  typename std::enable_if<bitstream_detail_11::is_resizable<S>::value>::type
  _maybe_resize(Idx size_in_bits)
  {
    _do_resize(size_in_bits);
  }

  /* conditional resize: non-resizable — no-op for seek/skip */
  template<typename S = Storage>
  typename std::enable_if<!bitstream_detail_11::is_resizable<S>::value>::type
  _maybe_resize(Idx)
  {
  }

  /* shrink dispatch */
  template<typename S = Storage>
  typename std::enable_if<bitstream_detail_11::is_resizable<S>::value>::type
  _do_shrink(Idx bits)
  {
    size_t elems = (size_t)((((bits) + 7) / 8 + traits::elem_size - 1) / traits::elem_size);
    _storage.resize(elems);
  }

  template<typename S = Storage>
  typename std::enable_if<!bitstream_detail_11::is_resizable<S>::value>::type
  _do_shrink(Idx)
  {
  }

  /* 32-bit read implementation */
  Idx
  _read_impl(Idx idx_,
             Idx bits_,
             bitstream_detail_11::idx_size_tag<4>) const
  {
    const Idx byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_rptr()[byte_idx];
    const Idx mask     = (bits_ == 32) ? ~(Idx)0 : (((Idx)1 << bits_) - 1);

#if BITSTREAM_T11_HAS_BSWAP
    if(bit_off + bits_ <= 32)
      {
        Idx acc = bitstream_detail_11::load32_be(src);
        return (acc >> (32 - bit_off - bits_)) & mask;
      }

    Idx acc = bitstream_detail_11::load32_be(src);
    acc &= ((Idx)1 << (32 - bit_off)) - 1;
    u8 remaining = (u8)(bits_ - (32 - bit_off));
    return (acc << remaining) | (src[4] >> (8 - remaining));
#else
    if(!bit_off && !(bits_ & 7))
      {
        Idx val = 0;
        for(Idx i = 0; i < (bits_ >> 3); i++)
          val = (val << 8) | src[i];
        return val;
      }

    if(bit_off + bits_ <= 32)
      {
        Idx acc = 0;
        Idx n = (bit_off + bits_ + 7) >> 3;
        for(Idx i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (n * 8 - bit_off - bits_)) & mask;
      }

    {
      Idx acc = 0;
      for(Idx i = 0; i < 4; i++)
        acc = (acc << 8) | src[i];
      acc &= ((Idx)1 << (32 - bit_off)) - 1;
      u8 remaining = (u8)(bits_ - (32 - bit_off));
      return (acc << remaining) | (src[4] >> (8 - remaining));
    }
#endif
  }

  /* 64-bit read implementation */
  Idx
  _read_impl(Idx idx_,
             Idx bits_,
             bitstream_detail_11::idx_size_tag<8>) const
  {
    const Idx byte_idx = idx_ >> 3;
    const u8  bit_off  = idx_ & 7;
    const u8 *src      = &_rptr()[byte_idx];
    const Idx mask     = (bits_ == 64) ? ~(Idx)0 : (((Idx)1 << bits_) - 1);

#if BITSTREAM_T11_HAS_BSWAP
    if(bit_off + bits_ <= 64)
      {
        Idx acc = bitstream_detail_11::load64_be(src);
        return (acc >> (64 - bit_off - bits_)) & mask;
      }

    Idx acc = bitstream_detail_11::load64_be(src);
    acc &= ((Idx)1 << (64 - bit_off)) - 1;
    u8 remaining = (u8)(bits_ - (64 - bit_off));
    return (acc << remaining) | (src[8] >> (8 - remaining));
#else
    if(!bit_off && !(bits_ & 7))
      {
        Idx val = 0;
        for(Idx i = 0; i < (bits_ >> 3); i++)
          val = (val << 8) | src[i];
        return val;
      }

    if(bit_off + bits_ <= 64)
      {
        Idx acc = 0;
        Idx n = (bit_off + bits_ + 7) >> 3;
        for(Idx i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (n * 8 - bit_off - bits_)) & mask;
      }

    {
      Idx acc = 0;
      for(Idx i = 0; i < 8; i++)
        acc = (acc << 8) | src[i];
      acc &= ((Idx)1 << (64 - bit_off)) - 1;
      u8 remaining = (u8)(bits_ - (64 - bit_off));
      return (acc << remaining) | (src[8] >> (8 - remaining));
    }
#endif
  }

  /* 32-bit _read_fixed: BITS 1-25 (always fits in 4 bytes) */
  template<Idx BITS>
  typename std::enable_if<(sizeof(Idx)==4 && BITS>=1 && BITS<=25), Idx>::type
  _read_fixed(Idx idx_) const
  {
    static const Idx BYTES = (7 + BITS + 7) >> 3;
    static const Idx MASK  = ((Idx)1 << BITS) - 1;

    const u8 *src = &_rptr()[idx_ >> 3];
    const u8  bit_off = idx_ & 7;

    Idx acc = 0;
    for(Idx i = 0; i < BYTES; i++)
      acc = (acc << 8) | src[i];

    return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;
  }

  /* 32-bit _read_fixed: BITS 26-32 (may span 5 bytes) */
  template<Idx BITS>
  typename std::enable_if<(sizeof(Idx)==4 && BITS>=26 && BITS<=32), Idx>::type
  _read_fixed(Idx idx_) const
  {
    static const Idx MASK = (BITS == 32) ? ~(Idx)0 : (((Idx)1 << BITS) - 1);

    const u8 *src = &_rptr()[idx_ >> 3];
    const u8  bit_off = idx_ & 7;

    if(bit_off + BITS <= 32)
      {
        static const Idx N = (BITS + 7) >> 3;
        Idx acc = 0;
        for(Idx i = 0; i < N; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (N * 8 - bit_off - BITS)) & MASK;
      }

    Idx acc = 0;
    for(Idx i = 0; i < 4; i++)
      acc = (acc << 8) | src[i];
    acc &= ((Idx)1 << (32 - bit_off)) - 1;
    u8 remaining = (u8)(BITS - (32 - bit_off));
    return (acc << remaining) | (src[4] >> (8 - remaining));
  }

  /* 64-bit _read_fixed: BITS 1-57 (always fits in 8 bytes) */
  template<Idx BITS>
  typename std::enable_if<(sizeof(Idx)==8 && BITS>=1 && BITS<=57), Idx>::type
  _read_fixed(Idx idx_) const
  {
    static const Idx BYTES = (7 + BITS + 7) >> 3;
    static const Idx MASK  = ((Idx)1 << BITS) - 1;

    const u8 *src = &_rptr()[idx_ >> 3];
    const u8  bit_off = idx_ & 7;

    Idx acc = 0;
    for(Idx i = 0; i < BYTES; i++)
      acc = (acc << 8) | src[i];

    return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;
  }

  /* 64-bit _read_fixed: BITS 58-64 (may span 9 bytes) */
  template<Idx BITS>
  typename std::enable_if<(sizeof(Idx)==8 && BITS>=58 && BITS<=64), Idx>::type
  _read_fixed(Idx idx_) const
  {
    static const Idx MASK = (BITS == 64) ? ~(Idx)0 : (((Idx)1 << BITS) - 1);

    const u8 *src = &_rptr()[idx_ >> 3];
    const u8  bit_off = idx_ & 7;

    if(bit_off + BITS <= 64)
      {
        static const Idx N = (BITS + 7) >> 3;
        Idx acc = 0;
        for(Idx i = 0; i < N; i++)
          acc = (acc << 8) | src[i];
        return (acc >> (N * 8 - bit_off - BITS)) & MASK;
      }

    Idx acc = 0;
    for(Idx i = 0; i < 8; i++)
      acc = (acc << 8) | src[i];
    acc &= ((Idx)1 << (64 - bit_off)) - 1;
    u8 remaining = (u8)(BITS - (64 - bit_off));
    return (acc << remaining) | (src[8] >> (8 - remaining));
  }

public:
  BitStreamT()
    : _storage(),
      _idx(0),
      _size(0)
  {
  }

  explicit
  BitStreamT(Storage storage_)
    : _storage(storage_),
      _idx(0),
      _size((Idx)(_storage.size() * traits::elem_size * BITS_PER_BYTE))
  {
  }

public:
  const Storage& storage() const { return _storage; }
  Storage&       storage()       { return _storage; }

  const u8* data() const { return _rptr(); }

public:
  void
  seek(Idx idx_)
  {
    _maybe_resize(idx_);
    _idx = idx_;
    _size = std::max(_idx,_size);
  }

  void rewind()           { _idx = 0; }
  void rewind(Idx bits_)  { _idx -= bits_; }
  void skip(Idx bits_)    { seek(_idx + bits_); }

  bool on_8bit_boundary()  const { return !(_idx & 0x7);  }
  bool on_16bit_boundary() const { return !(_idx & 0xF);  }
  bool on_32bit_boundary() const { return !(_idx & 0x1F); }
  bool on_64bit_boundary() const { return !(_idx & 0x3F); }

  u8 bits_to_8bit_boundary()  const { return ((0x08 - (_idx & 0x7))  & 0x7);  }
  u8 bits_to_16bit_boundary() const { return ((0x10 - (_idx & 0xF))  & 0xF);  }
  u8 bits_to_32bit_boundary() const { return ((0x20 - (_idx & 0x1F)) & 0x1F); }
  u8 bits_to_64bit_boundary() const { return ((0x40 - (_idx & 0x3F)) & 0x3F); }

  void skip_to_8bit_boundary()  { if(!on_8bit_boundary())  skip(bits_to_8bit_boundary()); }
  void skip_to_16bit_boundary() { if(!on_16bit_boundary()) skip(bits_to_16bit_boundary()); }
  void skip_to_32bit_boundary() { if(!on_32bit_boundary()) skip(bits_to_32bit_boundary()); }
  void skip_to_64bit_boundary() { if(!on_64bit_boundary()) skip(bits_to_64bit_boundary()); }

  void zero_till_8bit_boundary()  { if(!on_8bit_boundary())  write(bits_to_8bit_boundary(),0); }
  void zero_till_16bit_boundary() { if(!on_16bit_boundary()) write(bits_to_16bit_boundary(),0); }
  void zero_till_32bit_boundary() { if(!on_32bit_boundary()) write(bits_to_32bit_boundary(),0); }
  void zero_till_64bit_boundary() { if(!on_64bit_boundary()) write(bits_to_64bit_boundary(),0); }

  Idx tell()       const { return _idx; }
  Idx tell_bits()  const { return _idx; }
  Idx tell_bytes() const { return ((_idx + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE); }

  Idx size_bits()   const { return _size; }
  Idx size_8bits()  const { return ((_size + 7) / 8); }
  Idx size_32bits() const { return ((_size + 31) / 32); }

  void shrink_to_idx()  { _do_shrink(_idx); }
  void shrink_to_size() { _do_shrink(_size); }

  void
  set_size_bits(Idx size_)
  {
    _size = size_;
    _idx  = std::min(_idx,_size);
    shrink_to_size();
  }

  void set_size_8bits(Idx s)  { set_size_bits(s * 8);  }
  void set_size_32bits(Idx s) { set_size_bits(s * 32); }

public:
  Idx
  read_bit(Idx idx_) const
  {
    return ((_rptr()[idx_ >> 3] >> (7 - (idx_ & 7))) & 1);
  }

  Idx
  read(Idx idx_,
       Idx bits_) const
  {
    if(bits_ == 0)
      return 0;
    return _read_impl(idx_,bits_,bitstream_detail_11::idx_size_tag<sizeof(Idx)>());
  }

  Idx
  read(Idx bits_)
  {
    Idx v = read(_idx,bits_);
    _idx += bits_;
    return v;
  }

  template<Idx BITS>
  Idx
  read(Idx idx_) const
  {
    return _read_fixed<BITS>(idx_);
  }

  template<Idx BITS>
  Idx
  read()
  {
    Idx v = _read_fixed<BITS>(_idx);
    _idx += BITS;
    return v;
  }

public:
  void
  write(Idx idx_,
        Idx bits_,
        Idx val_)
  {
    _do_resize(idx_ + bits_);

    if(bits_ == 0)
      return;

    u8 *dst      = &_wptr()[idx_ >> 3];
    u8  bit_off  = idx_ & 7;
    Idx remaining = bits_;

    if(bit_off)
      {
        const u8 avail = 8 - bit_off;
        const u8 take  = (remaining < avail) ? (u8)remaining : avail;
        const u8 shift = avail - take;
        const u8 mask  = (u8)(((1U << take) - 1) << shift);
        dst[0] = (dst[0] & ~mask) | (u8)(((val_ >> (remaining - take)) & (((Idx)1 << take) - 1)) << shift);
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
        dst[0] = (dst[0] & ~mask) | (u8)((val_ & (((Idx)1 << remaining) - 1)) << shift);
      }
  }

  void
  write(Idx bits_,
        Idx val_)
  {
    write(_idx,bits_,val_);
    _idx += bits_;
    _size = std::max(_idx,_size);
  }

  void
  write_bytes(const u8 *src_,
              Idx       count_)
  {
    if(!(_idx & 7))
      {
        _do_resize(_idx + (count_ * 8));
        std::memcpy(&_wptr()[_idx >> 3],src_,(size_t)count_);
        _idx += count_ * 8;
        _size = std::max(_idx,_size);
      }
    else
      {
        for(Idx i = 0; i < count_; i++)
          write(8,(Idx)src_[i]);
      }
  }

public:
  template<typename OtherStorage>
  bool
  cmp(Idx                                idx_,
      const BitStreamT<OtherStorage,Idx> &other_,
      Idx                                other_idx_,
      Idx                                length_) const
  {
    for(Idx i = 0; i < length_; i++)
      {
        if(read_bit(idx_) != other_.read_bit(other_idx_))
          return false;
        idx_++;
        other_idx_++;
      }
    return true;
  }
};


typedef BitStreamT<std::vector<u8> >         BitStream;
typedef BitStreamT<BitStreamSpan<u8> >       BitStreamView;
typedef BitStreamT<BitStreamConstSpan<u8> >  BitStreamReader;

typedef BitStreamT<std::vector<u8>, u32>         BitStream32;
typedef BitStreamT<BitStreamSpan<u8>, u32>       BitStreamView32;
typedef BitStreamT<BitStreamConstSpan<u8>, u32>  BitStreamReader32;
