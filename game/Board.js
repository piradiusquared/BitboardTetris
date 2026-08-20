import { BOARD_COLS, BOARD_ROWS } from "./Constants.js";

/**
 * stores the playfield grid and handles board-level checks
 */
export class Board {
    constructor() {
        // grid values are 0 for empty cells, or a colour index for locked blocks
        this.grid = Array.from({ length: BOARD_ROWS }, () => Array(BOARD_COLS).fill(0));
    }

    /**
     * clears every cell without rebuilding the whole board object
     */
    reset() {
        this.grid.forEach(row => row.fill(0));
    }

    /**
     * checks whether a piece matrix can sit at a board position
     */
    isValidMove(matrix, cellX, cellY) {
        for (let row = 0; row < matrix.length; row++) {
            for (let col = 0; col < matrix[row].length; col++) {
                if (matrix[row][col] === 0) continue;

                let newX = cellX + col;
                let newY = cellY + row;

                // left wall, right wall, and floor collisions
                if (newX < 0 || newX >= BOARD_COLS || newY >= BOARD_ROWS) {
                    return false;
                }

                // allow blocks above the visible board, but block filled cells inside it
                if (newY >= 0 && this.grid[newY][newX] !== 0) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * places the current piece into the board, clears full rows, and returns the clear count
     */
    lockPiece(currentPiece) {
        const matrix = currentPiece.matrix;
        for (let row = 0; row < matrix.length; row++) {
            for (let col = 0; col < matrix[row].length; col++) {
                if (matrix[row][col] !== 0) {
                    let boardY = currentPiece.y + row;
                    let boardX = currentPiece.x + col;

                    if (boardY >= 0 && boardY < BOARD_ROWS && boardX >= 0 && boardX < BOARD_COLS) {
                        this.grid[boardY][boardX] = matrix[row][col];
                    }
                }
            }
        }

        // keep rows that still contain at least one empty cell
        let initialLen = this.grid.length;
        this.grid = this.grid.filter(row => row.includes(0));
        let linesCleared = initialLen - this.grid.length;

        // add new empty rows at the top after line clears
        while (this.grid.length < BOARD_ROWS) {
            this.grid.unshift(new Array(BOARD_COLS).fill(0));
        }

        return linesCleared;
    }
}
