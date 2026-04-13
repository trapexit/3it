/*
 * bitstream_dyn64.h - Header-only C89 dynamic bit stream (64-bit index)
 *
 * Extends BitStreamT64 with optional buffer resizing via a function
 * pointer.  When realloc_fn is NULL the struct behaves like a fixed
 * view (asserts on overflow, same semantics as BitStreamT64).  When
 * realloc_fn is non-NULL the buffer grows on demand, mirroring
 * BitStreamT<std::vector<u8>>.
 *
 * Requires compiler support for 64-bit integers (unsigned long long
 * or equivalent). Override BSD64_U64 if your platform uses a
 * different 64-bit type.
 *
 * The resize function has the signature:
 *
 *   void* realloc_fn(void *ptr, size_t new_bytes, void *ctx)
 *
 * Passing new_bytes == 0 is used to free; the caller is responsible
 * for ensuring the allocator supports this (e.g. the default wrapper
 * bsd64_stdlib_realloc does).
 *
 * All functions are static to allow header-only usage without linker
 * conflicts.  Compilers will inline at optimization levels >= -O1.
 *
 * Usage (dynamic, default malloc):
 *   BitStreamDyn64 s;
 *   bsd64_init_dyn(&s, bsd64_stdlib_realloc, NULL);
 *   bsd64_write(&s, 6, 0x1F);
 *   bsd64_rewind(&s);
 *   val = bsd64_read(&s, 6);
 *   bsd64_free(&s);
 *
 * Usage (fixed buffer, no resize):
 *   BitStreamDyn64 s;
 *   bsd64_init_fixed(&s, buffer, buffer_size_bytes, 0);
 *   bsd64_write(&s, 6, 0x1F);
 *
 * Usage (read-only fixed buffer):
 *   BitStreamDyn64 s;
 *   bsd64_init_ro(&s, data, byte_count, 0);
 *   val = bsd64_read(&s, 6);
 *
 * For compile-time known widths (all 1-64 pre-defined via X-macro):
 *   val = bsd64_read_fixed_6(&s);
 *   bsd64_write_fixed_6(&s, 0x1F);
 */

#ifndef BITSTREAM_DYN64_H
#define BITSTREAM_DYN64_H

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifndef BSD64_U8
typedef unsigned char      bsd64_u8;
#else
typedef BSD64_U8           bsd64_u8;
#endif

#ifndef BSD64_U64
typedef unsigned long long bsd64_u64;
#else
typedef BSD64_U64          bsd64_u64;
#endif

#define BSD64_BITS_PER_BYTE 8
#define BSD64_ONE  ((bsd64_u64)1)
#define BSD64_MASK(b) (((b) == 64) ? ~(bsd64_u64)0 : (BSD64_ONE << (b)) - 1)

typedef void* (*bsd64_realloc_fn)(void *ptr, size_t new_bytes, void *ctx);


/* bswap detection */
#if defined(__GNUC__) || defined(__clang__)
  #define BSD64_HAS_BSWAP 1
  #define bsd64_bswap64(v) __builtin_bswap64(v)
#elif defined(_MSC_VER)
  #include <stdlib.h>
  #define BSD64_HAS_BSWAP 1
  #define bsd64_bswap64(v) _byteswap_uint64(v)
#else
  #define BSD64_HAS_BSWAP 0
#endif

#if BSD64_HAS_BSWAP
static bsd64_u64
bsd64_load64_be(const bsd64_u8 *p)
{
  bsd64_u64 v;
  memcpy(&v, p, 8);
  return bsd64_bswap64(v);
}
#endif


typedef struct BitStreamDyn64
{
  bsd64_u8         *data;
  bsd64_u64         capacity;    /* allocated capacity in bits */
  bsd64_u64         size;        /* high-water mark in bits (valid region) */
  bsd64_u64         idx;         /* current cursor in bits */
  bsd64_realloc_fn  realloc_fn;  /* NULL = fixed/view; non-NULL = dynamic */
  void             *realloc_ctx; /* passed as ctx to realloc_fn */
} BitStreamDyn64;


/* ------------------------------------------------------------------ */
/* Default allocator wrapper (uses stdlib realloc / free)             */
/* ------------------------------------------------------------------ */

static void*
bsd64_stdlib_realloc(void *ptr, size_t new_bytes, void *ctx)
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
bsd64_init_dyn(BitStreamDyn64  *s,
               bsd64_realloc_fn realloc_fn,
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
bsd64_init_fixed(BitStreamDyn64 *s,
                 bsd64_u8       *data,
                 bsd64_u64       size_in_bytes,
                 bsd64_u64       idx)
{
  s->data        = data;
  s->capacity    = size_in_bytes * BSD64_BITS_PER_BYTE;
  s->size        = size_in_bytes * BSD64_BITS_PER_BYTE;
  s->idx         = idx;
  s->realloc_fn  = NULL;
  s->realloc_ctx = NULL;
}

/* Read-only fixed view: cast away const; caller must not write. */
static void
bsd64_init_ro(BitStreamDyn64  *s,
              const bsd64_u8  *data,
              bsd64_u64        size_in_bytes,
              bsd64_u64        idx)
{
  s->data        = (bsd64_u8 *)data;
  s->capacity    = size_in_bytes * BSD64_BITS_PER_BYTE;
  s->size        = size_in_bytes * BSD64_BITS_PER_BYTE;
  s->idx         = idx;
  s->realloc_fn  = NULL;
  s->realloc_ctx = NULL;
}

/* Free a dynamic buffer.  No-op on fixed views. */
static void
bsd64_free(BitStreamDyn64 *s)
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
bsd64_grow(BitStreamDyn64 *s, bsd64_u64 bits_needed)
{
  bsd64_u64  cap_bytes;
  bsd64_u64  need_bytes;
  void      *p;

  if(bits_needed <= s->capacity)
    return;

  need_bytes = (bits_needed + BSD64_BITS_PER_BYTE - 1) / BSD64_BITS_PER_BYTE;

  cap_bytes = (s->capacity / BSD64_BITS_PER_BYTE);
  cap_bytes = (cap_bytes < 8) ? 8 : cap_bytes * 2;
  if(cap_bytes < need_bytes)
    cap_bytes = need_bytes;

  p = s->realloc_fn(s->data, (size_t)cap_bytes, s->realloc_ctx);
  assert(p != NULL && "bsd64: realloc_fn returned NULL");
  s->data     = (bsd64_u8 *)p;
  s->capacity = cap_bytes * BSD64_BITS_PER_BYTE;
}

static void
bsd64_ensure(BitStreamDyn64 *s, bsd64_u64 bits_needed)
{
  if(s->realloc_fn)
    bsd64_grow(s, bits_needed);
  else
    assert(bits_needed <= s->capacity && "bsd64: overflow on fixed buffer");
}


/* ------------------------------------------------------------------ */
/* Navigation                                                          */
/* ------------------------------------------------------------------ */

static void
bsd64_seek(BitStreamDyn64 *s, bsd64_u64 idx)
{
  if(s->realloc_fn)
    bsd64_grow(s, idx);
  s->idx  = idx;
  if(s->idx > s->size)
    s->size = s->idx;
}

static void
bsd64_rewind(BitStreamDyn64 *s)
{
  s->idx = 0;
}

static void
bsd64_rewind_bits(BitStreamDyn64 *s, bsd64_u64 bits)
{
  s->idx -= bits;
}

static void
bsd64_skip(BitStreamDyn64 *s, bsd64_u64 bits)
{
  bsd64_seek(s, s->idx + bits);
}

static int
bsd64_on_8bit_boundary(const BitStreamDyn64 *s)
{
  return !(s->idx & 0x7);
}

static bsd64_u8
bsd64_bits_to_8bit_boundary(const BitStreamDyn64 *s)
{
  return (bsd64_u8)((0x08 - (s->idx & 0x7)) & 0x7);
}

static int
bsd64_on_16bit_boundary(const BitStreamDyn64 *s)
{
  return !(s->idx & 0xF);
}

static bsd64_u8
bsd64_bits_to_16bit_boundary(const BitStreamDyn64 *s)
{
  return (bsd64_u8)((0x10 - (s->idx & 0xF)) & 0xF);
}

static int
bsd64_on_32bit_boundary(const BitStreamDyn64 *s)
{
  return !(s->idx & 0x1F);
}

static bsd64_u8
bsd64_bits_to_32bit_boundary(const BitStreamDyn64 *s)
{
  return (bsd64_u8)((0x20 - (s->idx & 0x1F)) & 0x1F);
}

static int
bsd64_on_64bit_boundary(const BitStreamDyn64 *s)
{
  return !(s->idx & 0x3F);
}

static bsd64_u8
bsd64_bits_to_64bit_boundary(const BitStreamDyn64 *s)
{
  return (bsd64_u8)((0x40 - (s->idx & 0x3F)) & 0x3F);
}

static void
bsd64_skip_to_8bit_boundary(BitStreamDyn64 *s)
{
  if(s->idx & 0x7)
    bsd64_seek(s, s->idx + (0x8 - (s->idx & 0x7)));
}

static void
bsd64_skip_to_16bit_boundary(BitStreamDyn64 *s)
{
  if(s->idx & 0x0F)
    bsd64_seek(s, s->idx + (0x10 - (s->idx & 0x0F)));
}

static void
bsd64_skip_to_32bit_boundary(BitStreamDyn64 *s)
{
  if(s->idx & 0x1F)
    bsd64_seek(s, s->idx + (0x20 - (s->idx & 0x1F)));
}

static void
bsd64_skip_to_64bit_boundary(BitStreamDyn64 *s)
{
  if(s->idx & 0x3F)
    bsd64_seek(s, s->idx + (0x40 - (s->idx & 0x3F)));
}

static bsd64_u64
bsd64_size_bits(const BitStreamDyn64 *s)
{
  return s->size;
}

static bsd64_u64
bsd64_size_8bits(const BitStreamDyn64 *s)
{
  return (s->size + 7) / 8;
}

static bsd64_u64
bsd64_size_32bits(const BitStreamDyn64 *s)
{
  return (s->size + 31) / 32;
}

static bsd64_u64
bsd64_capacity_bits(const BitStreamDyn64 *s)
{
  return s->capacity;
}

static bsd64_u64
bsd64_capacity_bytes(const BitStreamDyn64 *s)
{
  return (s->capacity + BSD64_BITS_PER_BYTE - 1) / BSD64_BITS_PER_BYTE;
}

static bsd64_u64
bsd64_tell(const BitStreamDyn64 *s)
{
  return s->idx;
}

static bsd64_u64
bsd64_tell_bits(const BitStreamDyn64 *s)
{
  return s->idx;
}

static bsd64_u64
bsd64_tell_bytes(const BitStreamDyn64 *s)
{
  return (s->idx + (BSD64_BITS_PER_BYTE - 1)) / BSD64_BITS_PER_BYTE;
}

static bsd64_u64
bsd64_tell_u32(const BitStreamDyn64 *s)
{
  return bsd64_tell_bytes(s) / 4;
}


/* ------------------------------------------------------------------ */
/* Shrink / set size (dynamic only; no-op on fixed)                   */
/* ------------------------------------------------------------------ */

static void
bsd64_shrink_to_idx(BitStreamDyn64 *s)
{
  bsd64_u64 bytes;
  void     *p;

  if(!s->realloc_fn || !s->data)
    return;

  bytes = (s->idx + BSD64_BITS_PER_BYTE - 1) / BSD64_BITS_PER_BYTE;
  p = s->realloc_fn(s->data, (size_t)bytes, s->realloc_ctx);
  if(p || bytes == 0)
    {
      s->data     = (bsd64_u8 *)p;
      s->capacity = bytes * BSD64_BITS_PER_BYTE;
    }
}

static void
bsd64_shrink_to_size(BitStreamDyn64 *s)
{
  bsd64_u64 bytes;
  void     *p;

  if(!s->realloc_fn || !s->data)
    return;

  bytes = (s->size + BSD64_BITS_PER_BYTE - 1) / BSD64_BITS_PER_BYTE;
  p = s->realloc_fn(s->data, (size_t)bytes, s->realloc_ctx);
  if(p || bytes == 0)
    {
      s->data     = (bsd64_u8 *)p;
      s->capacity = bytes * BSD64_BITS_PER_BYTE;
    }
}

static void
bsd64_set_size_bits(BitStreamDyn64 *s, bsd64_u64 size)
{
  s->size = size;
  if(s->idx > s->size)
    s->idx = s->size;
  bsd64_shrink_to_size(s);
}

static void
bsd64_set_size_8bits(BitStreamDyn64 *s, bsd64_u64 n)
{
  bsd64_set_size_bits(s, n * 8);
}

static void
bsd64_set_size_32bits(BitStreamDyn64 *s, bsd64_u64 n)
{
  bsd64_set_size_bits(s, n * 32);
}


/* ------------------------------------------------------------------ */
/* Read                                                                */
/* ------------------------------------------------------------------ */

static bsd64_u64
bsd64_read_at(const BitStreamDyn64 *s,
              bsd64_u64             idx,
              bsd64_u64             bits)
{
  bsd64_u64       byte_idx;
  bsd64_u8        bit_off;
  const bsd64_u8 *src;
  bsd64_u64       mask;
  bsd64_u64       acc;
  bsd64_u8        remaining;

  assert((idx + bits) <= s->capacity);

  if(bits == 0)
    return 0;

  byte_idx = idx >> 3;
  bit_off  = (bsd64_u8)(idx & 7);
  src      = &s->data[byte_idx];
  mask     = BSD64_MASK(bits);

#if BSD64_HAS_BSWAP
  if(bit_off + bits <= 64)
    {
      acc = bsd64_load64_be(src);
      return (acc >> (64 - bit_off - bits)) & mask;
    }

  acc = bsd64_load64_be(src);
  acc &= (BSD64_ONE << (64 - bit_off)) - 1;
  remaining = (bsd64_u8)(bits - (64 - bit_off));
  return (acc << remaining) | (src[8] >> (8 - remaining));
#else
  if(!bit_off && !(bits & 7))
    {
      bsd64_u64 val = 0;
      bsd64_u64 i;
      for(i = 0; i < (bits >> 3); i++)
        val = (val << 8) | src[i];
      return val;
    }

  if(bit_off + bits <= 64)
    {
      bsd64_u64 n;
      acc = 0;
      n = (bit_off + bits + 7) >> 3;
      {
        bsd64_u64 i;
        for(i = 0; i < n; i++)
          acc = (acc << 8) | src[i];
      }
      return (acc >> (n * 8 - bit_off - bits)) & mask;
    }

  acc = 0;
  {
    bsd64_u64 i;
    for(i = 0; i < 8; i++)
      acc = (acc << 8) | src[i];
  }
  acc &= (BSD64_ONE << (64 - bit_off)) - 1;
  remaining = (bsd64_u8)(bits - (64 - bit_off));
  return (acc << remaining) | (src[8] >> (8 - remaining));
#endif
}

static bsd64_u64
bsd64_read(BitStreamDyn64 *s, bsd64_u64 bits)
{
  bsd64_u64 v = bsd64_read_at(s, s->idx, bits);
  s->idx += bits;
  return v;
}


/* ------------------------------------------------------------------ */
/* Write                                                               */
/* ------------------------------------------------------------------ */

static void
bsd64_write_at(BitStreamDyn64 *s,
               bsd64_u64       idx,
               bsd64_u64       bits,
               bsd64_u64       val)
{
  bsd64_u8  *dst;
  bsd64_u8   bit_off;
  bsd64_u64  remaining;

  bsd64_ensure(s, idx + bits);

  if(bits == 0)
    return;

  dst       = &s->data[idx >> 3];
  bit_off   = (bsd64_u8)(idx & 7);
  remaining = bits;

  if(bit_off)
    {
      bsd64_u8 avail = 8 - bit_off;
      bsd64_u8 take  = (remaining < avail) ? (bsd64_u8)remaining : avail;
      bsd64_u8 shift = avail - take;
      bsd64_u8 mask  = (bsd64_u8)(((1U << take) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bsd64_u8)(((val >> (remaining - take)) & ((BSD64_ONE << take) - 1)) << shift);
      dst++;
      remaining -= take;
    }

  while(remaining >= 8)
    {
      remaining -= 8;
      *dst++ = (bsd64_u8)((val >> remaining) & 0xFF);
    }

  if(remaining)
    {
      bsd64_u8 shift = 8 - (bsd64_u8)remaining;
      bsd64_u8 mask  = (bsd64_u8)(((1U << remaining) - 1) << shift);
      dst[0] = (dst[0] & ~mask) | (bsd64_u8)((val & ((BSD64_ONE << remaining) - 1)) << shift);
    }
}

static void
bsd64_write(BitStreamDyn64 *s, bsd64_u64 bits, bsd64_u64 val)
{
  bsd64_write_at(s, s->idx, bits, val);
  s->idx += bits;
  if(s->idx > s->size)
    s->size = s->idx;
}

static void
bsd64_write_bytes(BitStreamDyn64 *s, const bsd64_u8 *src, bsd64_u64 count)
{
  if(!(s->idx & 7))
    {
      bsd64_ensure(s, s->idx + count * 8);
      memcpy(&s->data[s->idx >> 3], src, (size_t)count);
      s->idx += count * 8;
      if(s->idx > s->size)
        s->size = s->idx;
    }
  else
    {
      bsd64_u64 i;
      for(i = 0; i < count; i++)
        bsd64_write(s, 8, src[i]);
    }
}

static void
bsd64_zero_till_8bit_boundary(BitStreamDyn64 *s)
{
  if(!bsd64_on_8bit_boundary(s))
    bsd64_write(s, bsd64_bits_to_8bit_boundary(s), 0);
}

static void
bsd64_zero_till_16bit_boundary(BitStreamDyn64 *s)
{
  if(!bsd64_on_16bit_boundary(s))
    bsd64_write(s, bsd64_bits_to_16bit_boundary(s), 0);
}

static void
bsd64_zero_till_32bit_boundary(BitStreamDyn64 *s)
{
  if(!bsd64_on_32bit_boundary(s))
    bsd64_write(s, bsd64_bits_to_32bit_boundary(s), 0);
}

static void
bsd64_zero_till_64bit_boundary(BitStreamDyn64 *s)
{
  if(!bsd64_on_64bit_boundary(s))
    bsd64_write(s, bsd64_bits_to_64bit_boundary(s), 0);
}


/* ------------------------------------------------------------------ */
/* Fixed-width read/write macros                                       */
/*                                                                     */
/* BSD64_DEFINE_FIXED(N) generates:                                    */
/*   bsd64_read_fixed_N_at(s, idx)       - random-access read         */
/*   bsd64_read_fixed_N(s)               - streaming read             */
/*   bsd64_write_fixed_N_at(s, idx, val) - random-access write        */
/*   bsd64_write_fixed_N(s, val)         - streaming write            */
/* ------------------------------------------------------------------ */

#if BSD64_HAS_BSWAP

#define BSD64_DEFINE_FIXED(N)                                              \
                                                                           \
static bsd64_u64                                                           \
bsd64_read_fixed_##N##_at(const BitStreamDyn64 *s,                         \
                          bsd64_u64             idx)                       \
{                                                                          \
  const bsd64_u64 BITS = (N);                                              \
  const bsd64_u64 MASK = BSD64_MASK(N);                                    \
  bsd64_u8        bit_off  = (bsd64_u8)(idx & 7);                         \
  const bsd64_u8 *src      = &s->data[idx >> 3];                          \
  bsd64_u64       acc;                                                     \
                                                                           \
  assert((idx + BITS) <= s->capacity);                                     \
                                                                           \
  acc = bsd64_load64_be(src);                                              \
  if(bit_off + BITS <= 64)                                                 \
    return (acc >> (64 - bit_off - BITS)) & MASK;                          \
                                                                           \
  {                                                                        \
    bsd64_u8 remaining;                                                    \
    acc &= (BSD64_ONE << (64 - bit_off)) - 1;                             \
    remaining = (bsd64_u8)(BITS - (64 - bit_off));                         \
    return (acc << remaining) | (src[8] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bsd64_u64                                                           \
bsd64_read_fixed_##N(BitStreamDyn64 *s)                                    \
{                                                                          \
  bsd64_u64 v = bsd64_read_fixed_##N##_at(s, s->idx);                     \
  s->idx += (N);                                                           \
  return v;                                                                \
}                                                                          \
                                                                           \
static void                                                                \
bsd64_write_fixed_##N##_at(BitStreamDyn64 *s,                              \
                           bsd64_u64       idx,                            \
                           bsd64_u64       val)                            \
{                                                                          \
  const bsd64_u64 BITS = (N);                                              \
  bsd64_u8  *dst;                                                          \
  bsd64_u8   bit_off;                                                      \
  bsd64_u64  remaining;                                                    \
                                                                           \
  bsd64_ensure(s, idx + BITS);                                             \
                                                                           \
  dst       = &s->data[idx >> 3];                                          \
  bit_off   = (bsd64_u8)(idx & 7);                                        \
  remaining = BITS;                                                        \
                                                                           \
  if(bit_off)                                                              \
    {                                                                      \
      bsd64_u8 avail = 8 - bit_off;                                       \
      bsd64_u8 take  = (BITS < avail) ? (bsd64_u8)BITS : avail;          \
      bsd64_u8 shift = avail - take;                                      \
      bsd64_u8 mask  = (bsd64_u8)(((1U << take) - 1) << shift);          \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsd64_u8)(((val >> (remaining - take)) &                         \
                     ((BSD64_ONE << take) - 1)) << shift);                \
      dst++;                                                               \
      remaining -= take;                                                   \
    }                                                                      \
                                                                           \
  while(remaining >= 8)                                                    \
    {                                                                      \
      remaining -= 8;                                                      \
      *dst++ = (bsd64_u8)((val >> remaining) & 0xFF);                     \
    }                                                                      \
                                                                           \
  if(remaining)                                                            \
    {                                                                      \
      bsd64_u8 shift = 8 - (bsd64_u8)remaining;                          \
      bsd64_u8 mask  = (bsd64_u8)(((1U << remaining) - 1) << shift);     \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsd64_u8)((val & ((BSD64_ONE << remaining) - 1)) << shift);     \
    }                                                                      \
}                                                                          \
                                                                           \
static void                                                                \
bsd64_write_fixed_##N(BitStreamDyn64 *s, bsd64_u64 val)                    \
{                                                                          \
  bsd64_write_fixed_##N##_at(s, s->idx, val);                             \
  s->idx += (N);                                                           \
  if(s->idx > s->size)                                                     \
    s->size = s->idx;                                                      \
}

#else /* !BSD64_HAS_BSWAP */

#define BSD64_DEFINE_FIXED(N)                                              \
                                                                           \
static bsd64_u64                                                           \
bsd64_read_fixed_##N##_at(const BitStreamDyn64 *s,                         \
                          bsd64_u64             idx)                       \
{                                                                          \
  const bsd64_u64 BITS  = (N);                                             \
  const bsd64_u64 BYTES = (7 + (N) + 7) >> 3;                             \
  const bsd64_u64 MASK  = BSD64_MASK(N);                                   \
  const bsd64_u8 *src   = &s->data[idx >> 3];                             \
  bsd64_u8        bit_off = (bsd64_u8)(idx & 7);                          \
  bsd64_u64       acc   = 0;                                               \
  bsd64_u64       i;                                                       \
                                                                           \
  assert((idx + BITS) <= s->capacity);                                     \
                                                                           \
  if(bit_off + BITS <= 64)                                                 \
    {                                                                      \
      for(i = 0; i < BYTES; i++)                                           \
        acc = (acc << 8) | src[i];                                         \
      return (acc >> (BYTES * 8 - bit_off - BITS)) & MASK;                 \
    }                                                                      \
                                                                           \
  {                                                                        \
    bsd64_u8 remaining;                                                    \
    for(i = 0; i < 8; i++)                                                 \
      acc = (acc << 8) | src[i];                                           \
    acc &= (BSD64_ONE << (64 - bit_off)) - 1;                             \
    remaining = (bsd64_u8)(BITS - (64 - bit_off));                         \
    return (acc << remaining) | (src[8] >> (8 - remaining));               \
  }                                                                        \
}                                                                          \
                                                                           \
static bsd64_u64                                                           \
bsd64_read_fixed_##N(BitStreamDyn64 *s)                                    \
{                                                                          \
  bsd64_u64 v = bsd64_read_fixed_##N##_at(s, s->idx);                     \
  s->idx += (N);                                                           \
  return v;                                                                \
}                                                                          \
                                                                           \
static void                                                                \
bsd64_write_fixed_##N##_at(BitStreamDyn64 *s,                              \
                           bsd64_u64       idx,                            \
                           bsd64_u64       val)                            \
{                                                                          \
  const bsd64_u64 BITS = (N);                                              \
  bsd64_u8  *dst;                                                          \
  bsd64_u8   bit_off;                                                      \
  bsd64_u64  remaining;                                                    \
                                                                           \
  bsd64_ensure(s, idx + BITS);                                             \
                                                                           \
  dst       = &s->data[idx >> 3];                                          \
  bit_off   = (bsd64_u8)(idx & 7);                                        \
  remaining = BITS;                                                        \
                                                                           \
  if(bit_off)                                                              \
    {                                                                      \
      bsd64_u8 avail = 8 - bit_off;                                       \
      bsd64_u8 take  = (BITS < avail) ? (bsd64_u8)BITS : avail;          \
      bsd64_u8 shift = avail - take;                                      \
      bsd64_u8 mask  = (bsd64_u8)(((1U << take) - 1) << shift);          \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsd64_u8)(((val >> (remaining - take)) &                         \
                     ((BSD64_ONE << take) - 1)) << shift);                \
      dst++;                                                               \
      remaining -= take;                                                   \
    }                                                                      \
                                                                           \
  while(remaining >= 8)                                                    \
    {                                                                      \
      remaining -= 8;                                                      \
      *dst++ = (bsd64_u8)((val >> remaining) & 0xFF);                     \
    }                                                                      \
                                                                           \
  if(remaining)                                                            \
    {                                                                      \
      bsd64_u8 shift = 8 - (bsd64_u8)remaining;                          \
      bsd64_u8 mask  = (bsd64_u8)(((1U << remaining) - 1) << shift);     \
      dst[0] = (dst[0] & ~mask) |                                         \
        (bsd64_u8)((val & ((BSD64_ONE << remaining) - 1)) << shift);     \
    }                                                                      \
}                                                                          \
                                                                           \
static void                                                                \
bsd64_write_fixed_##N(BitStreamDyn64 *s, bsd64_u64 val)                    \
{                                                                          \
  bsd64_write_fixed_##N##_at(s, s->idx, val);                             \
  s->idx += (N);                                                           \
  if(s->idx > s->size)                                                     \
    s->size = s->idx;                                                      \
}

#endif /* BSD64_HAS_BSWAP */


#define BSD64_X_ALL \
  X(1)  X(2)  X(3)  X(4)  X(5)  X(6)  X(7)  X(8)  \
  X(9)  X(10) X(11) X(12) X(13) X(14) X(15) X(16) \
  X(17) X(18) X(19) X(20) X(21) X(22) X(23) X(24) \
  X(25) X(26) X(27) X(28) X(29) X(30) X(31) X(32) \
  X(33) X(34) X(35) X(36) X(37) X(38) X(39) X(40) \
  X(41) X(42) X(43) X(44) X(45) X(46) X(47) X(48) \
  X(49) X(50) X(51) X(52) X(53) X(54) X(55) X(56) \
  X(57) X(58) X(59) X(60) X(61) X(62) X(63) X(64)

#define X(n) BSD64_DEFINE_FIXED(n)
BSD64_X_ALL
#undef X

#endif /* BITSTREAM_DYN64_H */
