#pragma once

#include <TFT_eSPI.h>

namespace theme
{
    constexpr uint16_t background = 0xF7DF; // near-white, faint blue tint (#F0F8FF)
    constexpr uint16_t accent     = 0x03B9; // strong blue - labels, graph line (#0077C8)
    constexpr uint16_t line       = 0xB6DF; // soft blue - separators, borders (#B3D9FF)
    constexpr uint16_t text       = 0x018C; // dark navy - values, readable on light bg (#003366)
}
