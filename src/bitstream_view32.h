/*
 * bitstream_view32.h - Mutable bitstream view, 64-bit cursor + 32-bit field width
 *
 * C analog of:
 *   using BitStreamView32 = BitStreamT<BitStreamSpan<u8>, u32>;
 *
 * Use bst32_init() to initialize from a writable buffer.
 * All bst32_* functions apply.
 */

#ifndef BITSTREAM_VIEW32_H
#define BITSTREAM_VIEW32_H

#include "bitstream_t32.h"

typedef BitStreamT32 BitStreamView32;

#endif /* BITSTREAM_VIEW32_H */
