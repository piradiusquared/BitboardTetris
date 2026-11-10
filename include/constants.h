#pragma once

#include <cstdint>
#include <array>
#include "raylib.h"

using namespace std;

namespace Constants {
    inline constexpr int BOARD_COL = 10;
    inline constexpr int BOARD_ROW = 22; // Row 0..1 = Hidden Spawn Rows, Row 2..21 = Visible Rows
    inline constexpr uint16_t ROW_MASK = 0x03FF;
};

enum PieceType {
    PIECE_I = 0, 
    PIECE_J, 
    PIECE_L,
    PIECE_O,
    PIECE_S, 
    PIECE_T, 
    PIECE_Z
};

const Color PIECE_COLORS[9] = {
    SKYBLUE,    // 0 = I-Piece
    BLUE,       // 1 = J-Piece
    ORANGE,     // 2 = L-Piece
    YELLOW,     // 3 = O-Piece
    GREEN,      // 4 = S-Piece
    PURPLE,     // 5 = T-Piece
    RED,        // 6 = Z-Piece
    BLACK,      // 7 = Empty cell
    GRAY        // 8 = garbage
};

// Piece data. note: LSB
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


// Rotation data
struct RotationVec {int x; int y; };

// kick table bullshit
const RotationVec KICKS_STD[8][5] = {
    {{0, 0}, {-1, 0}, {-1, -1}, {0, 2},  {-1, 2}},
    {{0, 0}, {1, 0},  {1, 1},   {0, -2}, {1, -2}},
    {{0, 0}, {1, 0},  {1, 1},   {0, -2}, {1, -2}},
    {{0, 0}, {-1, 0}, {-1, -1}, {0, 2},  {-1, 2}},

    {{0, 0}, {1, 0},  {1, -1},  {0, 2},  {1, 2}},
    {{0, 0}, {-1, 0}, {-1, 1},  {0, -2},  {-1, -2}},
    {{0, 0}, {-1, 0}, {-1, 1},  {0, -2},  {-1, -2}},
    {{0, 0}, {1, 0},  {1, -1},  {0, 2},  {1, 2}}
};

// SRS+ I table
const RotationVec KICKS_I[8][5] = {
    {{0, 0}, {1, 0},   {-2, 0}, {-2, -1}, {1, 2}},
    {{0, 0}, {-1, 0},  {2, 0},  {-1, -2}, {2, 1}},
    {{0, 0}, {-1, 0},  {2, 0},  {-1, 2},  {2, -1}},
    {{0, 0}, {-2, 0},  {1, -0}, {-2, 1},  {1, -2}},

    {{0, 0}, {2, 0},  {-1, 0},  {2, 1},   {-1, -2}},
    {{0, 0}, {1, 0},  {-2, 0},  {1, 2},   {-2, -1}},
    {{0, 0}, {1, 0},  {-2, 0},  {1, -2},  {-2, 1}},
    {{0, 0}, {-1, 0}, {-2, 0},  {2, -1},  {-1, 2}}
};

const RotationVec KICKS_OE[4][5] = {
    {{0, 0}, {0, -1}, {1, -1},  {-1, -1}, {0, 1}},
    {{0, 0}, {1, 0},  {1, -2},  {1, 1},   {0, -2}},
    {{0, 0}, {0, 1},  {-1, 1},  {1, 1},   {0, -1}},
    {{0, 0}, {-1, 0}, {-1, -2}, {-1, 1},  {0, -2}}
};