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

#include "subcmd_to_nfs_shpm.hpp"

#include "fmt.hpp"

#include "bitmaps.hpp"
#include "bytevec.hpp"
#include "read_file.hpp"
#include "convert.hpp"
#include "byteswap.hpp"
#include "file.hpp"

#define BPP_16   0x6
#define PACKED   0x80
#define UNPACKED 0x00
#define UNCODED  0x10


namespace fs = std::filesystem;

namespace l
{
  void
  write_file(const BitmapVec  &bitmaps_,
             const ByteVecVec &pdats_,
             const bool        packed_,
             const fs::path   &output_path_)
  {
    File f;
    fs::path output_path;
    uint32_t length_of_file;
    uint32_t object_count;

    object_count = pdats_.size();

    length_of_file = (16 + (object_count * 8));
    for(const auto &pdat : pdats_)
      length_of_file += 16 + pdat.size();

    f.open(output_path_,"wb+");

    f.big_endian();

    f.write("SHPM",4);
    f.write(length_of_file);
    f.write(object_count);
    f.write("SPoT",4);

    uint32_t offset = (16 + (object_count * 8));
    for(size_t i = 0; i < object_count; i++)
      {
        std::string name;

        name = bitmaps_[i].name_or_guess();

        f.write(name.c_str(),4);
        f.write(offset);
        offset += 16 + pdats_[i].size();
      }

    for(size_t i = 0; i < pdats_.size(); i++)
      {
        uint8_t type;
        uint16_t w;
        uint16_t h;

        type = (BPP_16 | UNCODED | (packed_ ? PACKED : UNPACKED));
        w = bitmaps_[i].w;
        h = bitmaps_[i].h;

        f.write(type);
        f.seek(3,SEEK_CUR);
        f.write(w);
        f.write(h);
        f.seek(8,SEEK_CUR);
        f.write(pdats_[i]);
      }

    f.close();
  }
}


void
SubCmd::to_nfs_shpm(const Options::ToNFSSHPM &opts_)
{
  fs::path output_path;
  BitmapVec bitmaps;
  ByteVecVec pdats;

  for(const auto &filepath : opts_.filepaths)
    convert::to_bitmap(filepath,bitmaps);

  for(const auto &bitmap : bitmaps)
    {
      pdats.emplace_back();

      if(opts_.packed)
        convert::bitmap_to_uncoded_packed_linear_16bpp(bitmap,pdats.back());
      else
        convert::bitmap_to_uncoded_unpacked_linear_16bpp(bitmap,pdats.back());
    }

  output_path = opts_.output_path;
  if(output_path.empty())
    {
      output_path  = opts_.filepaths[0];
      output_path += ".3sh";
    }

  fmt::print("{}:\n",output_path);
  l::write_file(bitmaps,pdats,opts_.packed,output_path);
  for(const auto &filepath : opts_.filepaths)
    fmt::print(" - {}\n",filepath);
}
