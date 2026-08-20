import { TETROMINOES, KICKS_180, KICKS_I, KICKS_SQ } from "./Constants.js";

/**
 * represents the active falling tetromino
 */
export class Piece {
    constructor(type) {
        this.type = type;

        // clone the shape so rotations do not mutate the shared tetromino data
        this.matrix = TETROMINOES[type].map(row => [...row]);
        this.x = 3;
        this.y = 0;
        this.rotation = 0;
    }

    /**
     * returns a rotated copy of the matrix without moving the actual piece yet
     */
    rotateMatrix(direction) {
        const matrix = this.matrix;

        // the O piece visually stays the same when rotated
        if (this.type === 'O') return matrix.map(row => [...row]);

        if (direction === "CLOCKWISE") {
            // transpose the matrix, then reverse each column
            return matrix[0].map((_, i) => matrix.map(row => row[i]).reverse());
        } else if (direction === "ANTICLOCKWISE") {
            // transpose in the opposite direction for counter-clockwise rotation
            return matrix[0].map((_, i) => matrix.map(row => row[row.length - 1 - i]));
        } else if (direction === "180") {
            return matrix.slice().reverse().map(row => row.slice().reverse());
        }
        return matrix;
    }

    /**
     * tries a rotation, then tests wall kick offsets until one fits
     */
    rotateWithKick(direction, board) {
        const stateMods = { "CLOCKWISE": 1, "ANTICLOCKWISE": 3, "180": 2 };
        const nextState = (this.rotation + stateMods[direction]) % 4;
        const nextMatrix = this.rotateMatrix(direction);

        // kick tables are keyed by old rotation state and new rotation state
        const lookupKey = `${this.rotation}${nextState}`;
        const tests = direction === "180" ? (KICKS_180[lookupKey] || [[0, 0]]) :
            ((this.type === 'I' ? KICKS_I : KICKS_SQ)[lookupKey] || [[0, 0]]);

        // first valid kick wins, which matches how SRS-style tests work
        for (const [offsetX, offsetY] of tests) {
            if (board.isValidMove(nextMatrix, this.x + offsetX, this.y + offsetY)) {
                this.matrix = nextMatrix;
                this.x += offsetX;
                this.y += offsetY;
                this.rotation = nextState;
                return true;
            }
        }
        return false;
    }

    /**
     * finds the lowest y position for ghost and hard drop
     */
    getDropY(board) {
        let testY = this.y;
        while (board.isValidMove(this.matrix, this.x, testY + 1)) testY++;
        return testY;
    }
}
