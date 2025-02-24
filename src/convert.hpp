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

#include "bitmap.hpp"
#include "bitmaps.hpp"
#include "pdat.hpp"
#include "plut.hpp"
#include "span.hpp"
#include "types_ints.h"

#include <filesystem>


union CelType
{
  struct
  {
    u8 bpp:5;
    u8 lrform:1;
    u8 packed:1;
    u8 coded:1;
  };

  u8 switchable;
};

namespace convert
{
  void banner_to_bitmap(cspan<u8>  data,
                        BitmapVec &bitmaps);
  void cel_to_bitmap(cspan<u8>  data,
                     BitmapVec &bitmaps);
  void anim_to_bitmap(cspan<u8>  data,
                      BitmapVec &bitmaps);
  void imag_to_bitmap(cspan<u8>  data,
                      BitmapVec &bitmaps);
  void lrform_to_bitmap(cspan<u8>  data,
                        BitmapVec &bitmaps);  
  void nfs_shpm_to_bitmap(cspan<u8>  data,
                          BitmapVec &bitmaps);
  void nfs_wwww_to_bitmap(cspan<u8>  data,
                          BitmapVec &bitmaps);

  void to_bitmap(cspan<u8>  data,
                 BitmapVec &bitmaps);
  void to_bitmap(const std::filesystem::path &filepath,
                 BitmapVec                   &bitmaps);

  void bitmap_to_cel(const Bitmap  &bitmap,
                     const CelType &celtype,
                     ByteVec       &pdat,
                     PLUT          &plut);

  void bitmap_to_uncoded_unpacked_lrform_16bpp(const Bitmap &bitmap,
                                               ByteVec      &pdat);

  void bitmap_to_uncoded_unpacked_linear_8bpp(const Bitmap &bitmap,
                                              ByteVec      &pdat);
  void bitmap_to_uncoded_unpacked_linear_16bpp(const Bitmap &bitmap,
                                               ByteVec      &pdat);

  void bitmap_to_uncoded_packed_linear_8bpp(const Bitmap &bitmap,
                                            ByteVec      &pdat);
  void bitmap_to_uncoded_packed_linear_16bpp(const Bitmap &bitmap,
                                             ByteVec      &pdat);

  void bitmap_to_coded_unpacked_linear_1bpp(const Bitmap &bitmap,
                                            ByteVec      &pdat,
                                            PLUT         &plut);
  void bitmap_to_coded_unpacked_linear_2bpp(const Bitmap &bitmap,
                                            ByteVec      &pdat,
                                            PLUT         &plut);
  void bitmap_to_coded_unpacked_linear_4bpp(const Bitmap &bitmap,
                                            ByteVec      &pdat,
                                            PLUT         &plut);
  void bitmap_to_coded_unpacked_linear_6bpp(const Bitmap &bitmap,
                                            ByteVec      &pdat,
                                            PLUT         &plut);
  void bitmap_to_coded_unpacked_linear_8bpp(const Bitmap &bitmap,
                                            ByteVec      &pdat,
                                            PLUT         &plut);
  void bitmap_to_coded_unpacked_linear_16bpp(const Bitmap &bitmap,
                                             ByteVec      &pdat,
                                             PLUT         &plut);

  void bitmap_to_coded_packed_linear_1bpp(const Bitmap &bitmap,
                                          ByteVec      &pdat,
                                          PLUT         &plut);
  void bitmap_to_coded_packed_linear_2bpp(const Bitmap &bitmap,
                                          ByteVec      &pdat,
                                          PLUT         &plut);
  void bitmap_to_coded_packed_linear_4bpp(const Bitmap &bitmap,
                                          ByteVec      &pdat,
                                          PLUT         &plut);
  void bitmap_to_coded_packed_linear_6bpp(const Bitmap &bitmap,
                                          ByteVec      &pdat,
                                          PLUT         &plut);
  void bitmap_to_coded_packed_linear_8bpp(const Bitmap &bitmap,
                                          ByteVec      &pdat,
                                          PLUT         &plut);
  void bitmap_to_coded_packed_linear_16bpp(const Bitmap &bitmap,
                                           ByteVec      &pdat,
                                           PLUT         &plut);

  void uncoded_unpacked_linear_8bpp_to_bitmap(cPDAT   pdat,
                                              Bitmap &bitmap);
  void uncoded_unpacked_linear_rep8_8bpp_to_bitmap(cPDAT   pdat,
                                                   Bitmap &bitmap);
  void uncoded_unpacked_linear_16bpp_to_bitmap(cPDAT   pdat,
                                               Bitmap &bitmap);

  void uncoded_unpacked_lrform_16bpp_to_bitmap(cPDAT   pdat,
                                               Bitmap &bitmap);

  void uncoded_packed_linear_8bpp_to_bitmap(cPDAT   pdat,
                                            Bitmap &bitmap);
  void uncoded_packed_linear_16bpp_to_bitmap(cPDAT   pdat,
                                             Bitmap &bitmap);

  void coded_packed_linear_1bpp_to_bitmap(cPDAT       pdat,
                                          const PLUT &plut,
                                          const u8    pluta,
                                          Bitmap     &bitmap);
  void coded_packed_linear_2bpp_to_bitmap(cPDAT       pdat,
                                          const PLUT &plut,
                                          const u8    pluta,
                                          Bitmap     &bitmap);
  void coded_packed_linear_4bpp_to_bitmap(cPDAT       pdat,
                                          const PLUT &plut,
                                          const u8    pluta,
                                          Bitmap     &bitmap);
  void coded_packed_linear_6bpp_to_bitmap(cPDAT       pdat,
                                          const PLUT &plut,
                                          const u8    pluta,
                                          Bitmap     &bitmap);
  void coded_packed_linear_8bpp_to_bitmap(cPDAT       pdat,
                                          const PLUT &plut,
                                          const u32   pdv,
                                          Bitmap     &bitmap);
  void coded_packed_linear_16bpp_to_bitmap(cPDAT       pdat,
                                           const PLUT &plut,
                                           const u8    pluta,
                                           Bitmap     &bitmap);

  void coded_unpacked_linear_1bpp_to_bitmap(cPDAT       pdat,
                                            const PLUT &plut,
                                            const u8    pluta,
                                            Bitmap     &bitmap);
  void coded_unpacked_linear_2bpp_to_bitmap(cPDAT       pdat,
                                            const PLUT &plut,
                                            const u8    pluta,
                                            Bitmap     &bitmap);
  void coded_unpacked_linear_4bpp_to_bitmap(cPDAT       pdat,
                                            const PLUT &plut,
                                            const u8    pluta,
                                            Bitmap     &bitmap);
  void coded_unpacked_linear_6bpp_to_bitmap(cPDAT       pdat,
                                            const PLUT &plut,
                                            const u8    pluta,
                                            Bitmap     &bitmap);
  void coded_unpacked_linear_8bpp_to_bitmap(cPDAT       pdat,
                                            const PLUT &plut,
                                            const u32   pdv,
                                            Bitmap     &bitmap);
  void coded_unpacked_linear_16bpp_to_bitmap(cPDAT       pdat,
                                             const PLUT &plut,
                                             const u8    pluta,
                                             Bitmap     &bitmap);
}
