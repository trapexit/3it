/*
 * bitstream_reader32.h - Header-only C89 bit stream reader
 *
 * Index/cursor:  64-bit (streams may be arbitrarily large)
 * Field width:   32-bit maximum (read returns u32, bits arg is u32)
 *
 * Publicly this remains a compact read-only view. Internally the
 * read/fixed-width core is shared with bitstream_dyn32.h by adapting
 * BitStreamReader32 to a fixed read-only BitStreamDyn32 view
 * (realloc_fn == NULL).
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

#ifndef BSD32_U8
  #ifdef BSR32_U8
    #define BSD32_U8 BSR32_U8
  #endif
#endif

#ifndef BSD32_U32
  #ifdef BSR32_U32
    #define BSD32_U32 BSR32_U32
  #endif
#endif

#ifndef BSD32_U64
  #ifdef BSR32_U64
    #define BSD32_U64 BSR32_U64
  #endif
#endif

#include "bitstream_dyn32.h"

typedef bsd32_u8  bsr32_u8;
typedef bsd32_u32 bsr32_u32;
typedef bsd32_u64 bsr32_u64;

#define BSR32_BITS_PER_BYTE BSD32_BITS_PER_BYTE
#define BSR32_HAS_BSWAP     BSD32_HAS_BSWAP

#if BSR32_HAS_BSWAP
  #define bsr32_bswap32(v) bsd32_bswap32(v)

static bsr32_u32
bsr32_load32_be(const bsr32_u8 *p)
{
  return (bsr32_u32)bsd32_load32_be((const bsd32_u8 *)p);
}
#endif


typedef struct BitStreamReader32
{
  const bsr32_u8 *data;
  bsr32_u64       size; /* in bits */
  bsr32_u64       idx;  /* in bits */
} BitStreamReader32;


static void
bsr32__to_dyn(const BitStreamReader32 *src, BitStreamDyn32 *dst)
{
  dst->data        = (bsd32_u8 *)src->data;
  dst->capacity    = (bsd32_u64)src->size;
  dst->size        = (bsd32_u64)src->size;
  dst->idx         = (bsd32_u64)src->idx;
  dst->realloc_fn  = NULL;
  dst->realloc_ctx = NULL;
}

static void
bsr32__from_dyn(BitStreamReader32 *dst, const BitStreamDyn32 *src)
{
  dst->data = (const bsr32_u8 *)src->data;
  dst->size = (bsr32_u64)src->size;
  dst->idx  = (bsr32_u64)src->idx;
}


static void
bsr32_init(BitStreamReader32 *r,
           const bsr32_u8    *data,
           bsr32_u64          size_in_bytes,
           bsr32_u64          idx)
{
  r->data = data;
  r->size = size_in_bytes * BSR32_BITS_PER_BYTE;
  r->idx  = idx;
}

static void
bsr32_seek(BitStreamReader32 *r,
           bsr32_u64          idx)
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
                  bsr32_u64          bits)
{
  r->idx -= bits;
}

static void
bsr32_skip(BitStreamReader32 *r,
           bsr32_u64          bits)
{
  r->idx += bits;
}

static int
bsr32_on_8bit_boundary(const BitStreamReader32 *r)
{
  return !(r->idx & 0x7);
}

static bsr32_u8
bsr32_bits_to_8bit_boundary(const BitStreamReader32 *r)
{
  return (bsr32_u8)((0x08 - (r->idx & 0x7)) & 0x7);
}

static int
bsr32_on_16bit_boundary(const BitStreamReader32 *r)
{
  return !(r->idx & 0xF);
}

static bsr32_u8
bsr32_bits_to_16bit_boundary(const BitStreamReader32 *r)
{
  return (bsr32_u8)((0x10 - (r->idx & 0xF)) & 0xF);
}

static int
bsr32_on_32bit_boundary(const BitStreamReader32 *r)
{
  return !(r->idx & 0x1F);
}

static bsr32_u8
bsr32_bits_to_32bit_boundary(const BitStreamReader32 *r)
{
  return (bsr32_u8)((0x20 - (r->idx & 0x1F)) & 0x1F);
}

static void
bsr32_skip_to_8bit_boundary(BitStreamReader32 *r)
{
  if(r->idx & 0x7)
    r->idx += 0x8 - (r->idx & 0x7);
}

static void
bsr32_skip_to_16bit_boundary(BitStreamReader32 *r)
{
  if(r->idx & 0x0F)
    r->idx += 0x10 - (r->idx & 0x0F);
}

static void
bsr32_skip_to_32bit_boundary(BitStreamReader32 *r)
{
  if(r->idx & 0x1F)
    r->idx += 0x20 - (r->idx & 0x1F);
}

static bsr32_u64
bsr32_size(const BitStreamReader32 *r)
{
  return r->size;
}

static bsr32_u64
bsr32_tell(const BitStreamReader32 *r)
{
  return r->idx;
}

static bsr32_u64
bsr32_tell_bits(const BitStreamReader32 *r)
{
  return r->idx;
}

static bsr32_u64
bsr32_tell_bytes(const BitStreamReader32 *r)
{
  return (r->idx + (BSR32_BITS_PER_BYTE - 1)) / BSR32_BITS_PER_BYTE;
}


/*
 * Read 'bits' bits starting at bit position 'idx' (random access).
 * bits must be 1-32.
 */
static bsr32_u32
bsr32_read_at(const BitStreamReader32 *r,
              bsr32_u64                idx,
              bsr32_u32                bits)
{
  BitStreamDyn32 dyn;

  bsr32__to_dyn(r, &dyn);
  return (bsr32_u32)bsd32_read_at(&dyn, (bsd32_u64)idx, (bsd32_u32)bits);
}


/*
 * Read 'bits' bits at the current cursor and advance.
 */
static bsr32_u32
bsr32_read(BitStreamReader32 *r,
           bsr32_u32          bits)
{
  BitStreamDyn32 dyn;
  bsr32_u32      v;

  bsr32__to_dyn(r, &dyn);
  v = (bsr32_u32)bsd32_read(&dyn, (bsd32_u32)bits);
  bsr32__from_dyn(r, &dyn);

  return v;
}


/*
 * BSR32_DEFINE_READ_FIXED(N) - generates two functions:
 *
 *   bsr32_read_fixed_N_at(r, idx)  - random access, N bits  (idx is u64)
 *   bsr32_read_fixed_N(r)          - streaming, N bits
 */

#define BSR32_DEFINE_READ_FIXED(N)                                          \
                                                                           \
static bsr32_u32                                                           \
bsr32_read_fixed_##N##_at(const BitStreamReader32 *r,                      \
                          bsr32_u64                idx)                    \
{                                                                          \
  BitStreamDyn32 dyn;                                                      \
                                                                           \
  bsr32__to_dyn(r, &dyn);                                                  \
  return (bsr32_u32)bsd32_read_fixed_##N##_at(&dyn, (bsd32_u64)idx);      \
}                                                                          \
                                                                           \
static bsr32_u32                                                           \
bsr32_read_fixed_##N(BitStreamReader32 *r)                                 \
{                                                                          \
  BitStreamDyn32 dyn;                                                      \
  bsr32_u32      v;                                                        \
                                                                           \
  bsr32__to_dyn(r, &dyn);                                                  \
  v = (bsr32_u32)bsd32_read_fixed_##N(&dyn);                               \
  bsr32__from_dyn(r, &dyn);                                                \
                                                                           \
  return v;                                                                \
}

#define BSR32_X_ALL BSD32_X_ALL

#define X(n) BSR32_DEFINE_READ_FIXED(n)
BSR32_X_ALL
#undef X

#endif /* BITSTREAM_READER32_H */
