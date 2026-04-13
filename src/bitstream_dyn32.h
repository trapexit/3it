/*
 * bitstream_dyn32.h - Header-only C89 dynamic bit stream (32-bit index)
 *
 * Extends BitStreamT32 with optional buffer resizing via a function
 * pointer.  When realloc_fn is NULL the struct behaves like a fixed
 * view (asserts on overflow, same semantics as BitStreamT32).  When
 * realloc_fn is non-NULL the buffer grows on demand, mirroring
 * BitStreamT<std::vector<u8>, u32>.
 *
 * The resize function has the signature:
 *
 *   void* realloc_fn(void *ptr, size_t new_bytes, void *ctx)
 *
 * Passing new_bytes == 0 is used to free; the caller is responsible
 * for ensuring the allocator supports this (e.g. the default wrapper
 * bsd32_stdlib_realloc does).
 *
 * All functions are static to allow header-only usage without linker
 * conflicts.  Compilers will inline at optimization levels >= -O1.
 *
 * Usage (dynamic, default malloc):
 *   BitStreamDyn32 s;
 *   bsd32_init_dyn(&s, bsd32_stdlib_realloc, NULL);
 *   bsd32_write(&s, 6, 0x1F);
 *   bsd32_rewind(&s);
 *   val = bsd32_read(&s, 6);
 *   bsd32_free(&s);
 *
 * Usage (fixed buffer, no resize):
 *   BitStreamDyn32 s;
 *   bsd32_init_fixed(&s, buffer, buffer_size_bytes, 0);
 *   bsd32_write(&s, 6, 0x1F);
 *
 * Usage (read-only fixed buffer):
 *   BitStreamDyn32 s;
 *   bsd32_init_ro(&s, data, byte_count, 0);
 *   val = bsd32_read(&s, 6);
 *
 * For compile-time known widths (all 1-32 pre-defined via X-macro):
 *   val = bsd32_read_fixed_6(&s);
 *   bsd32_write_fixed_6(&s, 0x1F);
 */

#ifndef BITSTREAM_DYN32_H
#define BITSTREAM_DYN32_H

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifndef BSD32_U8
typedef unsigned char  bsd32_u8;
#else
typedef BSD32_U8       bsd32_u8;
#endif

#ifndef BSD32_U32
typedef unsigned long  bsd32_u32;
#else
typedef BSD32_U32      bsd32_u32;
#endif

#define BSD32_BITS_PER_BYTE 8

typedef void* (*bsd32_realloc_fn)(void *ptr, size_t new_bytes, void *ctx);


/* bswap detection */
#if defined(__GNUC__) || defined(__clang__)
  #define BSD32_HAS_BSWAP 1
  #define bsd32_bswap32(v) __builtin_bswap32(v)
#elif defined(_MSC_VER)
  #define BSD32_HAS_BSWAP 1
  #define bsd32_bswap32(v) _byteswap_ulong(v)
#else
  #define BSD32_HAS_BSWAP 0
#endif

#if BSD32_HAS_BSWAP
static bsd32_u32
bsd32_load32_be(const bsd32_u8 *p)
{
  bsd32_u32 v;
  memcpy(&v, p, 4);
  return bsd32_bswap32(v);
}
#endif


typedef struct BitStreamDyn32
{
  bsd32_u8         *data;
  bsd32_u32         capacity;    /* allocated capacity in bits */
  bsd32_u32         size;        /* high-water mark in bits (valid region) */
  bsd32_u32         idx;         /* current cursor in bits */
  bsd32_realloc_fn  realloc_fn;  /* NULL = fixed/view; non-NULL = dynamic */
  void             *realloc_ctx; /* passed as ctx to realloc_fn */
} BitStreamDyn32;


/* ------------------------------------------------------------------ */
/* Default allocator wrapper (uses stdlib realloc / free)             */
/* ------------------------------------------------------------------ */

static void*
bsd32_stdlib_realloc(void *ptr, size_t new_bytes, void *ctx)
{
  (void)ctx;
  if(new_bytes == 0)
    {
      free(ptr);
      return NULL;
    }
  return realloc(ptr, new_bytes);
}


/* ------------------------------------------------------------------ */
/* Init                                                                */
/* ------------------------------------------------------------------ */

/* Dynamic: starts empty, grows on demand. */
static void
bsd32_init_dyn(BitStreamDyn32  *s,
               bsd32_realloc_fn realloc_fn,
               void            *realloc_ctx)
{
  s->data        = NULL;
  s->capacity    = 0;
  s->size        = 0;
  s->idx         = 0;
  s->realloc_fn  = realloc_fn;
  s->realloc_ctx = realloc_ctx;
}

/* Fixed mutable view: no resize, asserts on overflow. */
static void
bsd32_init_fixed(BitStreamDyn32 *s,
                 bsd32_u8       *data,
                 bsd32_u32       size_in_bytes,
                 bsd32_u32       idx)
{
  s->data        = data;
  s->capacity    = size_in_bytes * BSD32_BITS_PER_BYTE;
  s->size        = size_in_bytes * BSD32_BITS_PER_BYTE;
  s->idx         = idx;
  s->realloc_fn  = NULL;
  s->realloc_ctx = NULL;
}

/* Read-only fixed view: cast away const; caller must not write. */
static void
bsd32_init_ro(BitStreamDyn32  *s,
              const bsd32_u8  *data,
              bsd32_u32        size_in_bytes,
              bsd32_u32        idx)
{
  s->data        = (bsd32_u8 *)data;
  s->capacity    = size_in_bytes * BSD32_BITS_PER_BYTE;
  s->size        = size_in_bytes * BSD32_BITS_PER_BYTE;
  s->idx         = idx;
  s->realloc_fn  = NULL;
  s->realloc_ctx = NULL;
}

/* Free a dynamic buffer.  No-op on fixed views. */
static void
bsd32_free(BitStreamDyn32 *s)
{
  if(s->realloc_fn && s->data)
    s->realloc_fn(s->data, 0, s->realloc_ctx);
  s->data     = NULL;
  s->capacity = 0;
  s->size     = 0;
  s->idx      = 0;
}


/* ------------------------------------------------------------------ */
/* Internal: resize                                                    */
/* ------------------------------------------------------------------ */

static void
bsd32_grow(BitStreamDyn32 *s, bsd32_u32 bits_needed)
{
  bsd32_u32  cap_bytes;
  bsd32_u32  need_bytes;
  void      *p;

  if(bits_needed <= s->capacity)
    return;

  need_bytes = (bits_needed + BSD32_BITS_PER_BYTE - 1) / BSD32_BITS_PER_BYTE;

  /* double from current, but at least need_bytes, minimum 8 bytes */
  cap_bytes = (s->capacity / BSD32_BITS_PER_BYTE);
  cap_bytes = (cap_bytes < 8) ? 8 : cap_bytes * 2;
  if(cap_bytes < need_bytes)
    cap_bytes = need_bytes;

  p = s->realloc_fn(s->data, (size_t)cap_bytes, s->realloc_ctx);
  assert(p != NULL && "bsd32: realloc_fn returned NULL");
  s->data     = (bsd32_u8 *)p;
  s->capacity = cap_bytes * BSD32_BITS_PER_BYTE;
}

/*
 * Resize when writing (always enforces bounds).
 * Resize for seek is only done when dynamic (non-NULL realloc_fn).
 */
static void
bsd32_ensure(BitStreamDyn32 *s, bsd32_u32 bits_needed)
{
  if(s->realloc_fn)
    bsd32_grow(s, bits_needed);
  else
    assert(bits_needed <= s->capacity && "bsd32: overflow on fixed buffer");
}


/* ------------------------------------------------------------------ */
/* Navigation                                                          */
/* ------------------------------------------------------------------ */

static void
bsd32_seek(BitStreamDyn32 *s, bsd32_u32 idx)
{
  if(s->realloc_fn)
    bsd32_grow(s, idx);
  s->idx  = idx;
  if(s->idx > s->size)
    s->size = s->idx;
}

static void
bsd32_rewind(BitStreamDyn32 *s)
{
  s->idx = 0;
}

static void
bsd32_rewind_bits(BitStreamDyn32 *s, bsd32_u32 bits)
{
  s->idx -= bits;
}

static void
bsd32_skip(BitStreamDyn32 *s, bsd32_u32 bits)
{
  bsd32_seek(s, s->idx + bits);
}

static int
bsd32_on_8bit_boundary(const BitStreamDyn32 *s)
{
  return !(s->idx & 0x7UL);
}

static bsd32_u8
bsd32_bits_to_8bit_boundary(const BitStreamDyn32 *s)
{
  return (bsd32_u8)((0x08UL - (s->idx & 0x7UL)) & 0x7UL);
}

static int
bsd32_on_16bit_boundary(const BitStreamDyn32 *s)
{
  return !(s->idx & 0xFUL);
}

static bsd32_u8
bsd32_bits_to_16bit_boundary(const BitStreamDyn32 *s)
{
  return (bsd32_u8)((0x10UL - (s->idx & 0xFUL)) & 0xFUL);
}

static int
bsd32_on_32bit_boundary(const BitStreamDyn32 *s)
{
  return !(s->idx & 0x1FUL);
}

static bsd32_u8
bsd32_bits_to_32bit_boundary(const BitStreamDyn32 *s)
{
  return (bsd32_u8)((0x20UL - (s->idx & 0x1FUL)) & 0x1FUL);
}

static void
bsd32_skip_to_8bit_boundary(BitStreamDyn32 *s)
{
  if(s->idx & 0x7UL)
    bsd32_seek(s, s->idx + (0x8UL - (s->idx & 0x7UL)));
}

static void
bsd32_skip_to_16bit_boundary(BitStreamDyn32 *s)
{
  if(s->idx & 0x0FUL)
    bsd32_seek(s, s->idx + (0x10UL - (s->idx & 0x0FUL)));
}

static void
bsd32_skip_to_32bit_boundary(BitStreamDyn32 *s)
{
  if(s->idx & 0x1FUL)
    bsd32_seek(s, s->idx + (0x20UL - (s->idx & 0x1FUL)));
}

static bsd32_u32
bsd32_size_bits(const BitStreamDyn32 *s)
{
  return s->size;
}

static bsd32_u32
bsd32_size_8bits(const BitStreamDyn32 *s)
{
  return (s->size + 7) / 8;
}

static bsd32_u32
bsd32_size_32bits(const BitStreamDyn32 *s)
{
  return (s->size + 31) / 32;
}

static bsd32_u32
bsd32_capacity_bits(const BitStreamDyn32 *s)
{
  return s->capacity;
}

static bsd32_u32
bsd32_capacity_bytes(const BitStreamDyn32 *s)
{
  return (s->capacity + BSD32_BITS_PER_BYTE - 1) / BSD32_BITS_PER_BYTE;
}

static bsd32_u32
bsd32_tell(const BitStreamDyn32 *s)
{
  return s->idx;
}

static bsd32_u32
bsd32_tell_bits(const BitStreamDyn32 *s)
{
  return s->idx;
}

static bsd32_u32
bsd32_tell_bytes(const BitStreamDyn32 *s)
{
  return (s->idx + (BSD32_BITS_PER_BYTE - 1)) / BSD32_BITS_PER_BYTE;
}

static bsd32_u32
bsd32_tell_u32(const BitStreamDyn32 *s)
{
  return bsd32_tell_bytes(s) / 4;
}


/* ------------------------------------------------------------------ */
/* Shrink / set size (dynamic only; no-op on fixed)                   */
/* ------------------------------------------------------------------ */

static void
bsd32_shrink_to_idx(BitStreamDyn32 *s)
{
  bsd32_u32 bytes;
  void     *p;

  if(!s->realloc_fn || !s->data)
    return;

  bytes = (s->idx + BSD32_BITS_PER_BYTE - 1) / BSD32_BITS_PER_BYTE;
  p = s->realloc_fn(s->data, (size_t)bytes, s->realloc_ctx);
  if(p || bytes == 0)
    {
      s->data     = (bsd32_u8 *)p;
      s->capacity = bytes * BSD32_BITS_PER_BYTE;
    }
}

static void
bsd32_shrink_to_size(BitStreamDyn32 *s)
{
  bsd32_u32 bytes;
  void     *p;

  if(!s->realloc_fn || !s->data)
    return;

  bytes = (s->size + BSD32_BITS_PER_BYTE - 1) / BSD32_BITS_PER_BYTE;
  p = s->realloc_fn(s->data, (size_t)bytes, s->realloc_ctx);
  if(p || bytes == 0)
    {
      s->data     = (bsd32_u8 *)p;
      s->capacity = bytes * BSD32_BITS_PER_BYTE;
    }
}

static void
bsd32_set_size_bits(BitStreamDyn32 *s, bsd32_u32 size)
{
  s->size = size;
  if(s->idx > s->size)
    s->idx = s->size;
  bsd32_shrink_to_size(s);
}

static void
bsd32_set_size_8bits(BitStreamDyn32 *s, bsd32_u32 n)
{
  bsd32_set_size_bits(s, n * 8);
}

static void
bsd32_set_size_32bits(BitStreamDyn32 *s, bsd32_u32 n)
{
  bsd32_set_size_bits(s, n * 32);
}


/* ------------------------------------------------------------------ */
/* Read                                                                */
/* ------------------------------------------------------------------ */

static bsd32_u32
bsd32_read_at(const BitStreamDyn32 *s,
              bsd32_u32             idx,
              bsd32_u32             bits)
{
  bsd32_u32       byte_idx;
  bsd32_u8        bit_off;
  const bsd32_u8 *src;
  bsd32_u32       mask;
  bsd32_u32       acc;
  bsd32_u8        remaining;

  assert((idx + bits) <= s->capacity);

  if(bits == 0)
    return 0;

  byte_idx = idx >> 3;
  bit_off  = (bsd32_u8)(idx & 7);
  src      = &s->data[byte_idx];
  mask     = (bits == 32) ? ~0UL : ((1UL << bits) - 1);

#if BSD32_HAS_BSWAP
  if(bit_off + bits <= 32)
    {
      acc = bsd32_load32_be(src);
      return (acc >> (32 - bit_off - bits)) & mask;
    }

  acc = bsd32_load32_be(src);
  acc &= (1UL << (32 - bit_off)) - 1;
  remaining = (bsd32_u8)(bits - (32 - bit_off));
  return (acc << remaining) | (src[4] >> (8 - remaining));
#else
  if(!bit_off && !(bits & 7))
    {
      bsd32_u32 val = 0;
      bsd32_u32 i;
      for(i = 0; i < (bits >> 3); i++)
        val = (val << 8) | src[i];
      return val;
    }

  if(bit_off + bits <= 32)
    {
      bsd32_u32 n;
      acc = 0;
      n = (bit_off + bits + 7) >> 3;
      {
        bsd32_u32 i;
        for(i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
      }
      return (acc >> (n * 8 - bit_off - bits)) & mask;
    }

  acc = 0;
  {
    bsd32_u32 i;
    for(i = 0; i < 4; i++)
      acc = (acc << 8) | src[i];
  }
  acc &= (1UL << (32 - bit_off)) - 1;
  remaining = (bsd32_u8)(bits - (32 - bit_off));
  return (acc << remaining) | (src[4] >> (8 - remaining));
#endif
}

static bsd32_u32
bsd32_read(BitStreamDyn32 *s, bsd32_u32 bits)
{
  bsd32_u32 v = bsd32_read_at(s, s->idx, bits);
  s->idx += bits;
  return v;
}


/* ------------------------------------------------------------------ */
/* Write                                                               */
/* ------------------------------------------------------------------ */

static void
bsd32_write_at(BitStreamDyn32 *s,
               bsd32_u32       idx,
               bsd32_u32       bits,
               bsd32_u32       val)
{
  bsd32_u8  *dst;
  bsd32_u8   bit_off;
  bsd32_u32  remaining;

  bsd32_ensure(s, idx + bits);

  if(bits == 0)
    return;

  dst       = &s->data[idx >> 3];
  bit_off   = (bsd32_u8)(idx & 7);
  remaining = bits;

  if(bit_off)
    {
      bsd32_u8 avail = 8 - bit_off;
      bsd32_u8 take  = (remaining < avail) ? (bsd32_u8)remaining : avail;
      bsd32_u8 shift = avail - take;
      bsd32_u8 mask  = (bsd32_u8)(((1U << take) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bsd32_u8)(((val >> (remaining - take)) & ((1UL << take) - 1)) << shift);
      dst++;
      remaining -= take;
    }

  while(remaining >= 8)
    {
      remaining -= 8;
      *dst++ = (bsd32_u8)((val >> remaining) & 0xFF);
    }

  if(remaining)
    {
      bsd32_u8 shift = 8 - (bsd32_u8)remaining;
      bsd32_u8 mask  = (bsd32_u8)(((1U << remaining) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bsd32_u8)((val & ((1UL << remaining) - 1)) << shift);
    }
}

static void
bsd32_write(BitStreamDyn32 *s, bsd32_u32 bits, bsd32_u32 val)
{
  bsd32_write_at(s, s->idx, bits, val);
  s->idx += bits;
  if(s->idx > s->size)
    s->size = s->idx;
}

static void
bsd32_write_bytes(BitStreamDyn32 *s, const bsd32_u8 *src, bsd32_u32 count)
{
  if(!(s->idx & 7))
    {
      bsd32_ensure(s, s->idx + count * 8);
      memcpy(&s->data[s->idx >> 3], src, count);
      s->idx += count * 8;
      if(s->idx > s->size)
        s->size = s->idx;
    }
  else
    {
      bsd32_u32 i;
      for(i = 0; i < count; i++)
        bsd32_write(s, 8, src[i]);
    }
}

static void
bsd32_zero_till_8bit_boundary(BitStreamDyn32 *s)
{
  if(!bsd32_on_8bit_boundary(s))
    bsd32_write(s, bsd32_bits_to_8bit_boundary(s), 0);
}

static void
bsd32_zero_till_16bit_boundary(BitStreamDyn32 *s)
{
  if(!bsd32_on_16bit_boundary(s))
    bsd32_write(s, bsd32_bits_to_16bit_boundary(s), 0);
}

static void
bsd32_zero_till_32bit_boundary(BitStreamDyn32 *s)
{
  if(!bsd32_on_32bit_boundary(s))
    bsd32_write(s, bsd32_bits_to_32bit_boundary(s), 0);
}


/* ------------------------------------------------------------------ */
/* Fixed-width read/write macros                                       */
/*                                                                     */
/* BSD32_DEFINE_FIXED(N) generates:                                    */
/*   bsd32_read_fixed_N_at(s, idx)       - random-access read         */
/*   bsd32_read_fixed_N(s)               - streaming read             */
/*   bsd32_write_fixed_N_at(s, idx, val) - random-access write        */
/*   bsd32_write_fixed_N(s, val)         - streaming write            */
/* ------------------------------------------------------------------ */

#if BSD32_HAS_BSWAP

#define BSD32_DEFINE_FIXED(N)                                              \
                                                                           \
static bsd32_u32                                                           \
bsd32_read_fixed_##N##_at(const BitStreamDyn32 *s,                         \
                          bsd32_u32             idx)                       \
{                                                                          \
  const bsd32_u32 BITS = (N);                                              \
  const bsd32_u32 MASK = ((N) == 32) ? ~0UL : ((1UL << (N)) - 1);        \
  bsd32_u8        bit_off  = (bsd32_u8)(idx & 7);                         \
  const bsd32_u8 *src      = &s->data[idx >> 3];                          \
  bsd32_u32       acc;                                                     \
                                                                           \
  assert((idx + BITS) <= s->capacity);                                     \
                                                                           \
  acc = bsd32_load32_be(src);                                              \
  if(bit_off + BITS <= 32)                                                 \
    return (acc >> (32 - bit_off - BITS)) & MASK;                          \
                                                                           \
  {                                                                        \
    bsd32_u8 remaining;                                                    \
    acc &= (1UL << (32 - bit_off)) - 1;                                   \
    remaining = (bsd32_u8)(BITS - (32 - bit_off));                         \
    return (acc << remaining) | (src[4] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bsd32_u32                                                           \
bsd32_read_fixed_##N(BitStreamDyn32 *s)                                    \
{                                                                          \
  bsd32_u32 v = bsd32_read_fixed_##N##_at(s, s->idx);                     \
  s->idx += (N);                                                           \
  return v;                                                                \
}                                                                          \
                                                                           \
static void                                                                \
bsd32_write_fixed_##N##_at(BitStreamDyn32 *s,                              \
                           bsd32_u32       idx,                            \
                           bsd32_u32       val)                            \
{                                                                          \
  const bsd32_u32 BITS = (N);                                              \
  bsd32_u8  *dst;                                                          \
  bsd32_u8   bit_off;                                                      \
  bsd32_u32  remaining;                                                    \
                                                                           \
  bsd32_ensure(s, idx + BITS);                                             \
                                                                           \
  dst       = &s->data[idx >> 3];                                          \
  bit_off   = (bsd32_u8)(idx & 7);                                        \
  remaining = BITS;                                                        \
                                                                           \
  if(bit_off)                                                              \
    {                                                                      \
      bsd32_u8 avail = 8 - bit_off;                                       \
      bsd32_u8 take  = (BITS < avail) ? (bsd32_u8)BITS : avail;          \
      bsd32_u8 shift = avail - take;                                      \
      bsd32_u8 mask  = (bsd32_u8)(((1U << take) - 1) << shift);          \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsd32_u8)(((val >> (remaining - take)) &                         \
                     ((1UL << take) - 1)) << shift);                      \
      dst++;                                                               \
      remaining -= take;                                                   \
    }                                                                      \
                                                                           \
  while(remaining >= 8)                                                    \
    {                                                                      \
      remaining -= 8;                                                      \
      *dst++ = (bsd32_u8)((val >> remaining) & 0xFF);                     \
    }                                                                      \
                                                                           \
  if(remaining)                                                            \
    {                                                                      \
      bsd32_u8 shift = 8 - (bsd32_u8)remaining;                          \
      bsd32_u8 mask  = (bsd32_u8)(((1U << remaining) - 1) << shift);     \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsd32_u8)((val & ((1UL << remaining) - 1)) << shift);           \
    }                                                                      \
}                                                                          \
                                                                           \
static void                                                                \
bsd32_write_fixed_##N(BitStreamDyn32 *s, bsd32_u32 val)                    \
{                                                                          \
  bsd32_write_fixed_##N##_at(s, s->idx, val);                             \
  s->idx += (N);                                                           \
  if(s->idx > s->size)                                                     \
    s->size = s->idx;                                                      \
}

#else /* !BSD32_HAS_BSWAP */

#define BSD32_DEFINE_FIXED(N)                                              \
                                                                           \
static bsd32_u32                                                           \
bsd32_read_fixed_##N##_at(const BitStreamDyn32 *s,                         \
                          bsd32_u32             idx)                       \
{                                                                          \
  const bsd32_u32 BITS  = (N);                                             \
  const bsd32_u32 BYTES = (7 + (N) + 7) >> 3;                             \
  const bsd32_u32 MASK  = ((N) == 32) ? ~0UL : ((1UL << (N)) - 1);       \
  const bsd32_u8 *src   = &s->data[idx >> 3];                             \
  bsd32_u8        bit_off = (bsd32_u8)(idx & 7);                          \
  bsd32_u32       acc   = 0;                                               \
  bsd32_u32       i;                                                       \
                                                                           \
  assert((idx + BITS) <= s->capacity);                                     \
                                                                           \
  if(bit_off + BITS <= 32)                                                 \
    {                                                                      \
      for(i = 0; i < BYTES; i++)                                           \
        acc = (acc << 8) | src[i];                                         \
      return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;                 \
    }                                                                      \
                                                                           \
  {                                                                        \
    bsd32_u8 remaining;                                                    \
    for(i = 0; i < 4; i++)                                                 \
      acc = (acc << 8) | src[i];                                           \
    acc &= (1UL << (32 - bit_off)) - 1;                                   \
    remaining = (bsd32_u8)(BITS - (32 - bit_off));                         \
    return (acc << remaining) | (src[4] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bsd32_u32                                                           \
bsd32_read_fixed_##N(BitStreamDyn32 *s)                                    \
{                                                                          \
  bsd32_u32 v = bsd32_read_fixed_##N##_at(s, s->idx);                     \
  s->idx += (N);                                                           \
  return v;                                                                \
}                                                                          \
                                                                           \
static void                                                                \
bsd32_write_fixed_##N##_at(BitStreamDyn32 *s,                              \
                           bsd32_u32       idx,                            \
                           bsd32_u32       val)                            \
{                                                                          \
  const bsd32_u32 BITS = (N);                                              \
  bsd32_u8  *dst;                                                          \
  bsd32_u8   bit_off;                                                      \
  bsd32_u32  remaining;                                                    \
                                                                           \
  bsd32_ensure(s, idx + BITS);                                             \
                                                                           \
  dst       = &s->data[idx >> 3];                                          \
  bit_off   = (bsd32_u8)(idx & 7);                                        \
  remaining = BITS;                                                        \
                                                                           \
  if(bit_off)                                                              \
    {                                                                      \
      bsd32_u8 avail = 8 - bit_off;                                       \
      bsd32_u8 take  = (BITS < avail) ? (bsd32_u8)BITS : avail;          \
      bsd32_u8 shift = avail - take;                                      \
      bsd32_u8 mask  = (bsd32_u8)(((1U << take) - 1) << shift);          \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsd32_u8)(((val >> (remaining - take)) &                         \
                     ((1UL << take) - 1)) << shift);                      \
      dst++;                                                               \
      remaining -= take;                                                   \
    }                                                                      \
                                                                           \
  while(remaining >= 8)                                                    \
    {                                                                      \
      remaining -= 8;                                                      \
      *dst++ = (bsd32_u8)((val >> remaining) & 0xFF);                     \
    }                                                                      \
                                                                           \
  if(remaining)                                                            \
    {                                                                      \
      bsd32_u8 shift = 8 - (bsd32_u8)remaining;                          \
      bsd32_u8 mask  = (bsd32_u8)(((1U << remaining) - 1) << shift);     \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsd32_u8)((val & ((1UL << remaining) - 1)) << shift);           \
    }                                                                      \
}                                                                          \
                                                                           \
static void                                                                \
bsd32_write_fixed_##N(BitStreamDyn32 *s, bsd32_u32 val)                    \
{                                                                          \
  bsd32_write_fixed_##N##_at(s, s->idx, val);                             \
  s->idx += (N);                                                           \
  if(s->idx > s->size)                                                     \
    s->size = s->idx;                                                      \
}

#endif /* BSD32_HAS_BSWAP */


#define BSD32_X_ALL \
  X(1)  X(2)  X(3)  X(4)  X(5)  X(6)  X(7)  X(8)  \
  X(9)  X(10) X(11) X(12) X(13) X(14) X(15) X(16) \
  X(17) X(18) X(19) X(20) X(21) X(22) X(23) X(24) \
  X(25) X(26) X(27) X(28) X(29) X(30) X(31) X(32)

#define X(n) BSD32_DEFINE_FIXED(n)
BSD32_X_ALL
#undef X

#endif /* BITSTREAM_DYN32_H */
