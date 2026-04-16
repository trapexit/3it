/*
 * bitstream_t64.h - Header-only C89 combined bit stream (64-bit index)
 *
 * Publicly this remains the fixed-buffer bitstream with the original
 * 3-field struct layout. Internally the read/write machinery is shared
 * with bitstream_dyn64.h by adapting BitStreamT64 to a fixed
 * BitStreamDyn64 view (realloc_fn == NULL, assert on overflow).
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

#ifndef BSD64_U8
  #ifdef BST64_U8
    #define BSD64_U8 BST64_U8
  #endif
#endif

#ifndef BSD64_U64
  #ifdef BST64_U64
    #define BSD64_U64 BST64_U64
  #endif
#endif

#include "bitstream_dyn64.h"

typedef bsd64_u8  bst64_u8;
typedef bsd64_u64 bst64_u64;

#define BST64_BITS_PER_BYTE BSD64_BITS_PER_BYTE
#define BST64_ONE           BSD64_ONE
#define BST64_MASK(b)       BSD64_MASK(b)
#define BST64_HAS_BSWAP     BSD64_HAS_BSWAP

#if BST64_HAS_BSWAP
  #define bst64_bswap64(v) bsd64_bswap64(v)

static bst64_u64
bst64_load64_be(const bst64_u8 *p)
{
  return (bst64_u64)bsd64_load64_be((const bsd64_u8 *)p);
}
#endif


typedef struct BitStreamT64
{
  bst64_u8  *data;
  bst64_u64  size; /* in bits */
  bst64_u64  idx;  /* in bits */
} BitStreamT64;


static void
bst64__to_dyn(const BitStreamT64 *src, BitStreamDyn64 *dst)
{
  dst->data        = (bsd64_u8 *)src->data;
  dst->capacity    = (bsd64_u64)src->size;
  dst->size        = (bsd64_u64)src->size;
  dst->idx         = (bsd64_u64)src->idx;
  dst->realloc_fn  = NULL;
  dst->realloc_ctx = NULL;
}

static void
bst64__from_dyn(BitStreamT64 *dst, const BitStreamDyn64 *src)
{
  dst->data = (bst64_u8 *)src->data;
  dst->size = (bst64_u64)src->capacity;
  dst->idx  = (bst64_u64)src->idx;
}


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
/* Read / write shared with bitstream_dyn64.h                          */
/* ------------------------------------------------------------------ */

static bst64_u64
bst64_read_at(const BitStreamT64 *s,
              bst64_u64           idx,
              bst64_u64           bits)
{
  BitStreamDyn64 dyn;

  bst64__to_dyn(s, &dyn);
  return (bst64_u64)bsd64_read_at(&dyn, (bsd64_u64)idx, (bsd64_u64)bits);
}

static bst64_u64
bst64_read(BitStreamT64 *s,
           bst64_u64     bits)
{
  BitStreamDyn64 dyn;
  bst64_u64      v;

  bst64__to_dyn(s, &dyn);
  v = (bst64_u64)bsd64_read(&dyn, (bsd64_u64)bits);
  bst64__from_dyn(s, &dyn);

  return v;
}

static void
bst64_write_at(BitStreamT64 *s,
               bst64_u64     idx,
               bst64_u64     bits,
               bst64_u64     val)
{
  BitStreamDyn64 dyn;

  bst64__to_dyn(s, &dyn);
  bsd64_write_at(&dyn, (bsd64_u64)idx, (bsd64_u64)bits, (bsd64_u64)val);
  bst64__from_dyn(s, &dyn);
}

static void
bst64_write(BitStreamT64 *s,
            bst64_u64     bits,
            bst64_u64     val)
{
  BitStreamDyn64 dyn;

  bst64__to_dyn(s, &dyn);
  bsd64_write(&dyn, (bsd64_u64)bits, (bsd64_u64)val);
  bst64__from_dyn(s, &dyn);
}

static void
bst64_write_bytes(BitStreamT64   *s,
                  const bst64_u8 *src,
                  bst64_u64       count)
{
  BitStreamDyn64 dyn;

  bst64__to_dyn(s, &dyn);
  bsd64_write_bytes(&dyn, (const bsd64_u8 *)src, (bsd64_u64)count);
  bst64__from_dyn(s, &dyn);
}


/* ------------------------------------------------------------------ */
/* Fixed-width read/write wrappers                                     */
/* ------------------------------------------------------------------ */

#define BST64_DEFINE_FIXED(N)                                              \
                                                                           \
static bst64_u64                                                           \
bst64_read_fixed_##N##_at(const BitStreamT64 *s,                           \
                          bst64_u64           idx)                         \
{                                                                          \
  BitStreamDyn64 dyn;                                                      \
                                                                           \
  bst64__to_dyn(s, &dyn);                                                  \
  return (bst64_u64)bsd64_read_fixed_##N##_at(&dyn, (bsd64_u64)idx);      \
}                                                                          \
                                                                           \
static bst64_u64                                                           \
bst64_read_fixed_##N(BitStreamT64 *s)                                      \
{                                                                          \
  BitStreamDyn64 dyn;                                                      \
  bst64_u64      v;                                                        \
                                                                           \
  bst64__to_dyn(s, &dyn);                                                  \
  v = (bst64_u64)bsd64_read_fixed_##N(&dyn);                               \
  bst64__from_dyn(s, &dyn);                                                \
                                                                           \
  return v;                                                                \
}                                                                          \
                                                                           \
static void                                                                \
bst64_write_fixed_##N##_at(BitStreamT64 *s,                                \
                           bst64_u64     idx,                              \
                           bst64_u64     val)                              \
{                                                                          \
  BitStreamDyn64 dyn;                                                      \
                                                                           \
  bst64__to_dyn(s, &dyn);                                                  \
  bsd64_write_fixed_##N##_at(&dyn, (bsd64_u64)idx, (bsd64_u64)val);       \
  bst64__from_dyn(s, &dyn);                                                \
}                                                                          \
                                                                           \
static void                                                                \
bst64_write_fixed_##N(BitStreamT64 *s,                                     \
                      bst64_u64     val)                                   \
{                                                                          \
  BitStreamDyn64 dyn;                                                      \
                                                                           \
  bst64__to_dyn(s, &dyn);                                                  \
  bsd64_write_fixed_##N(&dyn, (bsd64_u64)val);                             \
  bst64__from_dyn(s, &dyn);                                                \
}

#define BST64_X_ALL BSD64_X_ALL

#define X(n) BST64_DEFINE_FIXED(n)
BST64_X_ALL
#undef X

#endif /* BITSTREAM_T64_H */
