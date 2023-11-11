#pragma once

#include "rgba8888.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <string>
#include <map>
#include <memory>

struct Bitmap
{
  Bitmap()
    : w(0),
      h(0),
      d()
  {
    set("rotation","0");
  }

  Bitmap(const std::size_t w_,
         const std::size_t h_)
  {
    reset(w_,h_);
    set("rotation","0");
  }

  void
  reset(const std::size_t w_,
        const std::size_t h_)
  {
    w = w_;
    h = h_;
    d = std::make_unique<uint8_t[]>(w * h * sizeof(RGBA8888));
    set("rotation","0");
  }

  RGBA8888*
  xy(const std::size_t x_,
     const std::size_t y_)
  {
    RGBA8888 *p;
    std::size_t offset;

    p = (RGBA8888*)d.get();
    offset = ((w * y_) + x_);

    return &p[offset];
  }

  const
  RGBA8888*
  xy(const std::size_t x_,
     const std::size_t y_) const
  {
    RGBA8888 *p;
    std::size_t offset;

    p = (RGBA8888*)d.get();
    offset = ((w * y_) + x_);

    return &p[offset];
  }

  RGBA8888*
  y(const std::size_t y_)
  {
    return xy(0,y_);
  }

  const
  RGBA8888*
  y(std::size_t y_) const
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

public:
  std::size_t w;
  std::size_t h;
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

  void rotate_to(unsigned);
  void rotate_90();
  void rotate_180();
  void rotate_270();
};
