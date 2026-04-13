/*
 * bitstream_const64.h - Read-only bitstream view, 64-bit index
 *
 * C analog of:
 *   using BitStreamReader = BitStreamT<BitStreamConstSpan<u8>>;
 *
 * Use bst64_init_ro() to initialize from a const buffer.
 * Call only bst64_read* functions; writing is undefined behaviour.
 * All bst64_* functions are accessible via the same BitStreamT64 struct.
 */

#ifndef BITSTREAM_CONST64_H
#define BITSTREAM_CONST64_H

#include "bitstream_t64.h"

typedef BitStreamT64 BitStreamConst64;

#endif /* BITSTREAM_CONST64_H */
