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

#include "bytevec.hpp"
#include "char4literal.hpp"
#include "chunk_ids.hpp"
#include "span.hpp"
#include "stbi.hpp"

#include <filesystem>

#define FILE_ID_UNKNOWN    CHAR4LITERAL('?','?','?','?')
#define FILE_ID_3DO        CHUNK_3DO
#define FILE_ID_3DO_CEL    CHUNK_CCB
#define FILE_ID_3DO_IMAGE  CHUNK_IMAG
#define FILE_ID_3DO_ANIM   CHUNK_ANIM
#define FILE_ID_3DO_BANNER CHAR4LITERAL('S','C','R','N')
#define FILE_ID_3DO_LRFORM CHAR4LITERAL('L','R','F','M')
#define FILE_ID_BMP        STBI_FILE_TYPE_BMP
#define FILE_ID_PNG        STBI_FILE_TYPE_PNG
#define FILE_ID_GIF        STBI_FILE_TYPE_GIF
#define FILE_ID_JPG        STBI_FILE_TYPE_JPG
#define FILE_ID_PSD        STBI_FILE_TYPE_PSD
#define FILE_ID_PIC        STBI_FILE_TYPE_PIC
#define FILE_ID_PNM        STBI_FILE_TYPE_PNM
#define FILE_ID_HDR        STBI_FILE_TYPE_HDR
#define FILE_ID_TGA        STBI_FILE_TYPE_TGA
#define FILE_ID_NFS_SHPM   CHAR4LITERAL('S','H','P','M')
#define FILE_ID_NFS_WWWW   CHAR4LITERAL('w','w','w','w')


namespace IdentifyFile
{
  uint32_t identify(cspan<uint8_t> data);
  uint32_t identify(const std::filesystem::path &filepath);
  bool     chunked_type(const uint32_t type);
}
