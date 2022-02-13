#pragma once

#include <cstdint>

#define TO_16_16(X)   (UINT32_C(X) << 16)
#define FROM_16_16(X) (UINT32_C(X) >> 16)

#define ONE_16_16 TO_16_16(1)
