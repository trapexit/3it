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

  data_.w("IMAG");              // id
  data_.u32be(sizeof(ImageControlChunk)); // chunk size
  data_.i32be(bitmap_.w);       // w
  data_.i32be(bitmap_.h);       // h
  data_.i32be(2 * bitmap_.w);   // bytes per row
  data_.u8(16);                 // bits per pixel
  data_.u8(3);                  // numcomponents
  data_.u8(1);                  // numplanes
  data_.u8(0);                  // colorspace
  data_.u8(0);                  // comptype
  data_.u8(0);                  // hvformat
  data_.u8(1);                  // pixelorder
  data_.u8(0);                  // version

  convert::bitmap_to_uncoded_unpacked_lrform_16bpp(bitmap_,pdat);

  data_.w("PDAT");              // id
  data_.u32be(4 + 4 + pdat.size()); // chunk size
  data_.w(pdat);
}
