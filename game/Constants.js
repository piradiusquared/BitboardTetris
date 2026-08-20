export const BOARD_COLS = 10;
export const BOARD_ROWS = 20;
export const BLOCK_SIZE = 30;
export const VISIBLE_ROWS = 20;
export const HIDDEN_ROWS = BOARD_ROWS - VISIBLE_ROWS;

// index 0 is empty, the other numbers map to these colours
export const COLOURS = [null, 'purple', 'cyan', 'blue', 'orange', 'green', 'red', 'yellow'];

// each matrix is one spawn orientation, with numbers matching the colour list above
export const TETROMINOES = {
    'T': [
        [0, 1, 0],
        [1, 1, 1],
        [0, 0, 0]
    ],
    'I': [
        [0, 0, 0, 0],
        [2, 2, 2, 2],
        [0, 0, 0, 0],
        [0, 0, 0, 0]
    ],
    'J': [
        [3, 0, 0],
        [3, 3, 3],
        [0, 0, 0]
    ],
    'L': [
        [0, 0, 4],
        [4, 4, 4],
        [0, 0, 0]
    ],
    'S': [
        [0, 5, 5],
        [5, 5, 0],
        [0, 0, 0]
    ],
    'Z': [
        [6, 6, 0],
        [0, 6, 6],
        [0, 0, 0]
    ],
    'O': [
        [0, 7, 7, 0],
        [0, 7, 7, 0],
        [0, 0, 0, 0]
    ]
};

// wall kick tests for normal pieces, based on SRS reference data
export const KICKS_SQ = {
    "01": [[0, 0], [-1, 0], [-1, -1], [0, 2], [-1, 2]],
    "10": [[0, 0], [1, 0], [1, 1], [0, -2], [1, -2]],
    "12": [[0, 0], [1, 0], [1, 1], [0, -2], [1, -2]],
    "21": [[0, 0], [-1, 0], [-1, -1], [0, 2], [-1, 2]],
    "23": [[0, 0], [1, 0], [1, -1], [0, 2], [1, 2]],
    "32": [[0, 0], [-1, 0], [-1, 1], [0, -2], [-1, -2]],
    "30": [[0, 0], [-1, 0], [-1, 1], [0, -2], [-1, -2]],
    "03": [[0, 0], [1, 0], [1, -1], [0, 2], [1, 2]]
}

// I piece has its own kick data because it rotates around a different shape
export const KICKS_I = {
    "01": [[0, 0], [-2, 0], [1, 0], [-2, 1], [1, -2]],
    "10": [[0, 0], [2, 0], [-1, 0], [2, -1], [-1, 2]],
    "12": [[0, 0], [-1, 0], [2, 0], [-1, -2], [2, 1]],
    "21": [[0, 0], [1, 0], [-2, 0], [1, 2], [-2, -1]],
    "23": [[0, 0], [2, 0], [-1, 0], [2, -1], [-1, 2]],
    "32": [[0, 0], [-2, 0], [1, 0], [-2, 1], [1, -2]],
    "30": [[0, 0], [1, 0], [-2, 0], [1, 2], [-2, -1]],
    "03": [[0, 0], [-1, 0], [2, 0], [-1, -2], [2, 1]]
}

// simplified 180 kick table
export const KICKS_180 = {
    "02": [[0, 0], [0, -1], [1, -1], [-1, -1], [0, 1]],
    "13": [[0, 0], [1, 0], [1, -2], [1, 1], [0, -2]],
    "20": [[0, 0], [0, 1], [-1, 1], [1, 1], [0, -1]],
    "31": [[0, 0], [-1, 0], [-1, -2], [-1, 1], [0, -2]]
};

const SETTINGS_STORAGE_KEY = "tetois-settings";

// these action names match the settings modal data-action values
export const DEFAULT_KEYBIND_SETTINGS = {
    KEYRIGHT: "ArrowRight",
    KEYLEFT: "ArrowLeft",
    ROTATE_CW: "ArrowUp",
    ROTATE_CCW: "KeyZ",
    ROTATE_180: "KeyX",
    SOFT_DROP: "ArrowDown",
    HARDDROP: "Space",
    HOLD: "KeyC"
};

// default handling numbers for movement and locking
export const DEFAULT_GAME_SETTINGS = {
    DAS: 130,
    ARR: 0,
    SDF: 1000,
    LOCK_DELAY: 500
};

// nicer labels for keys that would otherwise show technical code names
const KEY_LABELS = {
    Space: "Space",
    ShiftLeft: "Shift",
    ShiftRight: "Shift",
    Quote: "'",
    Semicolon: ";",
    ArrowLeft: "\u2190",
    ArrowRight: "\u2192",
    ArrowUp: "\u2191",
    ArrowDown: "\u2193"
};

function readStoredSettings() {
    // fallback keeps the game playable if localstorage is empty or invalid
    const fallback = {
        keybinds: { ...DEFAULT_KEYBIND_SETTINGS },
        mechanics: { ...DEFAULT_GAME_SETTINGS }
    };

    if (typeof localStorage === "undefined") {
        return fallback;
    }

    try {
        // localstorage stores text, so parse it back into an object first
        const savedSettings = JSON.parse(localStorage.getItem(SETTINGS_STORAGE_KEY));

        return {
            keybinds: {
                ...fallback.keybinds,
                ...(savedSettings?.keybinds || {})
            },
            mechanics: {
                ...fallback.mechanics,
                ...(savedSettings?.mechanics || {})
            }
        };
    } catch {
        return fallback;
    }
}

function invertKeybinds(actionKeybinds) {
    // input lookup needs the opposite shape: KeyboardEvent.code -> game action
    return Object.entries(actionKeybinds).reduce((keybinds, [action, code]) => {
        if (code) {
            keybinds[code] = action;
        }

        return keybinds;
    }, {});
}

// exported objects are mutable so settings changes can update them in-place
export const KEYBINDS = invertKeybinds(readStoredSettings().keybinds);

export const SETTINGS = {
    ...DEFAULT_GAME_SETTINGS,
    ...readStoredSettings().mechanics
};

export function applyStoredGameSettings() {
    const storedSettings = readStoredSettings();

    // mutate exported objects in place so modules that imported them see new values
    Object.keys(KEYBINDS).forEach((code) => {
        delete KEYBINDS[code];
    });
    Object.assign(KEYBINDS, invertKeybinds(storedSettings.keybinds));

    Object.assign(SETTINGS, DEFAULT_GAME_SETTINGS, storedSettings.mechanics);
}

/**
 * returns action -> key code, useful for displaying current binds
 */
export function getActionKeybinds() {
    return Object.entries(KEYBINDS).reduce((actionKeybinds, [code, action]) => {
        actionKeybinds[action] = code;
        return actionKeybinds;
    }, {});
}

/**
 * converts KeyboardEvent.code into a short label for the UI
 */
export function getKeyLabel(code) {
    if (!code) return "Unset";
    if (KEY_LABELS[code]) return KEY_LABELS[code];
    if (code.startsWith("Key")) return code.replace("Key", "");
    if (code.startsWith("Digit")) return code.replace("Digit", "");
    return code;
}

if (typeof window !== "undefined") {
    // settings modal fires this event whenever the user changes a setting
    window.addEventListener("tetois-settings-updated", applyStoredGameSettings);
}
