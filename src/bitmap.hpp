#pragma once

#include "rgba8888.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <string>
#include <map>
#include <memory>

#include "types_ints.h"

#include <stdint.h>

struct Bitmap
{
public:
  enum Color
    {
      TRANSPARENT = 0x00000000,
      BLACK       = 0x000000FF,
      NOTBLACK    = 0x010000FF
    };

public:
  Bitmap()
    : w(0),
      h(0),
      d()
  {
    set("rotation","0");
  }

  Bitmap(const uint64_t w_,
         const uint64_t h_)
  {
    reset(w_,h_);
    set("rotation","0");
  }

  void
  reset(const uint64_t w_,
        const uint64_t h_)
  {
    w = w_;
    h = h_;
    d = std::make_unique<uint8_t[]>(w * h * sizeof(RGBA8888));
    set("rotation","0");
  }

  RGBA8888&
  idx(const u64 idx_)
  {
    return ((RGBA8888*)d.get())[idx_];
  }

  RGBA8888*
  xy(const uint64_t x_,
     const uint64_t y_)
  {
    RGBA8888 *p;
    uint64_t offset;

    p = (RGBA8888*)d.get();
    offset = ((w * y_) + x_);

    return &p[offset];
  }

  const
  RGBA8888*
  xy(const uint64_t x_,
     const uint64_t y_) const
  {
    RGBA8888 *p;
    uint64_t offset;

    p = (RGBA8888*)d.get();
    offset = ((w * y_) + x_);

    return &p[offset];
  }

  RGBA8888*
  y(const uint64_t y_)
  {
    return xy(0,y_);
  }

  const
  RGBA8888*
  y(uint64_t y_) const
  {
    return xy(0,y_);
  }

  operator bool() const
  {
    return (bool)d;
  }

  void
  reset()
  {
    d.reset();
    _metadata.clear();
    set("rotation","0");
  }

  bool
  has_transparent() const
  {
    for(uint64_t y = 0; y < h; y++)
      {
        for(uint64_t x = 0; x < w; x++)
          {
            const RGBA8888 *c = xy(x,y);

            if(c->a == 0)
              return true;
          }
      }

    return false;
  }

  bool
  has_black() const
  {
    for(uint64_t y = 0; y < h; y++)
      {
        for(uint64_t x = 0; x < w; x++)
          {
            const RGBA8888 *c = xy(x,y);

            if((c->r == 0) &&
               (c->g == 0) &&
               (c->b == 0) &&
               (c->a > 0))
              return true;
          }
      }

    return false;
  }

public:
  uint64_t w;
  uint64_t h;
  std::shared_ptr<uint8_t[]> d;

private:
  std::map<std::string,std::string> _metadata;

public:
  void set(const std::string &key,
           const std::string &val);

public:
  bool has(const std::string &key) const;
  std::string get(const std::string &key,
                  const std::string &default_ = {}) const;

  std::string name(const std::string &default_ = {}) const;
  std::string name_or_guess() const;

public:
  uint32_t color_count() const;
  void replace_color(uint32_t const src,
                     uint32_t const dst);
  void make_transparent(const uint32_t color);

  void rotate_to(unsigned);
  void rotate_90();
  void rotate_180();
  void rotate_270();
};
