/*
 * bitstream_reader64.h - Header-only C89 bit stream reader (64-bit)
 *
 * All functions are static to allow header-only usage without linker
 * conflicts. Compilers will inline at optimization levels >= -O1.
 *
 * Requires compiler support for 64-bit integers (unsigned long long
 * or equivalent). Override BSR64_U64 if your platform uses a
 * different 64-bit type.
 *
 * Usage:
 *   BitStreamReader64 r;
 *   bsr64_init(&r, data, byte_count, 0);
 *   val = bsr64_read(&r, 6);
 *
 * For compile-time known widths, use the fixed-width functions
 * (all 1-64 are pre-defined via X-macro):
 *   val = bsr64_read_fixed_6(&r);
 */

#ifndef BITSTREAM_READER64_H
#define BITSTREAM_READER64_H

#include <assert.h>
#include <string.h>

#ifndef BSR64_U8
typedef unsigned char      bsr64_u8;
#else
typedef BSR64_U8           bsr64_u8;
#endif

#ifndef BSR64_U64
typedef unsigned long long bsr64_u64;
#else
typedef BSR64_U64          bsr64_u64;
#endif

#define BSR64_BITS_PER_BYTE 8
#define BSR64_ONE  ((bsr64_u64)1)
#define BSR64_MASK(b) (((b) == 64) ? ~(bsr64_u64)0 : (BSR64_ONE << (b)) - 1)


/* bswap detection */
#if defined(__GNUC__) || defined(__clang__)
  #define BSR64_HAS_BSWAP 1
  #define bsr64_bswap64(v) __builtin_bswap64(v)
#elif defined(_MSC_VER)
  #include <stdlib.h>
  #define BSR64_HAS_BSWAP 1
  #define bsr64_bswap64(v) _byteswap_uint64(v)
#else
  #define BSR64_HAS_BSWAP 0
#endif

#if BSR64_HAS_BSWAP
static bsr64_u64
bsr64_load64_be(const bsr64_u8 *p)
{
  bsr64_u64 v;
  memcpy(&v, p, 8);
  return bsr64_bswap64(v);
}
#endif


typedef struct BitStreamReader64
{
  const bsr64_u8 *data;
  bsr64_u64       size; /* in bits */
  bsr64_u64       idx;  /* in bits */
} BitStreamReader64;


static void
bsr64_init(BitStreamReader64 *r,
           const bsr64_u8    *data,
           bsr64_u64          size_in_bytes,
           bsr64_u64          idx)
{
  r->data = data;
  r->size = size_in_bytes * BSR64_BITS_PER_BYTE;
  r->idx  = idx;
}

static void
bsr64_seek(BitStreamReader64 *r,
           bsr64_u64          idx)
{
  r->idx = idx;
}

static void
bsr64_rewind(BitStreamReader64 *r)
{
  r->idx = 0;
}

static void
bsr64_rewind_bits(BitStreamReader64 *r,
                  bsr64_u64          bits)
{
  r->idx -= bits;
}

static void
bsr64_skip(BitStreamReader64 *r,
           bsr64_u64          bits)
{
  r->idx += bits;
}

static int
bsr64_on_32bit_boundary(const BitStreamReader64 *r)
{
  return !(r->idx & 0x1F);
}

static bsr64_u8
bsr64_bits_to_32bit_boundary(const BitStreamReader64 *r)
{
  return (bsr64_u8)((0x20 - (r->idx & 0x1F)) & 0x1F);
}

static int
bsr64_on_64bit_boundary(const BitStreamReader64 *r)
{
  return !(r->idx & 0x3F);
}

static bsr64_u8
bsr64_bits_to_64bit_boundary(const BitStreamReader64 *r)
{
  return (bsr64_u8)((0x40 - (r->idx & 0x3F)) & 0x3F);
}

static void
bsr64_skip_to_8bit_boundary(BitStreamReader64 *r)
{
  if(r->idx & 0x7)
    r->idx += 0x8 - (r->idx & 0x7);
}

static void
bsr64_skip_to_16bit_boundary(BitStreamReader64 *r)
{
  if(r->idx & 0x0F)
    r->idx += 0x10 - (r->idx & 0x0F);
}

static void
bsr64_skip_to_32bit_boundary(BitStreamReader64 *r)
{
  if(r->idx & 0x1F)
    r->idx += 0x20 - (r->idx & 0x1F);
}

static void
bsr64_skip_to_64bit_boundary(BitStreamReader64 *r)
{
  if(r->idx & 0x3F)
    r->idx += 0x40 - (r->idx & 0x3F);
}

static bsr64_u64
bsr64_size(const BitStreamReader64 *r)
{
  return r->size;
}

static bsr64_u64
bsr64_tell(const BitStreamReader64 *r)
{
  return r->idx;
}

static bsr64_u64
bsr64_tell_bits(const BitStreamReader64 *r)
{
  return r->idx;
}

static bsr64_u64
bsr64_tell_bytes(const BitStreamReader64 *r)
{
  return (r->idx + (BSR64_BITS_PER_BYTE - 1)) / BSR64_BITS_PER_BYTE;
}


/*
 * Read 'bits' bits starting at bit position 'idx' (random access).
 * bits must be 0-64.
 */
static bsr64_u64
bsr64_read_at(const BitStreamReader64 *r,
              bsr64_u64                idx,
              bsr64_u64                bits)
{
  bsr64_u64       byte_idx;
  bsr64_u8        bit_off;
  const bsr64_u8 *src;
  bsr64_u64       mask;
  bsr64_u64       acc;
  bsr64_u8        remaining;

  assert((idx + bits) <= r->size);

  if(bits == 0)
    return 0;

  byte_idx = idx >> 3;
  bit_off  = (bsr64_u8)(idx & 7);
  src      = &r->data[byte_idx];
  mask     = BSR64_MASK(bits);

#if BSR64_HAS_BSWAP
  if(bit_off + bits <= 64)
    {
      acc = bsr64_load64_be(src);
      return (acc >> (64 - bit_off - bits)) & mask;
    }

  /* spans 9 bytes: bit_off in [1..7] */
  acc = bsr64_load64_be(src);
  acc &= (BSR64_ONE << (64 - bit_off)) - 1;
  remaining = (bsr64_u8)(bits - (64 - bit_off));
  return (acc << remaining) | (src[8] >> (8 - remaining));
#else
  /* byte-aligned fast path */
  if(!bit_off && !(bits & 7))
    {
      bsr64_u64 val = 0;
      bsr64_u64 i;
      for(i = 0; i < (bits >> 3); i++)
        val = (val << 8) | src[i];
      return val;
    }

  /* fits in 8 bytes */
  if(bit_off + bits <= 64)
    {
      bsr64_u64 n;
      acc = 0;
      n = (bit_off + bits + 7) >> 3;
      {
        bsr64_u64 i;
        for(i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
      }
      return (acc >> (n * 8 - bit_off - bits)) & mask;
    }

  /* spans 9 bytes: bit_off in [1..7] */
  acc = 0;
  {
    bsr64_u64 i;
    for(i = 0; i < 8; i++)
      acc = (acc << 8) | src[i];
  }
  acc &= (BSR64_ONE << (64 - bit_off)) - 1;
  remaining = (bsr64_u8)(bits - (64 - bit_off));
  return (acc << remaining) | (src[8] >> (8 - remaining));
#endif
}


/*
 * Read 'bits' bits at the current cursor and advance.
 */
static bsr64_u64
bsr64_read(BitStreamReader64 *r,
           bsr64_u64          bits)
{
  bsr64_u64 v;

  v = bsr64_read_at(r, r->idx, bits);
  r->idx += bits;

  return v;
}


/*
 * BSR64_DEFINE_READ_FIXED(N) - generates two functions:
 *
 *   bsr64_read_fixed_N_at(r, idx)  - random access, N bits
 *   bsr64_read_fixed_N(r)          - streaming, N bits
 *
 * The bit width is baked into the code so the compiler can
 * eliminate all branches and unroll all loops.
 *
 * For widths 1-57, bit_off + N <= 64 always holds (max 7+57=64),
 * so the 9-byte path is dead code and optimized away.
 *
 * For widths 58-64, the 9-byte path may be taken when bit_off > 0.
 */

#if BSR64_HAS_BSWAP

#define BSR64_DEFINE_READ_FIXED(N)                                         \
                                                                           \
static bsr64_u64                                                           \
bsr64_read_fixed_##N##_at(const BitStreamReader64 *r,                      \
                          bsr64_u64                idx)                     \
{                                                                          \
  const bsr64_u64 BITS = (N);                                              \
  const bsr64_u64 MASK = BSR64_MASK(N);                                    \
                                                                           \
  bsr64_u64       byte_idx = idx >> 3;                                     \
  bsr64_u8        bit_off  = (bsr64_u8)(idx & 7);                         \
  const bsr64_u8 *src      = &r->data[byte_idx];                          \
  bsr64_u64       acc;                                                     \
                                                                           \
  assert((idx + BITS) <= r->size);                                         \
                                                                           \
  acc = bsr64_load64_be(src);                                              \
  if(bit_off + BITS <= 64)                                                 \
    return (acc >> (64 - bit_off - BITS)) & MASK;                          \
                                                                           \
  /* 9-byte span: only reachable when N >= 58 && bit_off > 0 */           \
  {                                                                        \
    bsr64_u8 remaining;                                                    \
    acc &= (BSR64_ONE << (64 - bit_off)) - 1;                             \
    remaining = (bsr64_u8)(BITS - (64 - bit_off));                         \
    return (acc << remaining) | (src[8] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bsr64_u64                                                           \
bsr64_read_fixed_##N(BitStreamReader64 *r)                                 \
{                                                                          \
  bsr64_u64 v = bsr64_read_fixed_##N##_at(r, r->idx);                     \
  r->idx += (N);                                                           \
  return v;                                                                \
}

#else /* !BSR64_HAS_BSWAP */

#define BSR64_DEFINE_READ_FIXED(N)                                         \
                                                                           \
static bsr64_u64                                                           \
bsr64_read_fixed_##N##_at(const BitStreamReader64 *r,                      \
                          bsr64_u64                idx)                     \
{                                                                          \
  const bsr64_u64 BITS  = (N);                                             \
  const bsr64_u64 BYTES = (7 + (N) + 7) >> 3;                             \
  const bsr64_u64 MASK  = BSR64_MASK(N);                                   \
                                                                           \
  bsr64_u64       byte_idx = idx >> 3;                                     \
  bsr64_u8        bit_off  = (bsr64_u8)(idx & 7);                         \
  const bsr64_u8 *src      = &r->data[byte_idx];                          \
  bsr64_u64       acc      = 0;                                            \
  bsr64_u64       i;                                                       \
                                                                           \
  assert((idx + BITS) <= r->size);                                         \
                                                                           \
  if(bit_off + BITS <= 64)                                                 \
    {                                                                      \
      for(i = 0; i < BYTES; i++)                                           \
        acc = (acc << 8) | src[i];                                         \
      return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;                 \
    }                                                                      \
                                                                           \
  /* 9-byte span: only reachable when N >= 58 && bit_off > 0 */           \
  {                                                                        \
    bsr64_u8 remaining;                                                    \
    for(i = 0; i < 8; i++)                                                 \
      acc = (acc << 8) | src[i];                                           \
    acc &= (BSR64_ONE << (64 - bit_off)) - 1;                             \
    remaining = (bsr64_u8)(BITS - (64 - bit_off));                         \
    return (acc << remaining) | (src[8] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bsr64_u64                                                           \
bsr64_read_fixed_##N(BitStreamReader64 *r)                                 \
{                                                                          \
  bsr64_u64 v = bsr64_read_fixed_##N##_at(r, r->idx);                     \
  r->idx += (N);                                                           \
  return v;                                                                \
}

#endif /* BSR64_HAS_BSWAP */


#define BSR64_X_ALL \
  X(1)  X(2)  X(3)  X(4)  X(5)  X(6)  X(7)  X(8)  \
  X(9)  X(10) X(11) X(12) X(13) X(14) X(15) X(16) \
  X(17) X(18) X(19) X(20) X(21) X(22) X(23) X(24) \
  X(25) X(26) X(27) X(28) X(29) X(30) X(31) X(32) \
  X(33) X(34) X(35) X(36) X(37) X(38) X(39) X(40) \
  X(41) X(42) X(43) X(44) X(45) X(46) X(47) X(48) \
  X(49) X(50) X(51) X(52) X(53) X(54) X(55) X(56) \
  X(57) X(58) X(59) X(60) X(61) X(62) X(63) X(64)

#define X(n) BSR64_DEFINE_READ_FIXED(n)
BSR64_X_ALL
#undef X

#endif /* BITSTREAM_READER64_H */
