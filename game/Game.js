import { TETROMINOES, SETTINGS, getActionKeybinds, getKeyLabel } from "./Constants.js";
import { Board } from "./Board.js";
import { Piece } from "./Piece.js";
import { Renderer } from "./Renderer.js";
import { Input } from "./Input.js";

// one object keeps the rules for each game mode in one place
const GAME_MODES = {
    line: {
        label: "40L",
        targetLines: 40,
        durationMs: null
    },
    ultra: {
        label: "ULTRA",
        targetLines: null,
        durationMs: 120000 // 2 minutes
    }
};

/**
 * main game controller: owns state, scoring, timing, and the game loop
 */
export class Game {
    constructor(mode = "line") {
        // fall back to 40L if localStorage has an unknown mode
        this.mode = GAME_MODES[mode] ? mode : "line";
        this.modeSettings = GAME_MODES[this.mode];

        this.board = new Board();
        this.renderer = new Renderer();
        this.input = new Input(this);

        this.dropCounter = 0;
        this.dropInterval = 1000;
        this.lastTime = 0;
        this.totalPlayTimeMs = 0;
        this.lockTimer = 0;

        // lock resets stop grounded pieces from being moved forever
        this.lockResetCount = 0;
        this.maxLockResets = 15;

        this.score = 0;
        this.piecesPlaced = 0;
        this.linesCleared = 0;
        this.isGameOver = false;
        this.isPaused = false;
        this.isQuitPromptOpen = false;
        this.wasPausedBeforeQuit = false;
        this.quitHoldDurationMs = 1200;
        this.quitHoldStart = null;
        this.quitHoldFrame = null;

        // used by the simple t-spin check
        this.lastMoveWasRotation = false;

        // cache DOM elements so the loop is not querying them every frame
        this.scoreValEl = document.getElementById("scoreVal");
        this.ppsValEl = document.getElementById("ppsVal");
        this.modeValEl = document.getElementById("modeVal");
        this.timeValEl = document.getElementById("timeVal");
        this.linesValEl = document.getElementById("linesVal");
        this.msgEl = document.getElementById("clearMessage");

        this.gameEndBackdrop = document.getElementById("game-end-backdrop");
        this.gameEndTitle = document.getElementById("gameEndTitle");
        this.gameEndMode = document.getElementById("gameEndMode");
        this.gameEndScore = document.getElementById("gameEndScore");
        this.gameEndTime = document.getElementById("gameEndTime");
        this.gameEndLines = document.getElementById("gameEndLines");
        this.replayButton = document.getElementById("replayGame");

        this.pauseBackdrop = document.getElementById("pause-backdrop");
        this.quitBackdrop = document.getElementById("quit-backdrop");
        this.quitProgressBar = document.getElementById("quitProgressBar");

        this.pauseControlButton = document.getElementById("pauseGameControl");
        this.resetControlButton = document.getElementById("resetGameControl");
        this.exitControlButton = document.getElementById("exitGameControl");
        this.continueButton = document.getElementById("continueGame");
        this.resetPausedButton = document.getElementById("resetPausedGame");
        this.cancelQuitButton = document.getElementById("cancelQuit");

        if (this.replayButton) {
            this.replayButton.addEventListener("click", () => {
                window.location.reload();
            });
        }

        this.bindGameControls();
        this.updateControlHints();

        window.addEventListener("tetois-settings-updated", () => {
            this.updateControlHints();
        });

        this.holdPiece = null;
        this.holdUsed = false;

        // two bags avoids running out while the next queue is being drawn
        this.bag = [];
        this.fillBag();
        this.fillBag();

        this.currentPiece = new Piece(this.bag.shift());
        this.input.onPieceSpawn();
        this.updateStats();
    }

    /**
     * connects pause/reset/exit buttons and page-level shortcuts
     */
    bindGameControls() {
        this.onGameControlKeyDown = (e) => {
            if (document.body.classList.contains("settings-modal-open")) {
                return;
            }

            if (this.isGameOver) {
                return;
            }

            // prevent held tab or r from repeatedly toggling/resetting
            if ((e.code === "Tab" || e.code === "KeyR") && e.repeat) {
                e.preventDefault();
                return;
            }

            if (e.code === "Tab") {
                e.preventDefault();
                this.togglePause();
            }

            if (e.code === "KeyR") {
                e.preventDefault();
                this.resetGame();
            }

            if (e.code === "Escape") {
                e.preventDefault();
                this.openQuitPrompt(true);
            }
        };

        this.onGameControlKeyUp = (e) => {
            if (document.body.classList.contains("settings-modal-open")) {
                return;
            }

            if (e.code === "Escape" && this.isQuitPromptOpen) {
                // releasing esc cancels the hold-to-exit timer
                e.preventDefault();
                this.closeQuitPrompt();
            }
        };

        document.addEventListener("keydown", this.onGameControlKeyDown);
        document.addEventListener("keyup", this.onGameControlKeyUp);

        if (this.pauseControlButton) {
            this.pauseControlButton.addEventListener("click", () => this.togglePause());
        }

        if (this.resetControlButton) {
            this.resetControlButton.addEventListener("click", () => this.resetGame());
        }

        if (this.exitControlButton) {
            this.exitControlButton.addEventListener("click", () => this.openQuitPrompt());
        }

        if (this.continueButton) {
            this.continueButton.addEventListener("click", () => this.resumeGame());
        }

        if (this.resetPausedButton) {
            this.resetPausedButton.addEventListener("click", () => this.resetGame());
        }

        if (this.cancelQuitButton) {
            this.cancelQuitButton.addEventListener("click", () => this.closeQuitPrompt());
        }
    }

    /**
     * refreshes the visible keybind hint text beside the board
     */
    updateControlHints() {
        // the game page hints use the same action ids as the settings modal
        const actionKeybinds = getActionKeybinds();

        document.querySelectorAll(".keybind-row[data-action]").forEach((row) => {
            const keyHint = row.querySelector("kbd");
            const code = actionKeybinds[row.dataset.action];

            if (keyHint) {
                keyHint.innerText = `[${getKeyLabel(code)}]`;
            }
        });
    }

    /**
     * flips between paused and unpaused state
     */
    togglePause() {
        if (this.isQuitPromptOpen || this.isGameOver) {
            return;
        }

        if (this.isPaused) {
            this.resumeGame();
        } else {
            this.pauseGame();
        }
    }

    /**
     * pauses the game and optionally shows the pause modal
     */
    pauseGame(showModal = true) {
        if (this.isGameOver) {
            return;
        }

        this.isPaused = true;
        this.input.releaseAll();

        if (showModal) {
            this.pauseBackdrop?.classList.remove("hidden");
            this.continueButton?.focus();
        }
    }

    /**
     * resumes gameplay after pause or cancelled quit
     */
    resumeGame() {
        if (this.isQuitPromptOpen || this.isGameOver) {
            return;
        }

        this.isPaused = false;
        this.pauseBackdrop?.classList.add("hidden");
    }

    /**
     * opens the quit prompt, optionally starting the hold-to-exit progress bar
     */
    openQuitPrompt(startHold = false) {
        if (this.isGameOver) {
            return;
        }

        // remember whether the user was already paused before opening quit
        if (!this.isQuitPromptOpen) {
            this.wasPausedBeforeQuit = this.isPaused;
        }

        this.isQuitPromptOpen = true;
        this.pauseGame(false);
        this.pauseBackdrop?.classList.add("hidden");
        this.quitBackdrop?.classList.remove("hidden");
        this.cancelQuitButton?.focus();

        if (startHold) {
            this.startQuitHold();
        }
    }

    /**
     * closes the quit prompt and returns to the previous pause state
     */
    closeQuitPrompt() {
        this.stopQuitHold();
        this.setQuitProgress(0);
        this.quitBackdrop?.classList.add("hidden");
        this.isQuitPromptOpen = false;

        if (this.wasPausedBeforeQuit) {
            this.pauseGame();
        } else {
            this.resumeGame();
        }
    }

    /**
     * animates the esc hold progress bar until it reaches the gamemode page
     */
    startQuitHold() {
        if (this.quitHoldFrame) {
            return;
        }

        this.quitHoldStart = performance.now();

        const updateQuitHold = (time) => {
            // clamp keeps progress between 0 and 1
            const progress = Math.min((time - this.quitHoldStart) / this.quitHoldDurationMs, 1);
            this.setQuitProgress(progress);

            if (progress >= 1) {
                window.location.href = "gamemode.html";
                return;
            }

            this.quitHoldFrame = requestAnimationFrame(updateQuitHold);
        };

        this.quitHoldFrame = requestAnimationFrame(updateQuitHold);
    }

    /**
     * cancels any active hold-to-exit animation
     */
    stopQuitHold() {
        if (this.quitHoldFrame) {
            cancelAnimationFrame(this.quitHoldFrame);
            this.quitHoldFrame = null;
        }

        this.quitHoldStart = null;
    }

    /**
     * updates the quit progress bar width
     */
    setQuitProgress(progress) {
        if (this.quitProgressBar) {
            this.quitProgressBar.style.width = `${progress * 100}%`;
        }
    }

    /**
     * adds one shuffled 7-bag of pieces to the queue
     */
    fillBag() {
        const temp = Object.keys(TETROMINOES);

        // fisher-yates shuffle
        for (let i = temp.length - 1; i > 0; i--) {
            const j = Math.floor(Math.random() * (i + 1));
            [temp[i], temp[j]] = [temp[j], temp[i]];
        }
        this.bag.push(...temp);
    }

    /**
     * checks whether the active piece is touching the stack or floor
     */
    isPieceGrounded() {
        return !this.board.isValidMove(
            this.currentPiece.matrix,
            this.currentPiece.x,
            this.currentPiece.y + 1
        );
    }

    /**
     * restarts lock delay after a valid grounded move, up to a limit
     */
    resetLockDelayIfAllowed() {
        if (!this.isPieceGrounded()) {
            this.lockTimer = 0;
            return;
        }

        if (this.lockResetCount < this.maxLockResets) {
            this.lockTimer = 0;
            this.lockResetCount++;
        }
    }

    /**
     * moves the active piece left or right by one cell
     */
    moveHorizontal(dx) {
        if (this.board.isValidMove(this.currentPiece.matrix, this.currentPiece.x + dx, this.currentPiece.y)) {
            this.currentPiece.x += dx;
            this.lastMoveWasRotation = false;
            this.resetLockDelayIfAllowed();
            return true;
        }
        return false;
    }

    /**
     * treats walls, floor, and filled cells as blocked for t-spin corner checks
     */
    isCellFilledOrBlocked(x, y) {
        if (x < 0 || x >= this.board.grid[0].length || y >= this.board.grid.length) {
            return true;
        }

        if (y < 0) {
            return false;
        }

        return this.board.grid[y][x] !== 0;
    }

    /**
     * simple t-spin detection using the 3-filled-corners rule
     */
    wasTSpin() {
        if (this.currentPiece.type !== "T" || !this.lastMoveWasRotation) {
            return false;
        }

        // t center is the middle of the 3x3 matrix
        const centerX = this.currentPiece.x + 1;
        const centerY = this.currentPiece.y + 1;

        const corners = [
            [centerX - 1, centerY - 1],
            [centerX + 1, centerY - 1],
            [centerX - 1, centerY + 1],
            [centerX + 1, centerY + 1]
        ];

        let filledCorners = 0;

        for (const [x, y] of corners) {
            if (this.isCellFilledOrBlocked(x, y)) {
                filledCorners++;
            }
        }

        return filledCorners >= 3;
    }

    /**
     * commits the active piece to the board and handles clears/end conditions
     */
    lockCurrentPiece() {
        const isTSpin = this.wasTSpin();
        const cleared = this.board.lockPiece(this.currentPiece);

        this.piecesPlaced++;
        this.linesCleared += cleared;
        this.calculateScore(cleared, isTSpin);
        this.updateStats();

        if (this.modeSettings.targetLines && this.linesCleared >= this.modeSettings.targetLines) {
            this.endGame("Run Complete");
            return;
        }

        this.spawnPiece();
        this.dropCounter = 0;
        this.lockTimer = 0;
        this.lastMoveWasRotation = false;
    }

    /**
     * handles one-shot actions like rotate, hard drop, and hold
     */
    handleAction(action) {
        switch (action) {
            case "ROTATE_CW":
                if (this.currentPiece.rotateWithKick("CLOCKWISE", this.board)) {
                    this.lastMoveWasRotation = true;
                    this.resetLockDelayIfAllowed();
                    this.input.refreshHorizontalAfterMovementChange();
                }
                break;

            case "ROTATE_CCW":
                if (this.currentPiece.rotateWithKick("ANTICLOCKWISE", this.board)) {
                    this.lastMoveWasRotation = true;
                    this.resetLockDelayIfAllowed();
                    this.input.refreshHorizontalAfterMovementChange();
                }
                break;

            case "ROTATE_180":
                if (this.currentPiece.rotateWithKick("180", this.board)) {
                    this.lastMoveWasRotation = true;
                    this.resetLockDelayIfAllowed();
                    this.input.refreshHorizontalAfterMovementChange();
                }
                break;

            case "HARDDROP":
                // hard drop jumps to the ghost position, then locks immediately
                this.currentPiece.y = this.currentPiece.getDropY(this.board);
                this.lockCurrentPiece();
                break;

            case "HOLD":
                if (!this.holdUsed) {
                    // first hold stores the piece and spawns a new one
                    if (this.holdPiece === null) {
                        this.holdPiece = this.currentPiece.type;
                        this.spawnPiece();
                    } else {
                        // later holds swap the active and held piece
                        const temp = this.currentPiece.type;
                        this.currentPiece = new Piece(this.holdPiece);
                        this.holdPiece = temp;
                        this.dropCounter = 0;
                        this.lockTimer = 0;
                        this.lockResetCount = 0;
                        this.lastMoveWasRotation = false;
                        this.input.onPieceSpawn();

                        if (!this.board.isValidMove(this.currentPiece.matrix, this.currentPiece.x, this.currentPiece.y)) {
                            this.endGame("Top Out");
                            return;
                        }
                    }

                    this.holdUsed = true;
                    this.dropCounter = 0;
                    this.lockTimer = 0;
                    this.renderer.draw(this);
                }
                break;
        }
    }

    /**
     * resets board state and starts a fresh run without reloading the page
     */
    resetGame() {
        this.board.reset();

        this.bag = [];
        this.fillBag();
        this.fillBag();

        this.holdPiece = null;
        this.holdUsed = false;

        this.currentPiece = new Piece(this.bag.shift());

        this.score = 0;
        this.piecesPlaced = 0;
        this.linesCleared = 0;
        this.totalPlayTimeMs = 0;
        this.isGameOver = false;
        this.isPaused = false;
        this.isQuitPromptOpen = false;
        this.wasPausedBeforeQuit = false;

        this.stopQuitHold();
        this.setQuitProgress(0);
        this.pauseBackdrop?.classList.add("hidden");
        this.quitBackdrop?.classList.add("hidden");

        this.dropCounter = 0;
        this.lockTimer = 0;
        this.lockResetCount = 0;
        this.lastMoveWasRotation = false;

        this.updateStats();

        if (this.msgEl) {
            clearTimeout(this.msgTimeout);
            this.msgEl.style.opacity = 0;
            setTimeout(() => {
                this.msgEl.innerText = "";
            }, 500);
        }

        this.input.releaseAll();
        this.input.onPieceSpawn();
        this.renderer.draw(this);
    }

    /**
     * pulls the next piece from the bag and checks for top out
     */
    spawnPiece() {
        if (this.bag.length <= 5) {
            this.fillBag();
        }

        this.currentPiece = new Piece(this.bag.shift());
        this.holdUsed = false;
        this.lockResetCount = 0;
        this.lastMoveWasRotation = false;
        this.input.onPieceSpawn();

        if (!this.board.isValidMove(this.currentPiece.matrix, this.currentPiece.x, this.currentPiece.y)) {
            this.endGame("Top Out");
        }
    }

    /**
     * main requestAnimationFrame loop
     */
    update(time = 0) {
        if (this.isGameOver) {
            return;
        }

        if (this.isPaused) {
            // keep the loop alive, but do not advance timers while paused
            this.lastTime = time;
            requestAnimationFrame((t) => this.update(t));
            return;
        }

        const deltaTime = time - this.lastTime;
        this.lastTime = time;
        this.totalPlayTimeMs += deltaTime;

        if (this.modeSettings.durationMs && this.totalPlayTimeMs >= this.modeSettings.durationMs) {
            this.totalPlayTimeMs = this.modeSettings.durationMs;
            this.updateStats();
            this.renderer.draw(this);
            this.endGame("Time Up");
            return;
        }

        this.dropCounter += deltaTime;
        this.input.processInput(deltaTime);

        if (!this.board.isValidMove(this.currentPiece.matrix, this.currentPiece.x, this.currentPiece.y + 1)) {
            // grounded pieces lock after lock delay
            this.lockTimer += deltaTime;

            if (this.lockTimer >= SETTINGS.LOCK_DELAY) {
                this.lockCurrentPiece();
            }
        } else {
            this.lockTimer = 0;

            // use a while loop so large frame delays still process all missed gravity steps
            while (this.dropCounter >= this.dropInterval) {
                if (this.board.isValidMove(this.currentPiece.matrix, this.currentPiece.x, this.currentPiece.y + 1)) {
                    this.currentPiece.y++;
                    this.lastMoveWasRotation = false;
                    this.dropCounter -= this.dropInterval;
                } else {
                    this.dropCounter = 0;
                    break;
                }
            }
        }

        this.renderer.draw(this);
        this.updateStats();
        requestAnimationFrame((t) => this.update(t));
    }

    /**
     * applies scoring for line clears and simple t-spins
     */
    calculateScore(lines, isTSpin = false) {
        if (lines === 0) return;

        let base = 0;
        let msg = "";

        if (isTSpin) {
            switch (lines) {
                case 1:
                    base = 800;
                    msg = "T-Spin Single";
                    break;
                case 2:
                    base = 1200;
                    msg = "T-Spin Double";
                    break;
                case 3:
                    base = 1600;
                    msg = "T-Spin Triple";
                    break;
            }

            this.score += base;

            if (this.msgEl) {
                this.msgEl.innerText = msg;
                this.msgEl.style.opacity = 1;

                clearTimeout(this.msgTimeout);
                this.msgTimeout = setTimeout(() => {
                    this.msgEl.style.opacity = 0;
                }, 1200);
            }

            return;
        }

        switch (lines) {
            case 1:
                base = 100;
                msg = "Single";
                break;
            case 2:
                base = 300;
                msg = "Double";
                break;
            case 3:
                base = 500;
                msg = "Triple";
                break;
            case 4:
                base = 800;
                msg = "Quad";
                break;
        }

        this.score += base;

        if (this.msgEl) {
            this.msgEl.innerText = msg;
            this.msgEl.style.opacity = 1;

            clearTimeout(this.msgTimeout);
            this.msgTimeout = setTimeout(() => {
                this.msgEl.style.opacity = 0;
            }, 1200);
        }
    }

    /**
     * updates score/time/mode text in the side panel
     */
    updateStats() {
        if (this.scoreValEl) {
            this.scoreValEl.innerText = this.score;
        }

        const seconds = this.totalPlayTimeMs / 1000;
        const pps = seconds > 0 ? (this.piecesPlaced / seconds).toFixed(2) : "0.00";

        if (this.ppsValEl) {
            this.ppsValEl.innerText = pps;
        }

        if (this.modeValEl) {
            this.modeValEl.innerText = this.modeSettings.label;
        }

        if (this.timeValEl) {
            this.timeValEl.innerText = this.getDisplayTime();
        }

        if (this.linesValEl) {
            const target = this.modeSettings.targetLines;
            this.linesValEl.innerText = target ? `${this.linesCleared}/${target}` : this.linesCleared;
        }
    }

    /**
     * returns elapsed time for 40L, or remaining time for ultra
     */
    getDisplayTime() {
        const timeMs = this.modeSettings.durationMs
            ? Math.max(0, this.modeSettings.durationMs - this.totalPlayTimeMs)
            : this.totalPlayTimeMs;

        return this.formatTime(timeMs);
    }

    /**
     * formats milliseconds as m:ss.cc
     */
    formatTime(timeMs) {
        const totalCentiseconds = Math.floor(timeMs / 10);
        const minutes = Math.floor(totalCentiseconds / 6000);
        const seconds = Math.floor((totalCentiseconds % 6000) / 100);
        const centiseconds = totalCentiseconds % 100;

        return `${minutes}:${String(seconds).padStart(2, "0")}.${String(centiseconds).padStart(2, "0")}`;
    }

    /**
     * stops input and shows the end-of-run modal
     */
    endGame(title) {
        if (this.isGameOver) {
            return;
        }

        this.isGameOver = true;
        this.isPaused = false;
        this.isQuitPromptOpen = false;
        this.stopQuitHold();
        this.pauseBackdrop?.classList.add("hidden");
        this.quitBackdrop?.classList.add("hidden");
        this.input.destroy();

        if (this.gameEndTitle) {
            this.gameEndTitle.innerText = title;
        }

        if (this.gameEndMode) {
            this.gameEndMode.innerText = `Mode: ${this.modeSettings.label}`;
        }

        if (this.gameEndScore) {
            this.gameEndScore.innerText = `Score: ${this.score}`;
        }

        if (this.gameEndTime) {
            const timeLabel = this.modeSettings.durationMs ? "Time Left" : "Time";
            this.gameEndTime.innerText = `${timeLabel}: ${this.getDisplayTime()}`;
        }

        if (this.gameEndLines) {
            this.gameEndLines.innerText = `Lines: ${this.linesCleared}`;
        }

        if (this.gameEndBackdrop) {
            this.gameEndBackdrop.classList.remove("hidden");
        }
    }

    /**
     * starts the animation loop
     */
    start() {
        requestAnimationFrame((t) => {
            this.lastTime = t;
            this.update(t);
        });
    }
}
