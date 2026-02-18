#include "pixel_writer_rgba8888.hpp"

#include <stdexcept>

void
PixelWriterRGBA8888::init(Bitmap &b_)
{
  _b   = &b_;
  _idx = 0;
}

Bitmap&
PixelWriterRGBA8888::bitmap() const
{
  return *_b;
}

u64
PixelWriterRGBA8888::idx() const
{
  return _idx;
}

u32
PixelWriterRGBA8888::w() const
{
  return _b->w;
}

u32
PixelWriterRGBA8888::h() const
{
  return _b->h;
}

void
PixelWriterRGBA8888::move_xy(const u32 x_,
                             const u32 y_)
{
  _idx = ((w() * y_) + x_);
}

void
PixelWriterRGBA8888::move_y(const u32 y_)
{
  move_xy(0,y_);
}

u32
PixelWriterRGBA8888::y() const
{
  return (_idx / w());
}

u32
PixelWriterRGBA8888::x() const
{
  return (_idx % w());
}

bool
PixelWriterRGBA8888::row_filled() const
{
  return ((_idx % w()) == 0);
}

u32
PixelWriterRGBA8888::row_pixels_remaining() const
{
  return (w() - x());
}

void
PixelWriterRGBA8888::write_rgba(const u8 r_,
                                const u8 g_,
                                const u8 b_,
                                const u8 a_)
{
  if(_idx >= (u64)(w() * h()))
    throw std::runtime_error("pixel writer overflow");

  RGBA8888 &rgba = _b->idx(_idx++);

  rgba.r = r_;
  rgba.g = g_;
  rgba.b = b_;
  rgba.a = a_;
}
