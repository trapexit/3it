#pragma once

#include <cstdint>

#define TO_12_20(X)   (UINT32_C(X) << 20)
#define FROM_12_20(X) (UINT32_C(X) >> 20)

#define ONE_12_20 TO_12_20(1)
