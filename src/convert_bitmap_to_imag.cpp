/*
  ISC License

  Copyright (c) 2023, Antonio SJ Musumeci <trapexit@spawn.link>

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

#include "convert_bitmap_to_imag.hpp"

#include "image_control_chunk.hpp"
#include "pdat.hpp"
#include "pixel_converter.hpp"
#include "convert.hpp"

#include "fmt.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <vector>

namespace
{
  using Histogram = std::array<uint64_t,256>;

  struct ChannelPalette
  {
    std::array<uint8_t,32> values;
    std::array<uint8_t,256> mapping;
    size_t count = 0;
  };

  struct ImagePalette
  {
    ChannelPalette red;
    ChannelPalette green;
    ChannelPalette blue;
    size_t count = 0;
  };

  // Documented on-disk A_VDL record: palettePtr, dmaControl, 33 VDL
  // commands, and one filler word.
  constexpr size_t   A_VDL_COMMAND_WORDS    = 33;
  constexpr size_t   A_VDL_RECORD_WORDS     = 36;
  constexpr uint32_t VDL_DMA_CONTROL        = 0x0025C201;
  constexpr uint32_t VDL_480_RESOLUTION     = 0x00080000;
  constexpr uint32_t VDL_DISPLAY_CONTROL    = 0xC001602C;
  constexpr uint32_t VDL_VERTICAL_INTERPOLATION = 0x00000008;
  constexpr uint32_t VDL_COLOR_32           = 0x20000000;
  constexpr uint32_t VDL_NOP                = 0xE1000000;

  struct Box
  {
    int top = 0;
    int bot = 0;
    uint64_t weight = 0;
  };

  static
  void
  append_u16be(ByteVec  &data_,
               uint16_t  value_)
  {
    data_.push_back((value_ >> 8) & 0xFF);
    data_.push_back((value_ >> 0) & 0xFF);
  }

  static
  void
  refresh_mapping(ChannelPalette &palette_)
  {
    for(size_t value = 0; value < 256; value++)
      {
        size_t closest = 0;
        int closest_distance = std::abs(int(value) - int(palette_.values[0]));

        for(size_t index = 1; index < palette_.count; index++)
          {
            const int distance = std::abs(int(value) - int(palette_.values[index]));

            if(distance < closest_distance)
              {
                closest = index;
                closest_distance = distance;
              }
          }

        palette_.mapping[value] = closest;
      }
  }

  static
  Histogram
  histogram(const Bitmap &bitmap_,
            const size_t  y_begin_,
            const size_t  y_end_,
            const size_t  component_)
  {
    Histogram histogram = {};

    for(size_t y = y_begin_; y < y_end_; y++)
      {
        for(size_t x = 0; x < bitmap_.w; x++)
          {
            const uint8_t *pixel = reinterpret_cast<const uint8_t*>(bitmap_.xy(x,y));

            histogram[pixel[component_]]++;
          }
      }

    return histogram;
  }

  static
  ChannelPalette
  legacy_palette(const Histogram &histogram_)
  {
    ChannelPalette palette;
    std::array<Box,32> boxes;
    size_t unique = 0;
    uint64_t total = 0;

    for(size_t i = 0; i < palette.values.size(); i++)
      palette.values[i] = i;
    for(size_t i = 0; i < histogram_.size(); i++)
      {
        if(histogram_[i])
          unique++;
        total += histogram_[i];
      }

    if((unique == 0) || (total == 0))
      throw fmt::exception("cannot create a palette for an empty image");

    palette.count = std::min<size_t>(unique,32);
    boxes[0].top = 0;
    boxes[0].bot = 255;
    while((boxes[0].top < 255) && !histogram_[boxes[0].top])
      boxes[0].top++;
    while((boxes[0].bot > 0) && !histogram_[boxes[0].bot])
      boxes[0].bot--;
    boxes[0].weight = total;

    size_t next = 1;
    while(next < palette.count)
      {
        size_t selected = 0;

        for(size_t i = 1; i < next; i++)
          {
            const uint64_t score = boxes[i].weight * uint64_t(boxes[i].bot - boxes[i].top);
            const uint64_t selected_score = boxes[selected].weight *
              uint64_t(boxes[selected].bot - boxes[selected].top);

            if(score > selected_score)
              selected = i;
          }

        if((boxes[selected].weight == 0) ||
           (boxes[selected].top == boxes[selected].bot))
          {
            boxes[selected].weight = 0;
            continue;
          }

        uint64_t lower_weight = 0;
        uint64_t upper_weight = boxes[selected].weight;
        int split = boxes[selected].top;

        for(; split < boxes[selected].bot; split++)
          {
            if((lower_weight + histogram_[split]) >=
               (upper_weight - histogram_[split]))
              break;
            lower_weight += histogram_[split];
            upper_weight -= histogram_[split];
          }

        if((lower_weight + histogram_[split]) <= upper_weight)
          {
            lower_weight += histogram_[split];
            upper_weight -= histogram_[split];
            split++;
          }

        boxes[next].top = split;
        boxes[next].bot = boxes[selected].bot;
        boxes[next].weight = upper_weight;
        boxes[selected].bot = split - 1;
        boxes[selected].weight = lower_weight;

        while((boxes[next].top < boxes[next].bot) && !histogram_[boxes[next].top])
          boxes[next].top++;
        while((boxes[selected].bot > boxes[selected].top) &&
              !histogram_[boxes[selected].bot])
          boxes[selected].bot--;

        next++;
      }

    for(size_t i = 0; i < palette.count; i++)
      palette.values[i] = ((boxes[i].top + boxes[i].bot) >> 1);

    refresh_mapping(palette);
    return palette;
  }

  static
  uint64_t
  palette_error(const Histogram      &histogram_,
                const ChannelPalette &palette_)
  {
    uint64_t error = 0;

    for(size_t value = 0; value < histogram_.size(); value++)
      {
        const int delta = int(value) - int(palette_.values[palette_.mapping[value]]);

        error += histogram_[value] * uint64_t(delta * delta);
      }

    return error;
  }

  static
  ChannelPalette
  modern_palette(const Histogram &histogram_)
  {
    ChannelPalette current = legacy_palette(histogram_);
    uint64_t current_error = palette_error(histogram_,current);

    for(size_t iteration = 0; iteration < 32; iteration++)
      {
        std::array<uint64_t,32> sums = {};
        std::array<uint64_t,32> weights = {};
        ChannelPalette candidate = current;

        for(size_t value = 0; value < histogram_.size(); value++)
          {
            const size_t index = current.mapping[value];

            sums[index] += histogram_[value] * value;
            weights[index] += histogram_[value];
          }

        for(size_t index = 0; index < current.count; index++)
          {
            if(weights[index])
              candidate.values[index] = ((sums[index] + (weights[index] / 2)) /
                                         weights[index]);
          }

        std::stable_sort(candidate.values.begin(),
                         candidate.values.begin() + candidate.count);
        refresh_mapping(candidate);

        const uint64_t candidate_error = palette_error(histogram_,candidate);
        if(candidate_error > current_error)
          break;
        if(candidate.values == current.values)
          break;

        current = candidate;
        current_error = candidate_error;
      }

    return current;
  }

  static
  ImagePalette
  image_palette(const Bitmap                &bitmap_,
                const size_t                 y_begin_,
                const size_t                 y_end_,
                const convert::ImagPalette   algorithm_)
  {
    ImagePalette palette;
    const Histogram red   = histogram(bitmap_,y_begin_,y_end_,0);
    const Histogram green = histogram(bitmap_,y_begin_,y_end_,1);
    const Histogram blue  = histogram(bitmap_,y_begin_,y_end_,2);

    if(algorithm_ == convert::ImagPalette::MODERN)
      {
        palette.red   = modern_palette(red);
        palette.green = modern_palette(green);
        palette.blue  = modern_palette(blue);
      }
    else
      {
        palette.red   = legacy_palette(red);
        palette.green = legacy_palette(green);
        palette.blue  = legacy_palette(blue);
      }

    palette.count = std::max({palette.red.count,
                              palette.green.count,
                              palette.blue.count,
                              size_t(3)});
    return palette;
  }

  static
  uint16_t
  palette_pixel(const RGBA8888    &pixel_,
                const ImagePalette &palette_)
  {
    return ((uint16_t(palette_.red.mapping[pixel_.r]) << 10) |
            (uint16_t(palette_.green.mapping[pixel_.g]) << 5) |
            (uint16_t(palette_.blue.mapping[pixel_.b]) << 0));
  }

  static
  void
  bitmap_to_custom_lrform(const Bitmap                    &bitmap_,
                          const std::vector<ImagePalette> &palettes_,
                          const bool                       per_row_,
                          ByteVec                         &pdat_)
  {
    pdat_.reserve(bitmap_.w * bitmap_.h * 2);
    for(size_t y = 0; y < bitmap_.h; y += 2)
      {
        const ImagePalette &even_palette = palettes_[per_row_ ? y : 0];
        const ImagePalette &odd_palette  = palettes_[per_row_ ? (y + 1) : 0];

        for(size_t x = 0; x < bitmap_.w; x++)
          {
            append_u16be(pdat_,palette_pixel(*bitmap_.xy(x,y),even_palette));
            append_u16be(pdat_,palette_pixel(*bitmap_.xy(x,y+1),odd_palette));
          }
      }
  }

  static
  uint8_t
  z24_first(const uint8_t value_)
  {
    if(value_ < 128)
      return (31 - (value_ / 8));
    return (46 - (value_ / 8));
  }

  static
  uint8_t
  z24_second(const uint8_t value_)
  {
    if(value_ < 128)
      return (value_ % 8);
    return ((value_ % 8) + 8);
  }

  static
  uint16_t
  z24_pixel(const RGBA8888 &pixel_,
            const bool      second_)
  {
    const uint8_t red = second_ ? z24_second(pixel_.r) : z24_first(pixel_.r);
    const uint8_t green = second_ ? z24_second(pixel_.g) : z24_first(pixel_.g);
    const uint8_t blue = second_ ? z24_second(pixel_.b) : z24_first(pixel_.b);

    return ((uint16_t(red) << 10) |
            (uint16_t(green) << 5) |
            (uint16_t(blue) << 0));
  }

  static
  void
  bitmap_to_v480(const Bitmap &bitmap_,
                 ByteVec      &pdat_)
  {
    // A VDL_480RES display consumes the two LRFORM halfwords as the same
    // horizontal pixel in successive fields. Pair adjacent source rows so
    // field line N contains spatial rows 2N and 2N+1.
    pdat_.reserve(bitmap_.w * bitmap_.h * 2);
    for(size_t field_line = 0; field_line < (bitmap_.h / 2); field_line++)
      {
        const size_t first_field_row = (field_line * 2);
        const size_t second_field_row = (first_field_row + 1);

        for(size_t x = 0; x < bitmap_.w; x++)
          {
            append_u16be(pdat_,RGBA8888Converter::to_rgb0555(
                           bitmap_.xy(x,first_field_row)));
            append_u16be(pdat_,RGBA8888Converter::to_rgb0555(
                           bitmap_.xy(x,second_field_row)));
          }
      }
  }

  static
  void
  bitmap_to_z24(const Bitmap &bitmap_,
                ByteVec      &pdat_)
  {
    pdat_.reserve(bitmap_.w * bitmap_.h * 4);
    for(size_t y = 0; y < bitmap_.h; y++)
      {
        for(size_t x = 0; x < bitmap_.w; x++)
          {
            const RGBA8888 &pixel = *bitmap_.xy(x,y);

            append_u16be(pdat_,z24_pixel(pixel,false));
            append_u16be(pdat_,z24_pixel(pixel,true));
          }
      }
  }

  static
  void
  write_vdl_record(DataRW             &data_,
                   const ImagePalette &palette_,
                   const bool          v480_)
  {
    size_t words = 0;

    data_.u32be(0);                     words++; // palettePtr, patched by readers
    data_.u32be(VDL_DMA_CONTROL |
                (v480_ ? VDL_480_RESOLUTION : 0)); words++;
    data_.u32be(VDL_DISPLAY_CONTROL &
                (v480_ ? ~VDL_VERTICAL_INTERPOLATION : ~uint32_t(0))); words++;

    for(size_t index = 0; index < palette_.count; index++)
      {
        const uint32_t command = ((uint32_t(index) << 24) |
                                  (uint32_t(palette_.red.values[index]) << 16) |
                                  (uint32_t(palette_.green.values[index]) << 8) |
                                  (uint32_t(palette_.blue.values[index]) << 0));

        data_.u32be(command);
        words++;
      }

    // Preserve the SDK converter's color-zero alias when the 33-command
    // A_VDL has room after its display-control and palette commands.
    if(words < (2 + A_VDL_COMMAND_WORDS))
      {
        data_.u32be(VDL_COLOR_32 |
                    (uint32_t(palette_.red.values[0]) << 16) |
                    (uint32_t(palette_.green.values[0]) << 8) |
                    (uint32_t(palette_.blue.values[0]) << 0));
        words++;
      }

    while(words < (2 + A_VDL_COMMAND_WORDS))
      {
        data_.u32be(VDL_NOP);
        words++;
      }

    data_.u32be(0);                              // A_VDL filler
  }

  static
  void
  validate_bitmap(const Bitmap                       &bitmap_,
                  const convert::ImagEncodingOptions &options_)
  {
    if((bitmap_.w == 0) || (bitmap_.h == 0) || !bitmap_.d)
      throw fmt::exception("cannot encode an empty bitmap");
    if((bitmap_.w > uint64_t(std::numeric_limits<int32_t>::max())) ||
       (bitmap_.h > uint64_t(std::numeric_limits<int32_t>::max())))
      throw fmt::exception("bitmap dimensions are too large: {}x{}",bitmap_.w,bitmap_.h);
    const bool v480 = ((options_.mode == convert::ImagMode::V480) ||
                       (options_.mode == convert::ImagMode::V480_VDL) ||
                       (options_.mode == convert::ImagMode::V480_XVDL));
    const bool custom = ((options_.mode == convert::ImagMode::VDL) ||
                         (options_.mode == convert::ImagMode::XVDL) ||
                         (options_.mode == convert::ImagMode::V480_VDL) ||
                         (options_.mode == convert::ImagMode::V480_XVDL));

    if(v480 &&
       ((bitmap_.w != 320) || (bitmap_.h != 480)))
      throw fmt::exception("v480 modes require a 320x480 bitmap: {}x{}",
                           bitmap_.w,bitmap_.h);
    if((options_.mode != convert::ImagMode::Z24) && (bitmap_.h & 1))
      throw fmt::exception("16-bit LRFORM IMAG height must be even: {}",bitmap_.h);
    if((options_.palette == convert::ImagPalette::MODERN) &&
       !custom)
      throw fmt::exception("modern palettes require VDL or XVDL mode");

    const uint64_t bytes_per_pixel = (options_.mode == convert::ImagMode::Z24) ? 4 : 2;
    if(bitmap_.w > (uint64_t(std::numeric_limits<int32_t>::max()) / bytes_per_pixel))
      throw fmt::exception("bitmap row is too large: {} pixels",bitmap_.w);
    if((bitmap_.w * bitmap_.h) >
       ((std::numeric_limits<uint32_t>::max() - 8) / bytes_per_pixel))
      throw fmt::exception("bitmap pixel data is too large for an IMAG chunk");
    if((options_.mode == convert::ImagMode::XVDL) &&
       (bitmap_.h > ((std::numeric_limits<uint32_t>::max() - 12) / 160)))
      throw fmt::exception("bitmap has too many rows for a VDL chunk: {}",bitmap_.h);
  }
}

void
convert::bitmap_to_imag(const Bitmap &bitmap_,
                        DataRW       &data_,
                        const ImagEncodingOptions &options_)
{
  ByteVec pdat;
  std::vector<ImagePalette> palettes;
  const bool v480 = ((options_.mode == ImagMode::V480) ||
                     (options_.mode == ImagMode::V480_VDL) ||
                     (options_.mode == ImagMode::V480_XVDL));
  const bool z24 = (options_.mode == ImagMode::Z24);
  const bool custom = ((options_.mode == ImagMode::VDL) ||
                       (options_.mode == ImagMode::XVDL) ||
                       (options_.mode == ImagMode::V480_VDL) ||
                       (options_.mode == ImagMode::V480_XVDL));
  const bool per_row = ((options_.mode == ImagMode::XVDL) ||
                        (options_.mode == ImagMode::V480_XVDL));

  validate_bitmap(bitmap_,options_);

  if(v480 && !custom)
    {
      bitmap_to_v480(bitmap_,pdat);
    }
  else if(custom)
    {
      if(per_row)
        {
          palettes.reserve(bitmap_.h);
          for(size_t y = 0; y < bitmap_.h; y++)
            palettes.emplace_back(image_palette(bitmap_,y,y+1,options_.palette));
        }
      else
        {
          palettes.emplace_back(image_palette(bitmap_,0,bitmap_.h,options_.palette));
        }

      bitmap_to_custom_lrform(bitmap_,palettes,per_row,pdat);
    }
  else if(z24)
    {
      bitmap_to_z24(bitmap_,pdat);
    }
  else
    {
      convert::bitmap_to_uncoded_unpacked_lrform_16bpp(bitmap_,pdat);
    }

  data_.w("IMAG");              // id
  data_.u32be(sizeof(ImageControlChunk)); // chunk size
  data_.i32be(static_cast<int32_t>(bitmap_.w)); // w
  data_.i32be(static_cast<int32_t>(bitmap_.h)); // h
  data_.i32be(static_cast<int32_t>((z24 ? 4 : 2) * bitmap_.w)); // bytes per row
  data_.u8(z24 ? 32 : 16);      // bits per pixel
  data_.u8(3);                  // numcomponents
  data_.u8(1);                  // numplanes
  data_.u8(0);                  // colorspace
  data_.u8(0);                  // comptype
  data_.u8(0);                  // hvformat
  data_.u8(z24 ? 3 : 1);        // pixelorder
  data_.u8(0);                  // version

  if(custom)
    {
      const uint32_t count = static_cast<uint32_t>(palettes.size());

      data_.w("VDL ");
      data_.u32be(12 + (count * A_VDL_RECORD_WORDS * sizeof(uint32_t)));
      data_.u32be(count);
      for(const ImagePalette &palette : palettes)
        write_vdl_record(data_,palette,v480);
    }

  data_.w("PDAT");              // id
  data_.u32be(static_cast<uint32_t>(4 + 4 + pdat.size())); // chunk size
  data_.w(pdat);
}
