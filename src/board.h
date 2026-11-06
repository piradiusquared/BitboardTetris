#pragma once

#include <array>
#include <cstdint>

constexpr int BOARD_COL = 10;
constexpr int BOARD_ROW = 22;

constexpr uint16_t ROW_MASK = 0x03FF;

class BitBoard {
private:
    std::array<uint16_t, BOARD_ROW> rows{};
    std::array<uint16_t, BOARD_COL> cols{};

public:
    bool is_filled(int col, int row) const;
    bool is_row_full(int row) const;

    void reset();
};

