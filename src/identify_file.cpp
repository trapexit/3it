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

#include "identify_file.hpp"

#include "read_file.hpp"

#include "stbi.hpp"

#include <cstring>


uint32_t
IdentifyFile::identify(cspan<uint8_t> data_)
{
  uint32_t type;

  type = stbi_identify(data_);
  if(type != FILE_ID_UNKNOWN)
    return type;

  type = CHAR4LITERAL(data_[0],data_[1],data_[2],data_[3]);
  switch(type)
    {
    case FILE_ID_3DO:
    case FILE_ID_3DO_CEL:
    case FILE_ID_3DO_IMAGE:
    case FILE_ID_3DO_ANIM:
    case FILE_ID_NFS_SHPM:
      return type;
    default:
      break;
    }

  if(!memcmp(&data_[0],"\x01""APPSCRN",8))
    return FILE_ID_3DO_BANNER;

  return FILE_ID_UNKNOWN;
}

uint32_t
IdentifyFile::identify(const std::filesystem::path &filepath_)
{
  ByteVec data;

  ReadFile::read(filepath_,data);

  return IdentifyFile::identify(data);
}

bool
IdentifyFile::chunked_type(const uint32_t type_)
{
  switch(type_)
    {
    case FILE_ID_3DO:
    case FILE_ID_3DO_CEL:
    case FILE_ID_3DO_IMAGE:
    case FILE_ID_3DO_ANIM:
      return true;
    default:
      return false;
    }
}
