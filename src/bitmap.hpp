#pragma once

#include <string>
#include <map>

#include <memory>

#include <cstddef>
#include <cstdint>
#include <cstdlib>


struct Bitmap
{
  Bitmap() = default;

  Bitmap(const int w_,
         const int h_,
         const int n_ = 4)
  {
    reset(w_,h_,n_);
  }

  void
  reset(const int w_,
        const int h_,
        const int n_ = 4)
  {
    w = w_;
    h = h_;
    n = n_;
    d = std::make_unique<uint8_t[]>(w * h * n);
  }

  uint8_t*
  xy(size_t x_,
     size_t y_)
  {
    size_t offset;

    offset = ((w * y_ * n) + (x_ * n));

    return &d.get()[offset];
  }

  const
  uint8_t*
  xy(size_t x_,
     size_t y_) const
  {
    size_t offset;

    offset = ((w * y_ * n) + (x_ * n));

    return &d.get()[offset];
  }

  uint8_t*
  y(size_t y_)
  {
    return xy(0,y_);
  }

  const
  uint8_t*
  y(size_t y_) const
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
  }

public:
  size_t w;
  size_t h;
  size_t n;
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
};
