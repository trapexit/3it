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

#include "bitmap.hpp"
#include "bits_and_bytes.hpp"
#include "bitstream.hpp"
#include "bpp.hpp"
#include "byte_reader.hpp"
#include "ccb_flags.hpp"
#include "cel_control_chunk.hpp"
#include "cel_packer.hpp"
#include "cel_types.hpp"
#include "chunk_ids.hpp"
#include "chunk_reader.hpp"
#include "identify_file.hpp"
#include "image_control_chunk.hpp"
#include "packed.hpp"
#include "pdat.hpp"
#include "pixel_converter.hpp"
#include "pixel_converter.hpp"
#include "pixel_writer.hpp"
#include "pixel_writer_coded_8bpp_amv.hpp"
#include "read_file.hpp"
#include "vecrw.hpp"
#include "video_image.hpp"

#include "fmt.hpp"

#include <cstddef>
#include <optional>

namespace fs = std::filesystem;

#define CODED    (1 << 7)
#define UNCODED  (0 << 7)
#define PACKED   (1 << 6)
#define UNPACKED (0 << 6)
#define LRFORM   (1 << 5)
#define LINEAR   (0 << 5)


static
u32
round_up(const u32 number_,
         const u32 multiple_)
{
  return (((number_ + multiple_ - 1) / multiple_) * multiple_);
}

static
void
resize_pdat(const size_t  w_,
            const size_t  h_,
            const size_t  bpp_,
            ByteVec      &pdat_)
{
  size_t size;

  size = ((round_up(w_ * bpp_,32) * h_) / 8);

  pdat_.resize(size);
}

static
int
coded_colors(int bpp_)
{
  switch(bpp_)
    {
    case 1:
      return 2;
    case 2:
      return 4;
    case 4:
      return 16;
    case 6:
    case 8:
    case 16:
    default:
      return 32;
    }
}

static
void
check_coded_colors(const Bitmap &bitmap_,
                   const int     bpp_)
{
  u32 colors;
  u32 max_colors;

  colors = bitmap_.color_count();
  max_colors = ::coded_colors(bpp_);
  if(colors > max_colors)
    throw fmt::exception("input image has {} colors, more than the {} coded colors ({}bpp) possible",
                         colors,
                         max_colors,
                         bpp_);
}

void
convert::banner_to_bitmap(cspan<u8>       data_,
                          std::vector<Bitmap> &bitmaps_)
{
  VideoImage vi;
  ByteReader br;

  br.reset(data_)
    .readbe(vi.vi_Version)
    .read(vi.vi_Pattern,sizeof(vi.vi_Pattern))
    .readbe(vi.vi_Size)
    .readbe(vi.vi_Height)
    .readbe(vi.vi_Width)
    .readbe(vi.vi_Depth)
    .readbe(vi.vi_Type)
    .readbe(vi.vi_Reserved1)
    .readbe(vi.vi_Reserved2)
    .readbe(vi.vi_Reserved3);

  switch(vi.vi_Depth)
    {
    case 16:
      {
        cPDAT pdat(data_,sizeof(vi));
        bitmaps_.emplace_back(vi.vi_Width,vi.vi_Height);
        convert::uncoded_unpacked_lrform_16bpp_to_bitmap(pdat,bitmaps_.back());
      }
      break;
    default:
      throw fmt::exception("unsupported bit depth: {}",vi.vi_Depth);
    }
}

static
void
to_bitmap(CelControlChunk &ccc_,
          cPDAT            pdat_,
          const PLUT      &plut_,
          Bitmap          &b_)
{
  b_.reset(ccc_.ccb_Width,ccc_.ccb_Height);
  switch(ccc_.type())
    {
    case UNCODED_UNPACKED_LINEAR_8BPP:
      if(ccc_.rep8())
        convert::uncoded_unpacked_linear_rep8_8bpp_to_bitmap(pdat_,b_);
      else
        convert::uncoded_unpacked_linear_8bpp_to_bitmap(pdat_,b_);
      break;
    case UNCODED_UNPACKED_LINEAR_16BPP:
      convert::uncoded_unpacked_linear_16bpp_to_bitmap(pdat_,b_);
      break;
    case UNCODED_UNPACKED_LRFORM_16BPP:
      if(ccc_.ccb_Height & 1)
        b_.reset(ccc_.ccb_Width,ccc_.ccb_Height+1);
      convert::uncoded_unpacked_lrform_16bpp_to_bitmap(pdat_,b_);
      break;
    case UNCODED_PACKED_LINEAR_8BPP:
      convert::uncoded_packed_linear_8bpp_to_bitmap(pdat_,b_);
      break;
    case UNCODED_PACKED_LINEAR_16BPP:
      convert::uncoded_packed_linear_16bpp_to_bitmap(pdat_,b_);
      break;
    case CODED_PACKED_LINEAR_1BPP:
      convert::coded_packed_linear_1bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
      break;
    case CODED_PACKED_LINEAR_2BPP:
      convert::coded_packed_linear_2bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
      break;
    case CODED_PACKED_LINEAR_4BPP:
    case CODED_PACKED_LRFORM_4BPP: // LRFORM flag meaningless
      convert::coded_packed_linear_4bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
      break;
    case CODED_PACKED_LINEAR_6BPP:
      convert::coded_packed_linear_6bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
      break;
    case CODED_PACKED_LINEAR_8BPP:
      convert::coded_packed_linear_8bpp_to_bitmap(pdat_,plut_,ccc_.pdv(),b_);
      break;
    case CODED_PACKED_LINEAR_16BPP:
      convert::coded_packed_linear_16bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
      break;

    case CODED_UNPACKED_LINEAR_1BPP:
      convert::coded_unpacked_linear_1bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
      break;
    case CODED_UNPACKED_LINEAR_2BPP:
      convert::coded_unpacked_linear_2bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
      break;
    case CODED_UNPACKED_LINEAR_4BPP:
      convert::coded_unpacked_linear_4bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
      break;
    case CODED_UNPACKED_LINEAR_6BPP:
      convert::coded_unpacked_linear_6bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
      break;
    case CODED_UNPACKED_LINEAR_8BPP:
      convert::coded_unpacked_linear_8bpp_to_bitmap(pdat_,plut_,ccc_.pdv(),b_);
      break;
    case CODED_UNPACKED_LINEAR_16BPP:
      convert::coded_unpacked_linear_16bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
      break;

    default:
      throw fmt::exception("not yet supported CEL format: "
                           "coded={}; packed={}; lrform={}; bpp={};",
                           ccc_.coded(),
                           ccc_.packed(),
                           ccc_.lrform(),
                           ccc_.bpp());
    }

  if(!ccc_.bgnd())
    b_.make_transparent(Bitmap::Color::BLACK);
  else if(ccc_.noblk())
    b_.replace_color(Bitmap::Color::BLACK,Bitmap::Color::NOTBLACK);
}

static
std::optional<PLUT>
get_cel_file_plut(cspan<u8> data_)
{
  PLUT plut;
  ChunkVec chunks;

  ChunkReader::chunkify(data_,chunks);
  for(auto const &chunk : chunks)
    {
      if(chunk.id() != CHUNK_PLUT)
        continue;

      plut = chunk;

      return plut;
    }

  return {};
}

void
convert::cel_to_bitmap(cspan<u8>       data_,
                       std::vector<Bitmap> &bitmaps_)
{
  ChunkVec chunks;
  CelControlChunk ccc;
  cPDAT pdat;
  PLUT plut;

  ChunkReader::chunkify(data_,chunks);

  for(const auto &chunk : chunks)
    {
      switch(chunk.id())
        {
        case CHUNK_CCB:
          {
            if(ccc && pdat)
              {
                bitmaps_.emplace_back();
                ::to_bitmap(ccc,pdat,plut,bitmaps_.back());
              }

            ccc  = chunk;
            pdat = cPDAT();
            plut = PLUT();
          }
          break;
        case CHUNK_PDAT:
          {
            u32 offset;

            if(ccc && pdat)
              {
                bitmaps_.emplace_back();
                ::to_bitmap(ccc,pdat,plut,bitmaps_.back());
              }

            offset = (ccc.ccbpre() ? 0 : (ccc.packed() ? 4 : 8));

            pdat = cPDAT(chunk.data(),chunk.data_size()-offset,offset);
          }
          break;
        case CHUNK_PLUT:
          plut = chunk;
          break;
        case CHUNK_ANIM:
          break;
        case CHUNK_XTRA:
          break;
        case CHUNK_CPYR:
          //fmt::print("Copyright: {}\n",(const char*)chunk.data());
          break;
        case CHUNK_DESC:
          //fmt::print("Description: {}\n",(const char*)chunk.data());
          break;
        case CHUNK_KWRD:
          //fmt::print("Keywords: {}\n",(const char*)chunk.data());
          break;
        case CHUNK_CRDT:
          //fmt::print("Credits: {}\n",(const char*)chunk.data());
          break;
        default:
          fmt::print("WARNING - unknown chunk ID: {}\n",chunk.idstr());
          break;
        }
    }

  if(ccc && pdat)
    {
      bitmaps_.emplace_back();
      ::to_bitmap(ccc,pdat,plut,bitmaps_.back());
    }
}

void
convert::anim_to_bitmap(cspan<u8>       data_,
                        std::vector<Bitmap> &bitmaps_)
{
  ChunkVec chunks;
  CelControlChunk ccc;
  PLUT plut;

  ChunkReader::chunkify(data_,chunks);

  for(auto &chunk : chunks)
    {
      switch(chunk.id())
        {
        case CHUNK_CCB:
          ccc = chunk;
          break;
        case CHUNK_PDAT:
          {
            cPDAT pdat(chunk.data(),chunk.data_size());
            bitmaps_.emplace_back();
            ::to_bitmap(ccc,pdat,plut,bitmaps_.back());
          }
          break;
        case CHUNK_PLUT:
          plut = chunk;
          break;
        case CHUNK_ANIM:
          break;
        case CHUNK_XTRA:
          break;
        case CHUNK_CPYR:
          //fmt::print("Copyright: {}\n",(const char*)chunk.data());
          break;
        case CHUNK_DESC:
          //fmt::print("Description: {}\n",(const char*)chunk.data());
          break;
        case CHUNK_KWRD:
          //fmt::print("Keywords: {}\n",(const char*)chunk.data());
          break;
        case CHUNK_CRDT:
          //fmt::print("Credits: {}\n",(const char*)chunk.data());
          break;
        default:
          //fmt::print("WARNING - unknown chunk ID: {}\n",chunk.idstr());
          break;
        }
    }
}

void
convert::lrform_to_bitmap(cspan<u8>  data_,
                          BitmapVec &bitmaps_)
{
  cPDAT pdat(data_.data(),data_.size());

  if(data_.size() == (320 * 240 * 2))
    bitmaps_.emplace_back(320,240);
  else if(data_.size() == (352 * 288 * 2))
    bitmaps_.emplace_back(352,288);
  else
    throw fmt::exception("data must be exactly 153600 or 202752 bytes");

 convert:uncoded_unpacked_lrform_16bpp_to_bitmap(pdat,bitmaps_.back());
}

void
convert::imag_to_bitmap(cspan<u8>  data_,
                        BitmapVec &bitmaps_)
{
  ImageControlChunk icc;
  ChunkVec chunks;

  ChunkReader::chunkify(data_,chunks);

  for(const auto &chunk : chunks)
    {
      switch(chunk.id())
        {
        case CHUNK_IMAG:
          icc = chunk;
          break;
        case CHUNK_PDAT:
          {
            cPDAT pdat(chunk.data(),chunk.data_size());

            if(icc.bitsperpixel  != 16)
              throw fmt::exception("bitsperpixel {} not yet supported",icc.bitsperpixel);
            if(icc.numcomponents != 3)
              throw fmt::exception("numcomponents {} not yet supported",icc.numcomponents);
            if(icc.numplanes     != 1)
              throw fmt::exception("numplanes {} not yet supported",icc.numplanes);
            if(icc.colorspace    != 0)
              throw fmt::exception("colorspace {} not yet supported",icc.colorspace);
            if(icc.comptype      != 0)
              throw fmt::exception("comptype {} not yet supported",icc.comptype);
            if(icc.hvformat      != 0)
              throw fmt::exception("hvformat {} not yet supported",icc.hvformat);
            if(icc.pixelorder    != 1)
              throw fmt::exception("pixelorder {} not yet supported",icc.pixelorder);

            bitmaps_.emplace_back(icc.w,icc.h);
            convert::uncoded_unpacked_lrform_16bpp_to_bitmap(pdat,bitmaps_.back());
          }
          break;
        case CHUNK_PLUT:
          break;
        case CHUNK_ANIM:
          break;
        case CHUNK_XTRA:
          break;
        case CHUNK_CPYR:
          //fmt::print("Copyright: {}\n",(const char*)chunk.data());
          break;
        case CHUNK_DESC:
          //fmt::print("Description: {}\n",(const char*)chunk.data());
          break;
        case CHUNK_KWRD:
          //fmt::print("Keywords: {}\n",(const char*)chunk.data());
          break;
        case CHUNK_CRDT:
          //fmt::print("Credits: {}\n",(const char*)chunk.data());
          break;
        default:
          //fmt::print("WARNING - unknown chunk ID: {}\n",chunk.idstr());
          break;
        }
    }
}

void
convert::to_bitmap(cspan<u8>  data_,
                   BitmapVec &bitmaps_)
{
  u32 type;

  type = IdentifyFile::identify(data_);
  switch(type)
    {
    case FILE_ID_3DO_CEL:
      convert::cel_to_bitmap(data_,bitmaps_);
      break;
    case FILE_ID_3DO_BANNER:
      convert::banner_to_bitmap(data_,bitmaps_);
      break;
    case FILE_ID_3DO_IMAGE:
      convert::imag_to_bitmap(data_,bitmaps_);
      break;
    case FILE_ID_3DO_ANIM:
      convert::anim_to_bitmap(data_,bitmaps_);
      break;
    case FILE_ID_3DO_LRFORM:
      convert::lrform_to_bitmap(data_,bitmaps_);
      break;
    case FILE_ID_BMP:
    case FILE_ID_PNG:
    case FILE_ID_JPG:
    case FILE_ID_GIF:
      bitmaps_.emplace_back();
      stbi_load(data_,bitmaps_.back());
      break;
    case FILE_ID_NFS_SHPM:
      convert::nfs_shpm_to_bitmap(data_,bitmaps_);
      break;
    case FILE_ID_NFS_WWWW:
      convert::nfs_wwww_to_bitmap(data_,bitmaps_);
      break;
    default:
      throw std::runtime_error("unknown image type");
    }

  unsigned i = 0;
  unsigned const width = (std::floor(std::log10(bitmaps_.size())) + 1);
  if(bitmaps_.size() > 1)
    {
      for(auto &bitmap : bitmaps_)
        bitmap.set("index",fmt::format("{:0{}}",i++,width));
    }
}

void
convert::to_bitmap(const fs::path &filepath_,
                   BitmapVec      &bitmaps_)
{
  ByteVec data;

  ReadFile::read(filepath_,data);
  if(data.empty())
    throw fmt::exception("file empty: {}",filepath_);

  convert::to_bitmap(data,bitmaps_);

  for(auto &bitmap : bitmaps_)
    {
      bitmap.set("filename",filepath_.filename().stem().string());
    }
}

void
convert::bitmap_to_uncoded_unpacked_linear_8bpp(const Bitmap &bitmap_,
                                                ByteVec      &pdat_)
{
  u8 rgb;
  const size_t w = bitmap_.w;
  const size_t h = bitmap_.h;
  VecRW pdat;

  // Approximate. VecRW will manage the size as needed.
  pdat_.reserve(w * h * sizeof(u8));
  pdat.reset(&pdat_);

  for(size_t y = 0; y < h; y++)
    {
      for(size_t x = 0; x < w; x++)
        {
          const RGBA8888 *p = bitmap_.xy(x,y);

          rgb = RGBA8888Converter::to_rgb332(p);


          pdat.u8(rgb);
        }

      pdat.skip_to_4byte_boundary();
    }
}

void
convert::bitmap_to_uncoded_unpacked_linear_16bpp(const Bitmap &bitmap_,
                                                 ByteVec      &pdat_)
{
  u16 rgb;
  int w = bitmap_.w;
  int h = bitmap_.h;
  VecRW pdat;

  // Approximate. VecRW will manage the size as needed.
  pdat_.reserve(w * h * sizeof(u16));
  pdat.reset(&pdat_);

  for(int y = 0; y < h; y++)
    {
      for(int x = 0; x < w; x++)
        {
          const RGBA8888 *p = bitmap_.xy(x,y);

          rgb = RGBA8888Converter::to_rgb0555(p);

          pdat.u16be(rgb);
        }

      pdat.skip_to_4byte_boundary();
    }
}

void
convert::bitmap_to_uncoded_unpacked_lrform_16bpp(const Bitmap &bitmap_,
                                                 ByteVec      &pdat_)
{
  int w;
  int h;
  VecRW pdat;
  u16 rgb;

  // LRForm height must be an even number of rows so truncate if odd.
  w = bitmap_.w;
  h = (bitmap_.h & ~0x1);

  // Approximate. VecRW will manage the size as needed.
  pdat_.reserve(w * h * sizeof(u16));
  pdat.reset(&pdat_);

  for(int y = 0; y < h; y += 2)
    {
      for(int x = 0; x < w; x++)
        {
          const RGBA8888 *lp = bitmap_.xy(x,y+0);
          rgb = RGBA8888Converter::to_rgb0555(lp);
          pdat.u16be(rgb);

          const RGBA8888 *rp = bitmap_.xy(x,y+1);
          rgb = RGBA8888Converter::to_rgb0555(rp);
          pdat.u16be(rgb);
        }

      pdat.skip_to_4byte_boundary();
    }
}

void
convert::bitmap_to_uncoded_packed_linear_8bpp(const Bitmap &bitmap_,
                                              ByteVec      &pdat_)
{
  RGBA8888Converter pc(BPP_8);

  CelPacker::pack(bitmap_,pc,pdat_);
}

void
convert::bitmap_to_uncoded_packed_linear_16bpp(const Bitmap &bitmap_,
                                               ByteVec      &pdat_)
{
  RGBA8888Converter pc(BPP_16);

  CelPacker::pack(bitmap_,pc,pdat_);
}

static
void
bitmap_to_coded_unpacked_linear_Xbpp(const Bitmap  &bitmap_,
                                     const u8  bpp_,
                                     ByteVec       &pdat_,
                                     PLUT          &plut_)
{
  BitStreamWriter bs;

  ::check_coded_colors(bitmap_,bpp_);

  resize_pdat(bitmap_.w,bitmap_.h,bpp_,pdat_);
  bs.reset(pdat_);

  if(bitmap_.has("external-palette"))
    {
      u32 filetype;
      ByteVec data;
      fs::path filepath;
      std::optional<PLUT> plut;

      filepath = bitmap_.get("external-palette");

      ReadFile::read(filepath,data);
      filetype = IdentifyFile::identify(data);
      if(!IdentifyFile::chunked_type(filetype))
        throw fmt::exception("'{}' does not appear to be a 3DO formated file",filepath);

      plut = ::get_cel_file_plut(data);
      if(plut)
        plut_ = plut.value();
      else
        throw fmt::exception("No CEL PLUT found in '{}'",filepath);
    }
  else
    {
      plut_.build(bitmap_);
    }

  for(size_t y = 0; y < bitmap_.h; y++)
    {
      u64 start;

      start = bs.tell();
      for(size_t x = 0; x < bitmap_.w; x++)
        {
          u16 color;
          const RGBA8888 *p = bitmap_.xy(x,y);

          color = RGBA8888Converter::to_rgb0555(p);
          color = plut_.lookup(color);

          bs.write(bpp_,color);
        }

      bs.skip_to_32bit_boundary();
      // Due to pipelining of the CEL engine the WOFFSET value is
      // words in a row minus 2. So we must pad a row if less than 8
      // bytes (2 words.)
      // https://3dodev.com/documentation/development/opera/pf25/ppgfldr/ggsfldr/gpgfldr/5gpge#the_woffset_value
      if((bs.tell() - start) < (2 * BITS_PER_WORD))
        bs.write(((2 * BITS_PER_WORD) - (bs.tell() - start)),0);
    }
}

void
convert::bitmap_to_coded_unpacked_linear_1bpp(const Bitmap &bitmap_,
                                              ByteVec      &pdat_,
                                              PLUT         &plut_)
{
  ::bitmap_to_coded_unpacked_linear_Xbpp(bitmap_,BPP_1,pdat_,plut_);
}

void
convert::bitmap_to_coded_unpacked_linear_2bpp(const Bitmap &bitmap_,
                                              ByteVec      &pdat_,
                                              PLUT         &plut_)
{
  ::bitmap_to_coded_unpacked_linear_Xbpp(bitmap_,BPP_2,pdat_,plut_);
}

void
convert::bitmap_to_coded_unpacked_linear_4bpp(const Bitmap &bitmap_,
                                              ByteVec      &pdat_,
                                              PLUT         &plut_)
{
  ::bitmap_to_coded_unpacked_linear_Xbpp(bitmap_,BPP_4,pdat_,plut_);
}

void
convert::bitmap_to_coded_unpacked_linear_6bpp(const Bitmap &bitmap_,
                                              ByteVec      &pdat_,
                                              PLUT         &plut_)
{
  ::bitmap_to_coded_unpacked_linear_Xbpp(bitmap_,BPP_6,pdat_,plut_);
}

void
convert::bitmap_to_coded_unpacked_linear_8bpp(const Bitmap &bitmap_,
                                              ByteVec      &pdat_,
                                              PLUT         &plut_)
{
  ::bitmap_to_coded_unpacked_linear_Xbpp(bitmap_,BPP_8,pdat_,plut_);
}

void
convert::bitmap_to_coded_unpacked_linear_16bpp(const Bitmap &bitmap_,
                                               ByteVec      &pdat_,
                                               PLUT         &plut_)
{
  ::bitmap_to_coded_unpacked_linear_Xbpp(bitmap_,BPP_16,pdat_,plut_);
}

static
void
bitmap_to_coded_packed_linear_Xbpp(const Bitmap &bitmap_,
                                   const int     bpp_,
                                   ByteVec      &pdat_,
                                   PLUT         &plut_)
{
  RGBA8888Converter pc(bpp_,plut_);

  ::check_coded_colors(bitmap_,bpp_);

  if(bitmap_.has("external-palette"))
    {
      u32 filetype;
      ByteVec data;
      fs::path filepath;
      std::optional<PLUT> plut;

      filepath = bitmap_.get("external-palette");

      ReadFile::read(filepath,data);
      filetype = IdentifyFile::identify(data);
      if(!IdentifyFile::chunked_type(filetype))
        throw fmt::exception("'{}' does not appear to be a 3DO formated file",filepath);

      plut = ::get_cel_file_plut(data);
      if(plut)
        plut_ = plut.value();
      else
        throw fmt::exception("No CEL PLUT found in '{}'",filepath);
    }
  else
    {
      plut_.build(bitmap_);
    }

  CelPacker::pack(bitmap_,pc,pdat_);
}

void
convert::bitmap_to_coded_packed_linear_1bpp(const Bitmap &bitmap_,
                                            ByteVec      &pdat_,
                                            PLUT         &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_1,pdat_,plut_);
}

void
convert::bitmap_to_coded_packed_linear_2bpp(const Bitmap &bitmap_,
                                            ByteVec      &pdat_,
                                            PLUT         &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_2,pdat_,plut_);
}

void
convert::bitmap_to_coded_packed_linear_4bpp(const Bitmap &bitmap_,
                                            ByteVec      &pdat_,
                                            PLUT         &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_4,pdat_,plut_);
}

void
convert::bitmap_to_coded_packed_linear_6bpp(const Bitmap &bitmap_,
                                            ByteVec      &pdat_,
                                            PLUT         &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_6,pdat_,plut_);
}

void
convert::bitmap_to_coded_packed_linear_8bpp(const Bitmap &bitmap_,
                                            ByteVec      &pdat_,
                                            PLUT         &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_8,pdat_,plut_);
}

void
convert::bitmap_to_coded_packed_linear_16bpp(const Bitmap &bitmap_,
                                             ByteVec      &pdat_,
                                             PLUT         &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_16,pdat_,plut_);
}

void
convert::uncoded_unpacked_lrform_16bpp_to_bitmap(cPDAT   pdat_,
                                                 Bitmap &bitmap_)
{
  size_t i;
  u8 hb,lb;
  PixelWriter pwl;
  PixelWriter pwr;

  i = 0;
  pwl.reset(bitmap_);
  pwr.reset(bitmap_);
  for(size_t y = 0; y < bitmap_.h;)
    {
      pwl.move_y(y++);
      pwr.move_y(y++);
      for(size_t x = 0; x < bitmap_.w; x++)
        {
          hb = pdat_[i++];
          lb = pdat_[i++];
          pwl.write_0555(hb,lb);

          hb = pdat_[i++];
          lb = pdat_[i++];
          pwr.write_0555(hb,lb);
        }
    }
}

void
convert::uncoded_unpacked_linear_8bpp_to_bitmap(cPDAT   pdat_,
                                                Bitmap &bitmap_)
{
  ByteReader br;
  PixelWriter pw;

  br.reset(pdat_);
  pw.reset(bitmap_);
  for(size_t y = 0; y < bitmap_.h; y++)
    {
      for(size_t x = 0; x < bitmap_.w; x++)
        {
          u32 p;

          p = br.u8();

          pw.write_332(p);
        }

      br.skip_to_4byte_boundary();
    }
}

void
convert::uncoded_unpacked_linear_rep8_8bpp_to_bitmap(cPDAT   pdat_,
                                                     Bitmap &bitmap_)
{
  ByteReader br;
  PixelWriter pw;

  br.reset(pdat_);
  pw.reset(bitmap_);
  for(size_t y = 0; y < bitmap_.h; y++)
    {
      for(size_t x = 0; x < bitmap_.w; x++)
        {
          u32 p;

          p = br.u8();

          pw.write_rep8_332(p);
        }

      br.skip_to_4byte_boundary();
    }
}

void
convert::uncoded_unpacked_linear_16bpp_to_bitmap(cPDAT   pdat_,
                                                 Bitmap &bitmap_)
{
  ByteReader br;
  PixelWriter pw;

  br.reset(pdat_);
  pw.reset(bitmap_);
  for(size_t y = 0; y < bitmap_.h; y++)
    {
      for(size_t x = 0; x < bitmap_.w; x++)
        {
          u32 p;

          p = br.u16be();

          pw.write_uncoded_16bpp(p);
        }

      br.skip_to_4byte_boundary();
    }
}

template<typename PW>
static
void
unpack_row(BitStreamReader &bs_,
           PW              &pw_,
           const u32        bpp_)
{
  u8 type;
  u32 pixel;
  u32 count;

  do
    {
      type = bs_.read(DATA_PACKET_DATA_TYPE_SIZE);
      switch(type)
        {
        case PACK_LITERAL:
          {
            count = bs_.read(DATA_PACKET_PIXEL_COUNT_SIZE) + 1;
            for(size_t i = 0; i < count; i++)
              {
                pixel = bs_.read(bpp_);
                pw_.write(pixel);
              }
          }
          break;
        case PACK_TRANSPARENT:
          {
            count = bs_.read(DATA_PACKET_PIXEL_COUNT_SIZE) + 1;
            pw_.write_transparent(count);
          }
          break;
        case PACK_PACKED:
          {
            count = bs_.read(DATA_PACKET_PIXEL_COUNT_SIZE) + 1;
            pixel = bs_.read(bpp_);
            for(u32 i = 0; i < count; i++)
              pw_.write(pixel);
          }
          break;
        case PACK_EOL:
          break;
        }
    } while((type != PACK_EOL) && !pw_.row_filled());
}

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
uncoded_packed_linear_Xbpp_to_bitmap(cPDAT        pdat_,
                                     Bitmap      &bitmap_,
                                     const u32    bpp_)
{
  PixelWriter pw;
  std::size_t offset;
  std::size_t offset_width;
  BitStreamReader bs(pdat_);

  offset = 0;
  offset_width = ::calc_offset_width(bpp_);
  pw.reset(bitmap_,bpp_);
  for(size_t y = 0; y < bitmap_.h; y++)
    {
      if((offset * BITS_PER_BYTE) >= bs.size())
        throw fmt::exception("attempted out of bound read - {} {}:{}",
                             __FILE__,__FUNCTION__,__LINE__);

      bs.seek(offset * BITS_PER_BYTE);
      pw.move_y(y);

      offset += ((bs.read(offset_width) + 2) * BYTES_PER_WORD);

      ::unpack_row(bs,pw,bpp_);
    }
}

void
convert::uncoded_packed_linear_16bpp_to_bitmap(cPDAT   pdat_,
                                               Bitmap &bitmap_)
{
  ::uncoded_packed_linear_Xbpp_to_bitmap(pdat_,bitmap_,BPP_16);
}

void
convert::uncoded_packed_linear_8bpp_to_bitmap(cPDAT   pdat_,
                                              Bitmap &bitmap_)
{
  ::uncoded_packed_linear_Xbpp_to_bitmap(pdat_,bitmap_,BPP_8);
}

static
void
coded_packed_linear_to_bitmap(const u32  bpp_,
                              cPDAT           pdat_,
                              const PLUT     &plut_,
                              const u8   pluta_,
                              Bitmap         &bitmap_)
{
  PixelWriter pw;
  std::size_t offset;
  std::size_t offset_width;
  BitStreamReader bs(pdat_);

  offset = 0;
  offset_width = calc_offset_width(bpp_);
  pw.reset(bitmap_,plut_,pluta_,bpp_);
  for(size_t y = 0; y < bitmap_.h; y++)
    {
      if((offset * BITS_PER_BYTE) >= bs.size())
        throw fmt::exception("attempted out of bound read - {} {}:{}",
                             __FILE__,__FUNCTION__,__LINE__);


      bs.seek(offset * BITS_PER_BYTE);
      pw.move_y(y);

      offset += ((bs.read(offset_width) + 2) * BYTES_PER_WORD);

      ::unpack_row(bs,pw,bpp_);
    }
}

void
convert::coded_packed_linear_1bpp_to_bitmap(cPDAT          pdat_,
                                            const PLUT    &plut_,
                                            const u8  pluta_,
                                            Bitmap        &bitmap_)
{
  ::coded_packed_linear_to_bitmap(1,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_packed_linear_2bpp_to_bitmap(cPDAT          pdat_,
                                            const PLUT    &plut_,
                                            const u8  pluta_,
                                            Bitmap        &bitmap_)
{
  ::coded_packed_linear_to_bitmap(2,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_packed_linear_4bpp_to_bitmap(cPDAT          pdat_,
                                            const PLUT    &plut_,
                                            const u8  pluta_,
                                            Bitmap        &bitmap_)
{
  ::coded_packed_linear_to_bitmap(4,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_packed_linear_6bpp_to_bitmap(cPDAT          pdat_,
                                            const PLUT    &plut_,
                                            const u8  pluta_,
                                            Bitmap        &bitmap_)
{
  ::coded_packed_linear_to_bitmap(6,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_packed_linear_8bpp_to_bitmap(cPDAT       pdat_,
                                            const PLUT &plut_,
                                            const u32   pdv_,
                                            Bitmap     &bitmap_)
{
  PixelWriterCoded8bppAMV pw;
  u32 offset;
  u32 offset_width;
  BitStreamReader bs(pdat_);
  const u32 bpp = 8;

  offset = 0;
  offset_width = ::calc_offset_width(8);
  pw.init(bitmap_,plut_,pdv_);
  for(u64 y = 0; y < bitmap_.h; y++)
    {
      if((offset * BITS_PER_BYTE) >= bs.size())
        throw fmt::exception("attempted out of bound read - {} {}:{}",
                             __FILE__,__FUNCTION__,__LINE__);


      bs.seek(offset * BITS_PER_BYTE);
      pw.move_y(y);

      offset += ((bs.read(offset_width) + 2) * BYTES_PER_WORD);

      ::unpack_row(bs,pw,bpp);
    }


  return;


  ::coded_packed_linear_to_bitmap(8,pdat_,plut_,0,bitmap_);
}

void
convert::coded_packed_linear_16bpp_to_bitmap(cPDAT          pdat_,
                                             const PLUT    &plut_,
                                             const u8  pluta_,
                                             Bitmap        &bitmap_)
{
  ::coded_packed_linear_to_bitmap(16,pdat_,plut_,pluta_,bitmap_);
}

static
void
coded_unpacked_linear_to_bitmap(const u32  bpp_,
                                cPDAT           pdat_,
                                const PLUT     &plut_,
                                const u8   pluta_,
                                Bitmap         &bitmap_)
{
  u32 p;
  PixelWriter pw;
  BitStreamReader bs(pdat_);

  pw.reset(bitmap_,plut_,pluta_,bpp_);
  for(size_t y = 0; y < bitmap_.h; y++)
    {
      for(size_t x = 0; x < bitmap_.w; x++)
        {
          p = bs.read(bpp_);

          pw.write(p);
        }

      bs.skip_to_32bit_boundary();
    }
}

void
convert::coded_unpacked_linear_1bpp_to_bitmap(cPDAT          pdat_,
                                              const PLUT    &plut_,
                                              const u8  pluta_,
                                              Bitmap        &bitmap_)
{
  ::coded_unpacked_linear_to_bitmap(1,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_unpacked_linear_2bpp_to_bitmap(cPDAT          pdat_,
                                              const PLUT    &plut_,
                                              const u8  pluta_,
                                              Bitmap        &bitmap_)
{
  ::coded_unpacked_linear_to_bitmap(2,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_unpacked_linear_4bpp_to_bitmap(cPDAT          pdat_,
                                              const PLUT    &plut_,
                                              const u8  pluta_,
                                              Bitmap        &bitmap_)
{
  ::coded_unpacked_linear_to_bitmap(4,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_unpacked_linear_6bpp_to_bitmap(cPDAT          pdat_,
                                              const PLUT    &plut_,
                                              const u8  pluta_,
                                              Bitmap        &bitmap_)
{
  ::coded_unpacked_linear_to_bitmap(6,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_unpacked_linear_8bpp_to_bitmap(cPDAT          pdat_,
                                              const PLUT    &plut_,
                                              const u32      pdv_,
                                              Bitmap        &bitmap_)
{
  u32 p;
  const u32 bpp = 8;
  BitStreamReader bs(pdat_);
  PixelWriterCoded8bppAMV pw;

  pw.init(bitmap_,plut_,pdv_);
  for(size_t y = 0; y < bitmap_.h; y++)
    {
      for(size_t x = 0; x < bitmap_.w; x++)
        {
          p = bs.read(bpp);

          pw.write(p);
        }

      bs.skip_to_32bit_boundary();
    }
}

void
convert::coded_unpacked_linear_16bpp_to_bitmap(cPDAT          pdat_,
                                               const PLUT    &plut_,
                                               const u8  pluta_,
                                               Bitmap        &bitmap_)
{
  ::coded_unpacked_linear_to_bitmap(16,pdat_,plut_,pluta_,bitmap_);
}

void
convert::bitmap_to_cel(const Bitmap  &bitmap_,
                       const CelType &celtype_,
                       ByteVec       &pdat_,
                       PLUT          &plut_)
{
  switch(celtype_.switchable)
    {
    case (UNCODED|UNPACKED|LRFORM|BPP_16):
      return convert::bitmap_to_uncoded_unpacked_lrform_16bpp(bitmap_,pdat_);

    case (UNCODED|UNPACKED|LINEAR|BPP_8):
      return convert::bitmap_to_uncoded_unpacked_linear_8bpp(bitmap_,pdat_);
    case (UNCODED|UNPACKED|LINEAR|BPP_16):
      return convert::bitmap_to_uncoded_unpacked_linear_16bpp(bitmap_,pdat_);

    case (UNCODED|PACKED|LINEAR|BPP_8):
      return convert::bitmap_to_uncoded_packed_linear_8bpp(bitmap_,pdat_);
    case (UNCODED|PACKED|LINEAR|BPP_16):
      return convert::bitmap_to_uncoded_packed_linear_16bpp(bitmap_,pdat_);

    case (CODED|UNPACKED|LINEAR|BPP_1):
      return convert::bitmap_to_coded_unpacked_linear_1bpp(bitmap_,pdat_,plut_);
    case (CODED|UNPACKED|LINEAR|BPP_2):
      return convert::bitmap_to_coded_unpacked_linear_2bpp(bitmap_,pdat_,plut_);
    case (CODED|UNPACKED|LINEAR|BPP_4):
      return convert::bitmap_to_coded_unpacked_linear_4bpp(bitmap_,pdat_,plut_);
    case (CODED|UNPACKED|LINEAR|BPP_6):
      return convert::bitmap_to_coded_unpacked_linear_6bpp(bitmap_,pdat_,plut_);
    case (CODED|UNPACKED|LINEAR|BPP_8):
      return convert::bitmap_to_coded_unpacked_linear_8bpp(bitmap_,pdat_,plut_);
    case (CODED|UNPACKED|LINEAR|BPP_16):
      return convert::bitmap_to_coded_unpacked_linear_16bpp(bitmap_,pdat_,plut_);

    case (CODED|PACKED|LINEAR|BPP_1):
      return convert::bitmap_to_coded_packed_linear_1bpp(bitmap_,pdat_,plut_);
    case (CODED|PACKED|LINEAR|BPP_2):
      return convert::bitmap_to_coded_packed_linear_2bpp(bitmap_,pdat_,plut_);
    case (CODED|PACKED|LINEAR|BPP_4):
      return convert::bitmap_to_coded_packed_linear_4bpp(bitmap_,pdat_,plut_);
    case (CODED|PACKED|LINEAR|BPP_6):
      return convert::bitmap_to_coded_packed_linear_6bpp(bitmap_,pdat_,plut_);
    case (CODED|PACKED|LINEAR|BPP_8):
      return convert::bitmap_to_coded_packed_linear_8bpp(bitmap_,pdat_,plut_);
    case (CODED|PACKED|LINEAR|BPP_16):
      return convert::bitmap_to_coded_packed_linear_16bpp(bitmap_,pdat_,plut_);

    default:
      throw fmt::exception("invalid combination of attributes: "
                           "coded={}, packed={}, linear={}, bpp={}",
                           (bool)celtype_.coded,
                           (bool)celtype_.packed,
                           !(bool)celtype_.lrform,
                           celtype_.bpp);
    }
}
