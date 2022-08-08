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
#include "chunkid.hpp"

#include "fmt.hpp"

#include <vector>

// https://3dodev.com/documentation/file_formats/games/nfs

/*
  Multibyte integers are big endian

  Header

  Offset | Length (bytes) | Description
  0x00   | 4              | ASCII 'SHPM'
  0x04   | 4              | length of file in bytes
  0x08   | 4              | object count
  0x0C   | 4              | ASCII 'SPoT'

  Object entry list
  There will be a pair of values per object.

  0x10   | 4              | ASCII identifier
  0x14   | 4              | offset in bytes from beginning of file to
                            object

  Object header
  0x00   | 1              | type
                            bit 7: packed?
                            bit 6: ???
                            bit 5: ???
                            bit 4: uncoded?
                            bit 3: ???
                            bit 2-0: CEL BPP? (1->1,2->2,3->4,4->6,5->8,6->16)
  0x01   | 1              | 0x00 ???
  0x02   | 1              | 0x00 ???
  0x03   | 1              | 0x00 ???
  0x04   | 2              | width in pixels
  0x06   | 2              | height in pixels
  0x08   | 4              | 0x00000000 ???
  0x0C   | 4              | 0x00000000 ???
  0x10   | variable       | pixel data in 3DO CEL file format
 */

#define BPP_16   0x6
#define PACKED   0x80
#define UNPACKED 0x00
#define UNCODED  0x10

static
void
nfs_shpm_obj_to_bitmap(cspan<uint8_t>  obj_,
                       Bitmap         &bitmap_)
{
  uint32_t w;
  uint32_t h;
  uint32_t type;
  ByteReader br;

  br.reset(obj_);

  type = br.u8();
  br.skip(3);
  w = br.u16be();
  h = br.u16be();
  br.skip(8);

  bitmap_.reset(w,h);
  switch(type)
    {
    case (PACKED|UNCODED|BPP_16):
      convert::uncoded_packed_linear_16bpp_to_bitmap(br,bitmap_);
      break;
    case (UNPACKED|UNCODED|BPP_16):
      convert::uncoded_unpacked_linear_16bpp_to_bitmap(br,bitmap_);
      break;
    default:
      throw fmt::exception("unknown NFS SHPM type: {:x}",type);
    }
}

void
convert::nfs_shpm_to_bitmap(cspan<uint8_t>  data_,
                            BitmapVec      &bitmaps_)
{
  ByteReader br;
  ChunkID  obj_id;
  uint32_t obj_offset;
  uint32_t obj_count;

  br.reset(data_);

  br.seek(8);
  br.readbe(obj_count);
  br.seek(16);
  for(uint32_t i = 0; i < obj_count; i++)
    {
      br.read((uint8_t*)&obj_id,4);
      br.readbe(obj_offset);

      bitmaps_.emplace_back();
      ::nfs_shpm_obj_to_bitmap(data_(obj_offset),bitmaps_.back());
    }
}
