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
#include "bytevec.hpp"
#include "convert.hpp"
#include "identify_file.hpp"
#include "options.hpp"
#include "read_file.hpp"
#include "stbi.hpp"
#include "template.hpp"
#include "video_image.hpp"

#include "fmt.hpp"

#include <filesystem>

namespace fs = std::filesystem;


namespace l
{
  static
  fs::path
  generate_filepath(fs::path const     input_filepath_,
                    fs::path const     output_filepath_,
                    std::string const  type_,
                    Bitmap const      &bitmap_)
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
                          "."+type_,
                          extra);

    return rv;
  }

  void
  to_stb_image(const fs::path         &input_filepath_,
               Options::ToImage const &opts_,
               const std::string       type_)
  {
    int rv;
    ByteVec data;
    BitmapVec bitmaps;

    ReadFile::read(input_filepath_,data);
    if(data.empty())
      throw fmt::exception("file empty");

    convert::to_bitmap(data,bitmaps);
    if(bitmaps.empty())
      throw fmt::exception("failed to convert");

    for(auto const &bitmap : bitmaps)
      {
        fs::path filepath;

        filepath = l::generate_filepath(input_filepath_,opts_.output_path,type_,bitmap);

        rv = stbi_write(bitmap,filepath,type_);
        if(rv)
          fmt::print(" - {}\n",filepath);
        else
          fmt::print(" - ERROR - failed writing file {}",filepath);
      }
  }

  static
  bool
  same_extension(const fs::path         &filepath_,
                 const Options::ToImage &opts_,
                 const std::string      &type_)
  {
    if(opts_.ignore_target_ext == false)
      return false;
    if(filepath_.has_extension() == false)
      return false;

    std::string type;

    type = '.' + type_;

    return (filepath_.extension() == type);
  }

  static
  void
  handle_file(const fs::path         &filepath_,
              const Options::ToImage &opts_,
              const std::string      &type_)
  {
    fmt::print("{}:\n",filepath_);

    if(l::same_extension(filepath_,opts_,type_))
      {
        fmt::print(" - WARNING - skipping file with target extension\n");
        return;
      }

    try
      {
        l::to_stb_image(filepath_,opts_,type_);
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
  handle_dir(const fs::path         &dirpath_,
             const Options::ToImage &opts_,
             const std::string      &type_)
  {
    for(const fs::directory_entry &de : fs::recursive_directory_iterator(dirpath_))
      {
        if(!de.is_regular_file())
          continue;

        l::handle_file(de.path(),opts_,type_);
      }
  }
}

namespace SubCmd
{
  void
  to_stb_image(const Options::ToImage &opts_,
               const std::string      &type_)
  {
    for(const auto &filepath : opts_.filepaths)
      {
        fs::directory_entry de(filepath);

        if(de.is_regular_file())
          l::handle_file(de.path(),opts_,type_);
        else if(de.is_directory())
          l::handle_dir(de.path(),opts_,type_);
      }
  }
}
