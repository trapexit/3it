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

#include "bitstream.hpp"
#include "byte_reader.hpp"
#include "ccb_flags.hpp"
#include "cel_control_chunk.hpp"
#include "cel_packer.hpp"
#include "cel_types.hpp"
#include "chunk_ids.hpp"
#include "chunk_reader.hpp"
#include "color_check.hpp"
#include "identify_file.hpp"
#include "image_control_chunk.hpp"
#include "pixel_converter.hpp"
#include "pixel_converter.hpp"
#include "pixel_writer.hpp"
#include "video_image.hpp"

#include "fmt.hpp"

static
uint32_t
round_up(const uint32_t number_,
         const uint32_t multiple_)
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
check_colors(const Bitmap &bitmap_,
             const int     bpp_)
{
  int colors;

  colors = ::coded_colors(bpp_);
  if(ColorCheck::has_more_than(bitmap_,colors))
    throw fmt::exception("image has more than {} colors",colors);
}

void
convert::banner_to_bitmap(cspan<uint8_t>       data_,
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
        bitmaps_.emplace_back(vi.vi_Width,vi.vi_Height,4);
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
  b_.reset(ccc_.ccb_Width,ccc_.ccb_Height,4);
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
      convert::coded_packed_linear_8bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
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
      convert::coded_unpacked_linear_8bpp_to_bitmap(pdat_,plut_,ccc_.pluta(),b_);
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
}

void
convert::cel_to_bitmap(cspan<uint8_t>       data_,
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
          if(ccc && pdat)
            {
              bitmaps_.emplace_back();
              ::to_bitmap(ccc,pdat,plut,bitmaps_.back());
            }

          ccc = chunk;
          pdat = cPDAT();
          plut = PLUT();
          break;
        case CHUNK_PDAT:
          if(ccc && pdat)
            {
              bitmaps_.emplace_back();
              ::to_bitmap(ccc,pdat,plut,bitmaps_.back());
            }

          pdat = cPDAT(chunk.data(),chunk.data_size());
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

  if(ccc && pdat)
    {
      bitmaps_.emplace_back();
      ::to_bitmap(ccc,pdat,plut,bitmaps_.back());
    }
}

void
convert::anim_to_bitmap(cspan<uint8_t>       data_,
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
convert::imag_to_bitmap(cspan<uint8_t>       data_,
                        std::vector<Bitmap> &bitmaps_)
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

            if(icc.bitsperpixel != 16)
              throw fmt::exception("bitsperpixel {} not yet supported",icc.bitsperpixel);
            if(icc.numcomponents != 3)
              throw fmt::exception("numcomponents {} not yet supported",icc.numcomponents);
            if(icc.numplanes != 1)
              throw fmt::exception("numplanes {} not yet supported",icc.numplanes);
            if(icc.colorspace != 0)
              throw fmt::exception("colorspace {} not yet supported",icc.colorspace);
            if(icc.comptype != 0)
              throw fmt::exception("comptype {} not yet supported",icc.comptype);
            if(icc.hvformat != 0)
              throw fmt::exception("hvformat {} not yet supported",icc.hvformat);
            if(icc.pixelorder != 1)
              throw fmt::exception("pixelorder {} not yet supported",icc.pixelorder);

            bitmaps_.emplace_back(icc.w,icc.h,4);
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
convert::to_bitmap(cspan<uint8_t>       data_,
                   std::vector<Bitmap> &bitmaps_)
{
  uint32_t type;

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
    case FILE_ID_BMP:
    case FILE_ID_PNG:
    case FILE_ID_JPG:
    case FILE_ID_GIF:
      bitmaps_.emplace_back();
      stbi_load(data_,bitmaps_.back());
      break;
    default:
      throw std::runtime_error("unknown file type");
    }
}

void
convert::bitmap_to_uncoded_unpacked_linear_8bpp(const Bitmap &bitmap_,
                                                ByteVec      &pdat_)
{
  uint8_t rgb;
  const size_t w = bitmap_.w;
  const size_t h = bitmap_.h;

  pdat_.reserve(w * h * sizeof(uint8_t));

  for(size_t y = 0; y < h; y++)
    {
      for(size_t x = 0; x < w; x++)
        {
          const uint8_t *p = bitmap_.xy(x,y);

          rgb = RGBA8888Converter::to_rgb332(p);

          pdat_.push_back(rgb);
        }
    }
}

void
convert::bitmap_to_uncoded_unpacked_linear_16bpp(const Bitmap &bitmap_,
                                                 ByteVec      &pdat_)
{
  const uint8_t *p;
  const uint8_t *pend;
  const int      w = bitmap_.w;
  const int      h = bitmap_.h;
  const int      n = bitmap_.n;
  const uint8_t *d = bitmap_.d.get();
  union { uint8_t u8[2]; uint16_t u16; } rgb;

  pdat_.reserve(w * h * sizeof(uint16_t));

  p    = d;
  pend = (d + (w * h * n));
  for(; p < pend; p += n)
    {
      rgb.u16 = RGBA8888Converter::to_rgb0555(p);
      rgb.u16 = byteswap_if_little_endian(rgb.u16);

      pdat_.push_back(rgb.u8[0]);
      pdat_.push_back(rgb.u8[1]);
    }
}

void
convert::bitmap_to_uncoded_unpacked_lrform_16bpp(const Bitmap &bitmap_,
                                                 ByteVec      &pdat_)
{
  const int w = bitmap_.w;
  const int h = bitmap_.h;
  union { uint8_t u8[2]; uint16_t u16; } rgb;

  pdat_.reserve(w * h * sizeof(uint16_t));

  for(int y = 0; y < h; y += 2)
    {
      for(int x = 0; x < w; x++)
        {
          const uint8_t *lp = bitmap_.xy(x,y+0);
          rgb.u16 = RGBA8888Converter::to_rgb0555(lp);
          rgb.u16 = byteswap_if_little_endian(rgb.u16);
          pdat_.push_back(rgb.u8[0]);
          pdat_.push_back(rgb.u8[1]);

          const uint8_t *rp = bitmap_.xy(x,y+1);
          rgb.u16 = RGBA8888Converter::to_rgb0555(rp);
          rgb.u16 = byteswap_if_little_endian(rgb.u16);
          pdat_.push_back(rgb.u8[0]);
          pdat_.push_back(rgb.u8[1]);
        }
    }
}

void
convert::bitmap_to_uncoded_packed_linear_8bpp(const Bitmap   &bitmap_,
                                              const uint32_t  transparent_color_,
                                              ByteVec        &pdat_)
{
  RGBA8888Converter pc(BPP_8);

  CelPacker::pack(bitmap_,pc,transparent_color_,pdat_);
}

void
convert::bitmap_to_uncoded_packed_linear_16bpp(const Bitmap   &bitmap_,
                                               const uint32_t  transparent_color_,
                                               ByteVec        &pdat_)
{
  RGBA8888Converter pc(BPP_16);

  CelPacker::pack(bitmap_,pc,transparent_color_,pdat_);
}

static
void
bitmap_to_coded_unpacked_linear_Xbpp(const Bitmap  &bitmap_,
                                     const uint8_t  bpp_,
                                     ByteVec       &pdat_,
                                     PLUT          &plut_)
{
  uint16_t color;
  BitStreamWriter bs;

  ::check_colors(bitmap_,bpp_);

  resize_pdat(bitmap_.w,bitmap_.h,bpp_,pdat_);
  bs.reset(pdat_);

  plut_.build(bitmap_);
  for(size_t y = 0; y < bitmap_.h; y++)
    {
      for(size_t x = 0; x < bitmap_.w; x++)
        {
          const uint8_t *p = bitmap_.xy(x,y);

          color = RGBA8888Converter::to_rgb0555(p);
          color = plut_.lookup(color);

          bs.write(bpp_,color);
        }

      bs.skip_to_32bit_boundary();
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
bitmap_to_coded_packed_linear_Xbpp(const Bitmap   &bitmap_,
                                   const int       bpp_,
                                   const uint32_t  transparent_color_,
                                   ByteVec        &pdat_,
                                   PLUT           &plut_)
{
  RGBA8888Converter pc(bpp_,plut_);

  ::check_colors(bitmap_,bpp_);

  plut_.build(bitmap_);

  CelPacker::pack(bitmap_,pc,transparent_color_,pdat_);
}

void
convert::bitmap_to_coded_packed_linear_1bpp(const Bitmap   &bitmap_,
                                            const uint32_t  transparent_color_,
                                            ByteVec        &pdat_,
                                            PLUT           &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_1,transparent_color_,pdat_,plut_);
}

void
convert::bitmap_to_coded_packed_linear_2bpp(const Bitmap   &bitmap_,
                                            const uint32_t  transparent_color_,
                                            ByteVec        &pdat_,
                                            PLUT           &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_2,transparent_color_,pdat_,plut_);
}

void
convert::bitmap_to_coded_packed_linear_4bpp(const Bitmap   &bitmap_,
                                            const uint32_t  transparent_color_,
                                            ByteVec        &pdat_,
                                            PLUT           &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_4,transparent_color_,pdat_,plut_);
}

void
convert::bitmap_to_coded_packed_linear_6bpp(const Bitmap   &bitmap_,
                                            const uint32_t  transparent_color_,
                                            ByteVec        &pdat_,
                                            PLUT           &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_6,transparent_color_,pdat_,plut_);
}

void
convert::bitmap_to_coded_packed_linear_8bpp(const Bitmap   &bitmap_,
                                            const uint32_t  transparent_color_,
                                            ByteVec        &pdat_,
                                            PLUT           &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_8,transparent_color_,pdat_,plut_);
}

void
convert::bitmap_to_coded_packed_linear_16bpp(const Bitmap   &bitmap_,
                                             const uint32_t  transparent_color_,
                                             ByteVec        &pdat_,
                                             PLUT           &plut_)
{
  ::bitmap_to_coded_packed_linear_Xbpp(bitmap_,BPP_16,transparent_color_,pdat_,plut_);
}

void
convert::uncoded_unpacked_lrform_16bpp_to_bitmap(cPDAT   pdat_,
                                                 Bitmap &bitmap_)
{
  size_t i;
  uint8_t hb,lb;
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
          uint32_t p;

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
          uint32_t p;

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
          uint32_t p;

          p = br.u16be();

          pw.write_uncoded_16bpp(p);
        }

      br.skip_to_4byte_boundary();
    }
}

static
void
unpack_row(BitStreamReader &bs_,
           PixelWriter     &pw_)
{
  uint8_t type;
  uint32_t pixel;
  uint32_t count;
  uint32_t bpp;

  bpp = pw_.bpp();
  do
    {
      type = bs_.read(2);
      switch(type)
        {
        case PACK_LITERAL:
          {
            count = bs_.read(6) + 1;
            for(size_t i = 0; i < count; i++)
              {
                pixel = bs_.read(bpp);
                pw_.write(pixel);
              }
          }
          break;
        case PACK_TRANSPARENT:
          {
            count = bs_.read(6) + 1;
            pw_.write(0,count);
          }
          break;
        case PACK_PACKED:
          {
            count = bs_.read(6) + 1;
            pixel = bs_.read(bpp);
            pw_.write(pixel,count);
          }
          break;
        case PACK_EOL:
          break;
        }
    } while((type != PACK_EOL) && !pw_.row_filled());
}

void
convert::uncoded_packed_linear_16bpp_to_bitmap(cPDAT   pdat_,
                                               Bitmap &bitmap_)
{
  size_t offset;
  PixelWriter pw;
  BitStreamReader bs(pdat_);

  offset = 0;
  pw.reset(bitmap_,16);
  for(size_t y = 0; y < bitmap_.h; y++)
    {
      bs.seek(offset<<3);
      pw.move_y(y);

      offset += ((bs.read(16) + 2) * 4);

      ::unpack_row(bs,pw);
    }
}

void
convert::uncoded_packed_linear_8bpp_to_bitmap(cPDAT   pdat_,
                                              Bitmap &bitmap_)
{
  size_t offset;
  PixelWriter pw;
  BitStreamReader bs(pdat_);

  offset = 0;
  pw.reset(bitmap_,8);
  for(size_t y = 0; y < bitmap_.h; y++)
    {
      bs.seek(offset<<3);
      pw.move_y(y);

      offset += ((bs.read(16) + 2) * 4);

      ::unpack_row(bs,pw);
    }
}

static
uint32_t
calc_offset_width(const uint32_t bpp_)
{
  switch(bpp_)
    {
    case 1:
    case 2:
    case 4:
    case 6:
      return 8;
    case 8:
    case 16:
    default:
      return 16;
    }
}

static
void
coded_packed_linear_to_bitmap(const uint32_t  bpp_,
                              cPDAT           pdat_,
                              const PLUT     &plut_,
                              const uint8_t   pluta_,
                              Bitmap         &bitmap_)
{
  size_t offset;
  uint32_t offset_width;
  PixelWriter pw;
  BitStreamReader bs(pdat_);

  offset_width = calc_offset_width(bpp_);

  offset = 0;
  pw.reset(bitmap_,plut_,pluta_,bpp_);
  for(size_t y = 0; y < bitmap_.h; y++)
    {
      bs.seek(offset<<3);
      pw.move_y(y);

      offset += ((bs.read(offset_width) + 2) * 4);

      ::unpack_row(bs,pw);
    }
}

void
convert::coded_packed_linear_1bpp_to_bitmap(cPDAT          pdat_,
                                            const PLUT    &plut_,
                                            const uint8_t  pluta_,
                                            Bitmap        &bitmap_)
{
  ::coded_packed_linear_to_bitmap(1,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_packed_linear_2bpp_to_bitmap(cPDAT          pdat_,
                                            const PLUT    &plut_,
                                            const uint8_t  pluta_,
                                            Bitmap        &bitmap_)
{
  ::coded_packed_linear_to_bitmap(2,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_packed_linear_4bpp_to_bitmap(cPDAT          pdat_,
                                            const PLUT    &plut_,
                                            const uint8_t  pluta_,
                                            Bitmap        &bitmap_)
{
  ::coded_packed_linear_to_bitmap(4,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_packed_linear_6bpp_to_bitmap(cPDAT          pdat_,
                                            const PLUT    &plut_,
                                            const uint8_t  pluta_,
                                            Bitmap        &bitmap_)
{
  ::coded_packed_linear_to_bitmap(6,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_packed_linear_8bpp_to_bitmap(cPDAT          pdat_,
                                            const PLUT    &plut_,
                                            const uint8_t  pluta_,
                                            Bitmap        &bitmap_)
{
  ::coded_packed_linear_to_bitmap(8,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_packed_linear_16bpp_to_bitmap(cPDAT          pdat_,
                                             const PLUT    &plut_,
                                             const uint8_t  pluta_,
                                             Bitmap        &bitmap_)
{
  ::coded_packed_linear_to_bitmap(16,pdat_,plut_,pluta_,bitmap_);
}

static
void
coded_unpacked_linear_to_bitmap(const uint32_t  bpp_,
                                cPDAT           pdat_,
                                const PLUT     &plut_,
                                const uint8_t   pluta_,
                                Bitmap         &bitmap_)
{
  uint32_t p;
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
                                              const uint8_t  pluta_,
                                              Bitmap        &bitmap_)
{
  ::coded_unpacked_linear_to_bitmap(1,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_unpacked_linear_2bpp_to_bitmap(cPDAT          pdat_,
                                              const PLUT    &plut_,
                                              const uint8_t  pluta_,
                                              Bitmap        &bitmap_)
{
  ::coded_unpacked_linear_to_bitmap(2,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_unpacked_linear_4bpp_to_bitmap(cPDAT          pdat_,
                                              const PLUT    &plut_,
                                              const uint8_t  pluta_,
                                              Bitmap        &bitmap_)
{
  ::coded_unpacked_linear_to_bitmap(4,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_unpacked_linear_6bpp_to_bitmap(cPDAT          pdat_,
                                              const PLUT    &plut_,
                                              const uint8_t  pluta_,
                                              Bitmap        &bitmap_)
{
  ::coded_unpacked_linear_to_bitmap(6,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_unpacked_linear_8bpp_to_bitmap(cPDAT          pdat_,
                                              const PLUT    &plut_,
                                              const uint8_t  pluta_,
                                              Bitmap        &bitmap_)
{
  ::coded_unpacked_linear_to_bitmap(8,pdat_,plut_,pluta_,bitmap_);
}

void
convert::coded_unpacked_linear_16bpp_to_bitmap(cPDAT          pdat_,
                                               const PLUT    &plut_,
                                               const uint8_t  pluta_,
                                               Bitmap        &bitmap_)
{
  ::coded_unpacked_linear_to_bitmap(16,pdat_,plut_,pluta_,bitmap_);
}

void
convert::bitmap_to_cel(const Bitmap   &bitmap_,
                       const CelType  &celtype_,
                       const uint32_t  transparent_color_,
                       ByteVec        &pdat_,
                       PLUT           &plut_)
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
      return convert::bitmap_to_uncoded_packed_linear_8bpp(bitmap_,transparent_color_,pdat_);
    case (UNCODED|PACKED|LINEAR|BPP_16):
      return convert::bitmap_to_uncoded_packed_linear_16bpp(bitmap_,transparent_color_,pdat_);

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
      return convert::bitmap_to_coded_packed_linear_1bpp(bitmap_,transparent_color_,pdat_,plut_);
    case (CODED|PACKED|LINEAR|BPP_2):
      return convert::bitmap_to_coded_packed_linear_2bpp(bitmap_,transparent_color_,pdat_,plut_);
    case (CODED|PACKED|LINEAR|BPP_4):
      return convert::bitmap_to_coded_packed_linear_4bpp(bitmap_,transparent_color_,pdat_,plut_);
    case (CODED|PACKED|LINEAR|BPP_6):
      return convert::bitmap_to_coded_packed_linear_6bpp(bitmap_,transparent_color_,pdat_,plut_);
    case (CODED|PACKED|LINEAR|BPP_8):
      return convert::bitmap_to_coded_packed_linear_8bpp(bitmap_,transparent_color_,pdat_,plut_);
    case (CODED|PACKED|LINEAR|BPP_16):
      return convert::bitmap_to_coded_packed_linear_16bpp(bitmap_,transparent_color_,pdat_,plut_);
    }
}
