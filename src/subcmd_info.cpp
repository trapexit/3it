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
#include "chunk.hpp"
#include "chunk_reader.hpp"
#include "fmt.hpp"
#include "identify_file.hpp"
#include "image_control_chunk.hpp"
#include "options.hpp"
#include "read_file.hpp"
#include "video_image.hpp"

#include <filesystem>
#include <cstdio>

namespace fs = std::filesystem;


namespace l
{
  static
  void
  print_3do_banner_info(cspan<uint8_t> data_)
  {
    VideoImage *vi;

    vi = (VideoImage*)data_.data();

    fmt::print("  - version: {}\n"
               "  - pattern: {:.7}\n"
               "  - size: {}\n"
               "  - height: {}\n"
               "  - width: {}\n"
               "  - depth: {}\n"
               "  - type: {}\n"
               ,
               vi->vi_Version,
               vi->vi_Pattern,
               byteswap_if_little_endian(vi->vi_Size),
               byteswap_if_little_endian(vi->vi_Width),
               byteswap_if_little_endian(vi->vi_Height),
               byteswap_if_little_endian(vi->vi_Depth),
               byteswap_if_little_endian(vi->vi_Type));
  }

  static
  void
  print_3do_cel_info(cspan<uint8_t> data_)

  {
    CelControlChunk ccc;
    ChunkVec chunks;

    ChunkReader::chunkify(data_,chunks);
    for(const auto &chunk : chunks)
      {
        switch(chunk.id())
          {
          case CHUNK_CCB:
            ccc = chunk;
            fmt::print(" - id: {}\n"
                       " - flags: 0x{:08X}\n"
                       "   - skip: {}\n"
                       "   - last: {}\n"
                       "   - npabs: {}\n"
                       "   - spabs: {}\n"
                       "   - ppabs: {}\n"
                       "   - ldsize: {}\n"
                       "   - ldprs: {}\n"
                       "   - ldppmp: {}\n"
                       "   - ldplut: {}\n"
                       "   - ccbpre: {}\n"
                       "   - yoxy: {}\n"
                       "   - acsc: {}\n"
                       "   - alsc: {}\n"
                       "   - acw: {}\n"
                       "   - accw: {}\n"
                       "   - twd: {}\n"
                       "   - lce: {}\n"
                       "   - ace: {}\n"
                       "   - maria: {}\n"
                       "   - pxor: {}\n"
                       "   - useav: {}\n"
                       "   - packed: {}\n"
                       "   - pover: 0b{:02b} ({})\n"
                       "   - plutpos: {}\n"
                       "   - bgnd: {}\n"
                       "   - noblk: {}\n"
                       "   - pluta: {}\n"
                       " - width: {}\n"
                       " - height: {}\n"
                       " - ppmpc: 0x{:08X}\n"
                       "   - P0 1S: 0b{:01b} ({})\n"
                       "   - P0 MS: 0b{:02b} ({})\n"
                       "   - P0 MF: 0b{:03b} ({})\n"
                       "   - P0 DF: 0b{:02b} ({})\n"
                       "   - P0 2S: 0b{:02b} ({})\n"
                       "   - P0 AV: 0b{:05b} ({})\n"
                       "   - P0 2D: 0b{:01b} ({})\n"
                       "   - P1 1S: 0b{:01b} ({})\n"
                       "   - P1 MS: 0b{:02b} ({})\n"
                       "   - P1 MF: 0b{:03b} ({})\n"
                       "   - P1 DF: 0b{:02b} ({})\n"
                       "   - P1 2S: 0b{:02b} ({})\n"
                       "   - P1 AV: 0b{:05b} ({})\n"
                       "   - P1 2D: 0b{:01b} ({})\n"
                       " - pre0: 0x{:08X}\n"
                       "   - literal: {}\n"
                       "   - bgnd: {}\n"
                       "   - skipx: {}\n"
                       "   - vcnt: {}\n"
                       "   - uncoded: {}\n"
                       "   - rep8: {}\n"
                       "   - bpp: {} ({}bpp)\n"
                       " - pre1: 0x{:08X}\n"
                       "   - woffset8: {}\n"
                       "   - woffset10: {}\n"
                       "   - noswap: {}\n"
                       "   - unclsb: {}\n"
                       "   - lrform: {}\n"
                       "   - tlhpcnt: {}\n"
                       ,
                       ccc.id.str(),
                       ccc.ccb_Flags,
                       !!(ccc.ccb_Flags & CCB_SKIP),
                       !!(ccc.ccb_Flags & CCB_LAST),
                       !!(ccc.ccb_Flags & CCB_NPABS),
                       !!(ccc.ccb_Flags & CCB_SPABS),
                       !!(ccc.ccb_Flags & CCB_PPABS),
                       !!(ccc.ccb_Flags & CCB_LDSIZE),
                       !!(ccc.ccb_Flags & CCB_LDPRS),
                       !!(ccc.ccb_Flags & CCB_LDPPMP),
                       !!(ccc.ccb_Flags & CCB_LDPLUT),
                       !!(ccc.ccb_Flags & CCB_CCBPRE),
                       !!(ccc.ccb_Flags & CCB_YOXY),
                       !!(ccc.ccb_Flags & CCB_ACSC),
                       !!(ccc.ccb_Flags & CCB_ALSC),
                       !!(ccc.ccb_Flags & CCB_ACW),
                       !!(ccc.ccb_Flags & CCB_ACCW),
                       !!(ccc.ccb_Flags & CCB_TWD),
                       !!(ccc.ccb_Flags & CCB_LCE),
                       !!(ccc.ccb_Flags & CCB_ACE),
                       !!(ccc.ccb_Flags & CCB_MARIA),
                       !!(ccc.ccb_Flags & CCB_PXOR),
                       !!(ccc.ccb_Flags & CCB_USEAV),
                       !!(ccc.ccb_Flags & CCB_PACKED),
                       ccc.pover(),
                       ccc.pover_str(),
                       !!(ccc.ccb_Flags & CCB_PLUTPOS),
                       !!(ccc.ccb_Flags & CCB_BGND),
                       !!(ccc.ccb_Flags & CCB_NOBLK),
                       ((ccc.ccb_Flags & CCB_PLUTA_MASK) >> CCB_PLUTA_SHIFT),
                       ccc.ccb_Width,
                       ccc.ccb_Height,
                       ccc.ccb_PPMPC,
                       ccc.pixc_1s(0),
                       ccc.pixc_1s_str(0),
                       ccc.pixc_ms(0),
                       ccc.pixc_ms_str(0),
                       ccc.pixc_mf(0),
                       ccc.pixc_mf_str(0),
                       ccc.pixc_df(0),
                       ccc.pixc_df_str(0),
                       ccc.pixc_2s(0),
                       ccc.pixc_2s_str(0),
                       ccc.pixc_av(0),
                       ccc.pixc_av_str(0),
                       ccc.pixc_2d(0),
                       ccc.pixc_2d_str(0),
                       ccc.pixc_1s(1),
                       ccc.pixc_1s_str(1),
                       ccc.pixc_ms(1),
                       ccc.pixc_ms_str(1),
                       ccc.pixc_mf(1),
                       ccc.pixc_mf_str(1),
                       ccc.pixc_df(1),
                       ccc.pixc_df_str(1),
                       ccc.pixc_2s(1),
                       ccc.pixc_2s_str(1),
                       ccc.pixc_av(1),
                       ccc.pixc_av_str(1),
                       ccc.pixc_2d(1),
                       ccc.pixc_2d_str(1),
                       ccc.ccb_PRE0,
                       !!(ccc.ccb_PRE0 & PRE0_LITERAL),
                       !!(ccc.ccb_PRE0 & PRE0_BGND),
                       ((ccc.ccb_PRE0 & PRE0_SKIPX_MASK) >> PRE0_SKIPX_SHIFT),
                       ((ccc.ccb_PRE0 & PRE0_VCNT_MASK) >> PRE0_VCNT_SHIFT),
                       !!(ccc.ccb_PRE0 & PRE0_UNCODED),
                       !!(ccc.ccb_PRE0 & PRE0_REP8),
                       ((ccc.ccb_PRE0 & PRE0_BPP_MASK) >> PRE0_BPP_SHIFT),
                       ccc.bpp(),
                       ccc.ccb_PRE1,
                       ((ccc.ccb_PRE1 & PRE1_WOFFSET8_MASK) >> PRE1_WOFFSET8_SHIFT),
                       ((ccc.ccb_PRE1 & PRE1_WOFFSET10_MASK) >> PRE1_WOFFSET10_SHIFT),
                       !!(ccc.ccb_PRE1 & PRE1_NOSWAP),
                       ((ccc.ccb_PRE1 & PRE1_TLLSB_MASK) >> PRE1_TLLSB_SHIFT),
                       !!(ccc.ccb_PRE1 & PRE1_LRFORM),
                       ((ccc.ccb_PRE1 & PRE1_TLHPCNT_MASK) >> PRE1_TLHPCNT_SHIFT)
                       );
            break;
          }
      }
  }

  void
  print_3do_image_info(cspan<uint8_t> data_)
  {
    ImageControlChunk icc;
    ChunkVec chunks;

    ChunkReader::chunkify(data_,chunks);
    for(const auto &chunk : chunks)
      {
        switch(chunk.id())
          {
          case CHUNK_IMAG:
            icc = chunk;
            break;
          }
      }

    fmt::print("  - id: {}\n"
               "  - w: {}\n"
               "  - h: {}\n"
               "  - bytesperrow: {}\n"
               "  - bitsperpixel: {}\n"
               "  - numcomponents: {}\n"
               "  - numplanes: {} ({})\n"
               "  - colorspace: {} ({})\n"
               "  - comptype: {} ({})\n"
               "  - hvformat: {} ({})\n"
               "  - pixelorder: {} ({})\n"
               "  - version: {}\n"
               ,
               icc.id.str(),
               icc.w,
               icc.h,
               icc.bytesperrow,
               icc.bitsperpixel,
               icc.numcomponents,
               icc.numplanes,
               icc.numplanes_str(),
               icc.colorspace,
               icc.colorspace_str(),
               icc.comptype,
               icc.comptype_str(),
               icc.hvformat,
               icc.hvformat_str(),
               icc.pixelorder,
               icc.pixelorder_str(),
               icc.version);
  }

  static
  void
  info(const fs::path &filepath_)
  {
    uint32_t type;
    ByteVec  data;

    fmt::print("{}:\n",filepath_);

    ReadFile::read(filepath_,data);

    type = IdentifyFile::identify(data);
    switch(type)
      {
      case FILE_ID_3DO_CEL:
        l::print_3do_cel_info(data);
        break;
      case FILE_ID_3DO_IMAGE:
        l::print_3do_image_info(data);
        break;
      case FILE_ID_3DO_BANNER:
        l::print_3do_banner_info(data);
        break;
      case FILE_ID_3DO_ANIM:
        l::print_3do_cel_info(data);
        break;
      case FILE_ID_BMP:
      case FILE_ID_PNG:
      case FILE_ID_JPG:
      case FILE_ID_GIF:
        fmt::print(" - not yet implemented\n");
        break;
      default:
        fmt::print(" - unknown file type\n");
        break;
      }
  }
}

namespace SubCmd
{
  void
  info(const Options::Info &opts_)
  {
    for(const auto &filepath : opts_.filepaths)
      l::info(filepath);
  }
}
