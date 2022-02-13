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

#include "ofstreambe.hpp"

#include "fmt.hpp"


void
WriteFile::cel(const std::filesystem::path &filepath_,
               const CelControlChunk       &ccc_,
               cspan<uint8_t>               pdat_,
               const PLUT                  &plut_)
{
  std::ofstreambe os;

  os.open(filepath_,std::ofstream::binary);
  if(!os)
    throw std::system_error(errno,std::system_category(),"failed to open "+filepath_.string());

  os.writebe(ccc_.id.uint32())
    .writebe(ccc_.chunk_size)
    .writebe(ccc_.ccb_version)
    .writebe(ccc_.ccb_Flags)
    .writebe(ccc_.ccb_NextPtr)
    .writebe(ccc_.ccb_CelData)
    .writebe(ccc_.ccb_PLUTPtr)
    .writebe(ccc_.ccb_X)
    .writebe(ccc_.ccb_Y)
    .writebe(ccc_.ccb_hdx)
    .writebe(ccc_.ccb_hdy)
    .writebe(ccc_.ccb_vdx)
    .writebe(ccc_.ccb_vdy)
    .writebe(ccc_.ccb_ddx)
    .writebe(ccc_.ccb_ddy)
    .writebe(ccc_.ccb_PPMPC)
    .writebe(ccc_.ccb_PRE0)
    .writebe(ccc_.ccb_PRE1)
    .writebe(ccc_.ccb_Width)
    .writebe(ccc_.ccb_Height);

  if(ccc_.coded())
    {
      os.write("PLUT",4)
        .writebe((uint32_t)(4 + 4 + 4 + (plut_.size() * 2)))
        .writebe((uint32_t)plut_.size());
      for(size_t i = 0; i < plut_.size(); i++)
        os.writebe(plut_[i]);
    }

  os.write("PDAT",4)
    .writebe((uint32_t)(pdat_.size() + 4 + 4))
    .write(&pdat_[0],pdat_.size());

  if(!os)
    throw std::system_error(errno,std::system_category(),"failed to write "+filepath_.string());

  os.close();
}
