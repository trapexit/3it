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

#include "cel_control_chunk.hpp"

#include "ccb_flags.hpp"
#include "cel_types.hpp"
#include "chunk_ids.hpp"

#include "byteswap.hpp"

#include "fmt.hpp"


CelControlChunk::CelControlChunk()
  : id(),
    chunk_size(0),
    ccb_version(0),
    ccb_Flags(0),
    ccb_NextPtr(0),
    ccb_CelData(0),
    ccb_PLUTPtr(0),
    ccb_X(0),
    ccb_Y(0),
    ccb_hdx(0),
    ccb_hdy(0),
    ccb_vdx(0),
    ccb_vdy(0),
    ccb_ddx(0),
    ccb_ddy(0),
    ccb_PPMPC(0),
    ccb_PRE0(0),
    ccb_PRE1(0),
    ccb_Width(0),
    ccb_Height(0)
{

}

CelControlChunk::operator bool() const
{
  return ((id == CHUNK_CCB) &&
          (chunk_size == sizeof(CelControlChunk)));
}


CelControlChunk&
CelControlChunk::operator=(const Chunk &chunk_)
{
  const CelControlChunk *ccb;

  ccb = (const CelControlChunk*)chunk_.base();
  *this = *ccb;
  byteswap_if_little_endian();

  return *this;
}

bool
CelControlChunk::coded() const
{
  return !(ccb_PRE0 & PRE0_UNCODED);
}

bool
CelControlChunk::packed() const
{
  return (ccb_Flags & CCB_PACKED);
}

bool
CelControlChunk::unpacked() const
{
  return !packed();
}

bool
CelControlChunk::lrform() const
{
  return (ccb_PRE1 & PRE1_LRFORM);
}

int
CelControlChunk::bpp() const
{
  switch((ccb_PRE0 & PRE0_BPP_MASK) >> PRE0_BPP_SHIFT)
    {
    case PRE0_BPP_1:
      return 1;
    case PRE0_BPP_2:
      return 2;
    case PRE0_BPP_4:
      return 4;
    case PRE0_BPP_6:
      return 6;
    case PRE0_BPP_8:
      return 8;
    case PRE0_BPP_16:
      return 16;
    }

  return 0;
}

void
CelControlChunk::bpp(const uint32_t bpp_)
{
  uint32_t v;

  switch(bpp_)
    {
    case 1:
      v = PRE0_BPP_1;
      break;
    case 2:
      v = PRE0_BPP_2;
      break;
    case 4:
      v = PRE0_BPP_4;
      break;
    case 6:
      v = PRE0_BPP_6;
      break;
    case 8:
      v = PRE0_BPP_8;
      break;
    case 16:
      v = PRE0_BPP_16;
      break;
    }

  ccb_PRE0 = ((ccb_PRE0 & ~PRE0_BPP_MASK) | (v << PRE0_BPP_SHIFT));
}

bool
CelControlChunk::rep8() const
{
  return (ccb_PRE0 & PRE0_REP8);
}

uint8_t
CelControlChunk::pluta() const
{
  return ((ccb_Flags & CCB_PLUTA_MASK) >> CCB_PLUTA_SHIFT);
}

uint32_t
CelControlChunk::pdv() const
{
  // FIXME

  return -1;
}

bool
CelControlChunk::ccbpre() const
{
  return (ccb_Flags & CCB_CCBPRE);
}

bool
CelControlChunk::bgnd() const
{
  return (ccb_Flags & CCB_BGND);
}

bool
CelControlChunk::noblk() const
{
  return (ccb_Flags & CCB_NOBLK);
}

uint8_t
CelControlChunk::pover() const
{
  return ((ccb_Flags & CCB_POVER_MASK) >> CCB_POVER_SHIFT);
}

std::string
CelControlChunk::pover_str() const
{
  switch(pover())
    {
    case 0:
      return "use P-mode specified in pixel from pixel decoder";
    case 1:
      return "xx";
    case 2:
      return "use P-mode 0";
    case 3:
      return "use P-mode 1";
    }

  return "invalid value";
}

#define PPMPC_VAL(PMODE,PART) \
  (((ccb_PPMPC >> (PMODE ? PPMP_P1_SHIFT : PPMP_P0_SHIFT)) & PPMPC_##PART##_MASK) >> PPMPC_##PART##_SHIFT)

uint8_t
CelControlChunk::pixc_1s(int pmode_) const
{
  return PPMPC_VAL(pmode_,1S);
}

std::string
CelControlChunk::pixc_1s_str(int pmode_) const
{
  switch(pixc_1s(pmode_))
    {
    case 0:
      return "pixel is from the data decoder";
    case 1:
      return "pixel from current frame buffer";
    }

  return "invalid value";
}

uint8_t
CelControlChunk::pixc_ms(int pmode_) const
{
  return PPMPC_VAL(pmode_,MS);
}

std::string
CelControlChunk::pixc_ms_str(int pmode_) const
{
  switch(pixc_ms(pmode_))
    {
    case 0:
      return "PMV from CCB";
    case 1:
      return "PMV is AMV from data decoder";
    case 2:
      return "PMV and PDV from color value out of data decoder";
    case 3:
      return "PMV alone from the color value out of data decoder";
    }

  return "invalid value";
}

uint8_t
CelControlChunk::pixc_mf(int pmode_) const
{
  return PPMPC_VAL(pmode_,MF);
}

std::string
CelControlChunk::pixc_mf_str(int pmode_) const
{
  uint8_t rv;

  if(pixc_ms(pmode_) != 0)
    return "MS != 0 so value not used";

  rv = pixc_mf(pmode_) + 1;

  return fmt::format("PMV = {}",rv);
}

uint8_t
CelControlChunk::pixc_df(int pmode_) const
{
  return PPMPC_VAL(pmode_,DF);
}

std::string
CelControlChunk::pixc_df_str(int pmode_) const
{
  int rv;

  if(pixc_ms(pmode_) == 2)
    return fmt::format("MS set to 2. Value not used.");

  rv = pixc_df(pmode_);
  switch(rv)
    {
    case 0:
      rv = 16;
      break;
    case 1:
      rv = 2;
      break;
    case 2:
      rv = 4;
      break;
    case 3:
      rv = 8;
      break;
    }

  return fmt::format("PDV = {}",rv);
}

uint8_t
CelControlChunk::pixc_2s(int pmode_) const
{
  return PPMPC_VAL(pmode_,2S);
}

std::string
CelControlChunk::pixc_2s_str(int pmode_) const
{
  switch(pixc_2s(pmode_))
    {
    case 0:
      return "0";
    case 1:
      return "CCB AV value";
    case 2:
      return "current fb pixel";
    case 3:
      return "pixel comes from data decoder";
    }

  return "invalid value";
}

uint8_t
CelControlChunk::pixc_av(int pmode_) const
{
  return PPMPC_VAL(pmode_,AV);
}

std::string
CelControlChunk::pixc_av_str(int pmode_) const
{
  if(pixc_2s(pmode_) == 1)
    return fmt::format("{}",pixc_av(pmode_));

  uint8_t av;
  std::string s;

  av = pixc_av(pmode_);
  switch(av >> 3)
    {
    case 0:
      s += "SDV = 1;";
      break;
    case 1:
      s += "SDV = 2;";
      break;
    case 2:
      s += "SDV = 4;";
      break;
    case 3:
      s += "SDV = lowest two bits of color from data decoder;";
      break;
    }

  switch((av >> 2) & 0x1)
    {
    case 0:
      s += "wrap preventer;";
      break;
    case 1:
      s += "no wrap preventer;";
      break;
    }

  switch((av >> 1) & 0x1)
    {
    case 0:
      s += "no sign extension;";
      break;
    case 1:
      s += "sign extension;";
      break;
    }

  switch(av & 0x1)
    {
    case 0:
      s += "add second source;";
      break;
    case 1:
      s += "sub second source;";
      break;
    }

  return s;
}

uint8_t
CelControlChunk::pixc_2d(int pmode_) const
{
  return PPMPC_VAL(pmode_,2D);
}

std::string
CelControlChunk::pixc_2d_str(int pmode_) const
{
  return fmt::format("{}",pixc_2d(pmode_) + 1);
}

uint32_t
CelControlChunk::type() const
{
  return CEL_TYPE(coded(),packed(),lrform(),bpp());
}

void
CelControlChunk::byteswap_if_little_endian()
{
  ::byteswap_if_little_endian(&id.u32);
  ::byteswap_if_little_endian(&chunk_size);
  ::byteswap_if_little_endian(&ccb_version);
  ::byteswap_if_little_endian(&ccb_Flags);
  ::byteswap_if_little_endian(&ccb_NextPtr);
  ::byteswap_if_little_endian(&ccb_CelData);
  ::byteswap_if_little_endian(&ccb_PLUTPtr);
  ::byteswap_if_little_endian(&ccb_X);
  ::byteswap_if_little_endian(&ccb_Y);
  ::byteswap_if_little_endian(&ccb_hdx);
  ::byteswap_if_little_endian(&ccb_hdy);
  ::byteswap_if_little_endian(&ccb_vdx);
  ::byteswap_if_little_endian(&ccb_vdy);
  ::byteswap_if_little_endian(&ccb_ddx);
  ::byteswap_if_little_endian(&ccb_ddy);
  ::byteswap_if_little_endian(&ccb_PPMPC);
  ::byteswap_if_little_endian(&ccb_PRE0);
  ::byteswap_if_little_endian(&ccb_PRE1);
  ::byteswap_if_little_endian(&ccb_Width);
  ::byteswap_if_little_endian(&ccb_Height);
}
