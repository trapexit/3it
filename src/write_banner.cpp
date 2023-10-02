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

#include "write_banner.hpp"

#include "byteswap.hpp"
#include "video_image.hpp"
#include "filerw.hpp"

#include "fmt.hpp"

#include <cstdio>
#include <cstring>

namespace fs = std::filesystem;


void
WriteFile::banner(const fs::path &filepath_,
                  const int       width_,
                  const int       height_,
                  ByteVec        &pdat_)
{
  FileRW f;

  f.open_write_trunc(filepath_);
  if(f.error())
    throw std::system_error(errno,std::system_category(),"failed to open "+filepath_.string());

  f.u8(0x01);                   // vi_Version
  f.w("APPSCRN");               // vi_Pattern
  f.u32be(pdat_.size());        // vi_Size
  f.u16be(height_);             // vi_Height
  f.u16be(width_);              // vi_Width
  f.u8(16);                     // vi_Depth
  f.u8(0);                      // vi_Type
  f.u8(0);                      // vi_Reserved1
  f.u8(0);                      // vi_Reserved2
  f.u32be(0);                   // vi_Reserved3
  f.w(pdat_);

  if(f.error())
    throw std::system_error(errno,std::system_category(),"failed to write "+filepath_.string());

  f.close();
}
