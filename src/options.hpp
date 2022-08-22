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
    Flag skip;
    Flag last;
    Flag npabs;
    Flag spabs;
    Flag ppabs;
    Flag ldsize;
    Flag ldprs;
    Flag ldppmp;
    Flag ldplut;
    Flag ccbpre;
    Flag yoxy;
    Flag acsc;
    Flag alsc;
    Flag acw;
    Flag accw;
    Flag twd;
    Flag lce;
    Flag ace;
    Flag maria;
    Flag pxor;
    Flag useav;
    Flag packed;
    Flag plutpos;
    Flag bgnd;
    Flag noblk;
  };

  struct Pre0Flags
  {
    Flag literal;
    Flag bgnd;
    Flag uncoded;
    Flag rep8;
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

  struct ToCEL
  {
    PathVec   filepaths;
    uint8_t   bpp;
    bool      coded;
    bool      lrform;
    bool      packed;
    uint32_t  transparent;
    CCBFlags  ccb_flags;
    Pre0Flags pre0_flags;
  };

  struct ToBanner
  {
    PathVec filepaths;
  };

  struct ToImage
  {
    PathVec filepaths;
  };

  struct ToNFSSHPM
  {
    PathVec  filepaths;
    Path     outputpath;
    bool     packed;
    uint32_t transparent;
  };

public:
  Info       info;
  ListChunks list_chunks;
  ToCEL      to_cel;
  ToBanner   to_banner;
  ToImage    to_image;
  ToNFSSHPM  to_nfs_shpm;
};
