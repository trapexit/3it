/*
 * bitstream_stream32.h - Owning/dynamic bitstream, 64-bit cursor + 32-bit field width
 *
 * C analog of:
 *   using BitStream32 = BitStreamT<std::vector<u8>, u32>;
 *
 * Use bsd32_init_dyn(&s, bsd32_stdlib_realloc, NULL) to initialize
 * with the default malloc-backed allocator.  Call bsd32_free(&s) when
 * done.  Supply a custom bsd32_realloc_fn to use an arena or pool.
 *
 * All bsd32_* functions apply.
 */

#ifndef BITSTREAM_STREAM32_H
#define BITSTREAM_STREAM32_H

#include "bitstream_dyn32.h"

typedef BitStreamDyn32 BitStream32;

#endif /* BITSTREAM_STREAM32_H */
