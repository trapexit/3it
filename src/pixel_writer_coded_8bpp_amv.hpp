#pragma once

#include "pixel_writer_rgba8888.hpp"

#include "plut.hpp"

class PixelWriterCoded8bppAMV : public PixelWriterRGBA8888
{
protected:
  PLUT _plut;
  u32  _pdv;

public:
  void init(Bitmap &b, PLUT plut, u32 pdv);

public:
  virtual void write(const u32 p);
  virtual void write_transparent(const u32 count);
};
