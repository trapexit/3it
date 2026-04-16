/*
 * bitstream_view64.h - Mutable bitstream view, 64-bit cursor + 64-bit field width
 *
 * C analog of:
 *   using BitStreamView = BitStreamT<BitStreamSpan<u8>>;
 *
 * Use bst64_init() to initialize from a writable buffer.
 * All bst64_* functions apply.
 */

#ifndef BITSTREAM_VIEW64_H
#define BITSTREAM_VIEW64_H

#include "bitstream_t64.h"

typedef BitStreamT64 BitStreamView64;

#endif /* BITSTREAM_VIEW64_H */
