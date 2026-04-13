/*
 * bitstream_t64.h - Header-only C89 combined bit stream (64-bit index)
 *
 * Combines read and write access into a single struct.  All functions
 * are static to allow header-only usage without linker conflicts.
 * Compilers will inline at optimization levels >= -O1.
 *
 * Requires compiler support for 64-bit integers (unsigned long long
 * or equivalent). Override BST64_U64 if your platform uses a
 * different 64-bit type.
 *
 * The caller manages the buffer and must ensure sufficient size.
 *
 * Usage (mutable / writer):
 *   BitStreamT64 s;
 *   bst64_init(&s, buffer, buffer_size_bytes, 0);
 *   bst64_write(&s, 6, 0x1F);
 *   val = bst64_read(&s, 6);
 *
 * Usage (read-only):
 *   BitStreamT64 s;
 *   bst64_init_ro(&s, data, byte_count, 0);
 *   val = bst64_read(&s, 6);
 *
 * For compile-time known widths (all 1-64 pre-defined via X-macro):
 *   val = bst64_read_fixed_6(&s);
 *   bst64_write_fixed_6(&s, 0x1F);
 */

#ifndef BITSTREAM_T64_H
#define BITSTREAM_T64_H

#include <assert.h>
#include <string.h>

#ifndef BST64_U8
typedef unsigned char      bst64_u8;
#else
typedef BST64_U8           bst64_u8;
#endif

#ifndef BST64_U64
typedef unsigned long long bst64_u64;
#else
typedef BST64_U64          bst64_u64;
#endif

#define BST64_BITS_PER_BYTE 8
#define BST64_ONE  ((bst64_u64)1)
#define BST64_MASK(b) (((b) == 64) ? ~(bst64_u64)0 : (BST64_ONE << (b)) - 1)


/* bswap detection */
#if defined(__GNUC__) || defined(__clang__)
  #define BST64_HAS_BSWAP 1
  #define bst64_bswap64(v) __builtin_bswap64(v)
#elif defined(_MSC_VER)
  #include <stdlib.h>
  #define BST64_HAS_BSWAP 1
  #define bst64_bswap64(v) _byteswap_uint64(v)
#else
  #define BST64_HAS_BSWAP 0
#endif

#if BST64_HAS_BSWAP
static bst64_u64
bst64_load64_be(const bst64_u8 *p)
{
  bst64_u64 v;
  memcpy(&v, p, 8);
  return bst64_bswap64(v);
}
#endif


typedef struct BitStreamT64
{
  bst64_u8  *data;
  bst64_u64  size; /* in bits */
  bst64_u64  idx;  /* in bits */
} BitStreamT64;


/* init with writable buffer */
static void
bst64_init(BitStreamT64 *s,
           bst64_u8     *data,
           bst64_u64     size_in_bytes,
           bst64_u64     idx)
{
  s->data = data;
  s->size = size_in_bytes * BST64_BITS_PER_BYTE;
  s->idx  = idx;
}

/* init with read-only buffer (casts away const; caller must not write) */
static void
bst64_init_ro(BitStreamT64    *s,
              const bst64_u8  *data,
              bst64_u64        size_in_bytes,
              bst64_u64        idx)
{
  s->data = (bst64_u8 *)data;
  s->size = size_in_bytes * BST64_BITS_PER_BYTE;
  s->idx  = idx;
}


/* ------------------------------------------------------------------ */
/* Navigation                                                          */
/* ------------------------------------------------------------------ */

static void
bst64_seek(BitStreamT64 *s,
           bst64_u64     idx)
{
  s->idx = idx;
}

static void
bst64_rewind(BitStreamT64 *s)
{
  s->idx = 0;
}

static void
bst64_rewind_bits(BitStreamT64 *s,
                  bst64_u64     bits)
{
  s->idx -= bits;
}

static void
bst64_skip(BitStreamT64 *s,
           bst64_u64     bits)
{
  s->idx += bits;
}

static int
bst64_on_8bit_boundary(const BitStreamT64 *s)
{
  return !(s->idx & 0x7);
}

static bst64_u8
bst64_bits_to_8bit_boundary(const BitStreamT64 *s)
{
  return (bst64_u8)((0x08 - (s->idx & 0x7)) & 0x7);
}

static int
bst64_on_16bit_boundary(const BitStreamT64 *s)
{
  return !(s->idx & 0xF);
}

static bst64_u8
bst64_bits_to_16bit_boundary(const BitStreamT64 *s)
{
  return (bst64_u8)((0x10 - (s->idx & 0xF)) & 0xF);
}

static int
bst64_on_32bit_boundary(const BitStreamT64 *s)
{
  return !(s->idx & 0x1F);
}

static bst64_u8
bst64_bits_to_32bit_boundary(const BitStreamT64 *s)
{
  return (bst64_u8)((0x20 - (s->idx & 0x1F)) & 0x1F);
}

static int
bst64_on_64bit_boundary(const BitStreamT64 *s)
{
  return !(s->idx & 0x3F);
}

static bst64_u8
bst64_bits_to_64bit_boundary(const BitStreamT64 *s)
{
  return (bst64_u8)((0x40 - (s->idx & 0x3F)) & 0x3F);
}

static void
bst64_skip_to_8bit_boundary(BitStreamT64 *s)
{
  if(s->idx & 0x7)
    s->idx += 0x8 - (s->idx & 0x7);
}

static void
bst64_skip_to_16bit_boundary(BitStreamT64 *s)
{
  if(s->idx & 0x0F)
    s->idx += 0x10 - (s->idx & 0x0F);
}

static void
bst64_skip_to_32bit_boundary(BitStreamT64 *s)
{
  if(s->idx & 0x1F)
    s->idx += 0x20 - (s->idx & 0x1F);
}

static void
bst64_skip_to_64bit_boundary(BitStreamT64 *s)
{
  if(s->idx & 0x3F)
    s->idx += 0x40 - (s->idx & 0x3F);
}

static bst64_u64
bst64_size(const BitStreamT64 *s)
{
  return s->size;
}

static bst64_u64
bst64_tell(const BitStreamT64 *s)
{
  return s->idx;
}

static bst64_u64
bst64_tell_bits(const BitStreamT64 *s)
{
  return s->idx;
}

static bst64_u64
bst64_tell_bytes(const BitStreamT64 *s)
{
  return (s->idx + (BST64_BITS_PER_BYTE - 1)) / BST64_BITS_PER_BYTE;
}

static bst64_u64
bst64_tell_u32(const BitStreamT64 *s)
{
  return bst64_tell_bytes(s) / 4;
}


/* ------------------------------------------------------------------ */
/* Read                                                                */
/* ------------------------------------------------------------------ */

/*
 * Read 'bits' bits starting at bit position 'idx' (random access).
 * bits must be 0-64.
 */
static bst64_u64
bst64_read_at(const BitStreamT64 *s,
              bst64_u64           idx,
              bst64_u64           bits)
{
  bst64_u64       byte_idx;
  bst64_u8        bit_off;
  const bst64_u8 *src;
  bst64_u64       mask;
  bst64_u64       acc;
  bst64_u8        remaining;

  assert((idx + bits) <= s->size);

  if(bits == 0)
    return 0;

  byte_idx = idx >> 3;
  bit_off  = (bst64_u8)(idx & 7);
  src      = &s->data[byte_idx];
  mask     = BST64_MASK(bits);

#if BST64_HAS_BSWAP
  if(bit_off + bits <= 64)
    {
      acc = bst64_load64_be(src);
      return (acc >> (64 - bit_off - bits)) & mask;
    }

  acc = bst64_load64_be(src);
  acc &= (BST64_ONE << (64 - bit_off)) - 1;
  remaining = (bst64_u8)(bits - (64 - bit_off));
  return (acc << remaining) | (src[8] >> (8 - remaining));
#else
  if(!bit_off && !(bits & 7))
    {
      bst64_u64 val = 0;
      bst64_u64 i;
      for(i = 0; i < (bits >> 3); i++)
        val = (val << 8) | src[i];
      return val;
    }

  if(bit_off + bits <= 64)
    {
      bst64_u64 n;
      acc = 0;
      n = (bit_off + bits + 7) >> 3;
      {
        bst64_u64 i;
        for(i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
      }
      return (acc >> (n * 8 - bit_off - bits)) & mask;
    }

  acc = 0;
  {
    bst64_u64 i;
    for(i = 0; i < 8; i++)
      acc = (acc << 8) | src[i];
  }
  acc &= (BST64_ONE << (64 - bit_off)) - 1;
  remaining = (bst64_u8)(bits - (64 - bit_off));
  return (acc << remaining) | (src[8] >> (8 - remaining));
#endif
}

/*
 * Read 'bits' bits at the current cursor and advance.
 */
static bst64_u64
bst64_read(BitStreamT64 *s,
           bst64_u64     bits)
{
  bst64_u64 v;

  v = bst64_read_at(s, s->idx, bits);
  s->idx += bits;

  return v;
}


/* ------------------------------------------------------------------ */
/* Write                                                               */
/* ------------------------------------------------------------------ */

/*
 * Write 'bits' bits of 'val' at bit position 'idx' (random access).
 * bits must be 0-64. val must fit in 'bits' bits.
 */
static void
bst64_write_at(BitStreamT64 *s,
               bst64_u64     idx,
               bst64_u64     bits,
               bst64_u64     val)
{
  bst64_u8  *dst;
  bst64_u8   bit_off;
  bst64_u64  remaining;

  assert((idx + bits) <= s->size);

  if(bits == 0)
    return;

  dst       = &s->data[idx >> 3];
  bit_off   = (bst64_u8)(idx & 7);
  remaining = bits;

  if(bit_off)
    {
      bst64_u8 avail = 8 - bit_off;
      bst64_u8 take  = (remaining < avail) ? (bst64_u8)remaining : avail;
      bst64_u8 shift = avail - take;
      bst64_u8 mask  = (bst64_u8)(((1U << take) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bst64_u8)(((val >> (remaining - take)) & ((BST64_ONE << take) - 1)) << shift);
      dst++;
      remaining -= take;
    }

  while(remaining >= 8)
    {
      remaining -= 8;
      *dst++ = (bst64_u8)((val >> remaining) & 0xFF);
    }

  if(remaining)
    {
      bst64_u8 shift = 8 - (bst64_u8)remaining;
      bst64_u8 mask  = (bst64_u8)(((1U << remaining) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bst64_u8)((val & ((BST64_ONE << remaining) - 1)) << shift);
    }
}

/*
 * Write 'bits' bits of 'val' at the current cursor and advance.
 */
static void
bst64_write(BitStreamT64 *s,
            bst64_u64     bits,
            bst64_u64     val)
{
  bst64_write_at(s, s->idx, bits, val);
  s->idx += bits;
}

/*
 * Write raw bytes at the current cursor. If byte-aligned, uses memcpy.
 */
static void
bst64_write_bytes(BitStreamT64   *s,
                  const bst64_u8 *src,
                  bst64_u64       count)
{
  if(!(s->idx & 7))
    {
      assert((s->idx + count * 8) <= s->size);
      memcpy(&s->data[s->idx >> 3], src, (size_t)count);
      s->idx += count * 8;
    }
  else
    {
      bst64_u64 i;
      for(i = 0; i < count; i++)
        bst64_write(s, 8, src[i]);
    }
}


/* ------------------------------------------------------------------ */
/* Fixed-width read/write macros                                       */
/*                                                                     */
/* BST64_DEFINE_FIXED(N) generates:                                    */
/*   bst64_read_fixed_N_at(s, idx)       - random-access read, N bits */
/*   bst64_read_fixed_N(s)               - streaming read, N bits     */
/*   bst64_write_fixed_N_at(s, idx, val) - random-access write        */
/*   bst64_write_fixed_N(s, val)         - streaming write            */
/*                                                                     */
/* For read widths 1-57, bit_off + N <= 64 always holds so the        */
/* 9-byte path is dead code and optimized away.                       */
/* ------------------------------------------------------------------ */

#if BST64_HAS_BSWAP

#define BST64_DEFINE_FIXED(N)                                              \
                                                                           \
static bst64_u64                                                           \
bst64_read_fixed_##N##_at(const BitStreamT64 *s,                           \
                          bst64_u64           idx)                         \
{                                                                          \
  const bst64_u64 BITS = (N);                                              \
  const bst64_u64 MASK = BST64_MASK(N);                                    \
  bst64_u64       byte_idx = idx >> 3;                                     \
  bst64_u8        bit_off  = (bst64_u8)(idx & 7);                         \
  const bst64_u8 *src      = &s->data[byte_idx];                          \
  bst64_u64       acc;                                                     \
                                                                           \
  assert((idx + BITS) <= s->size);                                         \
                                                                           \
  acc = bst64_load64_be(src);                                              \
  if(bit_off + BITS <= 64)                                                 \
    return (acc >> (64 - bit_off - BITS)) & MASK;                          \
                                                                           \
  {                                                                        \
    bst64_u8 remaining;                                                    \
    acc &= (BST64_ONE << (64 - bit_off)) - 1;                             \
    remaining = (bst64_u8)(BITS - (64 - bit_off));                         \
    return (acc << remaining) | (src[8] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bst64_u64                                                           \
bst64_read_fixed_##N(BitStreamT64 *s)                                      \
{                                                                          \
  bst64_u64 v = bst64_read_fixed_##N##_at(s, s->idx);                     \
  s->idx += (N);                                                           \
  return v;                                                                \
}                                                                          \
                                                                           \
static void                                                                \
bst64_write_fixed_##N##_at(BitStreamT64 *s,                                \
                           bst64_u64     idx,                              \
                           bst64_u64     val)                              \
{                                                                          \
  const bst64_u64 BITS = (N);                                              \
  bst64_u8  *dst;                                                          \
  bst64_u8   bit_off;                                                      \
  bst64_u64  remaining;                                                    \
                                                                           \
  assert((idx + BITS) <= s->size);                                         \
                                                                           \
  dst       = &s->data[idx >> 3];                                          \
  bit_off   = (bst64_u8)(idx & 7);                                        \
  remaining = BITS;                                                        \
                                                                           \
  if(bit_off)                                                              \
    {                                                                      \
      bst64_u8 avail = 8 - bit_off;                                       \
      bst64_u8 take  = (BITS < avail) ? (bst64_u8)BITS : avail;          \
      bst64_u8 shift = avail - take;                                      \
      bst64_u8 mask  = (bst64_u8)(((1U << take) - 1) << shift);          \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bst64_u8)(((val >> (remaining - take)) &                         \
                     ((BST64_ONE << take) - 1)) << shift);                \
      dst++;                                                               \
      remaining -= take;                                                   \
    }                                                                      \
                                                                           \
  while(remaining >= 8)                                                    \
    {                                                                      \
      remaining -= 8;                                                      \
      *dst++ = (bst64_u8)((val >> remaining) & 0xFF);                     \
    }                                                                      \
                                                                           \
  if(remaining)                                                            \
    {                                                                      \
      bst64_u8 shift = 8 - (bst64_u8)remaining;                          \
      bst64_u8 mask  = (bst64_u8)(((1U << remaining) - 1) << shift);     \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bst64_u8)((val & ((BST64_ONE << remaining) - 1)) << shift);     \
    }                                                                      \
}                                                                          \
                                                                           \
static void                                                                \
bst64_write_fixed_##N(BitStreamT64 *s,                                     \
                      bst64_u64     val)                                   \
{                                                                          \
  bst64_write_fixed_##N##_at(s, s->idx, val);                             \
  s->idx += (N);                                                           \
}

#else /* !BST64_HAS_BSWAP */

#define BST64_DEFINE_FIXED(N)                                              \
                                                                           \
static bst64_u64                                                           \
bst64_read_fixed_##N##_at(const BitStreamT64 *s,                           \
                          bst64_u64           idx)                         \
{                                                                          \
  const bst64_u64 BITS  = (N);                                             \
  const bst64_u64 BYTES = (7 + (N) + 7) >> 3;                             \
  const bst64_u64 MASK  = BST64_MASK(N);                                   \
  const bst64_u8 *src   = &s->data[idx >> 3];                             \
  bst64_u8        bit_off = (bst64_u8)(idx & 7);                          \
  bst64_u64       acc   = 0;                                               \
  bst64_u64       i;                                                       \
                                                                           \
  assert((idx + BITS) <= s->size);                                         \
                                                                           \
  if(bit_off + BITS <= 64)                                                 \
    {                                                                      \
      for(i = 0; i < BYTES; i++)                                           \
        acc = (acc << 8) | src[i];                                         \
      return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;                 \
    }                                                                      \
                                                                           \
  {                                                                        \
    bst64_u8 remaining;                                                    \
    for(i = 0; i < 8; i++)                                                 \
      acc = (acc << 8) | src[i];                                           \
    acc &= (BST64_ONE << (64 - bit_off)) - 1;                             \
    remaining = (bst64_u8)(BITS - (64 - bit_off));                         \
    return (acc << remaining) | (src[8] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bst64_u64                                                           \
bst64_read_fixed_##N(BitStreamT64 *s)                                      \
{                                                                          \
  bst64_u64 v = bst64_read_fixed_##N##_at(s, s->idx);                     \
  s->idx += (N);                                                           \
  return v;                                                                \
}                                                                          \
                                                                           \
static void                                                                \
bst64_write_fixed_##N##_at(BitStreamT64 *s,                                \
                           bst64_u64     idx,                              \
                           bst64_u64     val)                              \
{                                                                          \
  const bst64_u64 BITS = (N);                                              \
  bst64_u8  *dst;                                                          \
  bst64_u8   bit_off;                                                      \
  bst64_u64  remaining;                                                    \
                                                                           \
  assert((idx + BITS) <= s->size);                                         \
                                                                           \
  dst       = &s->data[idx >> 3];                                          \
  bit_off   = (bst64_u8)(idx & 7);                                        \
  remaining = BITS;                                                        \
                                                                           \
  if(bit_off)                                                              \
    {                                                                      \
      bst64_u8 avail = 8 - bit_off;                                       \
      bst64_u8 take  = (BITS < avail) ? (bst64_u8)BITS : avail;          \
      bst64_u8 shift = avail - take;                                      \
      bst64_u8 mask  = (bst64_u8)(((1U << take) - 1) << shift);          \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bst64_u8)(((val >> (remaining - take)) &                         \
                     ((BST64_ONE << take) - 1)) << shift);                \
      dst++;                                                               \
      remaining -= take;                                                   \
    }                                                                      \
                                                                           \
  while(remaining >= 8)                                                    \
    {                                                                      \
      remaining -= 8;                                                      \
      *dst++ = (bst64_u8)((val >> remaining) & 0xFF);                     \
    }                                                                      \
                                                                           \
  if(remaining)                                                            \
    {                                                                      \
      bst64_u8 shift = 8 - (bst64_u8)remaining;                          \
      bst64_u8 mask  = (bst64_u8)(((1U << remaining) - 1) << shift);     \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bst64_u8)((val & ((BST64_ONE << remaining) - 1)) << shift);     \
    }                                                                      \
}                                                                          \
                                                                           \
static void                                                                \
bst64_write_fixed_##N(BitStreamT64 *s,                                     \
                      bst64_u64     val)                                   \
{                                                                          \
  bst64_write_fixed_##N##_at(s, s->idx, val);                             \
  s->idx += (N);                                                           \
}

#endif /* BST64_HAS_BSWAP */


#define BST64_X_ALL \
  X(1)  X(2)  X(3)  X(4)  X(5)  X(6)  X(7)  X(8)  \
  X(9)  X(10) X(11) X(12) X(13) X(14) X(15) X(16) \
  X(17) X(18) X(19) X(20) X(21) X(22) X(23) X(24) \
  X(25) X(26) X(27) X(28) X(29) X(30) X(31) X(32) \
  X(33) X(34) X(35) X(36) X(37) X(38) X(39) X(40) \
  X(41) X(42) X(43) X(44) X(45) X(46) X(47) X(48) \
  X(49) X(50) X(51) X(52) X(53) X(54) X(55) X(56) \
  X(57) X(58) X(59) X(60) X(61) X(62) X(63) X(64)

#define X(n) BST64_DEFINE_FIXED(n)
BST64_X_ALL
#undef X

#endif /* BITSTREAM_T64_H */
