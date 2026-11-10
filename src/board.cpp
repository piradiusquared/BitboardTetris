#include "constants.h"

#include <array>

using namespace std;

struct BitBoard {
    // Row 0..1: Hidden above ceiling
    // Row 2..21: Visible 10x20 board
    array<uint16_t, Constants::BOARD_ROW> rows = {0};
    array<uint16_t, Constants::BOARD_COL> cols = {0};

    bool is_filled(int col, int row) const {
        if (col < 0 || col >= Constants::BOARD_COL || row < 0 || row >= Constants::BOARD_ROW) return false;
        return (rows[row] & (1 << col)) != 0;
    }

    bool is_row_full(int r) const {
        return rows[r] == 0b1111111111;
    }

    void reset() {
        rows = {0};
        cols = {0};
    }
};