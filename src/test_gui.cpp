#include "raylib.h"
#include <cstdint>
#include <iostream>

// Board dimensions
constexpr int BOARD_WIDTH = 10;
constexpr int BOARD_HEIGHT = 20;
constexpr int CELL_SIZE = 30; // 30x30 pixels per mino block

// -------------------------------------------------------------
// BITBOARD GAME STATE
// -------------------------------------------------------------
struct TetrisBoard {
    // 20 rows, each using the lowest 10 bits for columns 0 to 9.
    uint16_t rows[BOARD_HEIGHT] = {0};

    // Check if a cell is filled
    bool is_filled(int col, int row) const {
        if (col < 0 || col >= BOARD_WIDTH || row < 0 || row >= BOARD_HEIGHT) return false;
        return (rows[row] & (1 << col)) != 0;
    }

    // Set a cell (fill it)
    void set_cell(int col, int row) {
        if (col >= 0 && col < BOARD_WIDTH && row >= 0 && row < BOARD_HEIGHT) {
            rows[row] |= (1 << col);
        }
    }

    // Clear a cell
    void clear_cell(int col, int row) {
        if (col >= 0 && col < BOARD_WIDTH && row >= 0 && row < BOARD_HEIGHT) {
            rows[row] &= ~(1 << col);
        }
    }

    // Fast Bitboard Line Clear Check
    void clear_full_lines() {
        for (int r = BOARD_HEIGHT - 1; r >= 0; r--) {
            // 0x03FF is binary 0000 0011 1111 1111 (10 full columns)
            if (rows[r] == 0x03FF) {
                // Shift all rows above down by 1
                for (int y = r; y > 0; y--) {
                    rows[y] = rows[y - 1];
                }
                rows[0] = 0; // Top row becomes empty
                r++; // Re-check current row index
            }
        }
    }
};

// -------------------------------------------------------------
// MAIN PROGRAM & RAYLIB GAME LOOP
// -------------------------------------------------------------
int main() {
    // Window Setup (300px board + 200px side panel for stats/queue)
    const int screenWidth = (BOARD_WIDTH * CELL_SIZE) + 200;
    const int screenHeight = BOARD_HEIGHT * CELL_SIZE;

    InitWindow(screenWidth, screenHeight, "Bitboard Tetris Engine - C++ & Raylib");
    SetTargetFPS(60);

    TetrisBoard board;

    // Seed some debug minos in the bitboard
    board.set_cell(0, 19); // Bottom left
    board.set_cell(1, 19);
    board.set_cell(2, 19);
    board.set_cell(3, 19); // A partial row
    board.set_cell(4, 18);

    while (!WindowShouldClose()) {
        // -----------------------------------------------------
        // 1. UPDATE LOGIC & INPUTS
        // -----------------------------------------------------
        if (IsKeyPressed(KEY_SPACE)) {
            // Fill a test full line at the bottom when SPACE is pressed
            board.rows[19] = 0x03FF;
        }

        if (IsKeyPressed(KEY_C)) {
            // Clear any full lines
            board.clear_full_lines();
        }

        // -----------------------------------------------------
        // 2. RENDER / DRAWING
        // -----------------------------------------------------
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF)); // Dark gray background

        // Draw the 10x20 Grid from the Bitboard
        for (int row = 0; row < BOARD_HEIGHT; row++) {
            for (int col = 0; col < BOARD_WIDTH; col++) {
                int px = col * CELL_SIZE;
                int py = row * CELL_SIZE;

                if (board.is_filled(col, row)) {
                    // Filled cell -> Draw solid colored mino block
                    DrawRectangle(px + 1, py + 1, CELL_SIZE - 2, CELL_SIZE - 2, SKYBLUE);
                } else {
                    // Empty cell -> Draw dark grid outline
                    DrawRectangleLines(px, py, CELL_SIZE, CELL_SIZE, GetColor(0x2A2A2AFF));
                }
            }
        }

        // Draw Separator Line between Game Board and UI
        DrawLine(BOARD_WIDTH * CELL_SIZE, 0, BOARD_WIDTH * CELL_SIZE, screenHeight, GRAY);

        // Draw Side Panel Text
        DrawText("BITBOARD ENGINE", 320, 20, 18, WHITE);
        DrawText("SPACE: Fill Line 19", 320, 60, 14, LIGHTGRAY);
        DrawText("C: Run Line Clear", 320, 85, 14, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}