/*
 * bitstream_writer32.h - Header-only C89 bit stream writer
 *
 * Index/cursor:  64-bit (streams may be arbitrarily large)
 * Field width:   32-bit maximum (bits/val args are u32)
 *
 * Publicly this remains a compact fixed-capacity writer. Internally the
 * write/fixed-width core is shared with bitstream_dyn32.h by adapting
 * BitStreamWriter32 to a fixed BitStreamDyn32 view (realloc_fn == NULL).
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

#ifndef BSD32_U8
  #ifdef BSW32_U8
    #define BSD32_U8 BSW32_U8
  #endif
#endif

#ifndef BSD32_U32
  #ifdef BSW32_U32
    #define BSD32_U32 BSW32_U32
  #endif
#endif

#ifndef BSD32_U64
  #ifdef BSW32_U64
    #define BSD32_U64 BSW32_U64
  #endif
#endif

#include "bitstream_dyn32.h"

typedef bsd32_u8  bsw32_u8;
typedef bsd32_u32 bsw32_u32;
typedef bsd32_u64 bsw32_u64;

#define BSW32_BITS_PER_BYTE BSD32_BITS_PER_BYTE


typedef struct BitStreamWriter32
{
  bsw32_u8  *data;
  bsw32_u64  capacity; /* in bits */
  bsw32_u64  idx;      /* in bits */
} BitStreamWriter32;


static void
bsw32__to_dyn(const BitStreamWriter32 *src, BitStreamDyn32 *dst)
{
  dst->data        = (bsd32_u8 *)src->data;
  dst->capacity    = (bsd32_u64)src->capacity;
  dst->size        = (bsd32_u64)src->capacity;
  dst->idx         = (bsd32_u64)src->idx;
  dst->realloc_fn  = NULL;
  dst->realloc_ctx = NULL;
}

static void
bsw32__from_dyn(BitStreamWriter32 *dst, const BitStreamDyn32 *src)
{
  dst->data     = (bsw32_u8 *)src->data;
  dst->capacity = (bsw32_u64)src->capacity;
  dst->idx      = (bsw32_u64)src->idx;
}


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
  BitStreamDyn32 dyn;

  bsw32__to_dyn(w, &dyn);
  bsd32_write_at(&dyn, (bsd32_u64)idx, (bsd32_u32)bits, (bsd32_u32)val);
  bsw32__from_dyn(w, &dyn);
}


/*
 * Write 'bits' bits of 'val' at the current cursor and advance.
 */
static void
bsw32_write(BitStreamWriter32 *w,
            bsw32_u32          bits,
            bsw32_u32          val)
{
  BitStreamDyn32 dyn;

  bsw32__to_dyn(w, &dyn);
  bsd32_write(&dyn, (bsd32_u32)bits, (bsd32_u32)val);
  bsw32__from_dyn(w, &dyn);
}


/*
 * Write raw bytes at the current cursor. If byte-aligned, uses memcpy.
 */
static void
bsw32_write_bytes(BitStreamWriter32 *w,
                  const bsw32_u8    *src,
                  bsw32_u64          count)
{
  BitStreamDyn32 dyn;

  bsw32__to_dyn(w, &dyn);
  bsd32_write_bytes(&dyn, (const bsd32_u8 *)src, (bsd32_u64)count);
  bsw32__from_dyn(w, &dyn);
}


/*
 * BSW32_DEFINE_WRITE_FIXED(N) - generates two functions:
 *
 *   bsw32_write_fixed_N_at(w, idx, val)  - random access  (idx is u64)
 *   bsw32_write_fixed_N(w, val)          - streaming
 */

#define BSW32_DEFINE_WRITE_FIXED(N)                                         \
                                                                            \
static void                                                                 \
bsw32_write_fixed_##N##_at(BitStreamWriter32 *w,                            \
                           bsw32_u64          idx,                          \
                           bsw32_u32          val)                          \
{                                                                           \
  BitStreamDyn32 dyn;                                                       \
                                                                            \
  bsw32__to_dyn(w, &dyn);                                                   \
  bsd32_write_fixed_##N##_at(&dyn, (bsd32_u64)idx, (bsd32_u32)val);        \
  bsw32__from_dyn(w, &dyn);                                                 \
}                                                                           \
                                                                            \
static void                                                                 \
bsw32_write_fixed_##N(BitStreamWriter32 *w,                                 \
                      bsw32_u32          val)                               \
{                                                                           \
  BitStreamDyn32 dyn;                                                       \
                                                                            \
  bsw32__to_dyn(w, &dyn);                                                   \
  bsd32_write_fixed_##N(&dyn, (bsd32_u32)val);                              \
  bsw32__from_dyn(w, &dyn);                                                 \
}

#define BSW32_X_ALL BSD32_X_ALL

#define X(n) BSW32_DEFINE_WRITE_FIXED(n)
BSW32_X_ALL
#undef X

#endif /* BITSTREAM_WRITER32_H */
