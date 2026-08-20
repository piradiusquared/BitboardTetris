import { KEYBINDS, SETTINGS } from "./Constants.js";

/**
 * converts keyboard input into game actions and held movement
 */
export class Input {
    constructor(game) {
        this.game = game;

        // these actions can be held across multiple frames
        this.keyPress = {
            KEYLEFT: false,
            KEYRIGHT: false,
            SOFT_DROP: false
        };

        // DAS tracks how long left/right has been held before auto-repeat starts
        this.holdTime = {
            KEYLEFT: 0,
            KEYRIGHT: 0
        };

        // ARR tracks repeated movement after DAS has finished
        this.arrTime = {
            KEYLEFT: 0,
            KEYRIGHT: 0
        };

        this.instantArrDone = {
            KEYLEFT: false,
            KEYRIGHT: false
        };

        this.lastHorizontal = null;

        this.onKeyDown = (e) => {
            // keybinds are stored as KeyboardEvent.code -> action name
            const action = KEYBINDS[e.code];

            // ignore gameplay input while menus/settings are active
            if (
                document.body.classList.contains("settings-modal-open") ||
                this.game.isPaused ||
                this.game.isQuitPromptOpen ||
                this.game.isGameOver
            ) {
                if (action) {
                    e.preventDefault();
                }

                return;
            }

            if (!action || e.repeat) return;

            e.preventDefault();

            switch (action) {
                case "KEYLEFT":
                case "KEYRIGHT":
                    // move once immediately, then held movement is handled in processHorizontal
                    this.keyPress[action] = true;
                    this.holdTime[action] = 0;
                    this.arrTime[action] = 0;
                    this.instantArrDone[action] = false;
                    this.lastHorizontal = action;

                    this.game.moveHorizontal(action === "KEYLEFT" ? -1 : 1);
                    break;

                case "SOFT_DROP":
                    this.keyPress.SOFT_DROP = true;
                    break;

                default:
                    this.game.handleAction(action);
                    break;
            }
        };

        this.onKeyUp = (e) => {
            const action = KEYBINDS[e.code];
            if (!action) return;

            if (Object.prototype.hasOwnProperty.call(this.keyPress, action)) {
                this.keyPress[action] = false;
            }

            if (Object.prototype.hasOwnProperty.call(this.holdTime, action)) {
                this.holdTime[action] = 0;
                this.arrTime[action] = 0;
                this.instantArrDone[action] = false;

                // if both directions were held, releasing one should switch to the other
                if (this.lastHorizontal === action) {
                    if (action === "KEYLEFT" && this.keyPress.KEYRIGHT) {
                        this.lastHorizontal = "KEYRIGHT";
                        this.holdTime.KEYRIGHT = 0;
                        this.arrTime.KEYRIGHT = 0;
                        this.instantArrDone.KEYRIGHT = false;
                        this.game.moveHorizontal(1);
                    } else if (action === "KEYRIGHT" && this.keyPress.KEYLEFT) {
                        this.lastHorizontal = "KEYLEFT";
                        this.holdTime.KEYLEFT = 0;
                        this.arrTime.KEYLEFT = 0;
                        this.instantArrDone.KEYLEFT = false;
                        this.game.moveHorizontal(-1);
                    } else {
                        this.lastHorizontal = null;
                    }
                }
            }
        };

        document.addEventListener("keydown", this.onKeyDown);
        document.addEventListener("keyup", this.onKeyUp);
    }

    /**
     * runs once per frame to apply held movement
     */
    processInput(deltaTime) {
        this.processHorizontal(deltaTime);

        if (this.keyPress.SOFT_DROP) {
            // increase the drop counter instead of moving directly, so gravity still owns falling
            this.game.dropCounter += deltaTime * SETTINGS.SDF;
        }
    }

    /**
     * handles DAS/ARR repeated left and right movement
     */
    processHorizontal(deltaTime) {
        const leftHeld = this.keyPress.KEYLEFT;
        const rightHeld = this.keyPress.KEYRIGHT;

        if (!leftHeld && !rightHeld) return;

        let activeAction = null;
        let dx = 0;

        if (leftHeld && rightHeld) {
            // when both are held, the most recently pressed direction wins
            activeAction = this.lastHorizontal;
            if (!activeAction) return;
            dx = activeAction === "KEYLEFT" ? -1 : 1;
        } else if (leftHeld) {
            activeAction = "KEYLEFT";
            dx = -1;
            this.lastHorizontal = "KEYLEFT";
        } else if (rightHeld) {
            activeAction = "KEYRIGHT";
            dx = 1;
            this.lastHorizontal = "KEYRIGHT";
        }

        this.holdTime[activeAction] += deltaTime;

        if (this.holdTime[activeAction] < SETTINGS.DAS) {
            return;
        }

        if (SETTINGS.ARR === 0) {
            // ARR 0 means move instantly to the wall after DAS
            if (this.instantArrDone[activeAction]) return;

            while (this.game.moveHorizontal(dx)) {}
            this.instantArrDone[activeAction] = true;
            return;
        }

        this.arrTime[activeAction] += deltaTime;

        while (this.arrTime[activeAction] >= SETTINGS.ARR) {
            if (!this.game.moveHorizontal(dx)) {
                this.arrTime[activeAction] = 0;
                break;
            }
            this.arrTime[activeAction] -= SETTINGS.ARR;
        }
    }

    /**
     * reapplies held movement when a new piece appears
     */
    onPieceSpawn() {
        if (this.keyPress.KEYLEFT && !this.keyPress.KEYRIGHT) {
            this.lastHorizontal = "KEYLEFT";
            this.instantArrDone.KEYLEFT = false;
        } else if (this.keyPress.KEYRIGHT && !this.keyPress.KEYLEFT) {
            this.lastHorizontal = "KEYRIGHT";
            this.instantArrDone.KEYRIGHT = false;
        } else if (this.keyPress.KEYLEFT && this.keyPress.KEYRIGHT) {
            if (this.lastHorizontal) {
                this.instantArrDone[this.lastHorizontal] = false;
            }
        }

        this.processHorizontal(0);
    }

    /**
     * lets rotations/holds re-check auto movement after the piece changes position
     */
    refreshHorizontalAfterMovementChange() {
        if (this.keyPress.KEYLEFT) {
            this.instantArrDone.KEYLEFT = false;
        }
        if (this.keyPress.KEYRIGHT) {
            this.instantArrDone.KEYRIGHT = false;
        }

        this.processHorizontal(0);
    }

    /**
     * clears all held input, useful when pausing or resetting
     */
    releaseAll() {
        this.keyPress.KEYLEFT = false;
        this.keyPress.KEYRIGHT = false;
        this.keyPress.SOFT_DROP = false;

        this.holdTime.KEYLEFT = 0;
        this.holdTime.KEYRIGHT = 0;

        this.arrTime.KEYLEFT = 0;
        this.arrTime.KEYRIGHT = 0;

        this.instantArrDone.KEYLEFT = false;
        this.instantArrDone.KEYRIGHT = false;
        this.lastHorizontal = null;
    }

    /**
     * removes document listeners when the game ends
     */
    destroy() {
        document.removeEventListener("keydown", this.onKeyDown);
        document.removeEventListener("keyup", this.onKeyUp);
    }
}
