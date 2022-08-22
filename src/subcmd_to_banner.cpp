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
    BitmapVec bitmaps;

    convert::to_bitmap(filepath_,bitmaps);
    if(bitmaps.empty() || !bitmaps.front())
      throw fmt::exception("failed to convert");

    convert::bitmap_to_uncoded_unpacked_lrform_16bpp(bitmaps.front(),pdat);

    if(!pdat.empty())
      {
        fs::path filepath;

        filepath = l::generate_filepath(filepath_);

        WriteFile::banner(filepath,bitmaps.front().w,bitmaps.front().h,pdat);

        fmt::print(" - {}\n",filepath);
      }
  }

  static
  bool
  same_extension(const fs::path          &filepath_,
                 const Options::ToBanner &opts_)
  {
    if(opts_.ignore_target_ext == false)
      return false;
    if(filepath_.has_extension() == false)
      return false;

    return (filepath_.extension() == ".banner");
  }

  static
  void
  handle_file(const fs::path          &filepath_,
              const Options::ToBanner &opts_)
  {
    fmt::print("{}:\n",filepath_);

    if(l::same_extension(filepath_,opts_))
      {
        fmt::print(" - WARNING - skipping file with target extension\n");
        return;
      }

    try
      {
        l::to_banner(filepath_);
      }
    catch(const std::system_error &e_)
      {
        fmt::print(" - ERROR - {} ({})\n",e_.what(),e_.code().message());
      }
    catch(const std::runtime_error &e_)
      {
        fmt::print(" - ERROR - {}\n",e_.what());
      }
  }

  static
  void
  handle_dir(const fs::path          &dirpath_,
             const Options::ToBanner &opts_)
  {
    for(const fs::directory_entry &de : fs::recursive_directory_iterator(dirpath_))
      {
        if(!de.is_regular_file())
          continue;

        l::handle_file(de.path(),opts_);
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
        fs::directory_entry de(filepath);

        if(de.is_regular_file())
          l::handle_file(de.path(),opts_);
        else if(de.is_directory())
          l::handle_dir(de.path(),opts_);
      }
  }
}
