#pragma once
#include <stdint.h>
#include "level.h"
#include "game/config.h"
#include "core/graphics.h"

// On suppose LEVEL_ROWS = 21, LEVEL_COLS = 21 dans level.h
static const int MAZE_WIDTH  = LEVEL_COLS;
static const int MAZE_HEIGHT = LEVEL_ROWS;

enum class TileType : uint8_t {
    Empty,          // espace vide / accessible sans pastille
    Wall,           // '#'
    Pellet,         // '.'
    PowerPellet,    // 'o'
    GhostHouse,     // 'H'
    GhostDoor,      // 'D'
    Tunnel,         // 'T' (couloir spécial)
    PacSpawn,       // 'P'
    FruitSpawn      // 'F'
};

struct Maze {
    TileType tiles[MAZE_HEIGHT][MAZE_WIDTH];

    // Comptage
    int pellet_count       = 0;
    int power_pellet_count = 0;

    // Points spéciaux (indices de case)
    int pac_spawn_row   = 0;
    int pac_spawn_col   = 0;
    int fruit_row       = -1;
    int fruit_col       = -1;

    int ghost_door_row  = -1;
    int ghost_door_col  = -1;
    int ghost_center_row = -1;   // centre de la maison
    int ghost_center_col = -1;

    void draw() const;
};

// Chargement depuis une définition ASCII (21x21)
void maze_from_ascii(const char* ascii[MAZE_HEIGHT], Maze& maze);

// Niveau 1 : maze_B
extern const char* maze_B_ascii[MAZE_HEIGHT];
