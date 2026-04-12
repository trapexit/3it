/*
 * bitstream_writer64.h - Header-only C89 bit stream writer (64-bit)
 *
 * All functions are static to allow header-only usage without linker
 * conflicts. Compilers will inline at optimization levels >= -O1.
 *
 * Requires compiler support for 64-bit integers (unsigned long long
 * or equivalent). Override BSW64_U64 if your platform uses a
 * different 64-bit type.
 *
 * The caller manages the buffer and must ensure sufficient size.
 *
 * Usage:
 *   BitStreamWriter64 w;
 *   bsw64_init(&w, buffer, buffer_size_bytes, 0);
 *   bsw64_write(&w, 6, 0x1F);
 *
 * For compile-time known widths (all 1-64 pre-defined via X-macro):
 *   bsw64_write_fixed_6(&w, 0x1F);
 */

#ifndef BITSTREAM_WRITER64_H
#define BITSTREAM_WRITER64_H

#include <assert.h>
#include <string.h>

#ifndef BSW64_U8
typedef unsigned char      bsw64_u8;
#else
typedef BSW64_U8           bsw64_u8;
#endif

#ifndef BSW64_U64
typedef unsigned long long bsw64_u64;
#else
typedef BSW64_U64          bsw64_u64;
#endif

#define BSW64_BITS_PER_BYTE 8
#define BSW64_ONE  ((bsw64_u64)1)
#define BSW64_MASK(b) (((b) == 64) ? ~(bsw64_u64)0 : (BSW64_ONE << (b)) - 1)


typedef struct BitStreamWriter64
{
  bsw64_u8  *data;
  bsw64_u64  capacity; /* in bits */
  bsw64_u64  idx;      /* in bits */
} BitStreamWriter64;


static void
bsw64_init(BitStreamWriter64 *w,
           bsw64_u8          *data,
           bsw64_u64          capacity_in_bytes,
           bsw64_u64          idx)
{
  w->data     = data;
  w->capacity = capacity_in_bytes * BSW64_BITS_PER_BYTE;
  w->idx      = idx;
}

static void
bsw64_seek(BitStreamWriter64 *w,
           bsw64_u64          idx)
{
  w->idx = idx;
}

static void
bsw64_rewind(BitStreamWriter64 *w)
{
  w->idx = 0;
}

static void
bsw64_rewind_bits(BitStreamWriter64 *w,
                  bsw64_u64          bits)
{
  w->idx -= bits;
}

static void
bsw64_skip(BitStreamWriter64 *w,
           bsw64_u64          bits)
{
  w->idx += bits;
}

static int
bsw64_on_8bit_boundary(const BitStreamWriter64 *w)
{
  return !(w->idx & 0x7);
}

static bsw64_u8
bsw64_bits_to_8bit_boundary(const BitStreamWriter64 *w)
{
  return (bsw64_u8)((0x08 - (w->idx & 0x7)) & 0x7);
}

static int
bsw64_on_16bit_boundary(const BitStreamWriter64 *w)
{
  return !(w->idx & 0xF);
}

static bsw64_u8
bsw64_bits_to_16bit_boundary(const BitStreamWriter64 *w)
{
  return (bsw64_u8)((0x10 - (w->idx & 0xF)) & 0xF);
}

static int
bsw64_on_32bit_boundary(const BitStreamWriter64 *w)
{
  return !(w->idx & 0x1F);
}

static bsw64_u8
bsw64_bits_to_32bit_boundary(const BitStreamWriter64 *w)
{
  return (bsw64_u8)((0x20 - (w->idx & 0x1F)) & 0x1F);
}

static int
bsw64_on_64bit_boundary(const BitStreamWriter64 *w)
{
  return !(w->idx & 0x3F);
}

static bsw64_u8
bsw64_bits_to_64bit_boundary(const BitStreamWriter64 *w)
{
  return (bsw64_u8)((0x40 - (w->idx & 0x3F)) & 0x3F);
}

static bsw64_u64
bsw64_tell(const BitStreamWriter64 *w)
{
  return w->idx;
}

static bsw64_u64
bsw64_tell_bits(const BitStreamWriter64 *w)
{
  return w->idx;
}

static bsw64_u64
bsw64_tell_bytes(const BitStreamWriter64 *w)
{
  return (w->idx + (BSW64_BITS_PER_BYTE - 1)) / BSW64_BITS_PER_BYTE;
}

static bsw64_u64
bsw64_tell_u32(const BitStreamWriter64 *w)
{
  return bsw64_tell_bytes(w) / 4;
}


/*
 * Write 'bits' bits of 'val' at bit position 'idx' (random access).
 * bits must be 0-64. val must fit in 'bits' bits.
 */
static void
bsw64_write_at(BitStreamWriter64 *w,
               bsw64_u64          idx,
               bsw64_u64          bits,
               bsw64_u64          val)
{
  bsw64_u8  *dst;
  bsw64_u8   bit_off;
  bsw64_u64  remaining;

  assert((idx + bits) <= w->capacity);

  if(bits == 0)
    return;

  dst       = &w->data[idx >> 3];
  bit_off   = (bsw64_u8)(idx & 7);
  remaining = bits;

  /* first partial byte */
  if(bit_off)
    {
      bsw64_u8 avail = 8 - bit_off;
      bsw64_u8 take  = (remaining < avail) ? (bsw64_u8)remaining : avail;
      bsw64_u8 shift = avail - take;
      bsw64_u8 mask  = (bsw64_u8)(((1U << take) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bsw64_u8)(((val >> (remaining - take)) & ((BSW64_ONE << take) - 1)) << shift);
      dst++;
      remaining -= take;
    }

  /* full middle bytes */
  while(remaining >= 8)
    {
      remaining -= 8;
      *dst++ = (bsw64_u8)((val >> remaining) & 0xFF);
    }

  /* last partial byte */
  if(remaining)
    {
      bsw64_u8 shift = 8 - (bsw64_u8)remaining;
      bsw64_u8 mask  = (bsw64_u8)(((1U << remaining) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bsw64_u8)((val & ((BSW64_ONE << remaining) - 1)) << shift);
    }
}


/*
 * Write 'bits' bits of 'val' at the current cursor and advance.
 */
static void
bsw64_write(BitStreamWriter64 *w,
            bsw64_u64          bits,
            bsw64_u64          val)
{
  bsw64_write_at(w, w->idx, bits, val);
  w->idx += bits;
}


/*
 * Write raw bytes at the current cursor. If byte-aligned, uses memcpy.
 */
static void
bsw64_write_bytes(BitStreamWriter64 *w,
                  const bsw64_u8    *src,
                  bsw64_u64          count)
{
  if(!(w->idx & 7))
    {
      assert((w->idx + count * 8) <= w->capacity);
      memcpy(&w->data[w->idx >> 3], src, (size_t)count);
      w->idx += count * 8;
    }
  else
    {
      bsw64_u64 i;
      for(i = 0; i < count; i++)
        bsw64_write(w, 8, src[i]);
    }
}


/*
 * BSW64_DEFINE_WRITE_FIXED(N) - generates two functions:
 *
 *   bsw64_write_fixed_N_at(w, idx, val)  - random access
 *   bsw64_write_fixed_N(w, val)          - streaming
 */

#define BSW64_DEFINE_WRITE_FIXED(N)                                        \
                                                                           \
static void                                                                \
bsw64_write_fixed_##N##_at(BitStreamWriter64 *w,                           \
                           bsw64_u64          idx,                         \
                           bsw64_u64          val)                         \
{                                                                          \
  const bsw64_u64 BITS = (N);                                              \
  bsw64_u8  *dst;                                                          \
  bsw64_u8   bit_off;                                                      \
  bsw64_u64  remaining;                                                    \
                                                                           \
  assert((idx + BITS) <= w->capacity);                                     \
                                                                           \
  dst       = &w->data[idx >> 3];                                          \
  bit_off   = (bsw64_u8)(idx & 7);                                        \
  remaining = BITS;                                                        \
                                                                           \
  if(bit_off)                                                              \
    {                                                                      \
      bsw64_u8 avail = 8 - bit_off;                                       \
      bsw64_u8 take  = (BITS < avail) ? (bsw64_u8)BITS : avail;          \
      bsw64_u8 shift = avail - take;                                      \
      bsw64_u8 mask  = (bsw64_u8)(((1U << take) - 1) << shift);          \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsw64_u8)(((val >> (remaining - take)) &                         \
                     ((BSW64_ONE << take) - 1)) << shift);                \
      dst++;                                                               \
      remaining -= take;                                                   \
    }                                                                      \
                                                                           \
  while(remaining >= 8)                                                    \
    {                                                                      \
      remaining -= 8;                                                      \
      *dst++ = (bsw64_u8)((val >> remaining) & 0xFF);                     \
    }                                                                      \
                                                                           \
  if(remaining)                                                            \
    {                                                                      \
      bsw64_u8 shift = 8 - (bsw64_u8)remaining;                          \
      bsw64_u8 mask  = (bsw64_u8)(((1U << remaining) - 1) << shift);     \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsw64_u8)((val & ((BSW64_ONE << remaining) - 1)) << shift);     \
    }                                                                      \
}                                                                          \
                                                                           \
static void                                                                \
bsw64_write_fixed_##N(BitStreamWriter64 *w,                                \
                      bsw64_u64          val)                              \
{                                                                          \
  bsw64_write_fixed_##N##_at(w, w->idx, val);                             \
  w->idx += (N);                                                           \
}


#define BSW64_X_ALL \
  X(1)  X(2)  X(3)  X(4)  X(5)  X(6)  X(7)  X(8)  \
  X(9)  X(10) X(11) X(12) X(13) X(14) X(15) X(16) \
  X(17) X(18) X(19) X(20) X(21) X(22) X(23) X(24) \
  X(25) X(26) X(27) X(28) X(29) X(30) X(31) X(32) \
  X(33) X(34) X(35) X(36) X(37) X(38) X(39) X(40) \
  X(41) X(42) X(43) X(44) X(45) X(46) X(47) X(48) \
  X(49) X(50) X(51) X(52) X(53) X(54) X(55) X(56) \
  X(57) X(58) X(59) X(60) X(61) X(62) X(63) X(64)

#define X(n) BSW64_DEFINE_WRITE_FIXED(n)
BSW64_X_ALL
#undef X

#endif /* BITSTREAM_WRITER64_H */
