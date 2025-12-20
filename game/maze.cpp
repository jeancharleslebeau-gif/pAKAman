#include "maze.h"
#include "assets/assets.h"
#include "core/sprite.h"
#include "game/config.h"
#include "core/graphics.h"

// Caméra globale (définie dans game.cpp)
extern float g_camera_y;

// ---------------------------------------------------------------------
// Définition du labyrinthe B en ASCII (niveau 1)
// ---------------------------------------------------------------------
const char* maze_B_ascii[MAZE_HEIGHT] = {
    "####################",
    "#o.......#........o#",
    "#.##.###.#.#.##.##.#",
    "#.#......#.#.#...#.#",
    "#.#.##.#.#.#.#.#.#.#",
    "#.#.#..#...#.#.#.#.#",
    "#.#.#.##.###.#.#.#.#",
    "#...#............#.#",
    "###.#.####D###.###.#",
    "#...#.#HHHHHH#.#...#",
    "###.#.#HHHHHH#.#.###",
    "#...#.#HHHHHH#.#...#",
    "#.###.########.###.#",
    "#.#.....#...#....#.#",
    "#.#.###.#.#.#.##.#.#",
    "#.#.#.........##.#.#",
    "#.#.#.#.#.#.#.##.#.#",
    "#.#.o.#.#.#.#..o.#.#",
    "#.#.###.#.#.######.#",
    "#T.......P........T#",
    "####################",
};

// ---------------------------------------------------------------------
// Conversion ASCII -> Maze interne
// ---------------------------------------------------------------------
void maze_from_ascii(const char* ascii[MAZE_HEIGHT], Maze& maze)
{
    maze.pellet_count        = 0;
    maze.power_pellet_count  = 0;
    maze.pac_spawn_row       = 0;
    maze.pac_spawn_col       = 0;
    maze.fruit_row           = -1;
    maze.fruit_col           = -1;
    maze.ghost_door_row      = -1;
    maze.ghost_door_col      = -1;
    maze.ghost_center_row    = -1;
    maze.ghost_center_col    = -1;

    // Pour calculer le centre de la maison
    int house_sum_row = 0;
    int house_sum_col = 0;
    int house_count   = 0;

    for (int row = 0; row < MAZE_HEIGHT; ++row) {
        const char* line = ascii[row];
        for (int col = 0; col < MAZE_WIDTH; ++col) {
            char c = line[col];
            TileType t = TileType::Empty;

            switch (c) {
                case '#': t = TileType::Wall; break;
                case '.': t = TileType::Pellet; maze.pellet_count++; break;
                case 'o': t = TileType::PowerPellet; maze.power_pellet_count++; break;
                case 'H':
                    t = TileType::GhostHouse;
                    house_sum_row += row;
                    house_sum_col += col;
                    house_count++;
                    break;
                case 'D':
                    t = TileType::GhostDoor;
                    maze.ghost_door_row = row;
                    maze.ghost_door_col = col;
                    break;
                case 'T':
                    t = TileType::Tunnel;
                    break;
                case 'P':
                    t = TileType::PacSpawn;
                    maze.pac_spawn_row = row;
                    maze.pac_spawn_col = col;
                    break;
                case 'F':
                    t = TileType::FruitSpawn;
                    maze.fruit_row = row;
                    maze.fruit_col = col;
                    break;
                case ' ':
                    t = TileType::Empty;
                    break;
                default:
                    t = TileType::Empty;
                    break;
            }

            maze.tiles[row][col] = t;
        }
    }

    if (house_count > 0) {
        maze.ghost_center_row = house_sum_row / house_count;
        maze.ghost_center_col = house_sum_col / house_count;
    }
}

// ---------------------------------------------------------------------
// Rendu du Maze
// ---------------------------------------------------------------------
void Maze::draw() const {
    for (int row = 0; row < MAZE_HEIGHT; ++row) {
        for (int col = 0; col < MAZE_WIDTH; ++col) {
            TileType t = tiles[row][col];
            int px = col * TILE_SIZE;
            int py = row * TILE_SIZE;

            int screen_x = px;
            int screen_y = py - (int)g_camera_y;

            // Hors écran vertical : on skip
            if (screen_y < -TILE_SIZE || screen_y >= SCREEN_H)
                continue;

            switch (t) {
                case TileType::Wall:
                    draw_sprite16(screen_x, screen_y, tile_wall);
                    break;

                case TileType::Pellet:
                    draw_sprite16(screen_x, screen_y, tile_pacgum);
                    break;

                case TileType::PowerPellet:
                    draw_sprite16(screen_x, screen_y, tile_powerdot);
                    break;

                case TileType::Tunnel:
                    // Pour l’instant : même visuel que pellet vide, ou couleur spéciale
                    // Ici, on dessine juste le fond (rien) ou un petit marqueur si tu veux.
                    break;

                case TileType::GhostHouse:
                    // Maison : pour le moment, rien (ou une légère teinte différente plus tard)
                    break;

                case TileType::GhostDoor:
                    // Tu peux mettre un sprite distinct plus tard.
                    // Pour l’instant, dessinons un mur pour bien voir.
                    draw_sprite16(screen_x, screen_y, tile_wall);
                    break;

                case TileType::PacSpawn:
                case TileType::FruitSpawn:
                case TileType::Empty:
                default:
                    // Rien à dessiner
                    break;
            }
        }
    }
}
