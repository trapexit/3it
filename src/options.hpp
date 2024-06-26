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

#pragma once

#include <filesystem>
#include <vector>
#include <cstdint>

struct Options
{
public:
  typedef std::filesystem::path Path;
  typedef std::vector<Path>     PathVec;

public:
  enum class Flag
    {
     DEFAULT,
     SET,
     UNSET
    };

  struct CCBFlags
  {
    Flag skip    = Flag::DEFAULT;
    Flag last    = Flag::DEFAULT;
    Flag npabs   = Flag::DEFAULT;
    Flag spabs   = Flag::DEFAULT;
    Flag ppabs   = Flag::DEFAULT;
    Flag ldsize  = Flag::DEFAULT;
    Flag ldprs   = Flag::DEFAULT;
    Flag ldppmp  = Flag::DEFAULT;
    Flag ldplut  = Flag::DEFAULT;
    Flag ccbpre  = Flag::DEFAULT;
    Flag yoxy    = Flag::DEFAULT;
    Flag acsc    = Flag::DEFAULT;
    Flag alsc    = Flag::DEFAULT;
    Flag acw     = Flag::DEFAULT;
    Flag accw    = Flag::DEFAULT;
    Flag twd     = Flag::DEFAULT;
    Flag lce     = Flag::DEFAULT;
    Flag ace     = Flag::DEFAULT;
    Flag maria   = Flag::DEFAULT;
    Flag pxor    = Flag::DEFAULT;
    Flag useav   = Flag::DEFAULT;
    Flag packed  = Flag::DEFAULT;
    Flag plutpos = Flag::DEFAULT;
    Flag bgnd    = Flag::DEFAULT;
    Flag noblk   = Flag::DEFAULT;
  };

  struct Pre0Flags
  {
    Flag literal = Flag::DEFAULT;
    Flag bgnd    = Flag::DEFAULT;
    Flag uncoded = Flag::DEFAULT;
    Flag rep8    = Flag::DEFAULT;
  };

  struct Info
  {
    PathVec     filepaths;
    std::string format;
  };

  struct ListChunks
  {
    PathVec filepaths;
  };

  struct DumpPacked
  {
    Path filepath;
  };

  struct ToCEL
  {
    CCBFlags    ccb_flags;
    Path        external_palette;
    Path        output_path;
    PathVec     filepaths;
    Pre0Flags   pre0_flags;
    bool        coded             = false;
    bool        generate_all      = false;
    bool        ignore_target_ext = false;
    bool        lrform            = false;
    bool        packed            = false;
    bool        write_plut        = true;
    int         rotation          = 0;
    std::string find_smallest;
    std::uint32_t    transparent;
    std::uint8_t     bpp;
  };

  struct ToBanner
  {
    PathVec filepaths;
    Path    output_path;
    bool    ignore_target_ext = false;
  };

  struct ToIMAG
  {
    PathVec filepaths;
    Path    output_path;
    bool    ignore_target_ext = false;
  };

  struct ToLRFORM
  {
    PathVec filepaths;
    Path    output_path;
    bool    ignore_target_ext = false;
  };

  struct ToImage
  {
    PathVec filepaths;
    Path    output_path;
    bool    ignore_target_ext = false;
  };

  struct ToNFSSHPM
  {
    PathVec  filepaths;
    Path     output_path;
    bool     packed = false;
    uint32_t transparent;
  };

public:
  Info       info;
  ListChunks list_chunks;
  DumpPacked dump_packed;
  ToCEL      to_cel;
  ToBanner   to_banner;
  ToIMAG     to_imag;
  ToLRFORM   to_lrform;
  ToImage    to_image;
  ToNFSSHPM  to_nfs_shpm;
};
