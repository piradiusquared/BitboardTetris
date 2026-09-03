#pragma once

enum PieceType {
    PIECE_I, 
    PIECE_J, 
    PIECE_L,
    PIECE_O, 
    PIECE_S, 
    PIECE_T, 
    PIECE_Z
};

class BitBoard {
    array<uint16_t, BOARD_ROW> rows;
    array<uint16_t, BOARD_COL> cols;

    uint8_t color_board[BOARD_ROW][BOARD_COL];

    public:
        bool is_filled(int col, int row);
        bool is_row_full(int row);
        void reset();

        bool is_colliding(PieceType type, int rot, int pieceX, int pieceY);
        void lock_piece(uint8_t color_board[BOARD_ROW][BOARD_COL], PieceType type, int rot, int pieceX, int pieceY);
};

