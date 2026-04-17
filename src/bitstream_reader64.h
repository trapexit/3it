/*
 * bitstream_reader64.h - Header-only C89 bit stream reader (64-bit)
 *
 * Publicly this remains a compact read-only view. Internally the
 * read/fixed-width core is shared with bitstream_dyn64.h by adapting
 * BitStreamReader64 to a fixed read-only BitStreamDyn64 view
 * (realloc_fn == NULL).
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

#ifndef BSD64_U8
  #ifdef BSR64_U8
    #define BSD64_U8 BSR64_U8
  #endif
#endif

#ifndef BSD64_U64
  #ifdef BSR64_U64
    #define BSD64_U64 BSR64_U64
  #endif
#endif

#include "bitstream_dyn64.h"

typedef bsd64_u8  bsr64_u8;
typedef bsd64_u64 bsr64_u64;

#define BSR64_BITS_PER_BYTE BSD64_BITS_PER_BYTE
#define BSR64_ONE           BSD64_ONE
#define BSR64_MASK(b)       BSD64_MASK(b)
#define BSR64_HAS_BSWAP     BSD64_HAS_BSWAP

#if BSR64_HAS_BSWAP
  #define bsr64_bswap64(v) bsd64_bswap64(v)

static bsr64_u64
bsr64_load64_be(const bsr64_u8 *p)
{
  return (bsr64_u64)bsd64_load64_be((const bsd64_u8 *)p);
}
#endif


typedef struct BitStreamReader64
{
  const bsr64_u8 *data;
  bsr64_u64       size; /* in bits */
  bsr64_u64       idx;  /* in bits */
} BitStreamReader64;


static void
bsr64__to_dyn(const BitStreamReader64 *src, BitStreamDyn64 *dst)
{
  dst->data        = (bsd64_u8 *)src->data;
  dst->capacity    = (bsd64_u64)src->size;
  dst->size        = (bsd64_u64)src->size;
  dst->idx         = (bsd64_u64)src->idx;
  dst->realloc_fn  = NULL;
  dst->realloc_ctx = NULL;
}

static void
bsr64__from_dyn(BitStreamReader64 *dst, const BitStreamDyn64 *src)
{
  dst->data = (const bsr64_u8 *)src->data;
  dst->size = (bsr64_u64)src->size;
  dst->idx  = (bsr64_u64)src->idx;
}


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
  BitStreamDyn64 dyn;

  bsr64__to_dyn(r, &dyn);
  return (bsr64_u64)bsd64_read_at(&dyn, (bsd64_u64)idx, (bsd64_u64)bits);
}


/*
 * Read 'bits' bits at the current cursor and advance.
 */
static bsr64_u64
bsr64_read(BitStreamReader64 *r,
           bsr64_u64          bits)
{
  BitStreamDyn64 dyn;
  bsr64_u64      v;

  bsr64__to_dyn(r, &dyn);
  v = (bsr64_u64)bsd64_read(&dyn, (bsd64_u64)bits);
  bsr64__from_dyn(r, &dyn);

  return v;
}


/*
 * BSR64_DEFINE_READ_FIXED(N) - generates two functions:
 *
 *   bsr64_read_fixed_N_at(r, idx)  - random access, N bits
 *   bsr64_read_fixed_N(r)          - streaming, N bits
 */

#define BSR64_DEFINE_READ_FIXED(N)                                          \
                                                                           \
static bsr64_u64                                                           \
bsr64_read_fixed_##N##_at(const BitStreamReader64 *r,                      \
                          bsr64_u64                idx)                    \
{                                                                          \
  BitStreamDyn64 dyn;                                                      \
                                                                           \
  bsr64__to_dyn(r, &dyn);                                                  \
  return (bsr64_u64)bsd64_read_fixed_##N##_at(&dyn, (bsd64_u64)idx);      \
}                                                                          \
                                                                           \
static bsr64_u64                                                           \
bsr64_read_fixed_##N(BitStreamReader64 *r)                                 \
{                                                                          \
  BitStreamDyn64 dyn;                                                      \
  bsr64_u64      v;                                                        \
                                                                           \
  bsr64__to_dyn(r, &dyn);                                                  \
  v = (bsr64_u64)bsd64_read_fixed_##N(&dyn);                               \
  bsr64__from_dyn(r, &dyn);                                                \
                                                                           \
  return v;                                                                \
}

#define BSR64_X_ALL BSD64_X_ALL

#define X(n) BSR64_DEFINE_READ_FIXED(n)
BSR64_X_ALL
#undef X

#endif /* BITSTREAM_READER64_H */
