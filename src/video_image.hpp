#pragma once

#include <cstdint>

// https://3dodev.com/documentation/file_formats/media/image/bannerscreen

class VideoImage
{
public:
  uint8_t  vi_Version;
  char     vi_Pattern[7];
  uint32_t vi_Size;
  uint16_t vi_Height;
  uint16_t vi_Width;
  uint8_t  vi_Depth;
  uint8_t  vi_Type;
  uint8_t  vi_Reserved1;
  uint8_t  vi_Reserved2;
  uint32_t vi_Reserved3;
};

static_assert(sizeof(VideoImage) == 24,"VideoImage not properly packed");
