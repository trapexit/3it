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
#include "char4literal.hpp"
#include "chunkid.hpp"

#include "fmt.hpp"

#include <vector>

#include <cctype>

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
  0x01   | 3              | plut offset (signed 24bit int)
  0x04   | 2              | width in pixels
  0x06   | 2              | height in pixels
  0x08   | 4              | 0x00000000 ???
  0x0C   | 4              | 0x00000000 ???
  0x10   | variable       | pixel data in 3DO CEL file format
*/

#define BPP_4    0x03
#define BPP_6    0x04
#define BPP_8    0x05
#define BPP_16   0x06
#define PACKED   0x80
#define UNPACKED 0x00
#define CODED    0x00
#define UNCODED  0x10

namespace l
{
  static
  std::string
  strip(const char *s_)
  {
    std::string s;

    for(; *s_; s_++)
      {
        if(std::isspace(*s_))
          continue;
        s += *s_;
      }

    return s;
  }

  static
  void
  nfs_shpm_obj_to_bitmap(cspan<uint8_t>  data_,
                         size_t          obj_offset_,
                         Bitmap         &bitmap_)
  {
    uint32_t w;
    uint32_t h;
    uint8_t  type;
    ByteReader br;
    PLUT plut;
    int32_t plut_offset;

    br.reset(data_);
    br.seek(obj_offset_);

    type = br.u8();
    plut_offset = br.i24be();
    w = br.u16be();
    h = br.u16be();
    br.skip(8);

    if(!(type & UNCODED))
      {
        size_t cur_off;

        cur_off = br.tell();
        br.seek(cur_off + plut_offset);
        for(size_t i = 0; i < plut.size(); i++)
          plut[i] = br.u16be();
        br.seek(cur_off);
      }

    // FIXME: Not sure what PDV should be / where it would come from.
    bitmap_.reset(w,h);
    switch(type)
      {
      case (CODED|PACKED|BPP_4):
        convert::coded_packed_linear_4bpp_to_bitmap(br,plut,0,bitmap_);
        break;
      case (CODED|PACKED|BPP_6):
        convert::coded_packed_linear_6bpp_to_bitmap(br,plut,0,bitmap_);
        break;
      case (CODED|UNPACKED|BPP_6):
        convert::coded_unpacked_linear_6bpp_to_bitmap(br,plut,0,bitmap_);
        break;
      case (CODED|PACKED|BPP_8):
        {
          const int pdv = 1;
          convert::coded_packed_linear_8bpp_to_bitmap(br,plut,pdv,bitmap_);
        }
        break;
      case (CODED|UNPACKED|BPP_8):
        {
          const int pdv = 1;
          convert::coded_unpacked_linear_8bpp_to_bitmap(br,plut,pdv,bitmap_);
        }
        break;
      case (PACKED|UNCODED|BPP_16):
        convert::uncoded_packed_linear_16bpp_to_bitmap(br,bitmap_);
        break;
      case (UNPACKED|UNCODED|BPP_16):
        convert::uncoded_unpacked_linear_16bpp_to_bitmap(br,bitmap_);
        break;
      default:
        fmt::print(" * WARNING - unknown NFS SHPM type: 0x{:02x}; offset: {}\n",
                   type,
                   data_.off() + obj_offset_);
      }
  }
}

void
convert::nfs_shpm_to_bitmap(cspan<uint8_t>  data_,
                            BitmapVec      &bitmaps_)
{
  ByteReader br;
  char obj_id[5];
  uint32_t obj_count;
  uint32_t obj_offset;

  br.reset(data_);

  br.seek(8);
  br.readbe(obj_count);
  br.seek(16);
  for(uint32_t i = 0; i < obj_count; i++)
    {
      Bitmap bitmap;

      br.read(obj_id,4);
      obj_id[4] = 0;
      br.readbe(obj_offset);

      switch(CHAR4LITERAL(obj_id[0],obj_id[1],obj_id[2],obj_id[3]))
        {
        case CHAR4LITERAL('p','l','t','0'):
        case CHAR4LITERAL('p','l','t','1'):
        case CHAR4LITERAL('p','l','t','2'):
        case CHAR4LITERAL('p','l','t','3'):
        case CHAR4LITERAL('!','o','r','i'):
          continue;
        }

      l::nfs_shpm_obj_to_bitmap(data_,obj_offset,bitmap);
      if(bitmap)
        {
          bitmap.set("name",l::strip(obj_id));
          bitmaps_.emplace_back(bitmap);
        }
    }
}
