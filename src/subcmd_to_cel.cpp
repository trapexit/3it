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

#include "byteswap.hpp"
#include "bytevec.hpp"
#include "ccb_flags.hpp"
#include "cel_control_chunk.hpp"
#include "cel_packer.hpp"
#include "cel_types.hpp"
#include "chunk_ids.hpp"
#include "convert.hpp"
#include "fp12_20.hpp"
#include "fp16_16.hpp"
#include "options.hpp"
#include "read_file.hpp"
#include "stbi.hpp"
#include "template.hpp"
#include "write_cel.hpp"

#include "fmt.hpp"

#include <filesystem>
#include <cstdint>


namespace fs = std::filesystem;

namespace l
{
  static
  void
  modify_flag(const Options::Flag &flag_,
              const uint32_t       mask_,
              uint32_t            &flags)
  {
    if(flag_ == Options::Flag::SET)
      flags |= mask_;
    else if(flag_ == Options::Flag::UNSET)
      flags &= ~mask_;
  }

  static
  void
  modify_ccb_flags(const Options::CCBFlags &flags_,
                   CelControlChunk         &ccc_)
  {
    modify_flag(flags_.skip,CCB_SKIP,ccc_.ccb_Flags);
    modify_flag(flags_.last,CCB_LAST,ccc_.ccb_Flags);
    modify_flag(flags_.npabs,CCB_NPABS,ccc_.ccb_Flags);
    modify_flag(flags_.spabs,CCB_SPABS,ccc_.ccb_Flags);
    modify_flag(flags_.ppabs,CCB_PPABS,ccc_.ccb_Flags);
    modify_flag(flags_.ldsize,CCB_LDSIZE,ccc_.ccb_Flags);
    modify_flag(flags_.ldprs,CCB_LDPRS,ccc_.ccb_Flags);
    modify_flag(flags_.ldppmp,CCB_LDPPMP,ccc_.ccb_Flags);
    modify_flag(flags_.ldplut,CCB_LDPLUT,ccc_.ccb_Flags);
    modify_flag(flags_.ccbpre,CCB_CCBPRE,ccc_.ccb_Flags);
    modify_flag(flags_.yoxy,CCB_YOXY,ccc_.ccb_Flags);
    modify_flag(flags_.acsc,CCB_ACSC,ccc_.ccb_Flags);
    modify_flag(flags_.alsc,CCB_ALSC,ccc_.ccb_Flags);
    modify_flag(flags_.acw,CCB_ACW,ccc_.ccb_Flags);
    modify_flag(flags_.accw,CCB_ACCW,ccc_.ccb_Flags);
    modify_flag(flags_.twd,CCB_TWD,ccc_.ccb_Flags);
    modify_flag(flags_.lce,CCB_LCE,ccc_.ccb_Flags);
    modify_flag(flags_.ace,CCB_ACE,ccc_.ccb_Flags);
    modify_flag(flags_.maria,CCB_MARIA,ccc_.ccb_Flags);
    modify_flag(flags_.pxor,CCB_PXOR,ccc_.ccb_Flags);
    modify_flag(flags_.useav,CCB_USEAV,ccc_.ccb_Flags);
    modify_flag(flags_.packed,CCB_PACKED,ccc_.ccb_Flags);
    modify_flag(flags_.plutpos,CCB_PLUTPOS,ccc_.ccb_Flags);
    modify_flag(flags_.bgnd,CCB_BGND,ccc_.ccb_Flags);
    modify_flag(flags_.noblk,CCB_NOBLK,ccc_.ccb_Flags);
  }

  static
  void
  modify_pre0_flags(const Options::Pre0Flags &flags_,
                    CelControlChunk          &ccc_)
  {
    modify_flag(flags_.literal,PRE0_LITERAL,ccc_.ccb_PRE0);
    modify_flag(flags_.bgnd,PRE0_BGND,ccc_.ccb_PRE0);
    modify_flag(flags_.uncoded,PRE0_UNCODED,ccc_.ccb_PRE0);
    modify_flag(flags_.rep8,PRE0_REP8,ccc_.ccb_PRE0);
  }

  static
  uint32_t
  round_up(const uint32_t number_,
           const uint32_t multiple_)
  {
    return (((number_ + multiple_ - 1) / multiple_) * multiple_);
  }

  static
  void
  populate_ccc(const CelType   &celtype_,
               const int        w_,
               const int        h_,
               CelControlChunk &ccc_)
  {
    ccc_.id          = CHUNK_CCB;
    ccc_.chunk_size  = sizeof(CelControlChunk);
    ccc_.ccb_version = 0x00000000;

    ccc_.ccb_Flags  = 0;
    ccc_.ccb_Flags |= CCB_LAST;
    ccc_.ccb_Flags |= CCB_LDSIZE;
    ccc_.ccb_Flags |= CCB_LDPRS;
    ccc_.ccb_Flags |= CCB_LDPPMP;
    ccc_.ccb_Flags |= CCB_CCBPRE;
    ccc_.ccb_Flags |= CCB_YOXY;
    ccc_.ccb_Flags |= CCB_ACW;
    ccc_.ccb_Flags |= CCB_ACCW;
    ccc_.ccb_Flags |= CCB_ACE;
    ccc_.ccb_Flags |= CCB_USEAV;
    ccc_.ccb_Flags |= CCB_BGND;
    if(celtype_.packed)
      ccc_.ccb_Flags |= CCB_PACKED;
    if(celtype_.coded)
      ccc_.ccb_Flags |= CCB_LDPLUT;

    ccc_.ccb_hdx = ONE_12_20;
    ccc_.ccb_vdy = ONE_16_16;

    ccc_.ccb_PPMPC |= PPMP_OPAQUE;

    ccc_.bpp(celtype_.bpp);
    if(!celtype_.coded)
      ccc_.ccb_PRE0 |= PRE0_UNCODED;
    ccc_.ccb_PRE0 |= ((h_ - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT);

    ccc_.ccb_PRE1  = ((w_ - PRE1_TLHPCNT_PREFETCH) & PRE1_TLHPCNT_MASK);
    if(celtype_.lrform)
      ccc_.ccb_PRE1 |= PRE1_LRFORM;
    ccc_.ccb_PRE1 |= PRE1_TLLSB_PDC0;

    switch(celtype_.bpp)
      {
      case 1:
      case 2:
      case 4:
      case 6:
        ccc_.ccb_PRE1 |= (((round_up(w_ * celtype_.bpp,32) / 32) - PRE1_WOFFSET_PREFETCH) << PRE1_WOFFSET8_SHIFT);
        break;
      case 8:
      case 16:
        ccc_.ccb_PRE1 |= (((round_up(w_ * celtype_.bpp,32) / 32) - PRE1_WOFFSET_PREFETCH) << PRE1_WOFFSET10_SHIFT);
        break;
      }

    ccc_.ccb_Width  = w_;
    ccc_.ccb_Height = h_;
  }

  static
  fs::path
  generate_filepath(const fs::path         src_filepath_,
                    const fs::path         dst_filepath_,
                    const Bitmap          &bitmap_,
                    const CelControlChunk &ccc_)
  {
    fs::path filepath;
    std::unordered_map<std::string,std::string> extra =
      {
        {"coded",(ccc_.coded() ? "coded" : "uncoded")},
        {"packed",(ccc_.packed() ? "packed" : "unpacked")},
        {"lrform",(ccc_.lrform() ? "lrform" : "linear")},
        {"_lrform",(ccc_.lrform() ? "_lrform" : "")},
        {"bpp",fmt::to_string(ccc_.bpp())},
        {"w",fmt::to_string(ccc_.ccb_Width)},
        {"h",fmt::to_string(ccc_.ccb_Height)},
        {"flags",fmt::format("{:08x}",ccc_.ccb_Flags)},
        {"pixc",fmt::format("{:08x}",ccc_.ccb_PPMPC)},
        {"rotation",fmt::to_string(bitmap_.get("rotation"))},
        {"_name",bitmap_.has("name") ? "_" + bitmap_.get("name") : ""},
        {"index",bitmap_.get("index","0")},
        {"_index",bitmap_.has("index") ? "_" + bitmap_.get("index") : ""}
      };

    filepath = resolve_path_template(src_filepath_,
                                     dst_filepath_,
                                     ".cel",
                                     extra);

    return filepath;
  }

  static
  void
  write_file(const fs::path        &filepath_,
             const Options::ToCEL  &opts_,
             const CelControlChunk &ccc_,
             const ByteVec         &pdat_,
             const PLUT            &plut_)
  {
    PLUT plut;

    if(opts_.write_plut == true)
      plut = plut_;

    WriteFile::cel(filepath_,ccc_,pdat_,plut);
    fmt::print(" - {}\n",filepath_);
  }

  static
  void
  find_smallest_regular(Bitmap  &bitmap_,
                        CelType &celtype_,
                        PLUT    &plut_,
                        ByteVec &pdat_)
  {
    CelType tmp_celtype;
    PLUT    tmp_plut;
    ByteVec tmp_pdat;
    Bitmap  tmp_bitmap;
    PLUT    best_plut;
    ByteVec best_pdat;
    CelType best_celtype;
    Bitmap  best_bitmap;
    std::array<bool,2>    packeds   = {false, true};
    std::array<bool,2>    codeds    = {false, true};
    std::array<uint8_t,6> bpps      = {1,2,4,6,8,16};

    best_celtype = tmp_celtype = celtype_;
    best_plut    = tmp_plut    = plut_;
    best_pdat    = tmp_pdat    = pdat_;
    best_bitmap  = tmp_bitmap  = bitmap_;

    for(const auto bpp : bpps)
      {
        for(const auto packed : packeds)
          {
            for(const auto coded : codeds)
              {
                tmp_celtype.bpp    = bpp;
                tmp_celtype.lrform = false;
                tmp_celtype.packed = packed;
                tmp_celtype.coded  = coded;
                try
                  {
                    convert::bitmap_to_cel(bitmap_,tmp_celtype,tmp_pdat,tmp_plut);
                  }
                catch(...)
                  {
                    continue;
                  }

                if(!best_pdat.empty() && (tmp_pdat.size() >= best_pdat.size()))
                  continue;

                best_celtype = tmp_celtype;
                best_pdat    = tmp_pdat;
                best_plut    = tmp_plut;
                best_bitmap  = bitmap_;
              }
          }
      }

    celtype_ = best_celtype;
    plut_    = best_plut;
    pdat_    = best_pdat;
    bitmap_  = best_bitmap;
  }

  static
  void
  find_smallest_rotation(Bitmap  &bitmap_,
                         CelType &celtype_,
                         PLUT    &plut_,
                         ByteVec &pdat_)
  {
    CelType tmp_celtype;
    PLUT    tmp_plut;
    ByteVec tmp_pdat;
    Bitmap  tmp_bitmap;
    PLUT    best_plut;
    ByteVec best_pdat;
    CelType best_celtype;
    Bitmap  best_bitmap;
    std::array<int,4>     rotations = {0,90,180,270};
    std::array<bool,2>    packeds   = {false, true};
    std::array<bool,2>    codeds    = {false, true};
    std::array<uint8_t,6> bpps      = {1,2,4,6,8,16};

    best_celtype = tmp_celtype = celtype_;
    best_plut    = tmp_plut    = plut_;
    best_pdat    = tmp_pdat    = pdat_;
    best_bitmap  = tmp_bitmap  = bitmap_;
    for(auto rotation : rotations)
      {
        bitmap_.rotate_to(rotation);

        for(const auto bpp : bpps)
          {
            for(const auto packed : packeds)
              {
                for(const auto coded : codeds)
                  {
                    tmp_celtype.bpp    = bpp;
                    tmp_celtype.lrform = false;
                    tmp_celtype.packed = packed;
                    tmp_celtype.coded  = coded;
                    try
                      {
                        convert::bitmap_to_cel(bitmap_,tmp_celtype,tmp_pdat,tmp_plut);
                      }
                    catch(...)
                      {
                        continue;
                      }

                    if(!best_pdat.empty() && (tmp_pdat.size() >= best_pdat.size()))
                      continue;

                    best_celtype = tmp_celtype;
                    best_pdat    = tmp_pdat;
                    best_plut    = tmp_plut;
                    best_bitmap  = bitmap_;
                  }
              }
          }
      }

    celtype_ = best_celtype;
    plut_    = best_plut;
    pdat_    = best_pdat;
    bitmap_  = best_bitmap;
  }

  static
  void
  convert(const fs::path       &filepath_,
          const Options::ToCEL &opts_,
          Bitmap               &bitmap_)
  {
    PLUT plut;
    ByteVec pdat;
    CelType celtype;
    CelControlChunk ccc;
    fs::path filepath;

    celtype.bpp    = opts_.bpp;
    celtype.coded  = opts_.coded;
    celtype.lrform = opts_.lrform;
    celtype.packed = opts_.packed;

    if(opts_.find_smallest.empty())
      convert::bitmap_to_cel(bitmap_,celtype,pdat,plut);
    else if(opts_.find_smallest == "regular")
      l::find_smallest_regular(bitmap_,celtype,plut,pdat);
    else if(opts_.find_smallest == "rotation")
      l::find_smallest_rotation(bitmap_,celtype,plut,pdat);
    else
      throw std::runtime_error("Unknown request");

    if(pdat.empty())
      return;

    l::populate_ccc(celtype,bitmap_.w,bitmap_.h,ccc);

    l::modify_ccb_flags(opts_.ccb_flags,ccc);
    l::modify_pre0_flags(opts_.pre0_flags,ccc);

    filepath = l::generate_filepath(filepath_,
                                    opts_.output_path,
                                    bitmap_,
                                    ccc);

    l::write_file(filepath,opts_,ccc,pdat,plut);
  }

  static
  void
  generate_all_cel_types(const fs::path       &filepath_,
                         const Options::ToCEL &opts_,
                         const BitmapVec      &bitmaps_)
  {
    Options::ToCEL opts;
    std::string filepath_template;

    std::array<bool,2>    packeds   = {false, true};
    std::array<bool,2>    codeds    = {false, true};
    std::array<uint8_t,6> bpps      = {1,2,4,6,8,16};

    opts = opts_;
    filepath_template = "{filepath}_{coded}_{packed}_{bpp}bpp{_index}{ext}";
    opts.output_path = filepath_template;
    for(auto bitmap : bitmaps_)
      {
        for(const auto bpp : bpps)
          {
            for(const auto packed : packeds)
              {
                for(const auto coded : codeds)
                  {
                    opts.bpp    = bpp;
                    opts.coded  = coded;
                    opts.lrform = false;
                    opts.packed = packed;

                    try
                      {
                        l::convert(filepath_,opts,bitmap);
                      }
                    catch(const std::runtime_error &e_)
                      {

                      }
                  }
              }
          }
      }
  }

  static
  void
  to_cel(const fs::path       &filepath_,
         const Options::ToCEL &opts_)
  {
    ByteVec data;
    BitmapVec bitmaps;

    ReadFile::read(filepath_,data);
    if(data.empty())
      throw fmt::exception("file empty");

    convert::to_bitmap(data,bitmaps);
    if(bitmaps.empty())
      throw fmt::exception("failed to convert");

    for(auto &bitmap : bitmaps)
      {
        bitmap.rotate_to(opts_.rotation);

        if(opts_.lrform && (bitmap.h & 0x1))
          {
            bitmap.h--;
            fmt::print(" - WARNING - LRFORM needs to be an even number of vertical lines"
                       ": truncating from {} to {} lines\n",
                       bitmap.h + 1,
                       bitmap.h);
          }

        if(!opts_.external_palette.empty())
          bitmap.set("external-palette",opts_.external_palette.string());

        bitmap.replace_color(opts_.transparent,0x00000000);
      }

    if(opts_.generate_all)
      return generate_all_cel_types(filepath_,opts_,bitmaps);

    for(auto &bitmap : bitmaps)
      l::convert(filepath_,opts_,bitmap);
  }

  static
  bool
  same_extension(const fs::path       &filepath_,
                 const Options::ToCEL &opts_)
  {
    if(opts_.ignore_target_ext == false)
      return false;
    if(filepath_.has_extension() == false)
      return false;

    return (filepath_.extension() == ".cel");
  }

  static
  void
  handle_file(const fs::path       &filepath_,
              const Options::ToCEL &opts_)
  {
    fmt::print("{}:\n",filepath_);

    if(l::same_extension(filepath_,opts_))
      {
        fmt::print(" - INFO - skipping file with target extension\n");
        return;
      }

    try
      {
        l::to_cel(filepath_,opts_);
      }
    catch(const std::system_error &e_)
      {
        fmt::print(" - ERROR - {} - {} ({})\n",filepath_,e_.what(),e_.code().message());
      }
    catch(const std::runtime_error &e_)
      {
        fmt::print(" - ERROR - {} - {}\n",filepath_,e_.what());
      }
  }

  std::vector<fs::path>
  get_filepaths(const Options::PathVec &filepaths_)
  {
    std::vector<fs::path> rv;

    for(const auto &filepath : filepaths_)
      {
        fs::directory_entry de(filepath);

        if(de.is_regular_file())
          rv.emplace_back(de.path());
        else if(de.is_directory())
          {
            auto diriter = fs::recursive_directory_iterator(filepath);

            for(const fs::directory_entry &de : diriter)
              {
                if(de.is_regular_file())
                  rv.emplace_back(de.path());
              }
          }
      }

    return rv;
  }
}

namespace SubCmd
{
  void
  to_cel(const Options::ToCEL &opts_)
  {
    Options::PathVec filepaths;

    filepaths = l::get_filepaths(opts_.filepaths);

    for(const auto &filepath : filepaths)
      l::handle_file(filepath,opts_);
  }
}
