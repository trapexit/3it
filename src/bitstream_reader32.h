/*
 * bitstream_reader32.h - Header-only C89 bit stream reader (32-bit)
 *
 * All functions are static to allow header-only usage without linker
 * conflicts. Compilers will inline at optimization levels >= -O1.
 *
 * Usage:
 *   BitStreamReader32 r;
 *   bsr32_init(&r, data, byte_count, 0);
 *   val = bsr32_read(&r, 6);
 *
 * For compile-time known widths, use the fixed-width functions
 * (all 1-32 are pre-defined via X-macro):
 *   val = bsr32_read_fixed_6(&r);
 */

#ifndef BITSTREAM_READER32_H
#define BITSTREAM_READER32_H

#include <assert.h>
#include <string.h>

#ifndef BSR32_U8
typedef unsigned char  bsr32_u8;
#else
typedef BSR32_U8       bsr32_u8;
#endif

#ifndef BSR32_U32
typedef unsigned long  bsr32_u32;
#else
typedef BSR32_U32      bsr32_u32;
#endif

#define BSR32_BITS_PER_BYTE 8


/* bswap detection */
#if defined(__GNUC__) || defined(__clang__)
  #define BSR32_HAS_BSWAP 1
  #define bsr32_bswap32(v) __builtin_bswap32(v)
#elif defined(_MSC_VER)
  #include <stdlib.h>
  #define BSR32_HAS_BSWAP 1
  #define bsr32_bswap32(v) _byteswap_ulong(v)
#else
  #define BSR32_HAS_BSWAP 0
#endif

#if BSR32_HAS_BSWAP
static bsr32_u32
bsr32_load32_be(const bsr32_u8 *p)
{
  bsr32_u32 v;
  memcpy(&v, p, 4);
  return bsr32_bswap32(v);
}
#endif


typedef struct BitStreamReader32
{
  const bsr32_u8 *data;
  bsr32_u32       size; /* in bits */
  bsr32_u32       idx;  /* in bits */
} BitStreamReader32;


static void
bsr32_init(BitStreamReader32 *r,
           const bsr32_u8    *data,
           bsr32_u32          size_in_bytes,
           bsr32_u32          idx)
{
  r->data = data;
  r->size = size_in_bytes * BSR32_BITS_PER_BYTE;
  r->idx  = idx;
}

static void
bsr32_seek(BitStreamReader32 *r,
           bsr32_u32          idx)
{
  r->idx = idx;
}

static void
bsr32_rewind(BitStreamReader32 *r)
{
  r->idx = 0;
}

static void
bsr32_rewind_bits(BitStreamReader32 *r,
                  bsr32_u32          bits)
{
  r->idx -= bits;
}

static void
bsr32_skip(BitStreamReader32 *r,
           bsr32_u32          bits)
{
  r->idx += bits;
}

static int
bsr32_on_32bit_boundary(const BitStreamReader32 *r)
{
  return !(r->idx & 0x1FUL);
}

static bsr32_u8
bsr32_bits_to_32bit_boundary(const BitStreamReader32 *r)
{
  return (bsr32_u8)((0x20UL - (r->idx & 0x1FUL)) & 0x1FUL);
}

static void
bsr32_skip_to_8bit_boundary(BitStreamReader32 *r)
{
  if(r->idx & 0x7UL)
    r->idx += 0x8UL - (r->idx & 0x7UL);
}

static void
bsr32_skip_to_16bit_boundary(BitStreamReader32 *r)
{
  if(r->idx & 0x0FUL)
    r->idx += 0x10UL - (r->idx & 0x0FUL);
}

static void
bsr32_skip_to_32bit_boundary(BitStreamReader32 *r)
{
  if(r->idx & 0x1FUL)
    r->idx += 0x20UL - (r->idx & 0x1FUL);
}

static bsr32_u32
bsr32_size(const BitStreamReader32 *r)
{
  return r->size;
}

static bsr32_u32
bsr32_tell(const BitStreamReader32 *r)
{
  return r->idx;
}

static bsr32_u32
bsr32_tell_bits(const BitStreamReader32 *r)
{
  return r->idx;
}

static bsr32_u32
bsr32_tell_bytes(const BitStreamReader32 *r)
{
  return (r->idx + (BSR32_BITS_PER_BYTE - 1)) / BSR32_BITS_PER_BYTE;
}


/*
 * Read 'bits' bits starting at bit position 'idx' (random access).
 * bits must be 0-32.
 */
static bsr32_u32
bsr32_read_at(const BitStreamReader32 *r,
              bsr32_u32                idx,
              bsr32_u32                bits)
{
  bsr32_u32       byte_idx;
  bsr32_u8        bit_off;
  const bsr32_u8 *src;
  bsr32_u32       mask;
  bsr32_u32       acc;
  bsr32_u8        remaining;

  assert((idx + bits) <= r->size);

  if(bits == 0)
    return 0;

  byte_idx = idx >> 3;
  bit_off  = (bsr32_u8)(idx & 7);
  src      = &r->data[byte_idx];
  mask     = (bits == 32) ? ~0UL : ((1UL << bits) - 1);

#if BSR32_HAS_BSWAP
  if(bit_off + bits <= 32)
    {
      acc = bsr32_load32_be(src);
      return (acc >> (32 - bit_off - bits)) & mask;
    }

  acc = bsr32_load32_be(src);
  acc &= (1UL << (32 - bit_off)) - 1;
  remaining = (bsr32_u8)(bits - (32 - bit_off));
  return (acc << remaining) | (src[4] >> (8 - remaining));
#else
  /* byte-aligned fast path */
  if(!bit_off && !(bits & 7))
    {
      bsr32_u32 val = 0;
      bsr32_u32 i;
      for(i = 0; i < (bits >> 3); i++)
        val = (val << 8) | src[i];
      return val;
    }

  /* fits in 4 bytes */
  if(bit_off + bits <= 32)
    {
      bsr32_u32 n;
      acc = 0;
      n = (bit_off + bits + 7) >> 3;
      {
        bsr32_u32 i;
        for(i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
      }
      return (acc >> (n * 8 - bit_off - bits)) & mask;
    }

  /* spans 5 bytes: bit_off in [1..7] */
  acc = 0;
  {
    bsr32_u32 i;
    for(i = 0; i < 4; i++)
      acc = (acc << 8) | src[i];
  }
  acc &= (1UL << (32 - bit_off)) - 1;
  remaining = (bsr32_u8)(bits - (32 - bit_off));
  return (acc << remaining) | (src[4] >> (8 - remaining));
#endif
}


/*
 * Read 'bits' bits at the current cursor and advance.
 */
static bsr32_u32
bsr32_read(BitStreamReader32 *r,
           bsr32_u32          bits)
{
  bsr32_u32 v;

  v = bsr32_read_at(r,r->idx,bits);
  r->idx += bits;

  return v;
}


/*
 * BSR32_DEFINE_READ_FIXED(N) - generates two functions:
 *
 *   bsr32_read_fixed_N_at(r, idx)  - random access, N bits
 *   bsr32_read_fixed_N(r)          - streaming, N bits
 *
 * The bit width is baked into the code so the compiler can
 * eliminate all branches and unroll all loops.
 *
 * For widths 1-25, bit_off + N <= 32 always holds (max 7+25=32),
 * so the 5-byte path is dead code and optimized away.
 *
 * For widths 26-32, the 5-byte path may be taken when bit_off > 0.
 */

#if BSR32_HAS_BSWAP

#define BSR32_DEFINE_READ_FIXED(N)                                         \
                                                                           \
static bsr32_u32                                                           \
bsr32_read_fixed_##N##_at(const BitStreamReader32 *r,                      \
                          bsr32_u32                idx)                     \
{                                                                          \
  const bsr32_u32 BITS  = (N);                                             \
  const bsr32_u32 MASK  = ((N) == 32) ? ~0UL : ((1UL << (N)) - 1);       \
                                                                           \
  bsr32_u32       byte_idx = idx >> 3;                                     \
  bsr32_u8        bit_off  = (bsr32_u8)(idx & 7);                         \
  const bsr32_u8 *src      = &r->data[byte_idx];                          \
  bsr32_u32       acc;                                                     \
                                                                           \
  assert((idx + BITS) <= r->size);                                         \
                                                                           \
  acc = bsr32_load32_be(src);                                              \
  if(bit_off + BITS <= 32)                                                 \
    return (acc >> (32 - bit_off - BITS)) & MASK;                          \
                                                                           \
  /* 5-byte span: only reachable when N >= 26 && bit_off > 0 */           \
  {                                                                        \
    bsr32_u8 remaining;                                                    \
    acc &= (1UL << (32 - bit_off)) - 1;                                   \
    remaining = (bsr32_u8)(BITS - (32 - bit_off));                         \
    return (acc << remaining) | (src[4] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bsr32_u32                                                           \
bsr32_read_fixed_##N(BitStreamReader32 *r)                                 \
{                                                                          \
  bsr32_u32 v = bsr32_read_fixed_##N##_at(r, r->idx);                     \
  r->idx += (N);                                                           \
  return v;                                                                \
}

#else /* !BSR32_HAS_BSWAP */

#define BSR32_DEFINE_READ_FIXED(N)                                         \
                                                                           \
static bsr32_u32                                                           \
bsr32_read_fixed_##N##_at(const BitStreamReader32 *r,                      \
                          bsr32_u32                idx)                     \
{                                                                          \
  const bsr32_u32 BITS  = (N);                                             \
  const bsr32_u32 BYTES = (7 + (N) + 7) >> 3;                             \
  const bsr32_u32 MASK  = ((N) == 32) ? ~0UL : ((1UL << (N)) - 1);       \
                                                                           \
  bsr32_u32       byte_idx = idx >> 3;                                     \
  bsr32_u8        bit_off  = (bsr32_u8)(idx & 7);                         \
  const bsr32_u8 *src      = &r->data[byte_idx];                          \
  bsr32_u32       acc      = 0;                                            \
  bsr32_u32       i;                                                       \
                                                                           \
  assert((idx + BITS) <= r->size);                                         \
                                                                           \
  if(bit_off + BITS <= 32)                                                 \
    {                                                                      \
      for(i = 0; i < BYTES; i++)                                           \
        acc = (acc << 8) | src[i];                                         \
      return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;                 \
    }                                                                      \
                                                                           \
  /* 5-byte span: only reachable when N >= 26 && bit_off > 0 */           \
  {                                                                        \
    bsr32_u8 remaining;                                                    \
    for(i = 0; i < 4; i++)                                                 \
      acc = (acc << 8) | src[i];                                           \
    acc &= (1UL << (32 - bit_off)) - 1;                                   \
    remaining = (bsr32_u8)(BITS - (32 - bit_off));                         \
    return (acc << remaining) | (src[4] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bsr32_u32                                                           \
bsr32_read_fixed_##N(BitStreamReader32 *r)                                 \
{                                                                          \
  bsr32_u32 v = bsr32_read_fixed_##N##_at(r, r->idx);                     \
  r->idx += (N);                                                           \
  return v;                                                                \
}

#endif /* BSR32_HAS_BSWAP */


#define BSR32_X_ALL \
  X(1)  X(2)  X(3)  X(4)  X(5)  X(6)  X(7)  X(8)  \
  X(9)  X(10) X(11) X(12) X(13) X(14) X(15) X(16) \
  X(17) X(18) X(19) X(20) X(21) X(22) X(23) X(24) \
  X(25) X(26) X(27) X(28) X(29) X(30) X(31) X(32)

#define X(n) BSR32_DEFINE_READ_FIXED(n)
BSR32_X_ALL
#undef X

#endif /* BITSTREAM_READER32_H */
