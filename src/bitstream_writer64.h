/*
 * bitstream_writer64.h - Header-only C89 bit stream writer (64-bit)
 *
 * Publicly this remains a compact fixed-capacity writer. Internally the
 * write/fixed-width core is shared with bitstream_dyn64.h by adapting
 * BitStreamWriter64 to a fixed BitStreamDyn64 view (realloc_fn == NULL).
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

#ifndef BSD64_U8
  #ifdef BSW64_U8
    #define BSD64_U8 BSW64_U8
  #endif
#endif

#ifndef BSD64_U64
  #ifdef BSW64_U64
    #define BSD64_U64 BSW64_U64
  #endif
#endif

#include "bitstream_dyn64.h"

typedef bsd64_u8  bsw64_u8;
typedef bsd64_u64 bsw64_u64;

#define BSW64_BITS_PER_BYTE BSD64_BITS_PER_BYTE
#define BSW64_ONE           BSD64_ONE
#define BSW64_MASK(b)       BSD64_MASK(b)


typedef struct BitStreamWriter64
{
  bsw64_u8  *data;
  bsw64_u64  capacity; /* in bits */
  bsw64_u64  idx;      /* in bits */
} BitStreamWriter64;


static void
bsw64__to_dyn(const BitStreamWriter64 *src, BitStreamDyn64 *dst)
{
  dst->data        = (bsd64_u8 *)src->data;
  dst->capacity    = (bsd64_u64)src->capacity;
  dst->size        = (bsd64_u64)src->capacity;
  dst->idx         = (bsd64_u64)src->idx;
  dst->realloc_fn  = NULL;
  dst->realloc_ctx = NULL;
}

static void
bsw64__from_dyn(BitStreamWriter64 *dst, const BitStreamDyn64 *src)
{
  dst->data     = (bsw64_u8 *)src->data;
  dst->capacity = (bsw64_u64)src->capacity;
  dst->idx      = (bsw64_u64)src->idx;
}


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
  BitStreamDyn64 dyn;

  bsw64__to_dyn(w, &dyn);
  bsd64_write_at(&dyn, (bsd64_u64)idx, (bsd64_u64)bits, (bsd64_u64)val);
  bsw64__from_dyn(w, &dyn);
}


/*
 * Write 'bits' bits of 'val' at the current cursor and advance.
 */
static void
bsw64_write(BitStreamWriter64 *w,
            bsw64_u64          bits,
            bsw64_u64          val)
{
  BitStreamDyn64 dyn;

  bsw64__to_dyn(w, &dyn);
  bsd64_write(&dyn, (bsd64_u64)bits, (bsd64_u64)val);
  bsw64__from_dyn(w, &dyn);
}


/*
 * Write raw bytes at the current cursor. If byte-aligned, uses memcpy.
 */
static void
bsw64_write_bytes(BitStreamWriter64 *w,
                  const bsw64_u8    *src,
                  bsw64_u64          count)
{
  BitStreamDyn64 dyn;

  bsw64__to_dyn(w, &dyn);
  bsd64_write_bytes(&dyn, (const bsd64_u8 *)src, (bsd64_u64)count);
  bsw64__from_dyn(w, &dyn);
}


/*
 * BSW64_DEFINE_WRITE_FIXED(N) - generates two functions:
 *
 *   bsw64_write_fixed_N_at(w, idx, val)  - random access
 *   bsw64_write_fixed_N(w, val)          - streaming
 */

#define BSW64_DEFINE_WRITE_FIXED(N)                                         \
                                                                            \
static void                                                                 \
bsw64_write_fixed_##N##_at(BitStreamWriter64 *w,                            \
                           bsw64_u64          idx,                          \
                           bsw64_u64          val)                          \
{                                                                           \
  BitStreamDyn64 dyn;                                                       \
                                                                            \
  bsw64__to_dyn(w, &dyn);                                                   \
  bsd64_write_fixed_##N##_at(&dyn, (bsd64_u64)idx, (bsd64_u64)val);        \
  bsw64__from_dyn(w, &dyn);                                                 \
}                                                                           \
                                                                            \
static void                                                                 \
bsw64_write_fixed_##N(BitStreamWriter64 *w,                                 \
                      bsw64_u64          val)                               \
{                                                                           \
  BitStreamDyn64 dyn;                                                       \
                                                                            \
  bsw64__to_dyn(w, &dyn);                                                   \
  bsd64_write_fixed_##N(&dyn, (bsd64_u64)val);                              \
  bsw64__from_dyn(w, &dyn);                                                 \
}

#define BSW64_X_ALL BSD64_X_ALL

#define X(n) BSW64_DEFINE_WRITE_FIXED(n)
BSW64_X_ALL
#undef X

#endif /* BITSTREAM_WRITER64_H */
