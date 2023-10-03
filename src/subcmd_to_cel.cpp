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
  generate_filepath(const fs::path        &filepath_,
                    const CelControlChunk &ccc_)
  {
    fs::path filepath;

    filepath = filepath_;
    filepath += (ccc_.coded() ? "_coded" : "_uncoded");
    filepath += (ccc_.packed() ? "_packed" : "_unpacked");
    filepath += fmt::format("_{}bpp",ccc_.bpp());
    filepath += ".cel";

    return filepath;
  }

  static
  void
  to_cel(const fs::path       &filepath_,
         const Options::ToCEL &opts_)
  {
    CelType celtype;
    ByteVec data;
    BitmapVec bitmaps;

    ReadFile::read(filepath_,data);
    if(data.empty())
      throw fmt::exception("file empty");

    convert::to_bitmap(data,bitmaps);
    if(bitmaps.empty())
      throw fmt::exception("failed to convert");

    celtype.coded  = opts_.coded;
    celtype.packed = opts_.packed;
    celtype.lrform = opts_.lrform;
    celtype.bpp    = opts_.bpp;

    for(auto &bitmap : bitmaps)
      {
        if(celtype.lrform && (bitmap.h & 0x1))
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

    for(auto const &bitmap : bitmaps)
      {
        PLUT plut;
        ByteVec pdat;
        CelControlChunk ccc;
        fs::path filepath;

        convert::bitmap_to_cel(bitmap,celtype,pdat,plut);
        if(pdat.empty())
          continue;

        l::populate_ccc(celtype,bitmap.w,bitmap.h,ccc);

        l::modify_ccb_flags(opts_.ccb_flags,ccc);
        l::modify_pre0_flags(opts_.pre0_flags,ccc);

        if(opts_.output_path.empty())
          filepath = l::generate_filepath(filepath_,ccc);
        else
          filepath = opts_.output_path;

        if(opts_.write_plut == false)
          plut.clear();
        WriteFile::cel(filepath,ccc,pdat,plut);

        fmt::print(" - {}\n",filepath);
      }
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
        fmt::print(" - WARNING - skipping file with target extension\n");
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

  static
  void
  handle_dir(const fs::path       &dirpath_,
             const Options::ToCEL &opts_)
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
  to_cel(const Options::ToCEL &opts_)
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
