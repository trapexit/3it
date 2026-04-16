/*
 * bitstream_t32.h - Header-only C89 combined bit stream
 *
 * Index/cursor:  64-bit (streams may be arbitrarily large)
 * Field width:   32-bit maximum (read returns u32, bits/val args are u32)
 *
 * Publicly this remains the fixed-buffer bitstream with the original
 * 3-field struct layout. Internally the read/write machinery is shared
 * with bitstream_dyn32.h by adapting BitStreamT32 to a fixed
 * BitStreamDyn32 view (realloc_fn == NULL, assert on overflow).
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

#ifndef BSD32_U8
  #ifdef BST32_U8
    #define BSD32_U8 BST32_U8
  #endif
#endif

#ifndef BSD32_U32
  #ifdef BST32_U32
    #define BSD32_U32 BST32_U32
  #endif
#endif

#ifndef BSD32_U64
  #ifdef BST32_U64
    #define BSD32_U64 BST32_U64
  #endif
#endif

#include "bitstream_dyn32.h"

typedef bsd32_u8  bst32_u8;
typedef bsd32_u32 bst32_u32;
typedef bsd32_u64 bst32_u64;

#define BST32_BITS_PER_BYTE BSD32_BITS_PER_BYTE
#define BST32_HAS_BSWAP     BSD32_HAS_BSWAP

#if BST32_HAS_BSWAP
  #define bst32_bswap32(v) bsd32_bswap32(v)

static bst32_u32
bst32_load32_be(const bst32_u8 *p)
{
  return (bst32_u32)bsd32_load32_be((const bsd32_u8 *)p);
}
#endif


typedef struct BitStreamT32
{
  bst32_u8  *data;
  bst32_u64  size; /* in bits */
  bst32_u64  idx;  /* in bits */
} BitStreamT32;


static void
bst32__to_dyn(const BitStreamT32 *src, BitStreamDyn32 *dst)
{
  dst->data        = (bsd32_u8 *)src->data;
  dst->capacity    = (bsd32_u64)src->size;
  dst->size        = (bsd32_u64)src->size;
  dst->idx         = (bsd32_u64)src->idx;
  dst->realloc_fn  = NULL;
  dst->realloc_ctx = NULL;
}

static void
bst32__from_dyn(BitStreamT32 *dst, const BitStreamDyn32 *src)
{
  dst->data = (bst32_u8 *)src->data;
  dst->size = (bst32_u64)src->capacity;
  dst->idx  = (bst32_u64)src->idx;
}


/* init with writable buffer */
static void
bst32_init(BitStreamT32 *s,
           bst32_u8     *data,
           bst32_u64     size_in_bytes,
           bst32_u64     idx)
{
  s->data = data;
  s->size = size_in_bytes * BST32_BITS_PER_BYTE;
  s->idx  = idx;
}

/* init with read-only buffer (casts away const; caller must not write) */
static void
bst32_init_ro(BitStreamT32    *s,
              const bst32_u8  *data,
              bst32_u64        size_in_bytes,
              bst32_u64        idx)
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
           bst32_u64     idx)
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
                  bst32_u64     bits)
{
  s->idx -= bits;
}

static void
bst32_skip(BitStreamT32 *s,
           bst32_u64     bits)
{
  s->idx += bits;
}

static int
bst32_on_8bit_boundary(const BitStreamT32 *s)
{
  return !(s->idx & 0x7);
}

static bst32_u8
bst32_bits_to_8bit_boundary(const BitStreamT32 *s)
{
  return (bst32_u8)((0x08 - (s->idx & 0x7)) & 0x7);
}

static int
bst32_on_16bit_boundary(const BitStreamT32 *s)
{
  return !(s->idx & 0xF);
}

static bst32_u8
bst32_bits_to_16bit_boundary(const BitStreamT32 *s)
{
  return (bst32_u8)((0x10 - (s->idx & 0xF)) & 0xF);
}

static int
bst32_on_32bit_boundary(const BitStreamT32 *s)
{
  return !(s->idx & 0x1F);
}

static bst32_u8
bst32_bits_to_32bit_boundary(const BitStreamT32 *s)
{
  return (bst32_u8)((0x20 - (s->idx & 0x1F)) & 0x1F);
}

static void
bst32_skip_to_8bit_boundary(BitStreamT32 *s)
{
  if(s->idx & 0x7)
    s->idx += 0x8 - (s->idx & 0x7);
}

static void
bst32_skip_to_16bit_boundary(BitStreamT32 *s)
{
  if(s->idx & 0x0F)
    s->idx += 0x10 - (s->idx & 0x0F);
}

static void
bst32_skip_to_32bit_boundary(BitStreamT32 *s)
{
  if(s->idx & 0x1F)
    s->idx += 0x20 - (s->idx & 0x1F);
}

static bst32_u64
bst32_size(const BitStreamT32 *s)
{
  return s->size;
}

static bst32_u64
bst32_tell(const BitStreamT32 *s)
{
  return s->idx;
}

static bst32_u64
bst32_tell_bits(const BitStreamT32 *s)
{
  return s->idx;
}

static bst32_u64
bst32_tell_bytes(const BitStreamT32 *s)
{
  return (s->idx + (BST32_BITS_PER_BYTE - 1)) / BST32_BITS_PER_BYTE;
}

static bst32_u64
bst32_tell_u32(const BitStreamT32 *s)
{
  return bst32_tell_bytes(s) / 4;
}


/* ------------------------------------------------------------------ */
/* Read / write shared with bitstream_dyn32.h                          */
/* ------------------------------------------------------------------ */

static bst32_u32
bst32_read_at(const BitStreamT32 *s,
              bst32_u64           idx,
              bst32_u32           bits)
{
  BitStreamDyn32 dyn;

  bst32__to_dyn(s, &dyn);
  return (bst32_u32)bsd32_read_at(&dyn, (bsd32_u64)idx, (bsd32_u32)bits);
}

static bst32_u32
bst32_read(BitStreamT32 *s,
           bst32_u32     bits)
{
  BitStreamDyn32 dyn;
  bst32_u32      v;

  bst32__to_dyn(s, &dyn);
  v = (bst32_u32)bsd32_read(&dyn, (bsd32_u32)bits);
  bst32__from_dyn(s, &dyn);

  return v;
}

static void
bst32_write_at(BitStreamT32 *s,
               bst32_u64     idx,
               bst32_u32     bits,
               bst32_u32     val)
{
  BitStreamDyn32 dyn;

  bst32__to_dyn(s, &dyn);
  bsd32_write_at(&dyn, (bsd32_u64)idx, (bsd32_u32)bits, (bsd32_u32)val);
  bst32__from_dyn(s, &dyn);
}

static void
bst32_write(BitStreamT32 *s,
            bst32_u32     bits,
            bst32_u32     val)
{
  BitStreamDyn32 dyn;

  bst32__to_dyn(s, &dyn);
  bsd32_write(&dyn, (bsd32_u32)bits, (bsd32_u32)val);
  bst32__from_dyn(s, &dyn);
}

static void
bst32_write_bytes(BitStreamT32   *s,
                  const bst32_u8 *src,
                  bst32_u64       count)
{
  BitStreamDyn32 dyn;

  bst32__to_dyn(s, &dyn);
  bsd32_write_bytes(&dyn, (const bsd32_u8 *)src, (bsd32_u64)count);
  bst32__from_dyn(s, &dyn);
}


/* ------------------------------------------------------------------ */
/* Fixed-width read/write wrappers                                     */
/* ------------------------------------------------------------------ */

#define BST32_DEFINE_FIXED(N)                                              \
                                                                           \
static bst32_u32                                                           \
bst32_read_fixed_##N##_at(const BitStreamT32 *s,                           \
                          bst32_u64           idx)                         \
{                                                                          \
  BitStreamDyn32 dyn;                                                      \
                                                                           \
  bst32__to_dyn(s, &dyn);                                                  \
  return (bst32_u32)bsd32_read_fixed_##N##_at(&dyn, (bsd32_u64)idx);      \
}                                                                          \
                                                                           \
static bst32_u32                                                           \
bst32_read_fixed_##N(BitStreamT32 *s)                                      \
{                                                                          \
  BitStreamDyn32 dyn;                                                      \
  bst32_u32      v;                                                        \
                                                                           \
  bst32__to_dyn(s, &dyn);                                                  \
  v = (bst32_u32)bsd32_read_fixed_##N(&dyn);                               \
  bst32__from_dyn(s, &dyn);                                                \
                                                                           \
  return v;                                                                \
}                                                                          \
                                                                           \
static void                                                                \
bst32_write_fixed_##N##_at(BitStreamT32 *s,                                \
                           bst32_u64     idx,                              \
                           bst32_u32     val)                              \
{                                                                          \
  BitStreamDyn32 dyn;                                                      \
                                                                           \
  bst32__to_dyn(s, &dyn);                                                  \
  bsd32_write_fixed_##N##_at(&dyn, (bsd32_u64)idx, (bsd32_u32)val);       \
  bst32__from_dyn(s, &dyn);                                                \
}                                                                          \
                                                                           \
static void                                                                \
bst32_write_fixed_##N(BitStreamT32 *s,                                     \
                      bst32_u32     val)                                   \
{                                                                          \
  BitStreamDyn32 dyn;                                                      \
                                                                           \
  bst32__to_dyn(s, &dyn);                                                  \
  bsd32_write_fixed_##N(&dyn, (bsd32_u32)val);                             \
  bst32__from_dyn(s, &dyn);                                                \
}

#define BST32_X_ALL BSD32_X_ALL

#define X(n) BST32_DEFINE_FIXED(n)
BST32_X_ALL
#undef X

#endif /* BITSTREAM_T32_H */
