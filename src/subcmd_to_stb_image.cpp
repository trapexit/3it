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
#include "video_image.hpp"

#include "fmt.hpp"

#include <filesystem>

namespace fs = std::filesystem;


namespace l
{
  void
  to_stb_image(const fs::path    &filepath_,
               const std::string  type_)
  {
    int rv;
    ByteVec data;
    BitmapVec bitmaps;

    ReadFile::read(filepath_,data);
    if(data.empty())
      throw fmt::exception("file empty");

    convert::to_bitmap(data,bitmaps);
    if(bitmaps.empty())
      throw fmt::exception("failed to convert");

    if(bitmaps.size() > 1)
      {
        int padding;
        fs::path filepath;

        padding = fmt::format("{}",bitmaps.size()).size();
        for(size_t i = 0; i < bitmaps.size(); i++)
          {
            filepath = filepath_;
            filepath += fmt::format("_{:0{}}",i,padding);

            if(bitmaps[i].has("name"))
              filepath += fmt::format("_{}",bitmaps[i].name());
            filepath += '.';
            filepath += type_;

            rv = stbi_write(bitmaps[i],filepath,type_);
            if(rv)
              fmt::print(" * {}\n",filepath);
            else
              fmt::print(" * ERROR - failed writing file {}",filepath);
          }
      }
    else
      {
        fs::path filepath;

        filepath  = filepath_;
        if(bitmaps[0].metadata.count("name"))
          filepath += fmt::format("_{}",bitmaps[0].metadata.find("name")->second);
        filepath += '.';
        filepath += type_;

        rv = stbi_write(bitmaps[0],filepath,type_);
        if(rv)
          fmt::print("Converted {} to {}\n",filepath_,filepath);
        else
          fmt::print("ERROR - failed writing file {}",filepath_,filepath);
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
        try
          {
            l::to_stb_image(filepath,type_);
          }
        catch(const std::system_error &e_)
          {
            fmt::print("ERROR - {} ({}): {}\n",e_.what(),e_.code().message(),filepath);
          }
        catch(const std::runtime_error &e_)
          {
            fmt::print("ERROR - {}: {}\n",e_.what(),filepath);
          }
      }
  }
}
