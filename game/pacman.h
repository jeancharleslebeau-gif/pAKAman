#pragma once
#include <cstdint>
#include "game/config.h"

struct GameState; 
extern char debugText[64];

class Pacman {
public:

    // ------------------------------------------------------------
    // Directions logiques
    // ------------------------------------------------------------
    enum class Dir : uint8_t {
        None = 0,
        Left = 1,
        Right = 2,
        Up = 3,
        Down = 4
    };

    // ------------------------------------------------------------
    // Position logique (case-based)
    // ------------------------------------------------------------
    int tile_r = 0;          // ligne de tuile
    int tile_c = 0;          // colonne de tuile
	int prev_tile_r = 0;
	int prev_tile_c = 0;


    // Progression dans la tuile (0..TILE_SIZE)
    // signée pour gérer Up/Left (négatif)
    int pixel_offset = 0;

    // ------------------------------------------------------------
    // Position pixel dérivée (pour le rendu uniquement)
    // ------------------------------------------------------------
    int x = 0;
    int y = 0;

    // ------------------------------------------------------------
    // Direction actuelle + direction demandée par le joueur
    // ------------------------------------------------------------
    Dir dir      = Dir::None;   // direction réelle
    Dir next_dir = Dir::None;   // direction demandée (input)

    // ------------------------------------------------------------
    // Animation
    // ------------------------------------------------------------
    int animTick = 0;

    // ------------------------------------------------------------
    // Helpers case-based
    // ------------------------------------------------------------
    inline bool isCentered() const {
        return (pixel_offset == 0);
    }

    inline int tileRow() const { return tile_r; }
    inline int tileCol() const { return tile_c; }

    // ------------------------------------------------------------
    // Constructeur
    // ------------------------------------------------------------
    Pacman(int start_col = 0, int start_row = 0)
        : tile_r(start_row),
          tile_c(start_col),
          dir(Dir::None),
          next_dir(Dir::None),
          animTick(0)
    {
        // Position pixel centrée dans la tuile
        x = tile_c * TILE_SIZE + (TILE_SIZE - PACMAN_SIZE) / 2;
        y = tile_r * TILE_SIZE + (TILE_SIZE - PACMAN_SIZE) / 2;
        pixel_offset = 0;
    }

    // ------------------------------------------------------------
    // API principale
    // ------------------------------------------------------------
    void update(GameState& g);
    void draw(const GameState& g) const;

    // ------------------------------------------------------------
    // Helpers direction
    // ------------------------------------------------------------
    static int dirX(Dir d) {
        switch (d) {
            case Dir::Left:  return -1;
            case Dir::Right: return +1;
            default: return 0;
        }
    }

    static int dirY(Dir d) {
        switch (d) {
            case Dir::Up:    return -1;
            case Dir::Down:  return +1;
            default: return 0;
        }
    }
};
