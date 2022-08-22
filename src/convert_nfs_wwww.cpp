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

#include "convert.hpp"

#include "byte_reader.hpp"

#include "fmt.hpp"


// https://3dodev.com/documentation/file_formats/games/nfs


void
convert::nfs_wwww_to_bitmap(cspan<uint8_t>  data_,
                            BitmapVec      &bitmaps_)
{
  ByteReader br;
  uint32_t chunk_count;

  br.reset(data_);

  br.skip(4); // "wwww"
  chunk_count = br.u32be();

  for(uint32_t i = 1; i <= chunk_count; i++)
    {
      uint32_t start_offset;
      uint32_t end_offset;

      start_offset = br.u32be();
      if(start_offset == 0)
        continue;
      end_offset = ((i != chunk_count) ? br.u32be() : data_.size());
      br.rewind(4);

      cspan<uint8_t> data = data_(start_offset,end_offset);
      try
        {
          convert::to_bitmap(data,bitmaps_);
        }
      catch(const std::system_error &e_)
        {
          fmt::print(" * WARNING - {} ({}): offset={}\n",e_.what(),e_.code().message(),data.off());
        }
      catch(const std::runtime_error &e_)
        {
          fmt::print(" * WARNING - {}: offset={}\n",e_.what(),data.off());
        }
    }
}
