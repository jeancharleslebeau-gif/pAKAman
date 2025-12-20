#include "game.h"
#include "level.h"
#include "maze.h"
#include "core/graphics.h"
#include <string>

// caméra globale utilisée par maze/pacman/ghost
float g_camera_y = 0.0f;

// === Initialisation du jeu ===
void game_init(GameState& g) {
    g.score = 0;
    g.lives = 3;

    // Charge le niveau et place Pac-Man + fantômes
    level_init(g);
}

// Mise à jour caméra (suivi vertical de Pac-Man)
static void update_camera(const GameState& g)
{
    int maze_height_px = MAZE_HEIGHT * TILE_SIZE;
    int max_scroll = maze_height_px - SCREEN_H;
    if (max_scroll < 0) max_scroll = 0;

    int pac_center_y = g.pacman.y + PACMAN_SIZE / 2;
    int target_y = pac_center_y - SCREEN_H / 2;

    if (target_y < 0) target_y = 0;
    if (target_y > max_scroll) target_y = max_scroll;

    // Pour l’instant : pas de smoothing, on peut en ajouter plus tard
    g_camera_y = (float)target_y;
}

// === Mise à jour du jeu ===
void game_update(GameState& g) {
    // Pac-Man se déplace et mange les pac-gums
    g.pacman.update(g);

    // Fantômes
    for (auto& ghost : g.ghosts) {
        ghost.update(g);
    }

    // Caméra
    update_camera(g);
}

// === Dessin du jeu ===
void game_draw(const GameState& g) {
    gfx_clear(COLOR_BLACK);

    // Dessine le labyrinthe
    g.maze.draw();

    // Dessine Pac-Man
    g.pacman.draw();

    // Dessine les fantômes
    for (const auto& ghost : g.ghosts) {
        ghost.draw();
    }

    // HUD minimal (score + vies)
    gfx_text(4, 4, ("SCORE: " + std::to_string(g.score)).c_str(), COLOR_WHITE);
    gfx_text(180, 4, ("LIVES: " + std::to_string(g.lives)).c_str(), COLOR_YELLOW);
}

// === Fin de partie ===
bool game_is_over(const GameState& g) {
    // Pas encore de logique de fin
    return false;
}
