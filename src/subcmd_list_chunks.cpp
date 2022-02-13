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

#include "chunk_reader.hpp"
#include "fmt.hpp"
#include "identify_file.hpp"
#include "options.hpp"
#include "read_file.hpp"

#include <filesystem>

namespace fs = std::filesystem;


namespace l
{
  void
  list_chunks(const fs::path &filepath_)
  {
    uint32_t type;
    uint32_t offset;
    ByteVec  data;
    ChunkVec chunks;

    fmt::print("{}:\n",filepath_);

    ReadFile::read(filepath_,data);

    type = IdentifyFile::identify(data);
    if(!IdentifyFile::chunked_type(type))
      {
        fmt::print("ERROR - not a recognized 3DO chunked file: {}\n",filepath_);
        return;
      }

    ChunkReader::chunkify(data,chunks);

    offset = 0;
    for(uint32_t i = 0; i < chunks.size(); i++)
      {
        const auto &chunk = chunks[i];
        fmt::print(" - chunk:\n"
                   "   - num: {}\n"
                   "   - id: {}\n"
                   "   - size: {}\n"
                   "   - offset: {}\n"
                   ,
                   i,
                   chunk.idstr(),
                   chunk.size(),
                   offset);
        offset += chunk.size();
      }
  }
}

namespace SubCmd
{
  void
  list_chunks(const Options::ListChunks &opts_)
  {
    for(const auto &filepath : opts_.filepaths)
      l::list_chunks(filepath);
  }
}
