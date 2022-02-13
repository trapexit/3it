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

#include "bitmap.hpp"
#include "byteswap.hpp"
#include "bytevec.hpp"
#include "convert.hpp"
#include "options.hpp"
#include "read_file.hpp"
#include "stbi.hpp"
#include "video_image.hpp"
#include "write_banner.hpp"

#include "fmt.hpp"

#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;


namespace l
{
  static
  fs::path
  generate_filepath(const fs::path filepath_)
  {
    fs::path filepath;

    filepath  = filepath_;
    filepath += ".banner";

    return filepath;
  }

  static
  void
  to_banner(const fs::path &filepath_)
  {
    ByteVec pdat;
    ByteVec data;
    std::vector<Bitmap> bitmaps;

    ReadFile::read(filepath_,data);
    if(data.empty())
      throw fmt::exception("file empty: {}",filepath_);

    convert::to_bitmap(data,bitmaps);
    if(bitmaps.empty() || !bitmaps.front())
      throw fmt::exception("failed to convert: {}",filepath_);

    convert::bitmap_to_uncoded_unpacked_lrform_16bpp(bitmaps.front(),pdat);

    if(!pdat.empty())
      {
        fs::path filepath;

        filepath = l::generate_filepath(filepath_);

        WriteFile::banner(filepath,bitmaps.front().w,bitmaps.front().h,pdat);

        fmt::print("Converted {} to {}\n",filepath_,filepath);
      }
  }
}

namespace SubCmd
{
  void
  to_banner(const Options::ToBanner &opts_)
  {
    for(const auto &filepath : opts_.filepaths)
      {
        try
          {
            l::to_banner(filepath);
          }
        catch(const std::system_error &e_)
          {
            fmt::print("ERROR - {} ({})\n",e_.what(),e_.code().message());
          }
        catch(const std::runtime_error &e_)
          {
            fmt::print("ERROR - {}\n",e_.what());
          }
      }
  }
}
