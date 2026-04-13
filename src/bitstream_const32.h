/*
 * bitstream_const32.h - Read-only bitstream view, 32-bit index
 *
 * C analog of:
 *   using BitStreamReader32 = BitStreamT<BitStreamConstSpan<u8>, u32>;
 *
 * Use bst32_init_ro() to initialize from a const buffer.
 * Call only bst32_read* functions; writing is undefined behaviour.
 * All bst32_* functions are accessible via the same BitStreamT32 struct.
 */

#ifndef BITSTREAM_CONST32_H
#define BITSTREAM_CONST32_H

#include "bitstream_t32.h"

typedef BitStreamT32 BitStreamConst32;

#endif /* BITSTREAM_CONST32_H */
