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

#include "pdat.hpp"

#include <cstdint>
#include <cstring>

static
void*
stbi_malloc(uint32_t sz_)
{
  return new char[sz_];
}

static
void*
stbi_realloc(void     *p_,
             uint32_t  oldsz_,
             uint32_t  newsz_)
{
  void *np;

  if(newsz_ <= oldsz_)
    return p_;

  np = new char[newsz_];

  memcpy(np,p_,oldsz_);

  delete[] (char*)p_;

  return np;
}

static
void
stbi_delete(void *p_)
{
  delete[] (char*)p_;
}

#define STBI_MALLOC(sz) (stbi_malloc(sz))
#define STBI_REALLOC_SIZED(p,oldsz,newsz) (stbi_realloc(p,oldsz,newsz))
#define STBI_FREE(p) (stbi_delete(p))

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stbi.hpp"
#include "stb_image_write.h"


namespace fs = std::filesystem;


void
stbi_load(const fs::path &filepath_,
          Bitmap         &b_)
{
  int w,h,n;
  uint8_t *d;

  d = stbi_load(filepath_.string().c_str(),&w,&h,&n,4);
  if(!d)
    return;

  b_.d.reset(d);
  b_.w = (size_t)w;
  b_.h = (size_t)h;
}

void
stbi_load(cPDAT   data_,
          Bitmap &b_)
{
  int w,h,n;
  uint8_t *d;

  d = stbi_load_from_memory(data_.data(),data_.size(),&w,&h,&n,4);
  if(!d)
    return;

  b_.d.reset(d);
  b_.w = (size_t)w;
  b_.h = (size_t)h;
}

int
stbi_write(const Bitmap      &b_,
           const fs::path    &filepath_,
           const std::string  format_)
{
  std::string filepath;

  filepath = filepath_.string();

  if(format_ == "bmp")
    return stbi_write_bmp(filepath.c_str(),b_.w,b_.h,4,b_.d.get());
  if(format_ == "png")
    return stbi_write_png(filepath.c_str(),b_.w,b_.h,4,b_.d.get(),(b_.w * 4));
  if(format_ == "jpg")
    return stbi_write_jpg(filepath.c_str(),b_.w,b_.h,4,b_.d.get(),100);

  throw std::runtime_error("unknown stb_image format");
}

uint32_t
stbi_identify(cPDAT data_)
{
  int x,y,comp;
  stbi__context s;
  const uint8_t *buf     = data_.data();
  const size_t   buf_len = data_.size();

  stbi__start_mem(&s,buf,buf_len);
#ifndef STBI_NO_JPEG
  if(stbi__jpeg_info(&s,&x,&y,&comp))
    return STBI_FILE_TYPE_JPG;
#endif

#ifndef STBI_NO_PNG
  if(stbi__png_info(&s,&x,&y,&comp))
    return STBI_FILE_TYPE_PNG;
#endif

#ifndef STBI_NO_GIF
  if(stbi__gif_info(&s,&x,&y,&comp))
    return STBI_FILE_TYPE_GIF;
#endif

#ifndef STBI_NO_BMP
  if(stbi__bmp_info(&s,&x,&y,&comp))
    return STBI_FILE_TYPE_BMP;
#endif

#ifndef STBI_NO_PSD
  if(stbi__psd_info(&s,&x,&y,&comp))
    return STBI_FILE_TYPE_PSD;
#endif

#ifndef STBI_NO_PIC
  if(stbi__pic_info(&s,&x,&y,&comp))
    return STBI_FILE_TYPE_PIC;
#endif

#ifndef STBI_NO_PNM
  if(stbi__pnm_info(&s,&x,&y,&comp))
    return STBI_FILE_TYPE_PNM;
#endif

#ifndef STBI_NO_HDR
  if(stbi__hdr_info(&s,&x,&y,&comp))
    return STBI_FILE_TYPE_HDR;
#endif

#ifndef STBI_NO_TGA
  if (stbi__tga_info(&s,&x,&y,&comp))
    return STBI_FILE_TYPE_TGA;
#endif

  return STBI_FILE_TYPE_UNKNOWN;
}
