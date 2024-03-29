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
  _metadata.try_emplace(key_,val_);
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
