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

#include "bits_and_bytes.hpp"
#include "bitstream.hpp"
#include "bpp.hpp"
#include "cel_control_chunk.hpp"
#include "chunk_reader.hpp"
#include "fmt.hpp"
#include "identify_file.hpp"
#include "options.hpp"
#include "packed.hpp"
#include "read_file.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace l
{
  static
  std::size_t
  calc_offset_width(const std::size_t bpp_)
  {
    switch(bpp_)
      {
      case BPP_1:
      case BPP_2:
      case BPP_4:
      case BPP_6:
        return 8;
      case BPP_8:
      case BPP_16:
        return 16;
      }

    throw fmt::exception("invalid bpp: {}",bpp_);
  }

  static
  void
  unpack_row(const uint32_t         row_,
             BitStreamReader       &bs_,
             const CelControlChunk &ccc_)
  {
    uint8_t type;
    uint32_t pixel;
    uint32_t count;
    uint32_t bpp;
    uint32_t pixels_read;
    uint32_t width;
    uint32_t line_size;
    uint32_t start_offset;

    pixels_read = 0;
    line_size = 0;
    bpp = ccc_.bpp();
    width = ccc_.ccb_Width;
    do
      {
        uint32_t size;
        bool on_32bit_boundary;

        size = DATA_PACKET_DATA_TYPE_SIZE;
        on_32bit_boundary = bs_.on_32bit_boundary();
        type = bs_.read(DATA_PACKET_DATA_TYPE_SIZE);
        switch(type)
          {
          case PACK_LITERAL:
            {
              count = bs_.read(DATA_PACKET_PIXEL_COUNT_SIZE) + 1;
              pixels_read += count;

              size += DATA_PACKET_PIXEL_COUNT_SIZE;
              size += (count * bpp);

              fmt::print("l: count={}; size={}; colors=",count,size);
              for(size_t i = 0; i < count; i++)
                {
                  pixel = bs_.read(bpp);
                  fmt::print("{:x},",pixel);
                }
              fmt::print("\n");
            }
            break;
          case PACK_TRANSPARENT:
            {
              count = bs_.read(DATA_PACKET_PIXEL_COUNT_SIZE) + 1;
              pixels_read += count;
              size += DATA_PACKET_PIXEL_COUNT_SIZE;
              fmt::print("t: count={}; size={};\n",count,size);
            }
            break;
          case PACK_PACKED:
            {
              count = bs_.read(DATA_PACKET_PIXEL_COUNT_SIZE) + 1;
              pixel = bs_.read(bpp);
              pixels_read += count;
              size += DATA_PACKET_PIXEL_COUNT_SIZE;
              size += bpp;
              line_size += size;
              fmt::print("p: count={}; size={}; color={}\n",count,size,pixel);
            }
            break;
          case PACK_EOL:
            line_size += size;
            fmt::print("e: size={}; aligned={};\n",
                       DATA_PACKET_DATA_TYPE_SIZE,
                       on_32bit_boundary);

            break;
          }
      } while(type != PACK_EOL && pixels_read < width);

    fmt::print("row={} end; line_size={}; pixels_read={};\n",
               row_,
               line_size,
               pixels_read);
  }

  void
  dump_packed_instructions(fs::path const &filepath_)
  {
    uint32_t type;
    uint32_t offset;
    u32 row_offset;
    ByteVec data;
    ChunkVec chunks;
    CelControlChunk ccc;

    ReadFile::read(filepath_,data);

    type = IdentifyFile::identify(data);
    if(!IdentifyFile::chunked_type(type))
      {
        fmt::print("ERROR - not a recognized 3DO chunked file: {}\n",filepath_);
        return;
      }

    ChunkReader::chunkify(data,chunks);

    for(uint32_t i = 0; i < chunks.size(); i++)
      {
        const auto &chunk = chunks[i];
        if(chunk.id() == CHUNK_CCB)
          {
            ccc = chunk;
            if(ccc.unpacked())
              throw std::runtime_error("CEL not packed");
            continue;
          }

        if(chunk.id() != CHUNK_PDAT)
          continue;

        uint32_t offset_width;
        BitStreamReader bs(chunk.data(),chunk.data_size());

        offset = 0;
        offset_width = l::calc_offset_width(ccc.bpp());
        for(auto row = 0; row < ccc.ccb_Height; row++)
          {
            uint32_t next_offset;

            bs.seek(offset * BITS_PER_BYTE);
            next_offset = offset + ((bs.read(offset_width) + 2) * BYTES_PER_WORD);
            fmt::print("row={} start; offset={}; next_offset={};\n",
                       row,
                       offset,
                       next_offset);
            unpack_row(row,bs,ccc);

            bs.seek(offset * BITS_PER_BYTE);
            for(u64 i = offset; i < next_offset; i+=4)
              {
                u32 x = bs.read(BITS_PER_BYTE * 4);
                fmt::print("{:08X} ",x);
              }
            fmt::print("\n");
            
            offset = next_offset;
          }
      }
  }
}

namespace SubCmd
{
  void
  dump_packed_instructions(const Options::DumpPacked &opts_)
  {
    l::dump_packed_instructions(opts_.filepath);
  }
}
