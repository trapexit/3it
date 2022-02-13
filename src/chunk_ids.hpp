/*
  ISC License

  Copyright (c) 2022, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#pragma once

#include "char4literal.hpp"

#define CHUNK_3DO  CHAR4LITERAL('3','D','O',' ') /* wrapper chunk */
#define CHUNK_IMAG CHAR4LITERAL('I','M','A','G') /* the image control chunk */
#define CHUNK_PDAT CHAR4LITERAL('P','D','A','T') /* pixel data */
#define CHUNK_CCB  CHAR4LITERAL('C','C','B',' ') /* cel control */
#define CHUNK_ANIM CHAR4LITERAL('A','N','I','M') /* ANIM chunk */
#define CHUNK_PLUT CHAR4LITERAL('P','L','U','T') /* pixel lookup table */
#define CHUNK_VDL  CHAR4LITERAL('V','D','L',' ') /* VDL chunk */
#define CHUNK_CPYR CHAR4LITERAL('C','P','Y','R') /* copyright text*/
#define CHUNK_DESC CHAR4LITERAL('D','E','S','C') /* description text */
#define CHUNK_KWRD CHAR4LITERAL('K','W','R','D') /* keyword text */
#define CHUNK_CRDT CHAR4LITERAL('C','R','D','T') /* credits text */
#define CHUNK_XTRA CHAR4LITERAL('X','T','R','A') /* 3DO Animator creates these */
