#include "board.h"

bool BitBoard::is_filled(int col, int row) const {
    if (col < 0 || col >= BOARD_COL ||
        row < 0 || row >= BOARD_ROW) {
        return false;
    }

    return (rows[row] & (1 << col)) != 0;
}

<<<<<<< HEAD
class BitBoard {
    // Row 0..1: Hidden above ceiling
    // Row 2..21: Visible 10x20 board
    array<uint16_t, BOARD_ROW> rows = {0};
    array<uint16_t, BOARD_COL> cols = {0};

    public:
        bool is_filled(int col, int row) const {
            if (col < 0 || col >= BOARD_COL || row < 0 || row >= BOARD_ROW) return false;
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
=======
bool BitBoard::is_row_full(int row) const {
    return rows[row] == ROW_MASK;
}

void BitBoard::reset() {
    rows.fill(0);
    cols.fill(0);
}
>>>>>>> 85c9d7a (Starting refactoring)
