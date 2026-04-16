/*
 * bitstream_stream64.h - Owning/dynamic bitstream, 64-bit cursor + 64-bit field width
 *
 * C analog of:
 *   using BitStream = BitStreamT<std::vector<u8>>;
 *
 * Use bsd64_init_dyn(&s, bsd64_stdlib_realloc, NULL) to initialize
 * with the default malloc-backed allocator.  Call bsd64_free(&s) when
 * done.  Supply a custom bsd64_realloc_fn to use an arena or pool.
 *
 * All bsd64_* functions apply.
 */

#ifndef BITSTREAM_STREAM64_H
#define BITSTREAM_STREAM64_H

#include "bitstream_dyn64.h"

typedef BitStreamDyn64 BitStream64;

#endif /* BITSTREAM_STREAM64_H */
