#pragma once
#include <stdint.h>
#include "level.h"
#include "game/config.h"
#include "core/graphics.h"

// Dimensions du labyrinthe (ASCII)
static const int MAZE_WIDTH  = LEVEL_COLS;  // doit être 20
static const int MAZE_HEIGHT = LEVEL_ROWS;  // doit être 21

// ---------------------------------------------------------------------
// Types de cases du labyrinthe
// ---------------------------------------------------------------------
enum class TileType : uint8_t {
    Empty,              // espace vide
    Wall,               // '#'
    Pellet,             // '.'
    PowerPellet,        // 'o'

    GhostHouse,         // 'H' = intérieur de la maison des fantômes

    GhostDoorClosed,    // 'D' = porte fermée
    GhostDoorOpening,   // porte en animation d'ouverture
    GhostDoorOpen,      // porte ouverte

    Tunnel,             // 'T' = case adjacente au tunnel
    TunnelEntry,        // 'E' = portail de tunnel

    PacSpawn,           // 'P'
    FruitSpawn          // 'F'
};


// ---------------------------------------------------------------------
// Structure Maze
// ---------------------------------------------------------------------
struct Maze {

    TileType tiles[MAZE_HEIGHT][MAZE_WIDTH];

    // Comptage
    int pellet_count       = 0;
    int power_pellet_count = 0;

    // Points spéciaux
    int pac_spawn_row   = 0;
    int pac_spawn_col   = 0;
	int ghost_spawn_row[4];
	int ghost_spawn_col[4];
    int fruit_row       = -1;
    int fruit_col       = -1;

    // Porte fantôme (unique)
    int ghost_door_row  = -1;
    int ghost_door_col  = -1;

    // Centre de la maison fantôme (calculé via 'H')
    int ghost_center_row = -1;
    int ghost_center_col = -1;

    // Tunnel : on stocke deux entrées (E)
    int tunnel_entry_count = 0;
    int tunnel_entry_row[2];
    int tunnel_entry_col[2];

    // Rendu
    void draw() const;

    // Changer l'état de la porte fantôme
    void setGhostDoor(TileType newState);
};

// Chargement depuis une définition ASCII (20x21)
void maze_from_ascii(const char* ascii[MAZE_HEIGHT], Maze& maze);

// Niveau 1 : maze_B
extern const char* maze_B_ascii[MAZE_HEIGHT];

// Fonctions utilitaires
inline bool isGhostHouseTile(TileType t)
{
    return (t == TileType::GhostHouse);
}

inline bool isGhostDoorTile(TileType t)
{
    return (t == TileType::GhostDoorClosed ||
            t == TileType::GhostDoorOpening ||
            t == TileType::GhostDoorOpen);
}

inline bool isGhostRestrictedForPacman(TileType t)
{
    return isGhostHouseTile(t) || isGhostDoorTile(t);
}

