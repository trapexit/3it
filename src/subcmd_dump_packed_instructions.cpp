/*
  ISC License

  Copyright (c) 2025, Antonio SJ Musumeci <trapexit@spawn.link>

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
  unpack_row(const u32              row_,
             BitStreamReader       &bs_,
             const CelControlChunk &ccc_)
  {
    uint8_t type;
    u32 pixel;
    u32 count;
    u32 bpp;
    u32 pixels_read;
    u32 width;
    u32 line_size;
    u32 start_offset;

    pixels_read = 0;
    bpp = ccc_.bpp();
    width = ccc_.ccb_Width;
    line_size = ::calc_offset_width(bpp);
    do
      {
        u32 size;

        size = DATA_PACKET_DATA_TYPE_SIZE;
        type = bs_.read(DATA_PACKET_DATA_TYPE_SIZE);
        switch(type)
          {
          case PACK_LITERAL:
            {
              count = bs_.read(DATA_PACKET_PIXEL_COUNT_SIZE) + 1;
              pixels_read += count;

              size += DATA_PACKET_PIXEL_COUNT_SIZE;
              size += (count * bpp);
              line_size += size;

              fmt::print("literal: count={}; size={}; colors=",count,size);
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
              line_size += size;
              fmt::print("transparent: count={}; size={};\n",count,size);
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
              fmt::print("packed: count={}; color={}; size={};\n",count,pixel,size);
            }
            break;
          case PACK_EOL:
            line_size += size;
            fmt::print("eol: size={};\n",
                       DATA_PACKET_DATA_TYPE_SIZE);
            break;
          }
      } while(type != PACK_EOL && pixels_read < width);

    fmt::print("row={} end; line_size={}; pixels={}; leftover={};\n",
               row_,
               line_size,
               pixels_read,
               bs_.bits_to_32bit_boundary());
  }

  void
  dump_packed_instructions(fs::path const &filepath_)
  {
    u32 type;
    u32 offset;
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

    for(u32 i = 0; i < chunks.size(); i++)
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

        u32 offset_width;
        BitStreamReader bs(chunk.data(),chunk.data_size());

        offset = 0;
        offset_width = l::calc_offset_width(ccc.bpp());
        for(auto row = 0; row < ccc.ccb_Height; row++)
          {
            u32 next_offset;

            bs.seek(offset * BITS_PER_BYTE);
            row_offset = bs.read(offset_width) + 2;
            next_offset = offset + (row_offset * BYTES_PER_WORD);
            fmt::println("row={} start; data_range=[{},{});",
                         row,
                         offset,
                         next_offset);


            fmt::print("data: ");
            bs.seek(offset * BITS_PER_BYTE);
            for(u64 i = offset; i < next_offset; i+=4)
              {
                u32 x = bs.read(BITS_PER_BYTE * 4);
                fmt::print("{:08X} ",x);
              }
            fmt::println("\noffset: len={}+2; size={};",
                         row_offset-2,
                         offset_width);

            bs.seek((offset * BITS_PER_BYTE) + offset_width);
            unpack_row(row,bs,ccc);

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
