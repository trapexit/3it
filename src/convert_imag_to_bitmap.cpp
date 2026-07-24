/*
  ISC License

  Copyright (c) 2026, Antonio SJ Musumeci <trapexit@spawn.link>

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

#include "convert.hpp"

#include "chunk_ids.hpp"
#include "fmt.hpp"
#include "image_control_chunk.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace
{
  using ChannelCLUT = std::array<uint8_t,32>;

  struct CLUT
  {
    ChannelCLUT red;
    ChannelCLUT green;
    ChannelCLUT blue;
    RGBA8888    background;
  };

  struct VDLState
  {
    CLUT clut;
    bool clut_bypass = false;
    bool background  = false;
    bool bypass_random = false;
    bool bypass_msb_replicate = false;
  };

  struct ImagFrame
  {
    ImageControlChunk header;
    cspan<uint8_t> pdat;
    cspan<uint8_t> vdl;
    bool           has_vdl = false;
  };

  struct ImagParts
  {
    std::vector<ImagFrame> frames;
  };

  static
  uint32_t
  u32be(const uint8_t *data_)
  {
    return ((uint32_t(data_[0]) << 24) |
            (uint32_t(data_[1]) << 16) |
            (uint32_t(data_[2]) <<  8) |
            (uint32_t(data_[3]) <<  0));
  }

  static
  uint16_t
  u16be(const uint8_t *data_)
  {
    return ((uint16_t(data_[0]) << 8) |
            (uint16_t(data_[1]) << 0));
  }

  static
  size_t
  vdl_chunk_size(const uint32_t count_,
                 const size_t   record_size_)
  {
    constexpr size_t HEADER_SIZE = sizeof(uint32_t);

    if(count_ > ((std::numeric_limits<size_t>::max() - HEADER_SIZE) /
                 record_size_))
      throw fmt::exception("VDL record count is too large: {}",count_);

    return (HEADER_SIZE + (size_t(count_) * record_size_));
  }

  static
  uint8_t
  startup_clut_value(const uint8_t value_)
  {
    return ((255 * value_ + 15) / 31);
  }

  static
  CLUT
  startup_clut()
  {
    CLUT clut;

    for(size_t i = 0; i < 32; i++)
      {
        const uint8_t value = startup_clut_value(i);

        clut.red[i]   = value;
        clut.green[i] = value;
        clut.blue[i]  = value;
      }
    clut.background = RGBA8888(1,1,1,0xFF);

    return clut;
  }

  static
  uint8_t
  hard_clut_value(const uint8_t value_,
                  const VDLState &state_)
  {
    uint8_t low = 0;

    // RANDOMEN has no single static result. Use the floor of the temporal
    // mean of the three random low bits so file conversion stays deterministic.
    if(state_.bypass_random)
      low = 3;
    else if(state_.bypass_msb_replicate)
      low = (value_ >> 2);

    return ((value_ << 3) | low);
  }

  static
  uint8_t
  z24_clut_value(const uint8_t value_)
  {
    if(value_ < 8)
      return (value_ * 2);
    if(value_ < 16)
      return (value_ * 2 + 225);
    return ((31 - value_) * 16);
  }

  static
  ImageControlChunk
  parse_image_control(const uint8_t *data_,
                      const uint32_t chunk_size_)
  {
    ImageControlChunk header;

    if(chunk_size_ != sizeof(ImageControlChunk))
      throw fmt::exception("invalid IMAG control chunk size: {}",chunk_size_);

    header.id            = CHUNK_IMAG;
    header.chunk_size    = sizeof(ImageControlChunk);
    header.w             = static_cast<int32_t>(u32be(data_ +  8));
    header.h             = static_cast<int32_t>(u32be(data_ + 12));
    header.bytesperrow   = static_cast<int32_t>(u32be(data_ + 16));
    header.bitsperpixel  = data_[20];
    header.numcomponents = data_[21];
    header.numplanes     = data_[22];
    header.colorspace    = data_[23];
    header.comptype      = data_[24];
    header.hvformat      = data_[25];
    header.pixelorder    = data_[26];
    header.version       = data_[27];

    return header;
  }

  static
  void
  parse_imag(cspan<uint8_t> data_,
             ImagParts    &parts_)
  {
    size_t offset = 0;
    ImageControlChunk current_header;
    bool has_current_header = false;
    cspan<uint8_t> current_vdl;
    bool has_current_vdl = false;

    if(data_.size() < sizeof(ImageControlChunk))
      throw fmt::exception("IMAG file is truncated: {} bytes",data_.size());
    if(u32be(data_.data()) != CHUNK_IMAG)
      throw fmt::exception("IMAG control chunk must be first");

    while(offset < data_.size())
      {
        uint32_t chunk_id;
        uint32_t chunk_size;

        if((data_.size() - offset) < 8)
          throw fmt::exception("truncated chunk header at offset {}",offset);

        chunk_id   = u32be(data_.data() + offset + 0);
        chunk_size = u32be(data_.data() + offset + 4);
        if(chunk_size < 8)
          throw fmt::exception("invalid chunk size {} at offset {}",chunk_size,offset);
        if(size_t(chunk_size) > (data_.size() - offset))
          throw fmt::exception("chunk at offset {} exceeds file size",offset);

        switch(chunk_id)
          {
          case CHUNK_IMAG:
            current_header = parse_image_control(data_.data() + offset,
                                                 chunk_size);
            has_current_header = true;
            break;
          case CHUNK_PDAT:
            {
              ImagFrame frame;

              if(!has_current_header)
                throw fmt::exception("PDAT chunk at offset {} has no preceding IMAG control chunk",
                                     offset);

              frame.header = current_header;
              frame.pdat = cspan<uint8_t>(data_.data() + offset + 8,
                                          chunk_size - 8);
              frame.vdl = current_vdl;
              frame.has_vdl = has_current_vdl;
              parts_.frames.emplace_back(frame);
            }
            break;
          case CHUNK_VDL:
            current_vdl = cspan<uint8_t>(data_.data() + offset + 8,
                                         chunk_size - 8);
            has_current_vdl = true;
            break;
          default:
            break;
          }

        offset += chunk_size;
        const size_t padding = ((4 - (offset & 3)) & 3);
        if(padding > (data_.size() - offset))
          throw fmt::exception("chunk padding at offset {} exceeds file size",
                               offset);
        offset += padding;
      }

    if(parts_.frames.empty())
      throw fmt::exception("IMAG file has no PDAT chunk");
  }

  static
  void
  validate_common(const ImageControlChunk &header_)
  {
    const uint64_t width  = (header_.w > 0) ? uint64_t(header_.w) : 0;
    const uint64_t height = (header_.h > 0) ? uint64_t(header_.h) : 0;

    if((width == 0) || (height == 0))
      throw fmt::exception("invalid IMAG dimensions: {}x{}",header_.w,header_.h);
    if(width > (std::numeric_limits<size_t>::max() / height / sizeof(RGBA8888)))
      throw fmt::exception("IMAG dimensions are too large: {}x{}",header_.w,header_.h);
    if(header_.bytesperrow <= 0)
      throw fmt::exception("invalid bytesperrow: {}",header_.bytesperrow);
    if(header_.numcomponents != 3)
      throw fmt::exception("numcomponents {} not supported",header_.numcomponents);
    if(header_.numplanes != 1)
      throw fmt::exception("numplanes {} not supported",header_.numplanes);
    if(header_.colorspace != 0)
      throw fmt::exception("colorspace {} not supported",header_.colorspace);
    if(header_.comptype != 0)
      throw fmt::exception("comptype {} not supported",header_.comptype);
    if(header_.hvformat != 0)
      throw fmt::exception("hvformat {} not supported",header_.hvformat);
    if(header_.version != 0)
      throw fmt::exception("IMAG version {} not supported",header_.version);
  }

  static
  std::vector<VDLState>
  parse_vdl(cspan<uint8_t> vdl_,
            const size_t   height_)
  {
    constexpr size_t VDL_HEADER_SIZE = sizeof(uint32_t);
    constexpr size_t A_VDL_RECORD_SIZE = 36 * sizeof(uint32_t);
    constexpr size_t SDK_VDL_RECORD_SIZE = 40 * sizeof(uint32_t);
    std::vector<VDLState> states;
    VDLState current;
    uint32_t count;
    size_t record_size;
    size_t expected_size;
    size_t sdk_expected_size;

    if(vdl_.size() < VDL_HEADER_SIZE)
      throw fmt::exception("VDL chunk is truncated");

    count = u32be(vdl_.data());
    if(count == 0)
      throw fmt::exception("VDL chunk contains no records");
    if(count > height_)
      throw fmt::exception("VDL record count {} exceeds image height {}",count,height_);

    expected_size = vdl_chunk_size(count,A_VDL_RECORD_SIZE);
    sdk_expected_size = vdl_chunk_size(count,SDK_VDL_RECORD_SIZE);
    if(vdl_.size() == expected_size)
      record_size = A_VDL_RECORD_SIZE;
    else if(vdl_.size() == sdk_expected_size)
      record_size = SDK_VDL_RECORD_SIZE;
    else
      throw fmt::exception("VDL data size {} does not match {} records "
                           "({} documented or {} SDK bytes)",
                           vdl_.size(),count,expected_size,
                           sdk_expected_size);

    current.clut = startup_clut();
    states.reserve(count);
    for(size_t record_idx = 0; record_idx < count; record_idx++)
      {
        const uint8_t *record =
          vdl_.data() + VDL_HEADER_SIZE + (record_idx * record_size);
        const bool documented = (record_size == A_VDL_RECORD_SIZE);
        const uint32_t control = u32be(record + (documented ? 4 : 0));
        const uint32_t command_count = documented ? 33 :
          ((control >> 9) & 0x3F);
        const size_t command_word = documented ? 2 : 4;

        if((command_count == 0) || (command_count > 36))
          throw fmt::exception("invalid VDL command count {} in record {}",
                               command_count,record_idx);

        for(size_t command_idx = 0; command_idx < command_count; command_idx++)
          {
            const uint32_t command =
              u32be(record + ((command_word + command_idx) * 4));

            if((command & 0xFF000000U) == 0xE0000000U)
              {
                current.clut.background.r = ((command >> 16) & 0xFF);
                current.clut.background.g = ((command >>  8) & 0xFF);
                current.clut.background.b = ((command >>  0) & 0xFF);
                current.clut.background.a = 0xFF;
                continue;
              }
            if((command & 0xE0000000U) == 0xC0000000U)
              {
                current.clut_bypass = (command & 0x02000000U);
                current.background  = (command & 0x00400000U);
                current.bypass_random = (command & 0x00001000U);
                current.bypass_msb_replicate = (command & 0x00000800U);
                continue;
              }
            if(command & 0x80000000U)
              continue;

            const uint8_t select = ((command >> 29) & 0x03);
            const uint8_t index  = ((command >> 24) & 0x1F);
            const uint8_t red    = ((command >> 16) & 0xFF);
            const uint8_t green  = ((command >>  8) & 0xFF);
            const uint8_t blue   = ((command >>  0) & 0xFF);

            switch(select)
              {
              case 0:
                current.clut.red[index]   = red;
                current.clut.green[index] = green;
                current.clut.blue[index]  = blue;
                break;
              case 1:
                current.clut.blue[index] = blue;
                break;
              case 2:
                current.clut.green[index] = green;
                break;
              case 3:
                current.clut.red[index] = red;
                break;
              }
          }

        const size_t remaining = height_ - states.size();
        size_t persistence = (control & 0x1FF);

        if(persistence > remaining)
          throw fmt::exception("VDL record {} persists {} lines with only {} remaining",
                               record_idx,persistence,remaining);
        if(persistence == 0)
          persistence = remaining;
        for(size_t line = 0; line < persistence; line++)
          states.emplace_back(current);
      }

    if(states.size() < height_)
      states.resize(height_,states.back());

    return states;
  }

  static
  void
  decode_lrform(const ImagFrame &frame_,
                Bitmap          &bitmap_)
  {
    const size_t width  = frame_.header.w;
    const size_t height = frame_.header.h;
    const size_t stride = frame_.header.bytesperrow;
    const CLUT startup = startup_clut();
    std::vector<VDLState> custom;
    size_t required_size;

    if(frame_.header.pixelorder != 1)
      throw fmt::exception("pixelorder {} not supported for 16-bit IMAG",
                           frame_.header.pixelorder);
    if(height & 1)
      throw fmt::exception("16-bit LRFORM IMAG height must be even: {}",height);
    if(width > (std::numeric_limits<size_t>::max() / 2))
      throw fmt::exception("IMAG width is too large: {}",width);
    if(stride < (width * 2))
      throw fmt::exception("bytesperrow {} is too small for width {}",stride,width);
    if(stride & 1)
      throw fmt::exception("16-bit IMAG bytesperrow must be even: {}",stride);
    if(height > (std::numeric_limits<size_t>::max() / stride))
      throw fmt::exception("IMAG pixel data size overflows");

    required_size = stride * height;
    if(frame_.pdat.size() < required_size)
      throw fmt::exception("PDAT is truncated: {} bytes, expected at least {}",
                           frame_.pdat.size(),required_size);

    if(frame_.has_vdl)
      custom = parse_vdl(frame_.vdl,height);

    bitmap_.reset(width,height);
    for(size_t y = 0; y < height; y++)
      {
        const VDLState *state = custom.empty() ? nullptr : &custom[y];
        const CLUT &row_clut = state ? state->clut : startup;
        const size_t row_pair = ((y / 2) * stride * 2);
        const size_t row_offset = ((y & 1) * 2);

        for(size_t x = 0; x < width; x++)
          {
            const uint16_t pixel =
              u16be(frame_.pdat.data() + row_pair + (x * 4) + row_offset);
            const uint8_t red_idx   = ((pixel >> 10) & 0x1F);
            const uint8_t green_idx = ((pixel >>  5) & 0x1F);
            const uint8_t blue_idx  = ((pixel >>  0) & 0x1F);
            const bool hard_bypass =
              (state && state->clut_bypass && (pixel & 0x8000));
            RGBA8888 *out = bitmap_.xy(x,y);

            if(state && state->background && ((pixel & 0x7FFF) == 0))
              *out = row_clut.background;
            else
              {
                out->r = hard_bypass ? hard_clut_value(red_idx,*state) :
                  row_clut.red[red_idx];
                out->g = hard_bypass ? hard_clut_value(green_idx,*state) :
                  row_clut.green[green_idx];
                out->b = hard_bypass ? hard_clut_value(blue_idx,*state) :
                  row_clut.blue[blue_idx];
                out->a = 0xFF;
              }
          }
      }
  }

  static
  void
  decode_z24(const ImagFrame &frame_,
             Bitmap          &bitmap_)
  {
    const size_t width  = frame_.header.w;
    const size_t height = frame_.header.h;
    const size_t stride = frame_.header.bytesperrow;
    size_t required_size;

    if(frame_.header.pixelorder != 3)
      throw fmt::exception("pixelorder {} not supported for 32-bit IMAG",
                           frame_.header.pixelorder);
    if(frame_.has_vdl)
      throw fmt::exception("z24 IMAG must not contain a VDL chunk");
    if(width > (std::numeric_limits<size_t>::max() / 4))
      throw fmt::exception("IMAG width is too large: {}",width);
    if(stride < (width * 4))
      throw fmt::exception("bytesperrow {} is too small for z24 width {}",stride,width);
    if(stride & 3)
      throw fmt::exception("z24 IMAG bytesperrow must be divisible by four: {}",stride);
    if(height > (std::numeric_limits<size_t>::max() / stride))
      throw fmt::exception("IMAG pixel data size overflows");

    required_size = stride * height;
    if(frame_.pdat.size() < required_size)
      throw fmt::exception("PDAT is truncated: {} bytes, expected at least {}",
                           frame_.pdat.size(),required_size);

    bitmap_.reset(width,height);
    for(size_t y = 0; y < height; y++)
      {
        for(size_t x = 0; x < width; x++)
          {
            const uint8_t *encoded =
              frame_.pdat.data() + (y * stride) + (x * 4);
            const uint16_t first  = u16be(encoded + 0);
            const uint16_t second = u16be(encoded + 2);
            RGBA8888 *out = bitmap_.xy(x,y);

            out->r = ((z24_clut_value((first  >> 10) & 0x1F) +
                       z24_clut_value((second >> 10) & 0x1F)) >> 1);
            out->g = ((z24_clut_value((first  >>  5) & 0x1F) +
                       z24_clut_value((second >>  5) & 0x1F)) >> 1);
            out->b = ((z24_clut_value((first  >>  0) & 0x1F) +
                       z24_clut_value((second >>  0) & 0x1F)) >> 1);
            out->a = 0xFF;
          }
      }
  }
}

void
convert::imag_to_bitmap(cspan<u8>  data_,
                        BitmapVec &bitmaps_)
{
  ImagParts parts;

  parse_imag(data_,parts);

  for(const ImagFrame &frame : parts.frames)
    {
      Bitmap bitmap;

      validate_common(frame.header);

      switch(frame.header.bitsperpixel)
        {
        case 16:
          decode_lrform(frame,bitmap);
          break;
        case 32:
          decode_z24(frame,bitmap);
          break;
        default:
          throw fmt::exception("bitsperpixel {} not supported",
                               frame.header.bitsperpixel);
        }

      bitmaps_.emplace_back(std::move(bitmap));
    }
}
