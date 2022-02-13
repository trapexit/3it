#pragma once

#include "bitmap.hpp"
#include "byteswap.hpp"
#include "plut.hpp"

#include <cstdint>
#include <cstddef>


class PixelWriter
{
private:
  uint32_t _w;
  uint32_t _h;
  uint32_t _n;
  uint8_t *_data;
  size_t   _idx;

private:
  uint8_t  _bpp;
  bool     _coded;
  bool     _rep8;
  uint8_t  _pluta;
  PLUT     _plut;

public:
  size_t
  idx() const
  {
    return _idx;
  }

  uint8_t
  bpp() const
  {
    return _bpp;
  }

  void
  reset(Bitmap        &b_,
        const uint8_t  bpp_  = 0,
        const bool     rep8_ = false)
  {
    _data = b_.d.get();
    _w    = b_.w;
    _h    = b_.h;
    _n    = b_.n;
    _idx  = 0;

    _bpp   = bpp_;
    _coded = false;
    _rep8  = rep8_;
    _pluta = 0;
  }

  void
  reset(Bitmap        &b_,
        const PLUT    &plut_,
        const uint8_t  pluta_,
        const uint8_t  bpp_)
  {
    _data = b_.d.get();
    _w    = b_.w;
    _h    = b_.h;
    _n    = b_.n;
    _idx  = 0;

    _bpp   = bpp_;
    _coded = true;
    _rep8  = false;
    _pluta = pluta_;
    for(size_t i = 0; i < plut_.size(); i++)
      _plut[i] = plut_[i];
  }

  void
  move_xy(const uint32_t x_,
          const uint32_t y_)
  {
    _idx = ((_w * y_ * _n) + (x_ * _n));
  }

  void
  move_y(const uint32_t y_)
  {
    move_xy(0,y_);
  }

  uint32_t
  y() const
  {

    return (_idx / (_w * _n));
  }

  uint32_t
  x() const
  {
    return ((_idx / _n) % _w);
  }

  bool
  row_filled() const
  {
    return ((_idx % (_w * _n) == 0));
  }

  void
  write(const uint32_t p_)
  {
    switch(_bpp)
      {
      case 1:
        write_1bpp(p_);
        break;
      case 2:
        write_2bpp(p_);
        break;
      case 4:
        write_4bpp(p_);
        break;
      case 6:
        write_6bpp(p_);
        break;
      case 8:
        write_8bpp(p_);
        break;
      case 16:
        write_16bpp(p_);
        break;
      }
  }

  void
  write(const uint32_t p_,
        const uint32_t n_)
  {
    for(uint32_t i = 0; i < n_; i++)
      write(p_);
  }

  void
  write_rgb(const uint8_t r_,
            const uint8_t g_,
            const uint8_t b_)
  {
    write_rgba(r_,g_,b_);
  }

  void
  write_rgba(const uint8_t r_,
             const uint8_t g_,
             const uint8_t b_,
             const uint8_t a_ = 0xFF)
  {
    _data[_idx++] = r_;
    _data[_idx++] = g_;
    _data[_idx++] = b_;
    _data[_idx++] = a_;
  }

  void
  write_0555(const uint16_t rgb_)
  {
    uint8_t r,g,b;

    r = (((rgb_ >> 10) & 0x1F) << 3);
    g = (((rgb_ >>  5) & 0x1F) << 3);
    b = (((rgb_ >>  0) & 0x1F) << 3);

    write_rgb(r,g,b);
  }

  void
  write_coded_16bpp(const uint16_t rgb_)
  {
    uint16_t rgb;
    uint32_t amv;

    amv = ((rgb_ & 0x3FE) >> 5);
    rgb = (_plut[rgb_ & 0x1F] & 0x7FFF);

    write_uncoded_16bpp(rgb);
  }

  void
  write_uncoded_16bpp(const uint16_t rgb_)
  {
    write_0555(rgb_);
  }

  void
  write_16bpp(const uint16_t rgb_)
  {
    if(_coded)
      write_coded_16bpp(rgb_);
    else
      write_uncoded_16bpp(rgb_);
  }

  void
  write_0555(uint8_t hb_,
             uint8_t lb_)
  {
    union { uint8_t u8[2]; uint16_t u16; };

    u8[0] = hb_;
    u8[1] = lb_;
    u16   = byteswap_if_little_endian(u16);

    write_0555(u16);
  }

  void
  write_332(const uint8_t rgb_)
  {
    uint8_t r,g,b;

    r = ((rgb_ << 0) & 0xE0);
    g = ((rgb_ << 3) & 0xE0);
    b = ((rgb_ << 6) & 0xE0);

    write_rgb(r,g,b);
  }

  void
  write_rep8_332(const uint8_t rgb_)
  {
    uint8_t r,g,b;

    r  = ((rgb_ << 0) & 0xE0);
    r |= ((r & 0x80) >> 3);
    r |= ((r & 0x40) >> 3);

    g  = ((rgb_ << 3) & 0xE0);
    g |= ((g & 0x80) >> 3);
    g |= ((g & 0x40) >> 3);

    b  = ((rgb_ << 6) & 0xE0);
    b |= ((b & 0x80) >> 2);
    b |= ((b & 0x80) >> 4);
    b |= ((b & 0x40) >> 2);

    write_rgb(r,g,b);
  }

  void
  write_uncoded_8bpp(uint8_t rgb_)
  {
    if(_rep8)
      write_rep8_332(rgb_);
    else
      write_332(rgb_);
  }

  void
  write_coded_8bpp(uint8_t p_)
  {
    uint32_t amv;
    uint16_t rgb;

    amv = (p_ >> 5);
    rgb = _plut[p_ & 0x1F];

    write_uncoded_16bpp(rgb);
  }

  void
  write_8bpp(uint32_t p_)
  {
    if(_coded)
      write_coded_8bpp(p_);
    else
      write_uncoded_8bpp(p_);
  }

  void
  write_6bpp(uint32_t p_)
  {
    uint16_t p;

    p = _plut[p_ & 0x1F];

    write_uncoded_16bpp(p);
  }

  void
  write_4bpp(uint32_t p_)
  {
    uint16_t p;

    p = _plut[(p_ & 0x0F) | (_pluta & 0x10)];

    write_uncoded_16bpp(p);
  }

  void
  write_2bpp(uint32_t p_)
  {
    uint16_t p;

    p = _plut[(p_ & 0x03) | (_pluta & 0x1C)];

    write_uncoded_16bpp(p);
  }

  void
  write_1bpp(uint32_t p_)
  {
    uint16_t p;

    p = _plut[(p_ & 0x01) | (_pluta & 0x1E)];

    write_uncoded_16bpp(p);
  }
};
