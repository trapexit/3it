#include "pixel_writer_coded_8bpp_amv.hpp"

#include "CLI11.hpp"
#include "clamp.hpp"
#include "pixel_writer_rgba8888.hpp"
#include "scale.hpp"

void
PixelWriterCoded8bppAMV::init(Bitmap &b_,
                              PLUT    plut_,
                              u32     pdv_)
{
  PixelWriterRGBA8888::init(b_);
  _plut  = plut_;
  _pdv   = pdv_;
}

void
PixelWriterCoded8bppAMV::write(const u32 p_)
{
  u32 r,g,b;
  u32 amv;
  u16 rgb;

  rgb = _plut.at(p_ & 0x1F);
  amv = (((p_ >> 5) & 0x7) + 1);

  r = ((rgb >> 10) & 0x1F);
  g = ((rgb >>  5) & 0x1F);
  b = ((rgb >>  0) & 0x1F);

  r = ((r * amv) / _pdv);
  g = ((g * amv) / _pdv);
  b = ((b * amv) / _pdv);

  // FIXME: clamping is optional
  r = clamp_u5(r);
  g = clamp_u5(g);
  b = clamp_u5(b);

  r = scale_u5_to_u8(r);
  g = scale_u5_to_u8(g);
  b = scale_u5_to_u8(b);

  PixelWriterRGBA8888::write_rgba(r,g,b,0xFF);
}

void
PixelWriterCoded8bppAMV::write_transparent(const u32 count_)
{
  for(u32 i = 0; i < count_; i++)
    PixelWriterRGBA8888::write_rgba(0,0,0,0);
}
