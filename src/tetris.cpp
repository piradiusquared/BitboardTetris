#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <deque>
#include <time.h>
#include <algorithm>
#include <cmath>
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

        // Vertical Out-of-Bounds Check (Floor / Upper Ceiling)
        if (boardY < 0 || boardY >= BOARD_ROW) return true;

        // Left Wall Collision Check (pieceX < 0)
        if (pieceX < 0) {
            // Mask out bits that spill past Column 0 to the left
            // e.g. if pieceX = -1, check if bit 0 is set: ((1 << 1) - 1) = 0b0001
            uint16_t left_overflow_mask = (1 << (-pieceX)) - 1;
            if (row_mask & left_overflow_mask) return true; // Collision!
        }

        // Shift Piece Row to Board Column Position
        uint16_t shifted = (pieceX >= 0) ? (row_mask << pieceX) : (row_mask >> (-pieceX));

        // Right Wall Collision Check (bits spilled past Column 9)
        if (shifted & ~ROW_MASK) return true;

        // Placed Minos Overlap Check
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

int get_drop_y(BitBoard& board, PieceType type, int rot, int pieceX, int pieceY) {
    int test_y = pieceY;
    while (!is_colliding(board, type, rot, pieceX, test_y + 1)) {
        test_y++;
    }

    return test_y;
}

void lock_piece(BitBoard& board, uint8_t color_board[BOARD_ROW][BOARD_COL], PieceType type, int rot, int pieceX, int pieceY) {
    const PieceData& data = PIECES[type];

    int test_y = get_drop_y(board, type, rot, pieceX, pieceY);

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

// void set_test_row(
//     BitBoard& board, 
//     uint8_t color_board[BOARD_ROW][BOARD_COL], 
//     int row, 
//     uint16_t row_mask, 
//     uint8_t color_id = TEST_COLOR_ID
// ) {
//     board.rows[row] = row_mask;

//     for (int c = 0; c < BOARD_COL; c++) {
//         if (row_mask & (1 << c)) {
//             color_board[row][c] = color_id;
//         } else {
//             color_board[row][c] = EMPTY_COLOR_ID;
//         }
//     }
// }

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void fillBag(deque<PieceType> *bag) {
    int temp[7] = {0, 1, 2, 3, 4, 5, 6};


    for (int i = 6; i > 0; i--) {
        int j = rand() % (i + 1);
        swap(&temp[i], &temp[j]);   
    }

    for (int i : temp) {
        bag->push_back((PieceType) i);
    }
}


// garbage logic
struct GarbagePacket {
    int lines;
    int hole_col;
    float delayTimer;
};

struct AttackStatePlayer {
    int combo = -1;
    int b2bCount = 0;
    deque<GarbagePacket> incGarbage;
};

void update_garbage_timers(AttackStatePlayer& state, float dt) {
    for (auto& packet : state.incGarbage) {
        if (packet.delayTimer > 0.0f) {
            packet.delayTimer = max(0.0f, packet.delayTimer - dt);
        }
    }
}

int calculate_attack(AttackStatePlayer& state, int linesCleared, bool isSpin, bool isMini, bool isAllClear) {
    if (linesCleared == 0) {
        // Breaking B2B? If you want TETR.IO S2 Surge, release surge here if state.b2bChain >= 3
        state.combo = -1;
        if (state.b2bCount >= 4) {
            return state.b2bCount;            
        }
        return 0;
    }

    state.combo++;
    bool b2bEligible = (linesCleared == 4) || isSpin || isAllClear;

    // 1. Base Attack Table
    int base = 0;
    if (isSpin) {
        if (isMini) {
            base = (linesCleared == 2) ? 1 : 0; // Mini Single=0, Mini Double=1
        } else {
            if (linesCleared == 1) base = 2;      // TSS
            else if (linesCleared == 2) base = 4; // TSD
            else if (linesCleared == 3) base = 6; // TST
        }
    } else {
        if (linesCleared == 1) base = 0;      // Single
        else if (linesCleared == 2) base = 1; // Double
        else if (linesCleared == 3) base = 2; // Triple
        else if (linesCleared == 4) base = 4; // Quad
    }

    // Back-to-Back Bonus (+1 when chaining)
    int surgeBonus = 0;
    if (b2bEligible) {
        state.b2bCount++;
        if (state.b2bCount >= 2) {
            base += 1; // Standard +1 B2B bonus
        }
    } else {
        // Normal Single/Double/Triple breaks B2B (release TETR.IO S2 Surge if chain >= 3)
        if (state.b2bCount >= 3) {
            surgeBonus = state.b2bCount;
        }
        state.b2bCount = 0;
    }

    // Combo Multiplier
    int totalAttack = 0;
    if (base == 0) {
        // Singles use a gentle logarithmic scale during combos
        totalAttack = (int)floor(log1p(1.25 * state.combo));
    } else {
        float multiplier = 1.0f + (0.25f * state.combo);
        totalAttack = (int)floor(base * multiplier);
    }

    // Add Surge & Perfect Clear Bonus
    totalAttack += surgeBonus;
    if (isAllClear) {
        totalAttack += 5; // TETR.IO S2 All Clear bonus
    }

    return totalAttack;
}

void push_garbage_lines(
    BitBoard& board, 
    uint8_t color_board[BOARD_ROW][BOARD_COL], 
    int count, 
    int hole_col, 
    uint8_t garbage_color_id = TEST_COLOR_ID
) {
    for (int i = 0; i < count; i++) {
        // Shift all rows UPWARDS (towards row 0 ceiling)
        for (int r = 0; r < BOARD_ROW - 1; r++) {
            board.rows[r] = board.rows[r + 1];
            for (int c = 0; c < BOARD_COL; c++) {
                color_board[r][c] = color_board[r + 1][c];
            }
        }

        // Generate Bitmask for 9 filled blocks + 1 hole at hole_col
        uint16_t garbage_mask = ROW_MASK & ~(1 << hole_col);
        board.rows[BOARD_ROW - 1] = garbage_mask;

        // Assign GRAY color to garbage row (leaving hole_col empty)
        for (int c = 0; c < BOARD_COL; c++) {
            if (c == hole_col) {
                color_board[BOARD_ROW - 1][c] = EMPTY_COLOR_ID; // 7 (Black)
            } else {
                color_board[BOARD_ROW - 1][c] = garbage_color_id; // GRAY
            }
        }
    }
}

void resolve_garbage_and_attack(
    BitBoard& myBoard,
    uint8_t myColors[BOARD_ROW][BOARD_COL],
    AttackStatePlayer& myState,
    AttackStatePlayer& opponentState,
    int linesCleared,
    int attackGenerated
) {
    if (linesCleared > 0) {
        // Cancel our own incoming garbage first (FIFO)
        while (attackGenerated > 0 && !myState.incGarbage.empty()) {
            GarbagePacket& front = myState.incGarbage.front();
            if (attackGenerated >= front.lines) {
                attackGenerated -= front.lines;
                myState.incGarbage.pop_front();
            } else {
                front.lines -= attackGenerated;
                attackGenerated = 0;
            }
        }

        // 2. Send leftover lines to opponent as ONE clean packet (shared hole_col)
        if (attackGenerated > 0) {
            int hole = rand() % BOARD_COL;
            opponentState.incGarbage.push_back({ attackGenerated, hole, 0.5f });
        }
    } else {
        // No lines cleared -> Tank ready garbage up to the 8-line per-piece cap!
        int capRemaining = 8;

        while (capRemaining > 0 && !myState.incGarbage.empty()) {
            GarbagePacket& front = myState.incGarbage.front();

            // Stop if the front packet is still traveling in the buffer
            if (front.delayTimer > 0.0f) break;

            int linesToTank = min(front.lines, capRemaining);
            push_garbage_lines(myBoard, myColors, linesToTank, front.hole_col);

            front.lines -= linesToTank;
            capRemaining -= linesToTank;

            if (front.lines == 0) {
                myState.incGarbage.pop_front();
            }
        }
    }
}

//
// Handling
//

constexpr float DAS = 0.11f; // in ms
constexpr float ARR = 0.0f; 
constexpr float SDF = 200.0f; // <=0 for instant
constexpr float GRAVITY = 1.0f;

float dasTimer = 0.0f;
float arrTimer = 0.0f;
float gravTimer = 0.0f;
int heldDir = 0; // for das holding. -1 left, 1 right, 0 none



// -----------------------------------------------------------------------------
// GUI CONSTANTS & HELPERS
// -----------------------------------------------------------------------------
constexpr int CELL_SIZE = 30;
constexpr int MARGIN_LEFT = 160; // Room on left for stats/HOLD
constexpr int MARGIN_TOP = 60;   // 60px (2 cells) space above board for floating spawns!

constexpr int PREVIEW_CELL_SIZE = 22;
constexpr int BOX_WIDTH = 120;
constexpr int HEADER_H = 24;
constexpr int BOX_GAP = 16;


// Convert BitBoard row/col to screen coordinates
int col_to_px(int col) { return MARGIN_LEFT + (col * CELL_SIZE); }
int row_to_py(int row) { return MARGIN_TOP + ((row - 2) * CELL_SIZE); }


void draw_preview_piece(PieceType type, int centerX, int centerY, int cellSize, Color color) {
    const PieceData &data = PIECES[type];

    // Find the tight bounding box of the piece in spawn rotation (0°)
    int minC = 4, maxC = -1, minR = 4, maxR = -1;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (data.shape[0][r] & (1 << c)) {
                if (c < minC) minC = c;
                if (c > maxC) maxC = c;
                if (r < minR) minR = r;
                if (r > maxR) maxR = r;
            }
        }
    }

    // Calculate pixel width/height and top-left start position
    int widthPx  = (maxC - minC + 1) * cellSize;
    int heightPx = (maxR - minR + 1) * cellSize;
    int startX   = centerX - (widthPx / 2);
    int startY   = centerY - (heightPx / 2);

    // Draw minos
    for (int r = minR; r <= maxR; r++) {
        for (int c = minC; c <= maxC; c++) {
            if (data.shape[0][r] & (1 << c)) {
                int px = startX + (c - minC) * cellSize;
                int py = startY + (r - minR) * cellSize;
                DrawRectangle(px + 1, py + 1, cellSize - 2, cellSize - 2, color);
            }
        }
    }
}

void draw_hold_box(PieceType holdPiece, int boardLeft, int boardTop, const Color colors[]) {
    int x = boardLeft - BOX_WIDTH - BOX_GAP;
    int y = boardTop;
    int bodyH = 86;
    int totalH = HEADER_H + bodyH;

    // Background & White Header
    DrawRectangle(x, y, BOX_WIDTH, totalH, BLACK);
    DrawRectangle(x, y, BOX_WIDTH, HEADER_H, WHITE);
    DrawText("HOLD", x + 8, y + 4, 18, BLACK);

    // White Border
    DrawRectangleLinesEx(Rectangle{ (float)x, (float)y, (float)BOX_WIDTH, (float)totalH }, 2.0f, WHITE);

    // Draw held piece if one exists (-1 means empty)
    if (holdPiece >= 0 && holdPiece < 7) {
        int centerX = x + (BOX_WIDTH / 2);
        int centerY = y + HEADER_H + (bodyH / 2);
        draw_preview_piece(holdPiece, centerX, centerY, PREVIEW_CELL_SIZE, colors[holdPiece]);
    }
}

void draw_next_box(deque<PieceType> nextQueue, int boardRight, int boardTop, const Color colors[]) {
    int x = boardRight + BOX_GAP;
    int y = boardTop;
    int slotH  = 80; // Vertical height per preview piece
    int totalH = HEADER_H + (slotH * 5);

    // Background & White Header
    DrawRectangle(x, y, BOX_WIDTH, totalH, BLACK);
    DrawRectangle(x, y, BOX_WIDTH, HEADER_H, WHITE);
    DrawText("NEXT", x + 8, y + 4, 18, BLACK);

    // White Border
    DrawRectangleLinesEx(Rectangle{ (float)x, (float)y, (float)BOX_WIDTH, (float)totalH }, 2.0f, WHITE);

    // Draw all 5 upcoming pieces
    int centerX = x + (BOX_WIDTH / 2);
    for (int i = 0; i < 5; i++) {
        int centerY = y + HEADER_H + (i * slotH) + (slotH / 2);
        draw_preview_piece(nextQueue[i], centerX, centerY, PREVIEW_CELL_SIZE, colors[nextQueue[i]]);
    }
}

bool is_board_empty(const BitBoard& board) {
    for (int r = 0; r < BOARD_ROW; r++) {
        if (board.rows[r] != 0) return false;
    }
    return true;
}

void draw_garbage_meter(const AttackStatePlayer& state, int boardLeft, int boardTop) {
    int barW = 8;
    int barX = boardLeft - barW - 4; // Sits 4px to the left of the board border
    int boardBottom = boardTop + (20 * CELL_SIZE);

    // Draw dark background track for the meter
    DrawRectangle(barX, boardTop, barW, 20 * CELL_SIZE, GetColor(0x181818FF));

    // Draw packets stacking from bottom to top
    int currentY = boardBottom;
    for (const auto& packet : state.incGarbage) {
        int packetHeight = packet.lines * CELL_SIZE;

        // Clamp so the bar doesn't draw above the top of the board
        if (currentY - packetHeight < boardTop) {
            packetHeight = currentY - boardTop;
        }
        if (packetHeight <= 0) break;

        // Traveling garbage = dim Orange/Red; Ready to enter = bright Red
        Color barColor = (packet.delayTimer > 0.0f) ? Fade(ORANGE, 0.7f) : RED;

        DrawRectangle(barX, currentY - packetHeight, barW, packetHeight - 2, barColor);
        currentY -= packetHeight;
    }
}

int main() {
    srand(time(NULL)); // random needed for bag


    const int screenWidth = MARGIN_LEFT + (BOARD_COL * CELL_SIZE) + BOX_GAP + BOX_WIDTH + 32;
    const int screenHeight = MARGIN_TOP + (20 * CELL_SIZE) + 40; // 20 visible rows + margins

    // Raylib config

    InitWindow(screenWidth, screenHeight, "Bitboard Tetris");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    bool isPaused = false;
    
    // Tetris setup

    BitBoard board;
    uint8_t color_board[BOARD_ROW][BOARD_COL];
    // Fill whole color board with 7 (EMPTY / BLACK)
    for (int r = 0; r < BOARD_ROW; r++) {
        for (int c = 0; c < BOARD_COL; c++) color_board[r][c] = EMPTY_COLOR_ID;
    }
    // Set test floor at bottom visible row (row 21)
    // set_test_row(board, color_board, 21, 0b0001111000, TEST_COLOR_ID);

    PieceType holdPiece = PIECE_N; // use int now for negatives
    bool canHold = true;

    deque<PieceType> bag;
    fillBag(&bag);
    fillBag(&bag);

    PieceType currentPiece = bag.front();
    bag.pop_front();

    int pieceX = 3;
    int pieceY = 0; 
    int rotation = 0;

    AttackStatePlayer playerState;
    AttackStatePlayer opponentState;

    while (!WindowShouldClose()) {
        // ---------------------------------------------------------------------
        // 1. INPUTS
        // ---------------------------------------------------------------------
        // testing 1 to 7 for specific pieces
        // if (IsKeyPressed(KEY_ONE))   { currentPiece = PIECE_I; pieceX = 3; pieceY = 0; rotation = 0; }
        // if (IsKeyPressed(KEY_TWO))   { currentPiece = PIECE_J; pieceX = 3; pieceY = 0; rotation = 0; }
        // if (IsKeyPressed(KEY_THREE)) { currentPiece = PIECE_L; pieceX = 3; pieceY = 0; rotation = 0; }
        // if (IsKeyPressed(KEY_FOUR))  { currentPiece = PIECE_O; pieceX = 3; pieceY = 0; rotation = 0; }
        // if (IsKeyPressed(KEY_FIVE))  { currentPiece = PIECE_S; pieceX = 3; pieceY = 0; rotation = 0; }
        // if (IsKeyPressed(KEY_SIX))   { currentPiece = PIECE_T; pieceX = 3; pieceY = 0; rotation = 0; }
        // if (IsKeyPressed(KEY_SEVEN)) { currentPiece = PIECE_Z; pieceX = 3; pieceY = 0; rotation = 0; }

        if (IsKeyPressed(KEY_ESCAPE)) {
            isPaused = !isPaused;
        }

        if (!isPaused) {
            // garbage testing
            float dt = min(GetFrameTime(), 0.05f);
            update_garbage_timers(playerState, dt);

            if (IsKeyPressed(KEY_G)) {
                playerState.incGarbage.push_back({ 4, rand() % BOARD_COL, 0.5f});
            }
            if (IsKeyPressed(KEY_H)) {
                // Send 10 clean lines (great for testing the 8-line per-piece cap!)
                playerState.incGarbage.push_back({ 10, rand() % BOARD_COL, 0.5f });
            }
            if (IsKeyPressed(KEY_R)) {
                // Quick board & state reset if you top out while testing
                board.rows.fill(0);
                for (int r = 0; r < BOARD_ROW; r++)
                    for (int c = 0; c < BOARD_COL; c++) color_board[r][c] = EMPTY_COLOR_ID;
                playerState = AttackStatePlayer{};
                opponentState = AttackStatePlayer{};
            }

            // das and arr
            bool leftPress = IsKeyPressed(KEY_LEFT);
            bool rightPress = IsKeyPressed(KEY_RIGHT);
            bool leftDown = IsKeyDown(KEY_LEFT);
            bool rightDown = IsKeyDown(KEY_RIGHT);

            if (leftPress) {
                if (!is_colliding(board, currentPiece, rotation, pieceX - 1, pieceY)) pieceX--;
                heldDir = -1;
                dasTimer = 0.0f;
                arrTimer = 0.0f;
            } else if (rightPress) {
                if (!is_colliding(board, currentPiece, rotation, pieceX + 1, pieceY)) pieceX++;
                heldDir = 1;
                dasTimer = 0.0f;
                arrTimer = 0.0f;
            }

            if (heldDir == -1 && !leftDown)  heldDir = rightDown ? 1 : 0;
            if (heldDir ==  1 && !rightDown) heldDir = leftDown ? -1 : 0;

            if (heldDir == 0) {
                dasTimer = 0.0f;
                arrTimer = 0.0f;
            } else {
                dasTimer += dt;
                if (dasTimer >= DAS) {
                    if (ARR <= 0.0f) {
                        // 0 ARR: Instant snap to wall
                        while (!is_colliding(board, currentPiece, rotation, pieceX + heldDir, pieceY)) {
                            pieceX += heldDir;
                        }
                    } else {
                        arrTimer += dt;
                        while (arrTimer >= ARR) {
                            arrTimer -= ARR;
                            if (!is_colliding(board, currentPiece, rotation, pieceX + heldDir, pieceY)) {
                                pieceX += heldDir;
                            }
                        }
                    }
                }
            }

            // sdf and gravity logic
            if (IsKeyDown(KEY_DOWN) && SDF == 0.0f) {
                pieceY = get_drop_y(board, currentPiece, rotation, pieceX, pieceY);
                gravTimer = 0.0f;
            } else {
                float sdfRate = IsKeyDown(KEY_DOWN) ? SDF : 1.0f;
                gravTimer += dt * sdfRate;

                while (gravTimer >= GRAVITY) {
                    gravTimer -= gravTimer;
                    if (!is_colliding(board, currentPiece, rotation, pieceX, pieceY + 1)) {
                        pieceY++;
                    } else {
                        // add piece lock logic here
                        break;
                    }
                    
                }
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
                canHold = true;
                
                int cleared = clear_lines(board, color_board);
                bool allClear = is_board_empty(board);
                bool isSpin = false;
                bool isMini = false;

                int attack = calculate_attack(playerState, cleared, isSpin, isMini, allClear);
                resolve_garbage_and_attack(board, color_board, playerState, opponentState, cleared, attack);

                if (bag.size() <= 7) {
                    fillBag(&bag);
                }
                currentPiece = bag.front();
                bag.pop_front();
                pieceX = 3; pieceY = 0; rotation = 0;
                if (is_colliding(board, currentPiece, rotation, pieceX, pieceY)) {
                    cout << "Game over topped out" << "\n";
                    exit(1);
                }
            }

            if (IsKeyPressed(KEY_A)) {
                if (canHold) {
                    if (holdPiece == PIECE_N) {
                        holdPiece = currentPiece;
                        currentPiece = bag.front();
                        bag.pop_front();
                    } else {
                        PieceType temp = currentPiece;
                        currentPiece = holdPiece;
                        holdPiece = temp;
                    }
                    pieceX = 3; pieceY = 0; rotation = 0;
                    canHold = false;
                }
            }

        }


        // ---------------------------------------------------------------------
        // 2. RENDERING
        // ---------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(GetColor(0x0C0C0CFF)); // Dark background

        int boardRight = MARGIN_LEFT + (BOARD_COL * CELL_SIZE);

        draw_hold_box(holdPiece, MARGIN_LEFT, MARGIN_TOP, PIECE_COLORS);
        draw_next_box(bag, boardRight, MARGIN_TOP, PIECE_COLORS);
        draw_garbage_meter(playerState, MARGIN_LEFT, MARGIN_TOP);

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

        // Draw Board Outer Border Line (Around the 20 visible rows)
        DrawRectangleLinesEx(
            Rectangle{ (float)MARGIN_LEFT, (float)MARGIN_TOP, (float)(BOARD_COL * CELL_SIZE), (float)(20 * CELL_SIZE) },
            2.0f, WHITE
        );

        // Draw Active Falling Piece
        const PieceData& activeData = PIECES[currentPiece];
        int ghostY = get_drop_y(board, currentPiece, rotation, pieceX, pieceY);
        for (int r = 0; r < 4; r++) {
            uint16_t row_mask = activeData.shape[rotation][r];
            for (int c = 0; c < 4; c++) {
                if (row_mask & (1 << c)) {
                    int drawX = col_to_px(pieceX + c);
                    int drawY = row_to_py(pieceY + r);
                    int drawGhost = row_to_py(ghostY + r);

                    // Draw piece mino (will render above MARGIN_TOP if pieceY + r < 2)
                    DrawRectangle(drawX + 1, drawY + 1, CELL_SIZE - 2, CELL_SIZE - 2, PIECE_COLORS[currentPiece]);
                    DrawRectangle(drawX + 1, drawGhost + 1, CELL_SIZE - 2, CELL_SIZE - 2, Fade(PIECE_COLORS[currentPiece], 0.25f));
                }
            }
        }


        // DrawText("1-7: Switch Piece", MARGIN_LEFT + (BOARD_COL * CELL_SIZE) + 20, 60, 14, LIGHTGRAY);
        int totalSent = 0;
        for (const auto& p : opponentState.incGarbage) totalSent += p.lines;

        // Draw live stats below the Hold box
        int statX = MARGIN_LEFT - BOX_WIDTH - BOX_GAP;
        int statY = MARGIN_TOP + 140;
        DrawText(TextFormat("COMBO: %d", max(0, playerState.combo)), statX, statY, 16, WHITE);
        DrawText(TextFormat("B2B: %d", playerState.b2bCount), statX, statY + 25, 16, GOLD);
        DrawText(TextFormat("SENT: %d", totalSent), statX, statY + 50, 16, SKYBLUE);

        if (isPaused) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.75f));

            const char* title = "PAUSED";
            int titleWidth = MeasureText(title, 40);
            DrawText(title, (screenWidth - titleWidth) / 2, (screenHeight / 2) - 30, 40, WHITE);

            const char* hint = "Press ESC to Resume";
            int hintWidth = MeasureText(hint, 18);
            DrawText(hint, (screenWidth - hintWidth) / 2, (screenHeight / 2) + 20, 18, LIGHTGRAY);
        }
        
        EndDrawing();
    }

    CloseWindow();
    return 0;
}