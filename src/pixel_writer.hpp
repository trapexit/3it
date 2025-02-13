#pragma once

#include "bitmap.hpp"
#include "byteswap.hpp"
#include "ccb_flags.hpp"
#include "fmt.hpp"
#include "plut.hpp"
#include "rgba8888.hpp"

#include "scale.hpp"
#include "types_ints.h"

#include <cstdint>
#include <cstddef>

class PixelWriter
{
private:
  u32 _w;
  u32 _h;
  u32 _n;
  u8 *_data;
  size_t   _idx;

private:
  u8   _bpp;
  bool _coded;
  bool _rep8;
  u8   _pluta;
  PLUT _plut;
  u32  _pixc;

public:
  size_t
  idx() const
  {
    return _idx;
  }

  u8
  bpp() const
  {
    return _bpp;
  }

  void
  reset(Bitmap     &b_,
        const u8    bpp_  = 0,
        const bool  rep8_ = false)
  {
    _data = (u8*)b_.d.get();
    _w    = b_.w;
    _h    = b_.h;
    _n    = sizeof(RGBA8888);
    _idx  = 0;

    _bpp   = bpp_;
    _coded = false;
    _rep8  = rep8_;
    _pluta = 0;
    _pixc  = PPMP_OPAQUE;
  }

  void
  reset(Bitmap     &b_,
        const PLUT &plut_,
        const u8    pluta_,
        const u8    bpp_,
        const u32   pixc_ = PPMP_OPAQUE)
  {
    _data = (u8*)b_.d.get();
    _w    = b_.w;
    _h    = b_.h;
    _n    = sizeof(RGBA8888);
    _idx  = 0;

    _bpp   = bpp_;
    _coded = true;
    _rep8  = false;
    _pluta = pluta_;
    _plut  = plut_;
    _pixc  = pixc_;
  }

  void
  move_xy(const u32 x_,
          const u32 y_)
  {
    _idx = ((_w * y_ * _n) + (x_ * _n));
  }

  void
  move_y(const u32 y_)
  {
    move_xy(0,y_);
  }

  u32
  y() const
  {

    return (_idx / (_w * _n));
  }

  u32
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
  write(const u32 p_)
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
  write(const u32 p_,
        const u32 n_)
  {
    for(u32 i = 0; i < n_; i++)
      write(p_);
  }

  void
  write_rgb(const u8 r_,
            const u8 g_,
            const u8 b_)
  {
    write_rgba(r_,g_,b_);
  }

  void
  write_rgba(const u8 r_,
             const u8 g_,
             const u8 b_,
             const u8 a_ = 0xFF)
  {
    _data[_idx++] = r_;
    _data[_idx++] = g_;
    _data[_idx++] = b_;
    _data[_idx++] = a_;
  }

  void
  write_transparent(const u32 n_)
  {
    for(u32 i = 0; i < n_; i++)
      write_rgba(0,0,0,0);
  }

  void
  write_0555(const u16 rgb_)
  {
    u8 r,g,b;

    r = ((rgb_ >> 10) & 0x1F);
    r = scale_u5_to_u8(r);
    g = ((rgb_ >>  5) & 0x1F);
    g = scale_u5_to_u8(g);
    b = ((rgb_ >>  0) & 0x1F);
    b = scale_u5_to_u8(b);

    write_rgb(r,g,b);
  }

  void
  write_coded_16bpp(const u16 rgb_)
  {
    u16 rgb;
    u32 amv;

    amv = ((rgb_ & 0x3FE) >> 5);
    rgb = (_plut.at(rgb_ & 0x1F) & 0x7FFF);

    write_uncoded_16bpp(rgb);
  }

  void
  write_uncoded_16bpp(const u16 rgb_)
  {
    write_0555(rgb_);
  }

  void
  write_16bpp(const u16 rgb_)
  {
    if(_coded)
      write_coded_16bpp(rgb_);
    else
      write_uncoded_16bpp(rgb_);
  }

  void
  write_0555(u8 hb_,
             u8 lb_)
  {
    union { u8 a[2]; u16 b; };

    a[0] = hb_;
    a[1] = lb_;
    b    = byteswap_if_little_endian(b);

    write_0555(b);
  }

  void
  write_332(const u8 rgb_)
  {
    u8 r,g,b;

    r = ((rgb_ >> 5) & 0x7);
    r = scale_u3_to_u8(r);
    g = ((rgb_ >> 2) & 0x7);
    g = scale_u3_to_u8(g);
    b = ((rgb_ >> 0) & 0x3);
    b = scale_u2_to_u8(b);

    write_rgb(r,g,b);
  }

  void
  write_rep8_332(const u8 rgb_)
  {
    u8 r,g,b;

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
  write_uncoded_8bpp(u8 rgb_)
  {
    if(_rep8)
      write_rep8_332(rgb_);
    else
      write_332(rgb_);
  }

  void
  write_8bpp(u32 p_)
  {
    if(_coded)
      abort();
    else
      write_uncoded_8bpp(p_);
  }

  void
  write_6bpp(u32 p_)
  {
    u16 p;

    p = _plut.at(p_ & 0x1F);

    write_uncoded_16bpp(p);
  }

  void
  write_4bpp(u32 p_)
  {
    u16 p;

    p = _plut.at((p_ & 0x0F) | (_pluta & 0x10));

    write_uncoded_16bpp(p);
  }

  void
  write_2bpp(u32 p_)
  {
    u16 p;

    p = _plut.at((p_ & 0x03) | (_pluta & 0x1C));

    write_uncoded_16bpp(p);
  }

  void
  write_1bpp(u32 p_)
  {
    u16 p;

    p = _plut.at((p_ & 0x01) | (_pluta & 0x1E));

    write_uncoded_16bpp(p);
  }
};
