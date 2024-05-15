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

#include "bitmap.hpp"

#include <unordered_set>


bool
Bitmap::has(const std::string &key_) const
{
  return (_metadata.count(key_) > 0);
}

void
Bitmap::set(const std::string &key_,
            const std::string &val_)
{
  _metadata[key_] = val_;
}

std::string
Bitmap::get(const std::string &key_,
            const std::string &default_) const
{
  auto iter = _metadata.find(key_);

  if(iter != _metadata.end())
    return iter->second;

  return default_;
}

std::string
Bitmap::name(const std::string &default_) const
{
  return get("name",default_);
}

static
std::string
calculate_name(const std::string &filename_)
{
  std::string name;

  name = filename_;
  while(name.size() > 5)
    {
      if(*(name.rbegin() + 4) == '_')
        return name.substr(name.size() - 4);
      name.pop_back();
    }

  name.pop_back();
  return name;
}

std::string
Bitmap::name_or_guess() const
{
  auto iter = _metadata.find("name");

  if(iter != _metadata.end())
    return iter->second;

  return ::calculate_name(get("filename","????"));
}

uint32_t
Bitmap::color_count() const
{
  std::unordered_set<uint32_t> colors;

  for(size_t y = 0; y < h; y++)
    {
      for(size_t x = 0; x < w; x++)
        {
          const RGBA8888 *c = xy(x,y);

          if(c->a == 0)
            continue;

          colors.emplace(*(uint32_t*)c);
        }
    }

  return colors.size();
}

void
Bitmap::replace_color(uint32_t const src_,
                      uint32_t const dst_)
{
  RGBA8888 src(src_);
  RGBA8888 dst(dst_);

  for(size_t y = 0; y < h; y++)
    {
      for(size_t x = 0; x < w; x++)
        {
          RGBA8888 *c = xy(x,y);

          if(*c == src)
            *c = dst;
        }
    }
}

static
void
set_rotation_90_metadata(Bitmap &b_)
{
  std::string rotation;
  rotation = b_.get("rotation","0");

  if(rotation == "0")
    rotation = "90";
  else if(rotation == "90")
    rotation = "180";
  else if(rotation == "180")
    rotation = "270";
  else
    rotation = "0";

  b_.set("rotation",rotation);
}

void
Bitmap::rotate_90()
{
  Bitmap n;

  n.reset(h,w);

  for(size_t x = 0; x < w; x++)
    for(size_t y = 0; y < h; y++)
      *n.xy((h-y-1),x) = *xy(x,y);

  w = n.w;
  h = n.h;
  d = n.d;

  set_rotation_90_metadata(*this);
}

void
Bitmap::rotate_180()
{
  Bitmap n;

  n.reset(w,h);

  for(size_t x = 0; x < w; x++)
    for(size_t y = 0; y < h; y++)
      *n.xy((w-x-1),(h-y-1)) = *xy(x,y);

  w = n.w;
  h = n.h;
  d = n.d;

  set_rotation_90_metadata(*this);
  set_rotation_90_metadata(*this);
}

void
Bitmap::rotate_270()
{
  Bitmap n;

  n.reset(h,w);

  for(size_t x = 0; x < w; x++)
    for(size_t y = 0; y < h; y++)
      *n.xy(y,(w-x-1)) = *xy(x,y);

  w = n.w;
  h = n.h;
  d = n.d;

  set_rotation_90_metadata(*this);
  set_rotation_90_metadata(*this);
  set_rotation_90_metadata(*this);
}

void
Bitmap::rotate_to(unsigned rotation_)
{
  int cur_rotation;
  std::string cur_rotation_str;

  cur_rotation_str = get("rotation","0");

  sscanf(cur_rotation_str.c_str(),"%d",&cur_rotation);

  rotation_ &= 0xFFFF;
  switch((cur_rotation << 16) | rotation_)
    {
    default:
    case ((  0 << 16) |   0):
    case (( 90 << 16) |  90):
    case ((180 << 16) | 180):
    case ((270 << 16) | 270):
      break;

    case ((  0 << 16) |  90):
    case (( 90 << 16) | 180):
    case ((180 << 16) | 270):
    case ((270 << 16) |   0):
      rotate_90();
      break;

    case ((  0 << 16) | 180):
    case (( 90 << 16) | 270):
    case ((180 << 16) |   0):
    case ((270 << 16) |  90):
      rotate_180();
      break;

    case ((  0 << 16) | 270):
    case (( 90 << 16) |   0):
    case ((180 << 16) |  90):
    case ((270 << 16) | 180):
      rotate_270();
      break;
    }
}
