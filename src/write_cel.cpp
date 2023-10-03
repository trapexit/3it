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

#include "filerw.hpp"

#include "fmt.hpp"


void
WriteFile::cel(const std::filesystem::path &filepath_,
               const CelControlChunk       &ccc_,
               const ByteVec               &pdat_,
               const PLUT                  &plut_)
{
  FileRW f;

  f.open_write_trunc(filepath_);
  if(f.error())
    throw std::system_error(errno,std::system_category(),"failed to open "+filepath_.string());

  f.u32be(ccc_.id.uint32());
  f.u32be(ccc_.chunk_size);
  f.u32be(ccc_.ccb_version);
  f.u32be(ccc_.ccb_Flags);
  f.u32be(ccc_.ccb_NextPtr);
  f.u32be(ccc_.ccb_CelData);
  f.u32be(ccc_.ccb_PLUTPtr);
  f.i32be(ccc_.ccb_X);
  f.i32be(ccc_.ccb_Y);
  f.i32be(ccc_.ccb_hdx);
  f.i32be(ccc_.ccb_hdy);
  f.i32be(ccc_.ccb_vdx);
  f.i32be(ccc_.ccb_vdy);
  f.i32be(ccc_.ccb_ddx);
  f.i32be(ccc_.ccb_ddy);
  f.u32be(ccc_.ccb_PPMPC);
  f.u32be(ccc_.ccb_PRE0);
  f.u32be(ccc_.ccb_PRE1);
  f.i32be(ccc_.ccb_Width);
  f.i32be(ccc_.ccb_Height);

  if(ccc_.coded() && !plut_.empty())
    {
      f.w("PLUT");
      f.u32be(4 + 4 + 4 + (plut_.size() * 2));
      f.u32be(plut_.size());
      for(auto p : plut_)
        f.u16be(p);
    }

  f.w("PDAT");
  f.u32be(pdat_.size() + 4 + 4);
  f.w(pdat_);

  if(f.error())
    throw std::system_error(errno,std::system_category(),"failed to write "+filepath_.string());

  f.close();
}
