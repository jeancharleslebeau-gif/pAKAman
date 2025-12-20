#include "level.h"
#include "game.h"
#include "maze.h"
#include "pacman.h"
#include "ghost.h"

// Ici on utilise maze_B comme niveau 1

void level_init(GameState& g) {
    g.score = 0;
    g.lives = 3;
    g.ghosts.clear();

    // Charge le labyrinthe ASCII dans g.maze
    maze_from_ascii(maze_B_ascii, g.maze);

    // Position de départ de Pac-Man
    int prow = g.maze.pac_spawn_row;
    int pcol = g.maze.pac_spawn_col;
    int px   = pcol * TILE_SIZE;
    int py   = prow * TILE_SIZE;

    g.pacman.x = px + PACMAN_OFFSET;
    g.pacman.y = py + PACMAN_OFFSET;

    printf("Pac-Man start: row=%d col=%d -> x=%d y=%d\n", prow, pcol, g.pacman.x, g.pacman.y);

    // Position de la maison des fantômes (centre)
    int grow = g.maze.ghost_center_row;
    int gcol = g.maze.ghost_center_col;
    int gx   = gcol * TILE_SIZE;
    int gy   = grow * TILE_SIZE;

    // Création des 4 fantômes (simplement autour du centre)
    // id : 0 = rouge, 1 = bleu, 2 = rose, 3 = orange (comme dans ghost.cpp)
    // Couleurs : à adapter si tu as des constantes spécifiques
    g.ghosts.push_back(Ghost(COLOR_RED, 0, gcol, grow));
    g.ghosts.back().x = gx + GHOST_OFFSET;
    g.ghosts.back().y = gy + GHOST_OFFSET;

    g.ghosts.push_back(Ghost(COLOR_LIGHTBLUE, 1, gcol - 1, grow));
    g.ghosts.back().x = (gcol - 1) * TILE_SIZE + GHOST_OFFSET;
    g.ghosts.back().y = gy + GHOST_OFFSET;

    g.ghosts.push_back(Ghost(COLOR_PINK, 2, gcol + 1, grow));
    g.ghosts.back().x = (gcol + 1) * TILE_SIZE + GHOST_OFFSET;
    g.ghosts.back().y = gy + GHOST_OFFSET;

    g.ghosts.push_back(Ghost(COLOR_ORANGE, 3, gcol, grow + 1));
    g.ghosts.back().x = gx + GHOST_OFFSET;
    g.ghosts.back().y = (grow + 1) * TILE_SIZE + GHOST_OFFSET;
}
