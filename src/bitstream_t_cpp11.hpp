#pragma once

#include "bits_and_bytes.hpp"
#include "types_ints.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <utility>
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

/* tag dispatch for field width */
template<size_t N> struct word_size_tag {};

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


typedef void* (*bitstream_realloc_fn)(void *ptr, size_t new_bytes, void *ctx);

static inline void*
bitstream_stdlib_realloc(void *ptr, size_t new_bytes, void *ctx)
{
  (void)ctx;
  if(new_bytes == 0)
    {
      std::free(ptr);
      return NULL;
    }
  return std::realloc(ptr, new_bytes);
}


template<typename T = u8>
class BitStreamReallocStorage
{
  static_assert(!std::is_const<T>::value,
                "BitStreamReallocStorage requires a non-const element type");
  static_assert(std::is_trivially_copyable<T>::value,
                "BitStreamReallocStorage requires a trivially copyable element type");

private:
  T                   *_data;
  size_t               _size;
  bitstream_realloc_fn _realloc_fn;
  void                *_realloc_ctx;

  void
  _free()
  {
    if(_data)
      _realloc_fn(_data, 0, _realloc_ctx);
    _data = NULL;
    _size = 0;
  }

public:
  BitStreamReallocStorage()
    : BitStreamReallocStorage(NULL, 0, bitstream_stdlib_realloc, NULL)
  {
  }

  explicit
  BitStreamReallocStorage(bitstream_realloc_fn realloc_fn_,
                          void                *realloc_ctx_ = NULL)
    : BitStreamReallocStorage(NULL, 0, realloc_fn_, realloc_ctx_)
  {
  }

  explicit
  BitStreamReallocStorage(size_t               elems_,
                          bitstream_realloc_fn realloc_fn_ = bitstream_stdlib_realloc,
                          void                *realloc_ctx_ = NULL)
    : BitStreamReallocStorage(NULL, 0, realloc_fn_, realloc_ctx_)
  {
    resize(elems_);
  }

  BitStreamReallocStorage(T                   *data_,
                          size_t               elems_,
                          bitstream_realloc_fn realloc_fn_ = bitstream_stdlib_realloc,
                          void                *realloc_ctx_ = NULL)
    : _data(data_),
      _size(elems_),
      _realloc_fn(realloc_fn_ ? realloc_fn_ : bitstream_stdlib_realloc),
      _realloc_ctx(realloc_ctx_)
  {
  }

  ~BitStreamReallocStorage()
  {
    _free();
  }

  BitStreamReallocStorage(const BitStreamReallocStorage&) = delete;
  BitStreamReallocStorage& operator=(const BitStreamReallocStorage&) = delete;

  BitStreamReallocStorage(BitStreamReallocStorage &&other) noexcept
    : _data(other._data),
      _size(other._size),
      _realloc_fn(other._realloc_fn),
      _realloc_ctx(other._realloc_ctx)
  {
    other._data = NULL;
    other._size = 0;
  }

  BitStreamReallocStorage&
  operator=(BitStreamReallocStorage &&other) noexcept
  {
    if(this != &other)
      {
        _free();
        _data        = other._data;
        _size        = other._size;
        _realloc_fn  = other._realloc_fn;
        _realloc_ctx = other._realloc_ctx;
        other._data = NULL;
        other._size = 0;
      }
    return *this;
  }

  T*
  data()
  {
    return _data;
  }

  const T*
  data() const
  {
    return _data;
  }

  size_t
  size() const
  {
    return _size;
  }

  bitstream_realloc_fn
  realloc_fn() const
  {
    return _realloc_fn;
  }

  void*
  realloc_ctx() const
  {
    return _realloc_ctx;
  }

  void
  resize(size_t elems_)
  {
    if(elems_ == _size)
      return;

    if(elems_ == 0)
      {
        _free();
        return;
      }

    void *p = _realloc_fn(_data, elems_ * sizeof(T), _realloc_ctx);
    assert(p != NULL && "BitStreamReallocStorage: realloc_fn returned NULL");

    T *new_data = static_cast<T*>(p);
    if(elems_ > _size)
      std::memset(new_data + _size, 0, (elems_ - _size) * sizeof(T));

    _data = new_data;
    _size = elems_;
  }

  T*
  release()
  {
    T *data_ = _data;
    _data = NULL;
    _size = 0;
    return data_;
  }
};


template<typename Storage, typename Word = u64>
class BitStreamT
{
  static_assert(std::is_same<Word,u32>::value || std::is_same<Word,u64>::value,
                "Word must be u32 or u64");

  typedef bitstream_detail_11::storage_traits<Storage> traits;

  static const u64 WORD_BITS = sizeof(Word) * 8;

private:
  Storage _storage;
  u64     _idx;
  u64     _size;

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

  u64
  _byte_capacity() const
  {
    return (u64)(_storage.size() * traits::elem_size);
  }

  /* resize dispatch: resizable storage */
  template<typename S = Storage>
  typename std::enable_if<bitstream_detail_11::is_resizable<S>::value>::type
  _do_resize(u64 size_in_bits)
  {
    u64 bytes_needed = (size_in_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    if(bytes_needed > _byte_capacity())
      {
        size_t elems = (size_t)((bytes_needed + traits::elem_size - 1) / traits::elem_size);
        _storage.resize(elems);
      }
  }

  /* resize dispatch: non-resizable storage */
  template<typename S = Storage>
  typename std::enable_if<!bitstream_detail_11::is_resizable<S>::value>::type
  _do_resize(u64 size_in_bits)
  {
    u64 bytes_needed = (size_in_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    (void)bytes_needed;
    assert(bytes_needed <= _byte_capacity() && "BitStream: buffer overflow on non-resizable storage");
  }

  /* conditional resize: resizable */
  template<typename S = Storage>
  typename std::enable_if<bitstream_detail_11::is_resizable<S>::value>::type
  _maybe_resize(u64 size_in_bits)
  {
    _do_resize(size_in_bits);
  }

  /* conditional resize: non-resizable — no-op for seek/skip */
  template<typename S = Storage>
  typename std::enable_if<!bitstream_detail_11::is_resizable<S>::value>::type
  _maybe_resize(u64)
  {
  }

  /* shrink dispatch */
  template<typename S = Storage>
  typename std::enable_if<bitstream_detail_11::is_resizable<S>::value>::type
  _do_shrink(u64 bits)
  {
    size_t elems = (size_t)((((bits) + 7) / 8 + traits::elem_size - 1) / traits::elem_size);
    _storage.resize(elems);
  }

  template<typename S = Storage>
  typename std::enable_if<!bitstream_detail_11::is_resizable<S>::value>::type
  _do_shrink(u64)
  {
  }

  /* 32-bit read implementation */
  Word
  _read_impl(u64 idx_,
             Word bits_,
             bitstream_detail_11::word_size_tag<4>) const
  {
    const u64 byte_idx = idx_ >> 3;
    const u8  bit_off  = (u8)(idx_ & 7);
    const u8 *src      = &_rptr()[byte_idx];
    const Word mask    = (bits_ == 32) ? ~(Word)0 : (((Word)1 << bits_) - 1);

#if BITSTREAM_T11_HAS_BSWAP
    if(bit_off + bits_ <= 32)
      {
        Word acc = bitstream_detail_11::load32_be(src);
        return (acc >> (32 - bit_off - bits_)) & mask;
      }

    Word acc = bitstream_detail_11::load32_be(src);
    acc &= ((Word)1 << (32 - bit_off)) - 1;
    u8 remaining = (u8)(bits_ - (32 - bit_off));
    return (Word)((acc << remaining) | (src[4] >> (8 - remaining)));
#else
    if(!bit_off && !(bits_ & 7))
      {
        Word val = 0;
        for(u64 i = 0; i < (bits_ >> 3); i++)
          val = (Word)((val << 8) | src[i]);
        return val;
      }

    if(bit_off + bits_ <= 32)
      {
        Word acc = 0;
        u64 n = (bit_off + bits_ + 7) >> 3;
        for(u64 i = 0; i < n; i++)
          acc = (Word)((acc << 8) | src[i]);
        return (acc >> (n * 8 - bit_off - bits_)) & mask;
      }

    {
      Word acc = 0;
      for(u64 i = 0; i < 4; i++)
        acc = (Word)((acc << 8) | src[i]);
      acc &= ((Word)1 << (32 - bit_off)) - 1;
      u8 remaining = (u8)(bits_ - (32 - bit_off));
      return (Word)((acc << remaining) | (src[4] >> (8 - remaining)));
    }
#endif
  }

  /* 64-bit read implementation */
  Word
  _read_impl(u64 idx_,
             Word bits_,
             bitstream_detail_11::word_size_tag<8>) const
  {
    const u64 byte_idx = idx_ >> 3;
    const u8  bit_off  = (u8)(idx_ & 7);
    const u8 *src      = &_rptr()[byte_idx];
    const Word mask    = (bits_ == 64) ? ~(Word)0 : (((Word)1 << bits_) - 1);

#if BITSTREAM_T11_HAS_BSWAP
    if(bit_off + bits_ <= 64)
      {
        Word acc = bitstream_detail_11::load64_be(src);
        return (acc >> (64 - bit_off - bits_)) & mask;
      }

    Word acc = bitstream_detail_11::load64_be(src);
    acc &= ((Word)1 << (64 - bit_off)) - 1;
    u8 remaining = (u8)(bits_ - (64 - bit_off));
    return (Word)((acc << remaining) | (src[8] >> (8 - remaining)));
#else
    if(!bit_off && !(bits_ & 7))
      {
        Word val = 0;
        for(u64 i = 0; i < (bits_ >> 3); i++)
          val = (Word)((val << 8) | src[i]);
        return val;
      }

    if(bit_off + bits_ <= 64)
      {
        Word acc = 0;
        u64 n = (bit_off + bits_ + 7) >> 3;
        for(u64 i = 0; i < n; i++)
          acc = (Word)((acc << 8) | src[i]);
        return (acc >> (n * 8 - bit_off - bits_)) & mask;
      }

    {
      Word acc = 0;
      for(u64 i = 0; i < 8; i++)
        acc = (Word)((acc << 8) | src[i]);
      acc &= ((Word)1 << (64 - bit_off)) - 1;
      u8 remaining = (u8)(bits_ - (64 - bit_off));
      return (Word)((acc << remaining) | (src[8] >> (8 - remaining)));
    }
#endif
  }

  /* 32-bit _read_fixed: BITS 1-25 (always fits in 4 bytes) */
  template<u64 BITS>
  typename std::enable_if<(sizeof(Word)==4 && BITS>=1 && BITS<=25), Word>::type
  _read_fixed(u64 idx_) const
  {
    static const u64  BYTES = (7 + BITS + 7) >> 3;
    static const Word MASK  = ((Word)1 << BITS) - 1;

    const u8 *src = &_rptr()[idx_ >> 3];
    const u8  bit_off = (u8)(idx_ & 7);

    Word acc = 0;
    for(u64 i = 0; i < BYTES; i++)
      acc = (Word)((acc << 8) | src[i]);

    return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;
  }

  /* 32-bit _read_fixed: BITS 26-32 (may span 5 bytes) */
  template<u64 BITS>
  typename std::enable_if<(sizeof(Word)==4 && BITS>=26 && BITS<=32), Word>::type
  _read_fixed(u64 idx_) const
  {
    static const Word MASK = (BITS == 32) ? ~(Word)0 : (((Word)1 << BITS) - 1);

    const u8 *src = &_rptr()[idx_ >> 3];
    const u8  bit_off = (u8)(idx_ & 7);

    if(bit_off + BITS <= 32)
      {
        static const u64 N = (BITS + 7) >> 3;
        Word acc = 0;
        for(u64 i = 0; i < N; i++)
          acc = (Word)((acc << 8) | src[i]);
        return (acc >> (N * 8 - bit_off - BITS)) & MASK;
      }

    Word acc = 0;
    for(u64 i = 0; i < 4; i++)
      acc = (Word)((acc << 8) | src[i]);
    acc &= ((Word)1 << (32 - bit_off)) - 1;
    u8 remaining = (u8)(BITS - (32 - bit_off));
    return (Word)((acc << remaining) | (src[4] >> (8 - remaining)));
  }

  /* 64-bit _read_fixed: BITS 1-57 (always fits in 8 bytes) */
  template<u64 BITS>
  typename std::enable_if<(sizeof(Word)==8 && BITS>=1 && BITS<=57), Word>::type
  _read_fixed(u64 idx_) const
  {
    static const u64  BYTES = (7 + BITS + 7) >> 3;
    static const Word MASK  = ((Word)1 << BITS) - 1;

    const u8 *src = &_rptr()[idx_ >> 3];
    const u8  bit_off = (u8)(idx_ & 7);

    Word acc = 0;
    for(u64 i = 0; i < BYTES; i++)
      acc = (Word)((acc << 8) | src[i]);

    return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;
  }

  /* 64-bit _read_fixed: BITS 58-64 (may span 9 bytes) */
  template<u64 BITS>
  typename std::enable_if<(sizeof(Word)==8 && BITS>=58 && BITS<=64), Word>::type
  _read_fixed(u64 idx_) const
  {
    static const Word MASK = (BITS == 64) ? ~(Word)0 : (((Word)1 << BITS) - 1);

    const u8 *src = &_rptr()[idx_ >> 3];
    const u8  bit_off = (u8)(idx_ & 7);

    if(bit_off + BITS <= 64)
      {
        static const u64 N = (BITS + 7) >> 3;
        Word acc = 0;
        for(u64 i = 0; i < N; i++)
          acc = (Word)((acc << 8) | src[i]);
        return (acc >> (N * 8 - bit_off - BITS)) & MASK;
      }

    Word acc = 0;
    for(u64 i = 0; i < 8; i++)
      acc = (Word)((acc << 8) | src[i]);
    acc &= ((Word)1 << (64 - bit_off)) - 1;
    u8 remaining = (u8)(BITS - (64 - bit_off));
    return (Word)((acc << remaining) | (src[8] >> (8 - remaining)));
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
    : _storage(std::move(storage_)),
      _idx(0),
      _size((u64)(_storage.size() * traits::elem_size * BITS_PER_BYTE))
  {
  }

public:
  const Storage& storage() const { return _storage; }
  Storage&       storage()       { return _storage; }

  const u8* data() const { return _rptr(); }

public:
  void
  seek(u64 idx_)
  {
    _maybe_resize(idx_);
    _idx = idx_;
    _size = std::max(_idx,_size);
  }

  void rewind()           { _idx = 0; }
  void rewind(u64 bits_)  { _idx -= bits_; }
  void skip(u64 bits_)    { seek(_idx + bits_); }

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

  u64 tell()       const { return _idx; }
  u64 tell_bits()  const { return _idx; }
  u64 tell_bytes() const { return ((_idx + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE); }

  u64 size_bits()   const { return _size; }
  u64 size_8bits()  const { return ((_size + 7) / 8); }
  u64 size_32bits() const { return ((_size + 31) / 32); }

  void shrink_to_idx()  { _do_shrink(_idx); }
  void shrink_to_size() { _do_shrink(_size); }

  void
  set_size_bits(u64 size_)
  {
    _size = size_;
    _idx  = std::min(_idx,_size);
    shrink_to_size();
  }

  void set_size_8bits(u64 s)  { set_size_bits(s * 8);  }
  void set_size_32bits(u64 s) { set_size_bits(s * 32); }

public:
  Word
  read_bit(u64 idx_) const
  {
    return (Word)((_rptr()[idx_ >> 3] >> (7 - (idx_ & 7))) & 1);
  }

  Word
  read(u64  idx_,
       Word bits_) const
  {
    assert(bits_ <= WORD_BITS && "BitStream: field width exceeds word size");

    if(bits_ == 0)
      return 0;
    return _read_impl(idx_,bits_,bitstream_detail_11::word_size_tag<sizeof(Word)>());
  }

  Word
  read(Word bits_)
  {
    Word v = read(_idx,bits_);
    _idx += bits_;
    return v;
  }

  template<u64 BITS>
  Word
  read(u64 idx_) const
  {
    return _read_fixed<BITS>(idx_);
  }

  template<u64 BITS>
  Word
  read()
  {
    Word v = _read_fixed<BITS>(_idx);
    _idx += BITS;
    return v;
  }

public:
  void
  write(u64  idx_,
        Word bits_,
        Word val_)
  {
    assert(bits_ <= WORD_BITS && "BitStream: field width exceeds word size");

    _do_resize(idx_ + bits_);

    if(bits_ == 0)
      return;

    u8 *dst      = &_wptr()[idx_ >> 3];
    u8  bit_off  = (u8)(idx_ & 7);
    Word remaining = bits_;

    if(bit_off)
      {
        const u8 avail = 8 - bit_off;
        const u8 take  = (remaining < avail) ? (u8)remaining : avail;
        const u8 shift = avail - take;
        const u8 mask  = (u8)(((1U << take) - 1) << shift);
        dst[0] = (dst[0] & ~mask) | (u8)(((val_ >> (remaining - take)) & (((Word)1 << take) - 1)) << shift);
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
        dst[0] = (dst[0] & ~mask) | (u8)((val_ & (((Word)1 << remaining) - 1)) << shift);
      }
  }

  void
  write(Word bits_,
        Word val_)
  {
    write(_idx,bits_,val_);
    _idx += bits_;
    _size = std::max(_idx,_size);
  }

  void
  write_bytes(const u8 *src_,
              u64       count_)
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
        for(u64 i = 0; i < count_; i++)
          write((Word)8,(Word)src_[i]);
      }
  }

public:
  template<typename OtherStorage, typename OtherWord>
  bool
  cmp(u64                                       idx_,
      const BitStreamT<OtherStorage,OtherWord> &other_,
      u64                                       other_idx_,
      u64                                       length_) const
  {
    for(u64 i = 0; i < length_; i++)
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
typedef BitStreamT<BitStreamReallocStorage<u8> > BitStreamRealloc;

typedef BitStreamT<std::vector<u8>, u32>         BitStream32;
typedef BitStreamT<BitStreamSpan<u8>, u32>       BitStreamView32;
typedef BitStreamT<BitStreamConstSpan<u8>, u32>  BitStreamReader32;
typedef BitStreamT<BitStreamReallocStorage<u8>, u32> BitStreamRealloc32;
