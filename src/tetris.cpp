#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <array>
#include "raylib.h"

#include "board.cpp"
#include "constants.h"

using namespace std;
using namespace Constants;

int get_rotate_idx(int from, int to) {
    if (from == 0 && to == 1) return 0;
    if (from == 1 && to == 0) return 1;
    if (from == 1 && to == 2) return 2;
    if (from == 2 && to == 1) return 3;
    if (from == 2 && to == 3) return 4;
    if (from == 3 && to == 2) return 5;
    if (from == 3 && to == 0) return 6;
    if (from == 0 && to == 3) return 7;

    if (from == 0 && to == 2) return 0;
    if (from == 1 && to == 3) return 1;
    if (from == 2 && to == 0) return 2;
    if (from == 3 && to == 1) return 3;

    return -1; // shouldnt happen
}

// Collision check and rotation bs
bool is_colliding(const BitBoard& board, PieceType type, int rot, int pieceX, int pieceY) {
    const PieceData& data = PIECES[type];

    for (int r = 0; r < 4; r++) {
        uint16_t row_mask = data.shape[rot][r];
        if (row_mask == 0) continue; // Skip empty piece rows

        int boardY = pieceY + r;

        // 1. Vertical Out-of-Bounds Check (Floor / Upper Ceiling)
        if (boardY < 0 || boardY >= BOARD_ROW) return true;

        // 2. Left Wall Collision Check (pieceX < 0)
        if (pieceX < 0) {
            // Mask out bits that spill past Column 0 to the left
            // e.g. if pieceX = -1, check if bit 0 is set: ((1 << 1) - 1) = 0b0001
            uint16_t left_overflow_mask = (1 << (-pieceX)) - 1;
            if (row_mask & left_overflow_mask) return true; // Collision!
        }

        // 3. Shift Piece Row to Board Column Position
        uint16_t shifted = (pieceX >= 0) ? (row_mask << pieceX) : (row_mask >> (-pieceX));

        // 4. Right Wall Collision Check (bits spilled past Column 9)
        if (shifted & ~ROW_MASK) return true;

        // 5. Placed Minos Overlap Check
        if (board.rows[boardY] & shifted) return true;
    }

    return false; // Valid placement
}

bool try_rotate(const BitBoard& board, PieceType type, int& rotation, int rotationNext, int& pieceX, int& pieceY) {
    // O cannot rotate
    if (type == PIECE_O) {
        return true;
    }

    int rotateIdx = get_rotate_idx(rotation, rotationNext);
    for (int i = 0; i < 5; i++) {
        RotationVec kick = (type == PIECE_I) ? KICKS_I[rotateIdx][i] : KICKS_STD[rotateIdx][i];
        
        int testX = pieceX + kick.x;
        int testY = pieceY + kick.y;

        if (!is_colliding(board, type, rotationNext, testX, testY)) {
            pieceX = testX;
            pieceY = testY;
            rotation = rotationNext;
            return true;
        }
    }

    return false; // cannot rotate
}


void lock_piece(BitBoard& board, uint8_t color_board[BOARD_ROW][BOARD_COL], PieceType type, int rot, int pieceX, int pieceY) {
    const PieceData& data = PIECES[type];

    int test_y = pieceY;
    while (!is_colliding(board, type, rot, pieceX, test_y + 1)) {
        test_y++;
    }

    for (int r = 0; r < 4; r++) {
        uint16_t row_mask = data.shape[rot][r];
        if (row_mask == 0) continue;

        int boardY = test_y + r;
        if (boardY < 0 || boardY >= BOARD_ROW) continue;

        // 1. Write to BitBoard using Bitwise OR (|=)
        uint16_t shifted = (pieceX >= 0) ? (row_mask << pieceX) : (row_mask >> (-pieceX));
        board.rows[boardY] |= shifted;

        // 2. Write to Color Grid
        for (int c = 0; c < 4; c++) {
            if (row_mask & (1 << c)) {
                int boardX = pieceX + c;
                if (boardX >= 0 && boardX < BOARD_COL) {
                    color_board[boardY][boardX] = (uint8_t)(type);
                }
            }
        }
    }
}

int clear_lines(BitBoard& board, uint8_t color_board[BOARD_ROW][BOARD_COL]) {
    int lines_cleared = 0;

    // Scan from bottom row (21) up to top row (0)
    for (int r = BOARD_ROW - 1; r >= 0; r--) {
        if (board.is_row_full(r)) {
            lines_cleared++;

            // Shift all rows above 'r' down by 1
            for (int y = r; y > 0; y--) {
                // Shift BitBoard row
                board.rows[y] = board.rows[y - 1];

                // Shift Color Grid row
                for (int x = 0; x < BOARD_COL; x++) {
                    color_board[y][x] = color_board[y - 1][x];
                }
            }

            // Clear top row (row 0)
            board.rows[0] = 0;
            for (int x = 0; x < BOARD_COL; x++) {
                color_board[0][x] = 7; //empty colour
            }

            // Re-check current row index (since a new row shifted down into index 'r')
            r++; 
        }
    }

    return lines_cleared;
}

// Color ID shit
constexpr uint8_t EMPTY_COLOR_ID = 7; // Index 7 = BLACK (Empty)
constexpr uint8_t TEST_COLOR_ID  = 8; // Index 8 = GRAY (For test blocks / floor)

void sync_color_board_from_bitboard(
    const BitBoard& board, 
    uint8_t color_board[BOARD_ROW][BOARD_COL],
    uint8_t default_block_color = TEST_COLOR_ID,
    uint8_t empty_color = EMPTY_COLOR_ID
) {
    for (int r = 0; r < BOARD_ROW; r++) {
        for (int c = 0; c < BOARD_COL; c++) {
            if (board.is_filled(c, r)) {
                // If the bit is filled but has no piece color assigned, set to default block color
                if (color_board[r][c] == empty_color) {
                    color_board[r][c] = default_block_color;
                }
            } else {
                // If bit is 0, clear the color to EMPTY
                color_board[r][c] = empty_color;
            }
        }
    }
}

void set_test_row(
    BitBoard& board, 
    uint8_t color_board[BOARD_ROW][BOARD_COL], 
    int row, 
    uint16_t row_mask, 
    uint8_t color_id = TEST_COLOR_ID
) {
    board.rows[row] = row_mask;

    for (int c = 0; c < BOARD_COL; c++) {
        if (row_mask & (1 << c)) {
            color_board[row][c] = color_id;
        } else {
            color_board[row][c] = EMPTY_COLOR_ID;
        }
    }
}


// -----------------------------------------------------------------------------
// GUI CONSTANTS & HELPERS
// -----------------------------------------------------------------------------
constexpr int CELL_SIZE = 30;
constexpr int MARGIN_LEFT = 100; // Room on left for stats/HOLD
constexpr int MARGIN_TOP = 60;   // 60px (2 cells) space above board for floating spawns!

// Convert BitBoard row/col to screen coordinates
int col_to_px(int col) { return MARGIN_LEFT + (col * CELL_SIZE); }
int row_to_py(int row) { return MARGIN_TOP + ((row - 2) * CELL_SIZE); }

int main() {
    const int screenWidth = MARGIN_LEFT + (BOARD_COL * CELL_SIZE) + 180;
    const int screenHeight = MARGIN_TOP + (20 * CELL_SIZE) + 40; // 20 visible rows + margins

    InitWindow(screenWidth, screenHeight, "Bitboard Tetris");
    SetTargetFPS(60);

    BitBoard board;
    uint8_t color_board[BOARD_ROW][BOARD_COL];
    // Fill whole color board with 7 (EMPTY / BLACK)
    for (int r = 0; r < BOARD_ROW; r++) {
        for (int c = 0; c < BOARD_COL; c++) color_board[r][c] = EMPTY_COLOR_ID;
    }
    // Set test floor at bottom visible row (row 21)
    set_test_row(board, color_board, 21, 0b0001111000, TEST_COLOR_ID);

    PieceType currentPiece = PIECE_S;
    int pieceX = 3;
    int pieceY = 0; 
    int rotation = 0;

    while (!WindowShouldClose()) {
        // ---------------------------------------------------------------------
        // 1. INPUTS
        // ---------------------------------------------------------------------
        if (IsKeyPressed(KEY_ONE))   { currentPiece = PIECE_I; pieceX = 3; pieceY = 0; rotation = 0; }
        if (IsKeyPressed(KEY_TWO))   { currentPiece = PIECE_J; pieceX = 3; pieceY = 0; rotation = 0; }
        if (IsKeyPressed(KEY_THREE)) { currentPiece = PIECE_L; pieceX = 3; pieceY = 0; rotation = 0; }
        if (IsKeyPressed(KEY_FOUR))  { currentPiece = PIECE_O; pieceX = 3; pieceY = 0; rotation = 0; }
        if (IsKeyPressed(KEY_FIVE))  { currentPiece = PIECE_S; pieceX = 3; pieceY = 0; rotation = 0; }
        if (IsKeyPressed(KEY_SIX))   { currentPiece = PIECE_T; pieceX = 3; pieceY = 0; rotation = 0; }
        if (IsKeyPressed(KEY_SEVEN)) { currentPiece = PIECE_Z; pieceX = 3; pieceY = 0; rotation = 0; }

        if (IsKeyDown(KEY_LEFT)) {
            if (!is_colliding(board, currentPiece, rotation, pieceX - 1, pieceY)) pieceX--;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            if (!is_colliding(board, currentPiece, rotation, pieceX + 1, pieceY)) pieceX++;
        }
        if (IsKeyDown(KEY_DOWN)) {
            if (!is_colliding(board, currentPiece, rotation, pieceX, pieceY + 1)) pieceY++;
        }
        if (IsKeyPressed(KEY_UP)) {
            int nextRot = (rotation + 1) % 4;
            try_rotate(board, currentPiece, rotation, nextRot, pieceX, pieceY);
        }
        if (IsKeyPressed(KEY_LEFT_SHIFT)) {
            int r = (rotation - 1) % 4;
            int nextRot = (r < 0) ? (r + 4) : r;
            try_rotate(board, currentPiece, rotation, nextRot, pieceX, pieceY);
        }
        if (IsKeyPressed(KEY_W)) {
            int nextRot = (rotation + 2) % 4;
            try_rotate(board, currentPiece, rotation, nextRot, pieceX, pieceY);
        }
        if (IsKeyPressed(KEY_SPACE)) {
            lock_piece(board, color_board, currentPiece, rotation, pieceX, pieceY);
            int cleared = clear_lines(board, color_board);
            currentPiece = PIECE_T; pieceX = 3; pieceY = 0; rotation = 0;
            if (is_colliding(board, currentPiece, rotation, pieceX, pieceY)) {
                cout << "Game over topped out" << "\n";
                exit(1);
            }
        }

        // ---------------------------------------------------------------------
        // 2. RENDERING
        // ---------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(GetColor(0x0C0C0CFF)); // Dark background

        for (int row = 2; row < BOARD_ROW; row++) {
            for (int col = 0; col < BOARD_COL; col++) {
                int px = col_to_px(col);
                int py = row_to_py(row);

                uint8_t color_id = color_board[row][col];

                if (color_id >= 0) { // 0 is the i piece colour
                    // Draw locked colored block!
                    DrawRectangle(px + 1, py + 1, CELL_SIZE - 2, CELL_SIZE - 2, PIECE_COLORS[color_id]);
                } else {
                    // Empty grid outline
                    DrawRectangleLines(px, py, CELL_SIZE, CELL_SIZE, GetColor(0x1F1F1FFF));
                }
            }
        }

        // B. Draw Board Outer Border Line (Around the 20 visible rows)
        DrawRectangleLinesEx(
            Rectangle{ (float)MARGIN_LEFT, (float)MARGIN_TOP, (float)(BOARD_COL * CELL_SIZE), (float)(20 * CELL_SIZE) },
            2.0f, WHITE
        );

        // C. Draw Active Falling Piece (Includes minos floating ABOVE MARGIN_TOP!)
        const PieceData& activeData = PIECES[currentPiece];
        for (int r = 0; r < 4; r++) {
            uint16_t row_mask = activeData.shape[rotation][r];
            for (int c = 0; c < 4; c++) {
                if (row_mask & (1 << c)) {
                    int drawX = col_to_px(pieceX + c);
                    int drawY = row_to_py(pieceY + r);

                    // Draw piece mino (will render above MARGIN_TOP if pieceY + r < 2)
                    DrawRectangle(drawX + 1, drawY + 1, CELL_SIZE - 2, CELL_SIZE - 2, PIECE_COLORS[currentPiece]);
                }
            }
        }

        // D. Side Info
        DrawText("1-7: Switch Piece", MARGIN_LEFT + (BOARD_COL * CELL_SIZE) + 20, 60, 14, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}