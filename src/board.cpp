#include "board.h"

bool BitBoard::is_filled(int col, int row) const {
    if (col < 0 || col >= BOARD_COL ||
        row < 0 || row >= BOARD_ROW) {
        return false;
    }

    return (rows[row] & (1 << col)) != 0;
}

bool BitBoard::is_row_full(int row) const {
    return rows[row] == ROW_MASK;
}

void BitBoard::reset() {
    rows.fill(0);
    cols.fill(0);
}
