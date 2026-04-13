/*
 * bitstream_t32.h - Header-only C89 combined bit stream (32-bit index)
 *
 * Combines read and write access into a single struct.  All functions
 * are static to allow header-only usage without linker conflicts.
 * Compilers will inline at optimization levels >= -O1.
 *
 * The caller manages the buffer and must ensure sufficient size.
 *
 * Usage (mutable / writer):
 *   BitStreamT32 s;
 *   bst32_init(&s, buffer, buffer_size_bytes, 0);
 *   bst32_write(&s, 6, 0x1F);
 *   val = bst32_read(&s, 6);
 *
 * Usage (read-only):
 *   BitStreamT32 s;
 *   bst32_init_ro(&s, data, byte_count, 0);
 *   val = bst32_read(&s, 6);
 *
 * For compile-time known widths (all 1-32 pre-defined via X-macro):
 *   val = bst32_read_fixed_6(&s);
 *   bst32_write_fixed_6(&s, 0x1F);
 */

#ifndef BITSTREAM_T32_H
#define BITSTREAM_T32_H

#include <assert.h>
#include <string.h>

#ifndef BST32_U8
typedef unsigned char  bst32_u8;
#else
typedef BST32_U8       bst32_u8;
#endif

#ifndef BST32_U32
typedef unsigned long  bst32_u32;
#else
typedef BST32_U32      bst32_u32;
#endif

#define BST32_BITS_PER_BYTE 8


/* bswap detection */
#if defined(__GNUC__) || defined(__clang__)
  #define BST32_HAS_BSWAP 1
  #define bst32_bswap32(v) __builtin_bswap32(v)
#elif defined(_MSC_VER)
  #include <stdlib.h>
  #define BST32_HAS_BSWAP 1
  #define bst32_bswap32(v) _byteswap_ulong(v)
#else
  #define BST32_HAS_BSWAP 0
#endif

#if BST32_HAS_BSWAP
static bst32_u32
bst32_load32_be(const bst32_u8 *p)
{
  bst32_u32 v;
  memcpy(&v, p, 4);
  return bst32_bswap32(v);
}
#endif


typedef struct BitStreamT32
{
  bst32_u8  *data;
  bst32_u32  size; /* in bits */
  bst32_u32  idx;  /* in bits */
} BitStreamT32;


/* init with writable buffer */
static void
bst32_init(BitStreamT32 *s,
           bst32_u8     *data,
           bst32_u32     size_in_bytes,
           bst32_u32     idx)
{
  s->data = data;
  s->size = size_in_bytes * BST32_BITS_PER_BYTE;
  s->idx  = idx;
}

/* init with read-only buffer (casts away const; caller must not write) */
static void
bst32_init_ro(BitStreamT32    *s,
              const bst32_u8  *data,
              bst32_u32        size_in_bytes,
              bst32_u32        idx)
{
  s->data = (bst32_u8 *)data;
  s->size = size_in_bytes * BST32_BITS_PER_BYTE;
  s->idx  = idx;
}


/* ------------------------------------------------------------------ */
/* Navigation                                                          */
/* ------------------------------------------------------------------ */

static void
bst32_seek(BitStreamT32 *s,
           bst32_u32     idx)
{
  s->idx = idx;
}

static void
bst32_rewind(BitStreamT32 *s)
{
  s->idx = 0;
}

static void
bst32_rewind_bits(BitStreamT32 *s,
                  bst32_u32     bits)
{
  s->idx -= bits;
}

static void
bst32_skip(BitStreamT32 *s,
           bst32_u32     bits)
{
  s->idx += bits;
}

static int
bst32_on_8bit_boundary(const BitStreamT32 *s)
{
  return !(s->idx & 0x7UL);
}

static bst32_u8
bst32_bits_to_8bit_boundary(const BitStreamT32 *s)
{
  return (bst32_u8)((0x08UL - (s->idx & 0x7UL)) & 0x7UL);
}

static int
bst32_on_16bit_boundary(const BitStreamT32 *s)
{
  return !(s->idx & 0xFUL);
}

static bst32_u8
bst32_bits_to_16bit_boundary(const BitStreamT32 *s)
{
  return (bst32_u8)((0x10UL - (s->idx & 0xFUL)) & 0xFUL);
}

static int
bst32_on_32bit_boundary(const BitStreamT32 *s)
{
  return !(s->idx & 0x1FUL);
}

static bst32_u8
bst32_bits_to_32bit_boundary(const BitStreamT32 *s)
{
  return (bst32_u8)((0x20UL - (s->idx & 0x1FUL)) & 0x1FUL);
}

static void
bst32_skip_to_8bit_boundary(BitStreamT32 *s)
{
  if(s->idx & 0x7UL)
    s->idx += 0x8UL - (s->idx & 0x7UL);
}

static void
bst32_skip_to_16bit_boundary(BitStreamT32 *s)
{
  if(s->idx & 0x0FUL)
    s->idx += 0x10UL - (s->idx & 0x0FUL);
}

static void
bst32_skip_to_32bit_boundary(BitStreamT32 *s)
{
  if(s->idx & 0x1FUL)
    s->idx += 0x20UL - (s->idx & 0x1FUL);
}

static bst32_u32
bst32_size(const BitStreamT32 *s)
{
  return s->size;
}

static bst32_u32
bst32_tell(const BitStreamT32 *s)
{
  return s->idx;
}

static bst32_u32
bst32_tell_bits(const BitStreamT32 *s)
{
  return s->idx;
}

static bst32_u32
bst32_tell_bytes(const BitStreamT32 *s)
{
  return (s->idx + (BST32_BITS_PER_BYTE - 1)) / BST32_BITS_PER_BYTE;
}

static bst32_u32
bst32_tell_u32(const BitStreamT32 *s)
{
  return bst32_tell_bytes(s) / 4;
}


/* ------------------------------------------------------------------ */
/* Read                                                                */
/* ------------------------------------------------------------------ */

/*
 * Read 'bits' bits starting at bit position 'idx' (random access).
 * bits must be 0-32.
 */
static bst32_u32
bst32_read_at(const BitStreamT32 *s,
              bst32_u32           idx,
              bst32_u32           bits)
{
  bst32_u32       byte_idx;
  bst32_u8        bit_off;
  const bst32_u8 *src;
  bst32_u32       mask;
  bst32_u32       acc;
  bst32_u8        remaining;

  assert((idx + bits) <= s->size);

  if(bits == 0)
    return 0;

  byte_idx = idx >> 3;
  bit_off  = (bst32_u8)(idx & 7);
  src      = &s->data[byte_idx];
  mask     = (bits == 32) ? ~0UL : ((1UL << bits) - 1);

#if BST32_HAS_BSWAP
  if(bit_off + bits <= 32)
    {
      acc = bst32_load32_be(src);
      return (acc >> (32 - bit_off - bits)) & mask;
    }

  acc = bst32_load32_be(src);
  acc &= (1UL << (32 - bit_off)) - 1;
  remaining = (bst32_u8)(bits - (32 - bit_off));
  return (acc << remaining) | (src[4] >> (8 - remaining));
#else
  if(!bit_off && !(bits & 7))
    {
      bst32_u32 val = 0;
      bst32_u32 i;
      for(i = 0; i < (bits >> 3); i++)
        val = (val << 8) | src[i];
      return val;
    }

  if(bit_off + bits <= 32)
    {
      bst32_u32 n;
      acc = 0;
      n = (bit_off + bits + 7) >> 3;
      {
        bst32_u32 i;
        for(i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
      }
      return (acc >> (n * 8 - bit_off - bits)) & mask;
    }

  acc = 0;
  {
    bst32_u32 i;
    for(i = 0; i < 4; i++)
      acc = (acc << 8) | src[i];
  }
  acc &= (1UL << (32 - bit_off)) - 1;
  remaining = (bst32_u8)(bits - (32 - bit_off));
  return (acc << remaining) | (src[4] >> (8 - remaining));
#endif
}

/*
 * Read 'bits' bits at the current cursor and advance.
 */
static bst32_u32
bst32_read(BitStreamT32 *s,
           bst32_u32     bits)
{
  bst32_u32 v;

  v = bst32_read_at(s, s->idx, bits);
  s->idx += bits;

  return v;
}


/* ------------------------------------------------------------------ */
/* Write                                                               */
/* ------------------------------------------------------------------ */

/*
 * Write 'bits' bits of 'val' at bit position 'idx' (random access).
 * bits must be 0-32. val must fit in 'bits' bits.
 */
static void
bst32_write_at(BitStreamT32 *s,
               bst32_u32     idx,
               bst32_u32     bits,
               bst32_u32     val)
{
  bst32_u8  *dst;
  bst32_u8   bit_off;
  bst32_u32  remaining;

  assert((idx + bits) <= s->size);

  if(bits == 0)
    return;

  dst       = &s->data[idx >> 3];
  bit_off   = (bst32_u8)(idx & 7);
  remaining = bits;

  if(bit_off)
    {
      bst32_u8 avail = 8 - bit_off;
      bst32_u8 take  = (remaining < avail) ? (bst32_u8)remaining : avail;
      bst32_u8 shift = avail - take;
      bst32_u8 mask  = (bst32_u8)(((1U << take) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bst32_u8)(((val >> (remaining - take)) & ((1UL << take) - 1)) << shift);
      dst++;
      remaining -= take;
    }

  while(remaining >= 8)
    {
      remaining -= 8;
      *dst++ = (bst32_u8)((val >> remaining) & 0xFF);
    }

  if(remaining)
    {
      bst32_u8 shift = 8 - (bst32_u8)remaining;
      bst32_u8 mask  = (bst32_u8)(((1U << remaining) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bst32_u8)((val & ((1UL << remaining) - 1)) << shift);
    }
}

/*
 * Write 'bits' bits of 'val' at the current cursor and advance.
 */
static void
bst32_write(BitStreamT32 *s,
            bst32_u32     bits,
            bst32_u32     val)
{
  bst32_write_at(s, s->idx, bits, val);
  s->idx += bits;
}

/*
 * Write raw bytes at the current cursor. If byte-aligned, uses memcpy.
 */
static void
bst32_write_bytes(BitStreamT32   *s,
                  const bst32_u8 *src,
                  bst32_u32       count)
{
  if(!(s->idx & 7))
    {
      assert((s->idx + count * 8) <= s->size);
      memcpy(&s->data[s->idx >> 3], src, count);
      s->idx += count * 8;
    }
  else
    {
      bst32_u32 i;
      for(i = 0; i < count; i++)
        bst32_write(s, 8, src[i]);
    }
}


/* ------------------------------------------------------------------ */
/* Fixed-width read/write macros                                       */
/*                                                                     */
/* BST32_DEFINE_FIXED(N) generates:                                    */
/*   bst32_read_fixed_N_at(s, idx)  - random-access read, N bits      */
/*   bst32_read_fixed_N(s)          - streaming read, N bits          */
/*   bst32_write_fixed_N_at(s, idx, val) - random-access write        */
/*   bst32_write_fixed_N(s, val)         - streaming write            */
/*                                                                     */
/* For read widths 1-25, bit_off + N <= 32 always holds so the        */
/* 5-byte path is dead code and optimized away.                       */
/* ------------------------------------------------------------------ */

#if BST32_HAS_BSWAP

#define BST32_DEFINE_FIXED(N)                                              \
                                                                           \
static bst32_u32                                                           \
bst32_read_fixed_##N##_at(const BitStreamT32 *s,                           \
                          bst32_u32           idx)                         \
{                                                                          \
  const bst32_u32 BITS = (N);                                              \
  const bst32_u32 MASK = ((N) == 32) ? ~0UL : ((1UL << (N)) - 1);        \
  bst32_u32       byte_idx = idx >> 3;                                     \
  bst32_u8        bit_off  = (bst32_u8)(idx & 7);                         \
  const bst32_u8 *src      = &s->data[byte_idx];                          \
  bst32_u32       acc;                                                     \
                                                                           \
  assert((idx + BITS) <= s->size);                                         \
                                                                           \
  acc = bst32_load32_be(src);                                              \
  if(bit_off + BITS <= 32)                                                 \
    return (acc >> (32 - bit_off - BITS)) & MASK;                          \
                                                                           \
  {                                                                        \
    bst32_u8 remaining;                                                    \
    acc &= (1UL << (32 - bit_off)) - 1;                                   \
    remaining = (bst32_u8)(BITS - (32 - bit_off));                         \
    return (acc << remaining) | (src[4] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bst32_u32                                                           \
bst32_read_fixed_##N(BitStreamT32 *s)                                      \
{                                                                          \
  bst32_u32 v = bst32_read_fixed_##N##_at(s, s->idx);                     \
  s->idx += (N);                                                           \
  return v;                                                                \
}                                                                          \
                                                                           \
static void                                                                \
bst32_write_fixed_##N##_at(BitStreamT32 *s,                                \
                           bst32_u32     idx,                              \
                           bst32_u32     val)                              \
{                                                                          \
  const bst32_u32 BITS = (N);                                              \
  bst32_u8  *dst;                                                          \
  bst32_u8   bit_off;                                                      \
  bst32_u32  remaining;                                                    \
                                                                           \
  assert((idx + BITS) <= s->size);                                         \
                                                                           \
  dst       = &s->data[idx >> 3];                                          \
  bit_off   = (bst32_u8)(idx & 7);                                        \
  remaining = BITS;                                                        \
                                                                           \
  if(bit_off)                                                              \
    {                                                                      \
      bst32_u8 avail = 8 - bit_off;                                       \
      bst32_u8 take  = (BITS < avail) ? (bst32_u8)BITS : avail;          \
      bst32_u8 shift = avail - take;                                      \
      bst32_u8 mask  = (bst32_u8)(((1U << take) - 1) << shift);          \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bst32_u8)(((val >> (remaining - take)) &                         \
                     ((1UL << take) - 1)) << shift);                      \
      dst++;                                                               \
      remaining -= take;                                                   \
    }                                                                      \
                                                                           \
  while(remaining >= 8)                                                    \
    {                                                                      \
      remaining -= 8;                                                      \
      *dst++ = (bst32_u8)((val >> remaining) & 0xFF);                     \
    }                                                                      \
                                                                           \
  if(remaining)                                                            \
    {                                                                      \
      bst32_u8 shift = 8 - (bst32_u8)remaining;                          \
      bst32_u8 mask  = (bst32_u8)(((1U << remaining) - 1) << shift);     \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bst32_u8)((val & ((1UL << remaining) - 1)) << shift);           \
    }                                                                      \
}                                                                          \
                                                                           \
static void                                                                \
bst32_write_fixed_##N(BitStreamT32 *s,                                     \
                      bst32_u32     val)                                   \
{                                                                          \
  bst32_write_fixed_##N##_at(s, s->idx, val);                             \
  s->idx += (N);                                                           \
}

#else /* !BST32_HAS_BSWAP */

#define BST32_DEFINE_FIXED(N)                                              \
                                                                           \
static bst32_u32                                                           \
bst32_read_fixed_##N##_at(const BitStreamT32 *s,                           \
                          bst32_u32           idx)                         \
{                                                                          \
  const bst32_u32 BITS  = (N);                                             \
  const bst32_u32 BYTES = (7 + (N) + 7) >> 3;                             \
  const bst32_u32 MASK  = ((N) == 32) ? ~0UL : ((1UL << (N)) - 1);       \
  const bst32_u8 *src   = &s->data[idx >> 3];                             \
  bst32_u8        bit_off = (bst32_u8)(idx & 7);                          \
  bst32_u32       acc   = 0;                                               \
  bst32_u32       i;                                                       \
                                                                           \
  assert((idx + BITS) <= s->size);                                         \
                                                                           \
  if(bit_off + BITS <= 32)                                                 \
    {                                                                      \
      for(i = 0; i < BYTES; i++)                                           \
        acc = (acc << 8) | src[i];                                         \
      return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;                 \
    }                                                                      \
                                                                           \
  {                                                                        \
    bst32_u8 remaining;                                                    \
    for(i = 0; i < 4; i++)                                                 \
      acc = (acc << 8) | src[i];                                           \
    acc &= (1UL << (32 - bit_off)) - 1;                                   \
    remaining = (bst32_u8)(BITS - (32 - bit_off));                         \
    return (acc << remaining) | (src[4] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bst32_u32                                                           \
bst32_read_fixed_##N(BitStreamT32 *s)                                      \
{                                                                          \
  bst32_u32 v = bst32_read_fixed_##N##_at(s, s->idx);                     \
  s->idx += (N);                                                           \
  return v;                                                                \
}                                                                          \
                                                                           \
static void                                                                \
bst32_write_fixed_##N##_at(BitStreamT32 *s,                                \
                           bst32_u32     idx,                              \
                           bst32_u32     val)                              \
{                                                                          \
  const bst32_u32 BITS = (N);                                              \
  bst32_u8  *dst;                                                          \
  bst32_u8   bit_off;                                                      \
  bst32_u32  remaining;                                                    \
                                                                           \
  assert((idx + BITS) <= s->size);                                         \
                                                                           \
  dst       = &s->data[idx >> 3];                                          \
  bit_off   = (bst32_u8)(idx & 7);                                        \
  remaining = BITS;                                                        \
                                                                           \
  if(bit_off)                                                              \
    {                                                                      \
      bst32_u8 avail = 8 - bit_off;                                       \
      bst32_u8 take  = (BITS < avail) ? (bst32_u8)BITS : avail;          \
      bst32_u8 shift = avail - take;                                      \
      bst32_u8 mask  = (bst32_u8)(((1U << take) - 1) << shift);          \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bst32_u8)(((val >> (remaining - take)) &                         \
                     ((1UL << take) - 1)) << shift);                      \
      dst++;                                                               \
      remaining -= take;                                                   \
    }                                                                      \
                                                                           \
  while(remaining >= 8)                                                    \
    {                                                                      \
      remaining -= 8;                                                      \
      *dst++ = (bst32_u8)((val >> remaining) & 0xFF);                     \
    }                                                                      \
                                                                           \
  if(remaining)                                                            \
    {                                                                      \
      bst32_u8 shift = 8 - (bst32_u8)remaining;                          \
      bst32_u8 mask  = (bst32_u8)(((1U << remaining) - 1) << shift);     \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bst32_u8)((val & ((1UL << remaining) - 1)) << shift);           \
    }                                                                      \
}                                                                          \
                                                                           \
static void                                                                \
bst32_write_fixed_##N(BitStreamT32 *s,                                     \
                      bst32_u32     val)                                   \
{                                                                          \
  bst32_write_fixed_##N##_at(s, s->idx, val);                             \
  s->idx += (N);                                                           \
}

#endif /* BST32_HAS_BSWAP */


#define BST32_X_ALL \
  X(1)  X(2)  X(3)  X(4)  X(5)  X(6)  X(7)  X(8)  \
  X(9)  X(10) X(11) X(12) X(13) X(14) X(15) X(16) \
  X(17) X(18) X(19) X(20) X(21) X(22) X(23) X(24) \
  X(25) X(26) X(27) X(28) X(29) X(30) X(31) X(32)

#define X(n) BST32_DEFINE_FIXED(n)
BST32_X_ALL
#undef X

#endif /* BITSTREAM_T32_H */
