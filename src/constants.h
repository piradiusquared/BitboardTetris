#pragma once

#include <array>
#include <cstdint>

namespace Constants {
    inline constexpr int BOARD_COL = 10;
    inline constexpr int BOARD_ROW = 22; // Row 0..1 = Hidden Spawn Rows, Row 2..21 = Visible Rows
    inline constexpr uint16_t ROW_MASK = 0x03FF;
    
}
