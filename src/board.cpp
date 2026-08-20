#include <stdint.h>
#include <array>
#include "board.h"

using namespace std;

constexpr int BOARD_COL = 10;
constexpr int BOARD_ROW = 22; // Row 0..1 = Hidden Spawn Rows, Row 2..21 = Visible Rows
constexpr uint16_t ROW_MASK = 0x03FF;

class Board {
private:
    array<uint16_t, BOARD_ROW> rows = {0};

public:
    bool isFull(int col, int row) {
        return true;
    };

    void lockPiece() {

    };

    bool isValidMove() {
        return true;
    };

    void reset() {
        return;
    };
};