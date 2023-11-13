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
#include "convert_bitmap_to_imag.hpp"
#include "filerw.hpp"
#include "options.hpp"
#include "read_file.hpp"
#include "stbi.hpp"
#include "template.hpp"
#include "video_image.hpp"

#include "fmt.hpp"

#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;


namespace l
{
  static
  fs::path
  generate_filepath(fs::path const  input_filepath_,
                    fs::path const  output_filepath_,
                    Bitmap const   &bitmap_)
  {
    fs::path rv;
    std::unordered_map<std::string,std::string> extra =
      {
        {"w",fmt::format("{}",bitmap_.w)},
        {"h",fmt::format("{}",bitmap_.h)},
        {"rotation",fmt::format("{}",0)},
        {"index",bitmap_.get("index","0")},
        {"_index",bitmap_.has("index") ? "_" + bitmap_.get("index") : ""}
      };

    rv = resolve_template(input_filepath_,
                          output_filepath_,
                          ".imag",
                          extra);

    return rv;
  }

  static
  void
  to_imag(const fs::path &filepath_,
          const fs::path &output_filepath_)
  {
    BitmapVec bitmaps;

    convert::to_bitmap(filepath_,bitmaps);
    if(bitmaps.empty())
      throw fmt::exception("failed to convert");

    for(auto const &bitmap : bitmaps)
      {
        int rv;
        FileRW f;
        fs::path output_filepath;

        output_filepath = l::generate_filepath(filepath_,output_filepath_,bitmap);

        rv = f.open_write_trunc(output_filepath);
        if(rv < 0)
          {
            fmt::print(" - {}: {}\n",output_filepath,strerror(-rv));
            continue;
          }

        convert::bitmap_to_imag(bitmap,f);

        if((bitmap.w != 320) || (bitmap.h != 240))
          fmt::print(" - WARNING: 3DO SDK's LoadImage() really only supports 320x240.\n");

        std::error_code ec;
        fmt::print(" - output file: {}\n"
                   " - output file size: {}\n"
                   ,
                   output_filepath,
                   fs::file_size(output_filepath,ec));
      }
  }

  static
  bool
  same_extension(const fs::path        &filepath_,
                 const Options::ToIMAG &opts_)
  {
    if(opts_.ignore_target_ext == false)
      return false;
    if(filepath_.has_extension() == false)
      return false;

    return (filepath_.extension() == ".imag");
  }

  static
  void
  handle_file(const fs::path        &filepath_,
              const Options::ToIMAG &opts_)
  {
    fmt::print("{}:\n",filepath_);

    if(l::same_extension(filepath_,opts_))
      {
        fmt::print(" - WARNING - skipping file with target extension\n");
        return;
      }

    try
      {
        l::to_imag(filepath_,opts_.output_path);
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
             const Options::ToIMAG &opts_)
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
  to_imag(const Options::ToIMAG &opts_)
  {
    for(auto const &filepath : opts_.filepaths)
      {
        fs::directory_entry de(filepath);

        if(de.is_regular_file())
          l::handle_file(de.path(),opts_);
        else if(de.is_directory())
          l::handle_dir(de.path(),opts_);
      }
  }
}
