#include "maze.h"
#include "assets/assets.h"
#include "core/sprite.h"
#include "game/config.h"
#include "core/graphics.h"

// Caméra globale (définie dans game.cpp)
extern float g_camera_y;

// ---------------------------------------------------------------------
// Définition du labyrinthe B en ASCII (niveau 1, 20 colonnes)
// ---------------------------------------------------------------------
// T = case adjacente à l'entrée du tunnel
// E = portail (case où l'on arrive après wrap)
const char* maze_B_ascii[MAZE_HEIGHT] = {
    "####################",
    "#o.........#......o#",
    "#.##.#####.#.##.##.#",
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
    "#.......#...#......#",
    "#.#.#.#.#.#.####.#.#",
    "#.#.#.#.#.#.#....#.#",
    "#.#.###.#.#...##.#.#",
    "#.#...#.#.#.#....#.#",
    "#.#.###.#.#.######.#",
    "TE.......P........ET",   // entrée gauche T->E, entrée droite E->T
    "####################",
};


// Prépare l'affichage la porte selon son état
void Maze::setGhostDoor(TileType newState)
{
    tiles[ghost_door_row][ghost_door_col] = newState;
}


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
    maze.tunnel_entry_count  = 0;

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
                    t = TileType::GhostDoorClosed;
                    maze.ghost_door_row = row;
                    maze.ghost_door_col = col;
                    break;
                case 'T':
                    t = TileType::Tunnel;
                    break;
                case 'E':
                    t = TileType::TunnelEntry;
                    if (maze.tunnel_entry_count < 2) {
                        maze.tunnel_entry_row[maze.tunnel_entry_count] = row;
                        maze.tunnel_entry_col[maze.tunnel_entry_count] = col;
                        maze.tunnel_entry_count++;
                    }
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
                    // mur spécial avec ouverture
                    draw_sprite16(screen_x, screen_y, tile_tunnel_wall);
                    break;

                case TileType::TunnelEntry: {
                    // Détection automatique de la direction du tunnel
                    bool left  = (col > 0              && tiles[row][col-1] == TileType::Tunnel);
                    bool right = (col < MAZE_WIDTH-1   && tiles[row][col+1] == TileType::Tunnel);
                    bool up    = (row > 0              && tiles[row-1][col] == TileType::Tunnel);
                    bool down  = (row < MAZE_HEIGHT-1  && tiles[row+1][col] == TileType::Tunnel);

                    if (left)
                        draw_sprite16(screen_x, screen_y, tile_tunnel_entry_left);
                    else if (right)
                        draw_sprite16(screen_x, screen_y, tile_tunnel_entry_right);
                    else if (up)
                        draw_sprite16(screen_x, screen_y, tile_tunnel_entry_up);
                    else if (down)
                        draw_sprite16(screen_x, screen_y, tile_tunnel_entry_down);
                    else
                        draw_sprite16(screen_x, screen_y, tile_tunnel_entry_neutral);

                    break;
                }

                case TileType::GhostHouse:
                    // plus tard : couleur spéciale
                    break;

				
				case TileType::GhostDoorClosed:
					draw_sprite16(screen_x, screen_y, tile_ghost_door_closed);
					break;

				case TileType::GhostDoorOpening:
					draw_sprite16(screen_x, screen_y, tile_ghost_door_opening);
					break;

				case TileType::GhostDoorOpen:
					draw_sprite16(screen_x, screen_y, tile_ghost_door_open);
					break;
	

                default:
                    break;
            }
        }
    }
}

