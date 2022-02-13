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

#define CEL_TYPE(CODED,PACKED,LRFORM,BPP) (((((uint32_t)CODED)&0xFF)<<24) | \
                                           ((((uint32_t)PACKED)&0xFF)<<16) | \
                                           ((((uint32_t)LRFORM)&0xFF)<<8) | \
                                           ((((uint32_t)BPP)&0xFF)<<0))

#define UNCODED_UNPACKED_LRFORM_16BPP CEL_TYPE(false,false,true,16)
#define CODED_PACKED_LINEAR_1BPP      CEL_TYPE(true,true,false,1)
#define CODED_PACKED_LINEAR_2BPP      CEL_TYPE(true,true,false,2)
#define CODED_PACKED_LINEAR_4BPP      CEL_TYPE(true,true,false,4)
#define CODED_PACKED_LRFORM_4BPP      CEL_TYPE(true,true,true,4)
#define CODED_PACKED_LINEAR_6BPP      CEL_TYPE(true,true,false,6)
#define CODED_PACKED_LINEAR_8BPP      CEL_TYPE(true,true,false,8)
#define CODED_PACKED_LINEAR_16BPP     CEL_TYPE(true,true,false,16)
#define CODED_UNPACKED_LINEAR_1BPP    CEL_TYPE(true,false,false,1)
#define CODED_UNPACKED_LINEAR_2BPP    CEL_TYPE(true,false,false,2)
#define CODED_UNPACKED_LINEAR_4BPP    CEL_TYPE(true,false,false,4)
#define CODED_UNPACKED_LINEAR_6BPP    CEL_TYPE(true,false,false,6)
#define CODED_UNPACKED_LINEAR_8BPP    CEL_TYPE(true,false,false,8)
#define CODED_UNPACKED_LINEAR_16BPP   CEL_TYPE(true,false,false,16)
#define UNCODED_PACKED_LINEAR_8BPP    CEL_TYPE(false,true,false,8)
#define UNCODED_PACKED_LINEAR_16BPP   CEL_TYPE(false,true,false,16)
#define UNCODED_UNPACKED_LINEAR_8BPP  CEL_TYPE(false,false,false,8)
#define UNCODED_UNPACKED_LINEAR_16BPP CEL_TYPE(false,false,false,16)
