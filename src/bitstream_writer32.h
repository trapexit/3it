/*
 * bitstream_writer32.h - Header-only C89 bit stream writer
 *
 * Index/cursor:  64-bit (streams may be arbitrarily large)
 * Field width:   32-bit maximum (bits/val args are u32)
 *
 * All functions are static to allow header-only usage without linker
 * conflicts. Compilers will inline at optimization levels >= -O1.
 *
 * The caller manages the buffer and must ensure sufficient size.
 *
 * Usage:
 *   BitStreamWriter32 w;
 *   bsw32_init(&w, buffer, buffer_size_bytes, 0);
 *   bsw32_write(&w, 6, 0x1F);
 *
 * For compile-time known widths (all 1-32 pre-defined via X-macro):
 *   bsw32_write_fixed_6(&w, 0x1F);
 */

#ifndef BITSTREAM_WRITER32_H
#define BITSTREAM_WRITER32_H

#include <assert.h>
#include <string.h>

#ifndef BSW32_U8
typedef unsigned char      bsw32_u8;
#else
typedef BSW32_U8           bsw32_u8;
#endif

#ifndef BSW32_U32
typedef unsigned int       bsw32_u32;
#else
typedef BSW32_U32          bsw32_u32;
#endif

#ifndef BSW32_U64
typedef unsigned long long bsw32_u64;
#else
typedef BSW32_U64          bsw32_u64;
#endif

#define BSW32_BITS_PER_BYTE 8


typedef struct BitStreamWriter32
{
  bsw32_u8  *data;
  bsw32_u64  capacity; /* in bits */
  bsw32_u64  idx;      /* in bits */
} BitStreamWriter32;


static void
bsw32_init(BitStreamWriter32 *w,
           bsw32_u8          *data,
           bsw32_u64          capacity_in_bytes,
           bsw32_u64          idx)
{
  w->data     = data;
  w->capacity = capacity_in_bytes * BSW32_BITS_PER_BYTE;
  w->idx      = idx;
}

static void
bsw32_seek(BitStreamWriter32 *w,
           bsw32_u64          idx)
{
  w->idx = idx;
}

static void
bsw32_rewind(BitStreamWriter32 *w)
{
  w->idx = 0;
}

static void
bsw32_rewind_bits(BitStreamWriter32 *w,
                  bsw32_u64          bits)
{
  w->idx -= bits;
}

static void
bsw32_skip(BitStreamWriter32 *w,
           bsw32_u64          bits)
{
  w->idx += bits;
}

static int
bsw32_on_8bit_boundary(const BitStreamWriter32 *w)
{
  return !(w->idx & 0x7);
}

static bsw32_u8
bsw32_bits_to_8bit_boundary(const BitStreamWriter32 *w)
{
  return (bsw32_u8)((0x08 - (w->idx & 0x7)) & 0x7);
}

static int
bsw32_on_16bit_boundary(const BitStreamWriter32 *w)
{
  return !(w->idx & 0xF);
}

static bsw32_u8
bsw32_bits_to_16bit_boundary(const BitStreamWriter32 *w)
{
  return (bsw32_u8)((0x10 - (w->idx & 0xF)) & 0xF);
}

static int
bsw32_on_32bit_boundary(const BitStreamWriter32 *w)
{
  return !(w->idx & 0x1F);
}

static bsw32_u8
bsw32_bits_to_32bit_boundary(const BitStreamWriter32 *w)
{
  return (bsw32_u8)((0x20 - (w->idx & 0x1F)) & 0x1F);
}

static void
bsw32_skip_to_8bit_boundary(BitStreamWriter32 *w)
{
  if(w->idx & 0x7)
    w->idx += 0x8 - (w->idx & 0x7);
}

static void
bsw32_skip_to_16bit_boundary(BitStreamWriter32 *w)
{
  if(w->idx & 0x0F)
    w->idx += 0x10 - (w->idx & 0x0F);
}

static void
bsw32_skip_to_32bit_boundary(BitStreamWriter32 *w)
{
  if(w->idx & 0x1F)
    w->idx += 0x20 - (w->idx & 0x1F);
}

static bsw32_u64
bsw32_tell(const BitStreamWriter32 *w)
{
  return w->idx;
}

static bsw32_u64
bsw32_tell_bits(const BitStreamWriter32 *w)
{
  return w->idx;
}

static bsw32_u64
bsw32_tell_bytes(const BitStreamWriter32 *w)
{
  return (w->idx + (BSW32_BITS_PER_BYTE - 1)) / BSW32_BITS_PER_BYTE;
}

static bsw32_u64
bsw32_tell_u32(const BitStreamWriter32 *w)
{
  return bsw32_tell_bytes(w) / 4;
}


/*
 * Write 'bits' bits of 'val' at bit position 'idx' (random access).
 * bits must be 1-32. val must fit in 'bits' bits.
 */
static void
bsw32_write_at(BitStreamWriter32 *w,
               bsw32_u64          idx,
               bsw32_u32          bits,
               bsw32_u32          val)
{
  bsw32_u8  *dst;
  bsw32_u8   bit_off;
  bsw32_u32  remaining;

  assert((idx + bits) <= w->capacity);

  if(bits == 0)
    return;

  dst       = &w->data[idx >> 3];
  bit_off   = (bsw32_u8)(idx & 7);
  remaining = bits;

  /* first partial byte */
  if(bit_off)
    {
      bsw32_u8 avail = 8 - bit_off;
      bsw32_u8 take  = (remaining < avail) ? (bsw32_u8)remaining : avail;
      bsw32_u8 shift = avail - take;
      bsw32_u8 mask  = (bsw32_u8)(((1U << take) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bsw32_u8)(((val >> (remaining - take)) & (((bsw32_u32)1 << take) - 1)) << shift);
      dst++;
      remaining -= take;
    }

  /* full middle bytes */
  while(remaining >= 8)
    {
      remaining -= 8;
      *dst++ = (bsw32_u8)((val >> remaining) & 0xFF);
    }

  /* last partial byte */
  if(remaining)
    {
      bsw32_u8 shift = 8 - (bsw32_u8)remaining;
      bsw32_u8 mask  = (bsw32_u8)(((1U << remaining) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bsw32_u8)((val & (((bsw32_u32)1 << remaining) - 1)) << shift);
    }
}


/*
 * Write 'bits' bits of 'val' at the current cursor and advance.
 */
static void
bsw32_write(BitStreamWriter32 *w,
            bsw32_u32          bits,
            bsw32_u32          val)
{
  bsw32_write_at(w, w->idx, bits, val);
  w->idx += bits;
}


/*
 * Write raw bytes at the current cursor. If byte-aligned, uses memcpy.
 */
static void
bsw32_write_bytes(BitStreamWriter32 *w,
                  const bsw32_u8    *src,
                  bsw32_u64          count)
{
  if(!(w->idx & 7))
    {
      assert((w->idx + count * 8) <= w->capacity);
      memcpy(&w->data[w->idx >> 3], src, (size_t)count);
      w->idx += count * 8;
    }
  else
    {
      bsw32_u64 i;
      for(i = 0; i < count; i++)
        bsw32_write(w, 8, src[i]);
    }
}


/*
 * BSW32_DEFINE_WRITE_FIXED(N) - generates two functions:
 *
 *   bsw32_write_fixed_N_at(w, idx, val)  - random access  (idx is u64)
 *   bsw32_write_fixed_N(w, val)          - streaming
 */

#define BSW32_DEFINE_WRITE_FIXED(N)                                        \
                                                                           \
static void                                                                \
bsw32_write_fixed_##N##_at(BitStreamWriter32 *w,                           \
                           bsw32_u64          idx,                         \
                           bsw32_u32          val)                         \
{                                                                          \
  const bsw32_u32 BITS = (N);                                              \
  bsw32_u8  *dst;                                                          \
  bsw32_u8   bit_off;                                                      \
  bsw32_u32  remaining;                                                    \
                                                                           \
  assert((idx + BITS) <= w->capacity);                                     \
                                                                           \
  dst       = &w->data[idx >> 3];                                          \
  bit_off   = (bsw32_u8)(idx & 7);                                        \
  remaining = BITS;                                                        \
                                                                           \
  if(bit_off)                                                              \
    {                                                                      \
      bsw32_u8 avail = 8 - bit_off;                                       \
      bsw32_u8 take  = (BITS < avail) ? (bsw32_u8)BITS : avail;          \
      bsw32_u8 shift = avail - take;                                      \
      bsw32_u8 mask  = (bsw32_u8)(((1U << take) - 1) << shift);          \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsw32_u8)(((val >> (remaining - take)) &                         \
                     (((bsw32_u32)1 << take) - 1)) << shift);            \
      dst++;                                                               \
      remaining -= take;                                                   \
    }                                                                      \
                                                                           \
  while(remaining >= 8)                                                    \
    {                                                                      \
      remaining -= 8;                                                      \
      *dst++ = (bsw32_u8)((val >> remaining) & 0xFF);                     \
    }                                                                      \
                                                                           \
  if(remaining)                                                            \
    {                                                                      \
      bsw32_u8 shift = 8 - (bsw32_u8)remaining;                          \
      bsw32_u8 mask  = (bsw32_u8)(((1U << remaining) - 1) << shift);     \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsw32_u8)((val & (((bsw32_u32)1 << remaining) - 1)) << shift);  \
    }                                                                      \
}                                                                          \
                                                                           \
static void                                                                \
bsw32_write_fixed_##N(BitStreamWriter32 *w,                                \
                      bsw32_u32          val)                              \
{                                                                          \
  bsw32_write_fixed_##N##_at(w, w->idx, val);                             \
  w->idx += (N);                                                           \
}


#define BSW32_X_ALL \
  X(1)  X(2)  X(3)  X(4)  X(5)  X(6)  X(7)  X(8)  \
  X(9)  X(10) X(11) X(12) X(13) X(14) X(15) X(16) \
  X(17) X(18) X(19) X(20) X(21) X(22) X(23) X(24) \
  X(25) X(26) X(27) X(28) X(29) X(30) X(31) X(32)

#define X(n) BSW32_DEFINE_WRITE_FIXED(n)
BSW32_X_ALL
#undef X

#endif /* BITSTREAM_WRITER32_H */
