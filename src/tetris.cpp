#include <cstdint>
#include <array>

using namespace std;

// bitboard
constexpr int BOARD_COL = 10;
constexpr int BOARD_ROW = 22; // 2 hidden, 20 visible
constexpr uint16_t ROW_MASK = 0x03FF;

struct BitBoard
{
    array<uint16_t, BOARD_ROW> rows = {0};

    bool is_row_full(int row) const {
        return (rows[row] & ROW_MASK) == ROW_MASK;
    }
};

// Pieces
enum PieceType { PIECE_I, PIECE_O, PIECE_J, PIECE_L, PIECE_S, PIECE_Z, PIECE_T };

struct PieceShapes {
    // 3 by 4 matrix, according to wiki
    uint16_t shapeN[3][3];
    uint16_t shapeI[4][4];
    uint16_t shapeO[3][4];
};
