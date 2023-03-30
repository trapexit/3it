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
#include "ofstreambe.hpp"

#include "fmt.hpp"

#include <cstdio>
#include <cstring>

namespace fs = std::filesystem;


void
WriteFile::banner(const fs::path &filepath_,
                  const int       width_,
                  const int       height_,
                  cspan<uint8_t>  pdat_)
{
  VideoImage vi;
  std::ofstreambe os;

  os.open(filepath_,std::ofstream::binary|std::ofstream::trunc);
  if(!os)
    throw std::system_error(errno,std::system_category(),"failed to open "+filepath_.string());

  vi.vi_Version   = 0x01;
  memcpy(&vi.vi_Pattern[0],"APPSCRN",sizeof(vi.vi_Pattern));
  vi.vi_Size      = pdat_.size();
  vi.vi_Height    = height_;
  vi.vi_Width     = width_;
  vi.vi_Depth     = 16;
  vi.vi_Type      = 0x00;
  vi.vi_Reserved1 = 0x00;
  vi.vi_Reserved2 = 0x00;
  vi.vi_Reserved3 = 0x00000000;

  os.writebe(vi.vi_Version)
    .write(vi.vi_Pattern,sizeof(vi.vi_Pattern))
    .writebe(vi.vi_Size)
    .writebe(vi.vi_Height)
    .writebe(vi.vi_Width)
    .writebe(vi.vi_Depth)
    .writebe(vi.vi_Type)
    .writebe(vi.vi_Reserved1)
    .writebe(vi.vi_Reserved2)
    .writebe(vi.vi_Reserved3)
    .write(&pdat_[0],pdat_.size());

  if(!os)
    throw std::system_error(errno,std::system_category(),"failed to write "+filepath_.string());

  os.close();
}
