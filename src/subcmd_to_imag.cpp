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
#include "vecrw.hpp"
#include "video_image.hpp"

#include "fmt.hpp"

#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;


namespace l
{
  static
  convert::ImagMode
  imag_mode(const std::string &mode_)
  {
    if(mode_ == "fixed")
      return convert::ImagMode::FIXED;
    if(mode_ == "v480")
      return convert::ImagMode::V480;
    if(mode_ == "vdl")
      return convert::ImagMode::VDL;
    if(mode_ == "xvdl")
      return convert::ImagMode::XVDL;
    if(mode_ == "v480-vdl")
      return convert::ImagMode::V480_VDL;
    if(mode_ == "v480-xvdl")
      return convert::ImagMode::V480_XVDL;
    if(mode_ == "z24")
      return convert::ImagMode::Z24;

    throw fmt::exception("unknown IMAG mode: {}",mode_);
  }

  static
  convert::ImagPalette
  imag_palette(const std::string &palette_)
  {
    if(palette_ == "legacy")
      return convert::ImagPalette::LEGACY;
    if(palette_ == "modern")
      return convert::ImagPalette::MODERN;

    throw fmt::exception("unknown IMAG palette: {}",palette_);
  }

  static
  fs::path
  generate_filepath(const fs::path  src_filepath_,
                    const fs::path  dst_filepath_,
                    const Bitmap   &bitmap_)
  {
    fs::path filepath;
    std::unordered_map<std::string,std::string> extra =
      {
        {"w",fmt::format("{}",bitmap_.w)},
        {"h",fmt::format("{}",bitmap_.h)},
        {"_name",bitmap_.has("name") ? "_" + bitmap_.get("name") : ""},
        {"index",bitmap_.get("index","0")},
        {"_index",bitmap_.has("index") ? "_" + bitmap_.get("index") : ""}
      };

    filepath = resolve_path_template(src_filepath_,
                                     dst_filepath_,
                                     ".imag",
                                     extra);

    return filepath;
  }

  static
  bool
  to_imag(const fs::path        &input_filepath_,
          const Options::ToIMAG &opts_)
  {
    BitmapVec bitmaps;
    convert::ImagEncodingOptions encoding;
    bool success = true;

    encoding.mode = l::imag_mode(opts_.mode);
    encoding.palette = l::imag_palette(opts_.palette);

    convert::to_bitmap(input_filepath_,bitmaps);
    if(bitmaps.empty())
      throw fmt::exception("failed to convert");

    for(auto const &bitmap : bitmaps)
      {
        int rv;
        FileRW f;
        ByteVec encoded;
        VecRW writer;
        fs::path output_filepath;

        output_filepath = l::generate_filepath(input_filepath_,
                                               opts_.output_path,
                                               bitmap);

        writer.reset(&encoded);
        convert::bitmap_to_imag(bitmap,writer,encoding);

        rv = f.open_write_trunc(output_filepath);
        if(rv < 0)
          {
            fmt::print(" - {}: {}\n",output_filepath,strerror(-rv));
            success = false;
            continue;
          }

        f.w(encoded);

        if((encoding.mode == convert::ImagMode::V480) ||
           (encoding.mode == convert::ImagMode::V480_VDL) ||
           (encoding.mode == convert::ImagMode::V480_XVDL))
          fmt::print(" - NOTE: v480 output requires a VDL_480RES display path; "
                     "LoadImage() does not display it directly.\n");
        else if((bitmap.w != 320) || (bitmap.h != 240))
          fmt::print(" - WARNING: 3DO SDK's LoadImage() really only supports 320x240.\n");

        fmt::print(" - {}\n",output_filepath);
      }

    return success;
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
  bool
  handle_file(const fs::path        &filepath_,
              const Options::ToIMAG &opts_)
  {
    fmt::print("{}:\n",filepath_);

    if(l::same_extension(filepath_,opts_))
      {
        fmt::print(" - WARNING - skipping file with target extension\n");
        return true;
      }

    try
      {
        return l::to_imag(filepath_,opts_);
      }
    catch(const std::system_error &e_)
      {
        fmt::print(" - ERROR - {} ({})\n",e_.what(),e_.code().message());
      }
    catch(const std::runtime_error &e_)
      {
        fmt::print(" - ERROR - {}\n",e_.what());
      }

    return false;
  }

  static
  bool
  handle_dir(const fs::path        &dirpath_,
             const Options::ToIMAG &opts_)
  {
    bool success = true;

    for(const fs::directory_entry &de : fs::recursive_directory_iterator(dirpath_))
      {
        if(!de.is_regular_file())
          continue;

        if(!l::handle_file(de.path(),opts_))
          success = false;
      }

    return success;
  }
}

namespace SubCmd
{
  void
  to_imag(const Options::ToIMAG &opts_)
  {
    bool success = true;

    for(auto const &filepath : opts_.filepaths)
      {
        fs::directory_entry de(filepath);

        if(de.is_regular_file())
          {
            if(!l::handle_file(de.path(),opts_))
              success = false;
          }
        else if(de.is_directory())
          {
            if(!l::handle_dir(de.path(),opts_))
              success = false;
          }
      }

    if(!success)
      throw fmt::exception("one or more IMAG conversions failed");
  }
}
