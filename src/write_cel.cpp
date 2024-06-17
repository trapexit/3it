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

#include "write_cel.hpp"

#include "bytevec.hpp"
#include "cel_control_chunk.hpp"
#include "chunk_ids.hpp"
#include "chunk_sizes.hpp"
#include "filerw.hpp"


static
void
write_ccc(FileRW                &f_,
          const CelControlChunk &ccc_)
{
  f_.u32be(CHUNK_CCB);
  f_.u32be(ccc_.chunk_size);
  f_.u32be(ccc_.ccb_version);
  f_.u32be(ccc_.ccb_Flags);
  f_.u32be(ccc_.ccb_NextPtr);
  f_.u32be(ccc_.ccb_CelData);
  f_.u32be(ccc_.ccb_PLUTPtr);
  f_.i32be(ccc_.ccb_X);
  f_.i32be(ccc_.ccb_Y);
  f_.i32be(ccc_.ccb_hdx);
  f_.i32be(ccc_.ccb_hdy);
  f_.i32be(ccc_.ccb_vdx);
  f_.i32be(ccc_.ccb_vdy);
  f_.i32be(ccc_.ccb_ddx);
  f_.i32be(ccc_.ccb_ddy);
  f_.u32be(ccc_.ccb_PPMPC);
  f_.u32be(ccc_.ccb_PRE0);
  f_.u32be(ccc_.ccb_PRE1);
  f_.i32be(ccc_.ccb_Width);
  f_.i32be(ccc_.ccb_Height);
}

static
void
write_pdat(FileRW        &f_,
           const ByteVec &pdat_)
{
  f_.u32be(CHUNK_PDAT);
  f_.u32be(CHUNK_HDR_SIZE + pdat_.size());
  f_.w(pdat_);
}

static
void
write_plut(FileRW                &f_,
           const CelControlChunk &ccc_,
           const PLUT            &plut_)
{
  // While the file format has a PLUT size/len value it is not used by
  // LoadCel/ParseCel. As a result it can lead to issues if the PLUT
  // data is not at least equal to the data to be loaded.
  // https://3dodev.com/documentation/development/opera/pf25/ppgfldr/ggsfldr/gpgfldr/5gpgh
  if(ccc_.coded() && !plut_.empty())
    {
      uint32_t size;

      size = plut_.min_size(ccc_.bpp());

      f_.u32be(CHUNK_PLUT);
      f_.u32be(CHUNK_HDR_SIZE +
              CHUNK_PLUT_SIZE_SIZE +
              (size * CHUNK_PLUT_VAL_SIZE));
      f_.u32be(size);
      for(auto p : plut_)
        f_.u16be(p);
      for(auto i = plut_.size(); i < size; i++)
        f_.u16be(0);
    }
}

void
WriteFile::cel(const std::filesystem::path &filepath_,
               const CelControlChunk       &ccc_,
               const ByteVec               &pdat_,
               const PLUT                  &plut_)
{
  int rv;
  FileRW f;

  rv = f.open_write_trunc(filepath_);
  if((rv < 0) || f.error())
    throw std::system_error(-rv,
                            std::system_category(),
                            "failed to open "+filepath_.string());

  write_ccc(f,ccc_);
  write_pdat(f,pdat_);
  write_plut(f,ccc_,plut_);

  if(f.error())
    throw std::system_error(errno,std::system_category(),"failed to write "+filepath_.string());

  f.close();
}
