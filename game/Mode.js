const modeLinks = document.querySelectorAll('.game-mode-link[data-mode]');

// save the chosen mode before game.html loads
modeLinks.forEach((link) => {
    link.addEventListener('click', () => {
        // data-mode comes from the clicked gamemode panel
        localStorage.setItem('tetois-game-mode', link.dataset.mode);
    });
});
