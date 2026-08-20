import { BLOCK_SIZE, TETROMINOES, COLOURS, HIDDEN_ROWS, BOARD_COLS, VISIBLE_ROWS } from "./Constants.js";

/**
 * handles all canvas drawing for the board, hold, and next queue
 */
export class Renderer {
    constructor() {
        this.boardCanvas = document.getElementById('tetrisBoard');
        this.boardContext = this.boardCanvas.getContext('2d');

        this.holdCanvas = document.getElementById('tetrisHold');
        this.holdContext = this.holdCanvas.getContext('2d');

        this.nextCanvas = document.getElementById('tetrisNext');
        this.nextContext = this.nextCanvas.getContext('2d');
    }

    /**
     * finds the smallest rectangle that contains the non-empty cells of a piece
     */
    getPieceBounds(matrix) {
        let minX = Infinity;
        let maxX = -Infinity;
        let minY = Infinity;
        let maxY = -Infinity;

        for (let y = 0; y < matrix.length; y++) {
            for (let x = 0; x < matrix[y].length; x++) {
                if (matrix[y][x] > 0) {
                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                }
            }
        }

        // empty fallback, mostly defensive
        if (minX === Infinity) {
            return { minX: 0, maxX: 0, minY: 0, maxY: 0, width: 0, height: 0 };
        }

        return {
            minX,
            maxX,
            minY,
            maxY,
            width: maxX - minX + 1,
            height: maxY - minY + 1
        };
    }

    /**
     * draws a piece matrix onto a canvas context
     */
    drawPiece(context, matrix, x, y, ghost = false, yOffset = 0) {
        if (!matrix) return;

        matrix.forEach((row, j) => {
            row.forEach((value, i) => {
                if (value <= 0) return;

                const drawX = (x + i) * BLOCK_SIZE;
                const drawY = (y + j - yOffset) * BLOCK_SIZE;

                // skip cells outside the visible canvas
                if (drawY + BLOCK_SIZE <= 0) return;
                if (drawY >= context.canvas.height) return;
                if (drawX + BLOCK_SIZE <= 0) return;
                if (drawX >= context.canvas.width) return;

                context.fillStyle = ghost
                    ? 'rgba(255, 255, 255, 0.2)'
                    : COLOURS[value];

                context.fillRect(
                    drawX,
                    drawY,
                    BLOCK_SIZE - 1,
                    BLOCK_SIZE - 1
                );
            });
        });
    }

    /**
     * draws locked blocks already stored in the board grid
     */
    drawBoard(context, grid) {
        grid.forEach((row, y) => {
            row.forEach((value, x) => {
                if (value <= 0) return;

                // hidden rows exist in logic but are shifted above the canvas
                const drawY = (y - HIDDEN_ROWS) * BLOCK_SIZE;
                if (drawY + BLOCK_SIZE <= 0) return;
                if (drawY >= context.canvas.height) return;

                context.fillStyle = COLOURS[value];
                context.fillRect(
                    x * BLOCK_SIZE,
                    drawY,
                    BLOCK_SIZE - 1,
                    BLOCK_SIZE - 1
                );
            });
        });
    }

    /**
     * draws the faint board grid lines
     */
    drawGrid(context) {
        context.save();
        context.strokeStyle = 'rgba(255, 255, 255, 0.08)';
        context.lineWidth = 1;

        for (let x = 0; x <= BOARD_COLS; x++) {
            const px = x * BLOCK_SIZE + 0.5;
            context.beginPath();
            context.moveTo(px, 0);
            context.lineTo(px, VISIBLE_ROWS * BLOCK_SIZE);
            context.stroke();
        }

        for (let y = 0; y <= VISIBLE_ROWS; y++) {
            const py = y * BLOCK_SIZE + 0.5;
            context.beginPath();
            context.moveTo(0, py);
            context.lineTo(BOARD_COLS * BLOCK_SIZE, py);
            context.stroke();
        }

        context.restore();
    }

    /**
     * centers a piece inside a preview box, used for hold and next
     */
    drawPreviewPiece(context, matrix, boxX, boxY, boxWidth, boxHeight, ghost = false) {
        if (!matrix) return;

        const bounds = this.getPieceBounds(matrix);

        const piecePixelWidth = bounds.width * BLOCK_SIZE;
        const piecePixelHeight = bounds.height * BLOCK_SIZE;

        const offsetX = boxX + (boxWidth - piecePixelWidth) / 2;
        const offsetY = boxY + (boxHeight - piecePixelHeight) / 2;

        for (let y = bounds.minY; y <= bounds.maxY; y++) {
            for (let x = bounds.minX; x <= bounds.maxX; x++) {
                const value = matrix[y][x];
                if (value <= 0) continue;

                const drawX = offsetX + (x - bounds.minX) * BLOCK_SIZE;
                const drawY = offsetY + (y - bounds.minY) * BLOCK_SIZE;

                context.fillStyle = ghost
                    ? 'rgba(255, 255, 255, 0.18)'
                    : COLOURS[value];

                context.fillRect(
                    drawX,
                    drawY,
                    BLOCK_SIZE - 1,
                    BLOCK_SIZE - 1
                );
            }
        }
    }

    /**
     * draws the held piece, dimmed if hold was already used this turn
     */
    drawHold(game) {
        if (!this.holdContext) return;

        if (game.holdPiece) {
            this.drawPreviewPiece(
                this.holdContext,
                TETROMINOES[game.holdPiece],
                0,
                0,
                this.holdCanvas.width,
                this.holdCanvas.height,
                game.holdUsed
            );
        }
    }

    /**
     * draws the next five pieces in the queue
     */
    drawNext(game) {
        if (!this.nextContext) return;

        const previewCount = 5;
        const sectionHeight = this.nextCanvas.height / previewCount;

        for (let i = 0; i < previewCount; i++) {
            const type = game.bag[i];
            if (!type) continue;

            this.drawPreviewPiece(
                this.nextContext,
                TETROMINOES[type],
                0,
                i * sectionHeight,
                this.nextCanvas.width,
                sectionHeight,
                false
            );
        }
    }

    /**
     * fills a canvas black before redrawing the next frame
     */
    clearCanvas(context) {
        context.fillStyle = '#000';
        context.fillRect(0, 0, context.canvas.width, context.canvas.height);
    }

    /**
     * redraws the whole game state for the current frame
     */
    draw(game) {
        this.clearCanvas(this.boardContext);
        this.clearCanvas(this.holdContext);
        this.clearCanvas(this.nextContext);

        this.drawGrid(this.boardContext);
        this.drawBoard(this.boardContext, game.board.grid);

        const piece = game.currentPiece;
        if (piece) {
            // ghost piece is just the active piece drawn at its drop position
            this.drawPiece(
                this.boardContext,
                piece.matrix,
                piece.x,
                piece.getDropY(game.board),
                true,
                HIDDEN_ROWS
            );

            this.drawPiece(
                this.boardContext,
                piece.matrix,
                piece.x,
                piece.y,
                false,
                HIDDEN_ROWS
            );
        }

        this.drawHold(game);
        this.drawNext(game);
    }
}
