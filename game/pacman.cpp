#include "pacman.h"
#include "core/audio.h"
#include "assets/assets.h"
#include "core/graphics.h"
#include "core/input.h"
#include "core/sprite.h"
#include "maze.h"
#include "game/config.h"
#include "game/game.h"

// caméra globale définie dans game.cpp
extern float g_camera_y;

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

    // --- Direction change si centré ---
    int centerX = x + PACMAN_SIZE / 2;
    int centerY = y + PACMAN_SIZE / 2;
    int colCenter = col * TILE_SIZE + TILE_SIZE / 2;
    int rowCenter = row * TILE_SIZE + TILE_SIZE / 2;
    bool isCentered = (abs(centerX - colCenter) <= PACMAN_SPEED) &&
                      (abs(centerY - rowCenter) <= PACMAN_SPEED);

    if (isCentered) {
        // Snap automatique au centre du couloir
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
            bool isWall = (t == TileType::Wall || t == TileType::GhostDoor);
            if (!isWall) {
                dir = requestedDir;
                if (debug) printf("[T=%d] Dir change accepted: new=%d\n", animTick, (int)dir);
            } else if (debug) {
                printf("[T=%d] Dir change blocked by wall/door\n", animTick);
            }
        }
    }

    // --- Tentative d’avancée ---
    int nextX = x + dirX() * PACMAN_SPEED;
    int nextY = y + dirY() * PACMAN_SPEED;
    bool blocked = false;

    auto is_blocking = [&](int r, int c) {
        TileType t = g.maze.tiles[r][c];
        return (t == TileType::Wall || t == TileType::GhostDoor);
    };

    switch (dir) {
        case Dir::Right: {
            int right = nextX + PACMAN_SIZE - 1;
            int rowTop    = nextY / TILE_SIZE;
            int rowBottom = (nextY + PACMAN_SIZE - 1) / TILE_SIZE;
            int colRight  = right / TILE_SIZE;
            blocked = is_blocking(rowTop, colRight) || is_blocking(rowBottom, colRight);
            if (debug) printf("[T=%d] Collision RIGHT: colRight=%d rows=%d,%d blocked=%d\n",
                              animTick, colRight, rowTop, rowBottom, blocked);
            break;
        }
        case Dir::Left: {
            int left = nextX;
            int rowTop    = nextY / TILE_SIZE;
            int rowBottom = (nextY + PACMAN_SIZE - 1) / TILE_SIZE;
            int colLeft   = left / TILE_SIZE;
            blocked = is_blocking(rowTop, colLeft) || is_blocking(rowBottom, colLeft);
            if (debug) printf("[T=%d] Collision LEFT: colLeft=%d rows=%d,%d blocked=%d\n",
                              animTick, colLeft, rowTop, rowBottom, blocked);
            break;
        }
        case Dir::Down: {
            int bottom = nextY + PACMAN_SIZE - 1;
            int colLeft  = nextX / TILE_SIZE;
            int colRight = (nextX + PACMAN_SIZE - 1) / TILE_SIZE;
            int rowBottom = bottom / TILE_SIZE;
            blocked = is_blocking(rowBottom, colLeft) || is_blocking(rowBottom, colRight);
            if (debug) printf("[T=%d] Collision DOWN: rowBottom=%d cols=%d,%d blocked=%d\n",
                              animTick, rowBottom, colLeft, colRight, blocked);
            break;
        }
        case Dir::Up: {
            int top = nextY;
            int colLeft  = nextX / TILE_SIZE;
            int colRight = (nextX + PACMAN_SIZE - 1) / TILE_SIZE;
            int rowTop   = top / TILE_SIZE;
            blocked = is_blocking(rowTop, colLeft) || is_blocking(rowTop, colRight);
            if (debug) printf("[T=%d] Collision UP: rowTop=%d cols=%d,%d blocked=%d\n",
                              animTick, rowTop, colLeft, colRight, blocked);
            break;
        }
        default:
            break;
    }

    if (!blocked) {
        x = nextX;
        y = nextY;
        if (debug) printf("[T=%d] Moved: x=%d y=%d\n", animTick, x, y);
    } else {
        if (debug) printf("[T=%d] Collision: blocked dir=%d x=%d y=%d\n", animTick, (int)dir, x, y);
    }

    // --- Collecte ---
    // On recalcule la case après déplacement
    row = y / TILE_SIZE;
    col = x / TILE_SIZE;

    TileType& cell = g.maze.tiles[row][col];
    if (cell == TileType::Pellet) {
        cell = TileType::Empty;
        g.maze.pellet_count--;
        g.score += 10;
        audio_play_pacgomme();
        if (debug) printf("[T=%d] Pellet eaten, score=%d\n", animTick, g.score);
    } else if (cell == TileType::PowerPellet) {
        cell = TileType::Empty;
        g.maze.power_pellet_count--;
        g.score += 50;
        audio_play_power();
        // TODO: passer les fantômes en mode frightened
        if (debug) printf("[T=%d] Power pellet eaten, score=%d\n", animTick, g.score);
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
