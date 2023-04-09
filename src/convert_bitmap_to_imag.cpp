/*
  ISC License

  Copyright (c) 2023, Antonio SJ Musumeci <trapexit@spawn.link>

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

#include "convert_bitmap_to_imag.hpp"

#include "image_control_chunk.hpp"
#include "pdat.hpp"
#include "convert.hpp"

void
convert::bitmap_to_imag(const Bitmap &bitmap_,
                        DataRW       &data_)
{
  ByteVec pdat;
  ImageControlChunk icc;

  icc.id = "IMAG";
  icc.chunk_size = sizeof(icc);
  icc.w = bitmap_.w;
  icc.h = bitmap_.h;
  icc.bytesperrow = (2 * bitmap_.w);
  icc.bitsperpixel = 16;
  icc.numcomponents = 3;
  icc.numplanes = 1;
  icc.colorspace = 0;
  icc.comptype = 0;
  icc.hvformat = 0;
  icc.pixelorder = 1;
  icc.version = 0;

  data_.write((const uint8_t*)"IMAG",4);
  data_.writebe(icc.chunk_size);
  data_.writebe(icc.w);
  data_.writebe(icc.h);
  data_.writebe(icc.bytesperrow);
  data_.writebe(icc.bitsperpixel);
  data_.writebe(icc.numcomponents);
  data_.writebe(icc.numplanes);
  data_.writebe(icc.colorspace);
  data_.writebe(icc.comptype);
  data_.writebe(icc.hvformat);
  data_.writebe(icc.pixelorder);
  data_.writebe(icc.version);

  convert::bitmap_to_uncoded_unpacked_lrform_16bpp(bitmap_,pdat);

  data_.write((const uint8_t*)"PDAT",4);
  data_.writebe((uint32_t)(4 + 4 + pdat.size()));
  data_.write(pdat.data(),pdat.size());
}
