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
  uint32_t pdv;

  pdv = ((ccb_PPMPC >> PPMP_1_SHIFT) & PPMPC_SF_MASK);
  switch(pdv)
    {
    case PPMPC_SF_2:
      return 2;
    case PPMPC_SF_4:
      return 4;
    case PPMPC_SF_8:
      return 8;
    case PPMPC_SF_16:
      return 16;
    }
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
