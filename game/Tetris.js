import { Game } from "./Game.js";

// gamemode page stores the selected mode here, default to 40L if none exists
const selectedMode = localStorage.getItem("tetois-game-mode") || "line";

// create and start the game controller
const game = new Game(selectedMode);
game.start();
