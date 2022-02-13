#pragma once

#include "stb_image.h"

#include "bitmap.hpp"
#include "bytevec.hpp"
#include "char4literal.hpp"
#include "pdat.hpp"

#include <filesystem>

#define STBI_FILE_TYPE_BMP     CHAR4LITERAL('B','M','P',' ')
#define STBI_FILE_TYPE_PNG     CHAR4LITERAL('P','N','G',' ')
#define STBI_FILE_TYPE_GIF     CHAR4LITERAL('G','I','F',' ')
#define STBI_FILE_TYPE_JPG     CHAR4LITERAL('J','P','E','G')
#define STBI_FILE_TYPE_PSD     CHAR4LITERAL('P','S','D',' ')
#define STBI_FILE_TYPE_PIC     CHAR4LITERAL('P','I','C',' ')
#define STBI_FILE_TYPE_PNM     CHAR4LITERAL('P','N','M',' ')
#define STBI_FILE_TYPE_HDR     CHAR4LITERAL('H','D','R',' ')
#define STBI_FILE_TYPE_TGA     CHAR4LITERAL('T','G','A',' ')
#define STBI_FILE_TYPE_UNKNOWN CHAR4LITERAL('?','?','?','?')


void     stbi_load(const std::filesystem::path &filepath,
                   Bitmap                      &bitmap);
void     stbi_load(cPDAT   data,
                   Bitmap &bitmap);
int      stbi_write(const Bitmap                &bitmap,
                    const std::filesystem::path &filepath,
                    const std::string            format,
                    const bool                   append_extension);
uint32_t stbi_identify(cPDAT data);
