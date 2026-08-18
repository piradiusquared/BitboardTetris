#include <cstdint>
#include <array>
#include "raylib.h"

using namespace std;

// -----------------------------------------------------------------------------
// CONSTANTS & BITBOARD STRUCT
// -----------------------------------------------------------------------------
constexpr int BOARD_COL = 10;
constexpr int BOARD_ROW = 22; // Row 0..1 = Hidden Spawn Rows, Row 2..21 = Visible Rows
constexpr uint16_t ROW_MASK = 0x03FF;

struct BitBoard {
    // Row 0..1: Hidden above ceiling
    // Row 2..21: Visible 10x20 board
    array<uint16_t, BOARD_ROW> rows = {0};

    bool is_filled(int col, int row) const {
        if (col < 0 || col >= BOARD_COL || row < 0 || row >= BOARD_ROW) return false;
        return (rows[row] & (1 << col)) != 0;
    }
};

// -----------------------------------------------------------------------------
// PIECE DEFINITIONS (LSB-First)
// -----------------------------------------------------------------------------
enum PieceType {
    PIECE_I = 0, PIECE_J = 1, PIECE_L = 2,
    PIECE_O = 3, PIECE_S = 4, PIECE_T = 5, PIECE_Z = 6
};

struct PieceData {
    uint16_t shape[4][4];
};

constexpr array<PieceData, 7> PIECES = {{
    // 0. I-PIECE
    {{{0b0000, 0b1111, 0b0000, 0b0000},
      {0b0100, 0b0100, 0b0100, 0b0100},
      {0b0000, 0b0000, 0b1111, 0b0000},
      {0b0010, 0b0010, 0b0010, 0b0010}}},

    // 1. J-PIECE
    {{{0b0001, 0b0111, 0b0000, 0b0000},
      {0b0110, 0b0010, 0b0010, 0b0000},
      {0b0000, 0b0111, 0b0100, 0b0000},
      {0b0010, 0b0010, 0b0011, 0b0000}}},

    // 2. L-PIECE
    {{{0b0100, 0b0111, 0b0000, 0b0000},
      {0b0010, 0b0010, 0b0110, 0b0000},
      {0b0000, 0b0111, 0b0001, 0b0000},
      {0b0011, 0b0010, 0b0010, 0b0000}}},

    // 3. O-PIECE
    {{{0b0110, 0b0110, 0b0000, 0b0000},
      {0b0110, 0b0110, 0b0000, 0b0000},
      {0b0110, 0b0110, 0b0000, 0b0000},
      {0b0110, 0b0110, 0b0000, 0b0000}}},

    // 4. S-PIECE
    {{{0b0110, 0b0011, 0b0000, 0b0000},
      {0b0010, 0b0110, 0b0100, 0b0000},
      {0b0000, 0b0110, 0b0011, 0b0000},
      {0b0001, 0b0011, 0b0010, 0b0000}}},

    // 5. T-PIECE
    {{{0b0010, 0b0111, 0b0000, 0b0000},
      {0b0010, 0b0110, 0b0010, 0b0000},
      {0b0000, 0b0111, 0b0010, 0b0000},
      {0b0010, 0b0011, 0b0010, 0b0000}}},

    // 6. Z-PIECE
    {{{0b0011, 0b0110, 0b0000, 0b0000},
      {0b0100, 0b0110, 0b0010, 0b0000},
      {0b0000, 0b0011, 0b0110, 0b0000},
      {0b0010, 0b0011, 0b0001, 0b0000}}}
}};

// Collision Check Function
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

    InitWindow(screenWidth, screenHeight, "Bitboard Tetris - TETR.IO Style Spawning");
    SetTargetFPS(60);

    BitBoard board;
    // Set test floor at bottom visible row (row 21)
    board.rows[21] = 0b0011111100;

    // TETR.IO Spawn Parameters:
    // pieceX = 3 centers 3-wide & 4-wide pieces horizontally
    // pieceY = 0 places piece in the 2 hidden spawn rows above the ceiling!
    PieceType currentPiece = PIECE_S;
    int pieceX = 3;
    int pieceY = 0; 
    int rotation = 0;

    const Color PIECE_COLORS[7] = {
        SKYBLUE, BLUE, ORANGE, YELLOW, GREEN, PURPLE, RED
    };

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

        if (IsKeyPressed(KEY_LEFT)) {
            if (!is_colliding(board, currentPiece, rotation, pieceX - 1, pieceY)) pieceX--;
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            if (!is_colliding(board, currentPiece, rotation, pieceX + 1, pieceY)) pieceX++;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            if (!is_colliding(board, currentPiece, rotation, pieceX, pieceY + 1)) pieceY++;
        }
        if (IsKeyPressed(KEY_UP)) {
            int nextRot = (rotation + 1) % 4;
            if (!is_colliding(board, currentPiece, nextRot, pieceX, pieceY)) rotation = nextRot;
        }

        // ---------------------------------------------------------------------
        // 2. RENDERING
        // ---------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(GetColor(0x0C0C0CFF)); // Dark background

        // A. Draw 20 Visible Grid Cells
        for (int row = 2; row < BOARD_ROW; row++) {
            for (int col = 0; col < BOARD_COL; col++) {
                int px = col_to_px(col);
                int py = row_to_py(row);

                if (board.is_filled(col, row)) {
                    DrawRectangle(px + 1, py + 1, CELL_SIZE - 2, CELL_SIZE - 2, GRAY);
                } else {
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
        DrawText("TETR.IO SPAWN TEST", MARGIN_LEFT + (BOARD_COL * CELL_SIZE) + 20, 20, 16, WHITE);
        DrawText("1-7: Switch Piece", MARGIN_LEFT + (BOARD_COL * CELL_SIZE) + 20, 60, 14, LIGHTGRAY);
        DrawText("Notice piece floating", MARGIN_LEFT + (BOARD_COL * CELL_SIZE) + 20, 90, 14, GREEN);
        DrawText("above top white border!", MARGIN_LEFT + (BOARD_COL * CELL_SIZE) + 20, 110, 14, GREEN);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}