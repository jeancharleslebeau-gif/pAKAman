#include "pacman.h"
#include "core/audio.h"
#include "assets/assets.h"
#include "core/graphics.h"
#include "core/input.h"
#include "core/sprite.h"
#include "maze.h"
#include "game/config.h"
#include "game/game.h"

extern float g_camera_y;

// Déclaration de la fonction dans game.cpp
void game_trigger_frightened(GameState& g);

int Pacman::dirX() const {
    switch (dir) {
        case Dir::Left:  return -1;
        case Dir::Right: return +1;
        default:
            if (debug) printf("dirX default, dir=%d\n", (int)dir);
            return 0;
    }
}
int Pacman::dirY() const {
    switch (dir) {
        case Dir::Up:    return -1;
        case Dir::Down:  return +1;
        default:
            if (debug) printf("dirY default, dir=%d\n", (int)dir);
            return 0;
    }
}

// Tunnel wrap T->E côté Pac-Man
static bool pacman_try_tunnel_wrap(GameState& g, Pacman& p, int& nextX, int& nextY)
{
    int row = p.y / TILE_SIZE;
    int col = p.x / TILE_SIZE;

    int dr = p.dirY();
    int dc = p.dirX();

    int curr_row = row;
    int curr_col = col;
    int next_row = row + dr;
    int next_col = col + dc;

    if (curr_row < 0 || curr_row >= MAZE_HEIGHT ||
        curr_col < 0 || curr_col >= MAZE_WIDTH ||
        next_row < 0 || next_row >= MAZE_HEIGHT ||
        next_col < 0 || next_col >= MAZE_WIDTH)
        return false;

    TileType curr_t = g.maze.tiles[curr_row][curr_col];
    TileType next_t = g.maze.tiles[next_row][next_col];

    // Pac-Man doit être CENTRÉ dans la case E
    if (!p.isCentered())
        return false;

    // Condition pacman va de E vers T 
    if (!(curr_t == TileType::TunnelEntry && next_t == TileType::Tunnel))
        return false;

    // Pac-Man doit se diriger vers T
    if (dr == 0 && dc == 0)
        return false;

    // On cherche les deux T
    int t0r = -1, t0c = -1;
    int t1r = -1, t1c = -1;
    int countT = 0;

    for (int r = 0; r < MAZE_HEIGHT; r++)
    for (int c = 0; c < MAZE_WIDTH; c++)
    {
        if (g.maze.tiles[r][c] == TileType::Tunnel)
        {
            if (countT == 0) { t0r = r; t0c = c; }
            else if (countT == 1) { t1r = r; t1c = c; }
            countT++;
        }
    }

    if (countT != 2)
        return false;

    int dest_r, dest_c;

    // Déterminer l'autre T
    if (next_row == t0r && next_col == t0c)
    {
        dest_r = t1r;
        dest_c = t1c;
    }
    else
    {
        dest_r = t0r;
        dest_c = t0c;
    }

    // Positionner Pac-Man sur l'autre T
    p.x = dest_c * TILE_SIZE + PACMAN_OFFSET;
    p.y = dest_r * TILE_SIZE + PACMAN_OFFSET;

    // Trouver l'E adjacent pour la sortie
    const int dr2[4] = {-1, +1, 0, 0};
    const int dc2[4] = {0, 0, -1, +1};
    const Pacman::Dir d2[4] = {
        Pacman::Dir::Up, Pacman::Dir::Down,
        Pacman::Dir::Left, Pacman::Dir::Right
    };

    for (int i = 0; i < 4; i++)
    {
        int nr = dest_r + dr2[i];
        int nc = dest_c + dc2[i];

        if (nr < 0 || nr >= MAZE_HEIGHT || nc < 0 || nc >= MAZE_WIDTH)
            continue;

        if (g.maze.tiles[nr][nc] == TileType::TunnelEntry)
        {
            p.dir = d2[i];
            break;
        }
    }

    nextX = p.x + p.dirX() * PACMAN_SPEED;
    nextY = p.y + p.dirY() * PACMAN_SPEED;

    return true;
}


void Pacman::update(GameState& g) {
    Keys k;
    input_poll(k);

    Dir requestedDir = Dir::None;
    const int DEADZONE = 500;
    if (k.left  || (k.joxx < JOYX_MID - DEADZONE)) requestedDir = Dir::Left;
    if (k.right || (k.joxx > JOYX_MID + DEADZONE)) requestedDir = Dir::Right;
    if (k.up    || (k.joxy > JOYX_MID + DEADZONE)) requestedDir = Dir::Up;
    if (k.down  || (k.joxy < JOYX_MID - DEADZONE)) requestedDir = Dir::Down;

    int row = y / TILE_SIZE;
    int col = x / TILE_SIZE;

    int centerX = x + PACMAN_SIZE / 2;
    int centerY = y + PACMAN_SIZE / 2;
    int colCenter = col * TILE_SIZE + TILE_SIZE / 2;
    int rowCenter = row * TILE_SIZE + TILE_SIZE / 2;
    bool isCentered = (abs(centerX - colCenter) <= PACMAN_SPEED) &&
                      (abs(centerY - rowCenter) <= PACMAN_SPEED);

    if (isCentered) {
        if (dir == Dir::Left || dir == Dir::Right) {
            y = row * TILE_SIZE + (TILE_SIZE/2 - PACMAN_SIZE/2);
        } else if (dir == Dir::Up || dir == Dir::Down) {
            x = col * TILE_SIZE + (TILE_SIZE/2 - PACMAN_SIZE/2);
        }

        if (requestedDir != Dir::None) {
            int drow = (requestedDir == Dir::Up)   ? -1 : (requestedDir == Dir::Down) ? +1 : 0;
            int dcol = (requestedDir == Dir::Left) ? -1 : (requestedDir == Dir::Right) ? +1 : 0;
            int chkRow = row + drow;
            int chkCol = col + dcol;

            TileType t = g.maze.tiles[chkRow][chkCol];
            bool isWall = (
				t == TileType::Wall ||
				t == TileType::GhostDoorClosed ||
				t == TileType::GhostDoorOpening ||
				t == TileType::GhostDoorOpen ||
				t == TileType::GhostHouse
			);

            if (!isWall) {
                dir = requestedDir;
            }
        }
    }

    int nextX = x + dirX() * PACMAN_SPEED;
    int nextY = y + dirY() * PACMAN_SPEED;
    bool blocked = false;

    auto is_blocking = [&](int r, int c) {
        TileType t = g.maze.tiles[r][c];
        return (
			t == TileType::Wall ||
			t == TileType::GhostDoorClosed ||
			t == TileType::GhostDoorOpening ||
			t == TileType::GhostDoorOpen ||
			t == TileType::GhostHouse
		);

    };

    switch (dir) {
        case Dir::Right: {
            int right = nextX + PACMAN_SIZE - 1;
            int rowTop    = nextY / TILE_SIZE;
            int rowBottom = (nextY + PACMAN_SIZE - 1) / TILE_SIZE;
            int colRight  = right / TILE_SIZE;
            blocked = is_blocking(rowTop, colRight) || is_blocking(rowBottom, colRight);
            break;
        }
        case Dir::Left: {
            int left = nextX;
            int rowTop    = nextY / TILE_SIZE;
            int rowBottom = (nextY + PACMAN_SIZE - 1) / TILE_SIZE;
            int colLeft   = left / TILE_SIZE;
            blocked = is_blocking(rowTop, colLeft) || is_blocking(rowBottom, colLeft);
            break;
        }
        case Dir::Down: {
            int bottom = nextY + PACMAN_SIZE - 1;
            int colLeft  = nextX / TILE_SIZE;
            int colRight = (nextX + PACMAN_SIZE - 1) / TILE_SIZE;
            int rowBottom = bottom / TILE_SIZE;
            blocked = is_blocking(rowBottom, colLeft) || is_blocking(rowBottom, colRight);
            break;
        }
        case Dir::Up: {
            int top = nextY;
            int colLeft  = nextX / TILE_SIZE;
            int colRight = (nextX + PACMAN_SIZE - 1) / TILE_SIZE;
            int rowTop   = top / TILE_SIZE;
            blocked = is_blocking(rowTop, colLeft) || is_blocking(rowTop, colRight);
            break;
        }
        default:
            break;
    }

    // Tunnel wrap
    int tmpX = nextX;
    int tmpY = nextY;
    if (!blocked && pacman_try_tunnel_wrap(g, *this, tmpX, tmpY)) {
        x = tmpX;
        y = tmpY;
    } else if (!blocked) {
        x = nextX;
        y = nextY;
    }

    // Collecte
    row = y / TILE_SIZE;
    col = x / TILE_SIZE;

    TileType& cell = g.maze.tiles[row][col];
    if (cell == TileType::Pellet) {
        cell = TileType::Empty;
        g.maze.pellet_count--;
        g.score += 10;
        audio_play_pacgomme();
    } else if (cell == TileType::PowerPellet) {
        cell = TileType::Empty;
        g.maze.power_pellet_count--;
        g.score += 50;
        audio_play_power();
        game_trigger_frightened(g);
    }

    animTick++;
}

void Pacman::draw() const {
    static const uint16_t* right[3] = { pacman_right_0, pacman_right_1, pacman_right_2 };
    static const uint16_t* left [3] = { pacman_left_0,  pacman_left_1,  pacman_left_2  };
    static const uint16_t* up   [3] = { pacman_up_0,    pacman_up_1,    pacman_up_2    };
    static const uint16_t* down [3] = { pacman_down_0,  pacman_down_1,  pacman_down_2  };

    const uint16_t* const* anim = nullptr;
    switch (dir) {
        case Dir::Right: anim = right; break;
        case Dir::Left:  anim = left;  break;
        case Dir::Up:    anim = up;    break;
        case Dir::Down:  anim = down;  break;
        default:         anim = right; break;
    }

    int frame = (animTick / 6) % 3;
    int screen_x = x;
    int screen_y = y - (int)g_camera_y;

    gfx_drawSprite(screen_x, screen_y, anim[frame], PACMAN_SIZE, PACMAN_SIZE);
}
