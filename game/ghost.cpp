#include "ghost.h"
#include "assets/assets.h"
#include "core/graphics.h"
#include "core/sprite.h"
#include "maze.h"
#include "game/config.h"
#include "game/game.h"
#include <cstdlib> // rand()

// caméra globale
extern float g_camera_y;

// Helpers pour Ghost
inline int dirX(Ghost::Dir d) {
    switch (d) {
        case Ghost::Dir::Left:  return -1;
        case Ghost::Dir::Right: return +1;
        default: return 0;
    }
}
inline int dirY(Ghost::Dir d) {
    switch (d) {
        case Ghost::Dir::Up:    return -1;
        case Ghost::Dir::Down:  return +1;
        default: return 0;
    }
}

// Helpers pour Pacman
inline int pacDirX(Pacman::Dir d) {
    switch (d) {
        case Pacman::Dir::Left:  return -1;
        case Pacman::Dir::Right: return +1;
        default: return 0;
    }
}
inline int pacDirY(Pacman::Dir d) {
    switch (d) {
        case Pacman::Dir::Up:    return -1;
        case Pacman::Dir::Down:  return +1;
        default: return 0;
    }
}

// === Stratégies de déplacement ===

// 1. Rouge : chasseur direct
Ghost::Dir chooseDirectChase(GameState& g, int row, int col) {
    int pacRow = g.pacman.y / TILE_SIZE;
    int pacCol = g.pacman.x / TILE_SIZE;
    int dRow = pacRow - row;
    int dCol = pacCol - col;

    if (abs(dCol) > abs(dRow))
        return (dCol > 0) ? Ghost::Dir::Right : Ghost::Dir::Left;
    else
        return (dRow > 0) ? Ghost::Dir::Down : Ghost::Dir::Up;
}

// 2. Bleu : anticipateur
Ghost::Dir chooseInterceptor(GameState& g, int row, int col) {
    int pacRow = g.pacman.y / TILE_SIZE;
    int pacCol = g.pacman.x / TILE_SIZE;

    int targetRow = pacRow + 4 * pacDirY(g.pacman.dir);
    int targetCol = pacCol + 4 * pacDirX(g.pacman.dir);

    int dRow = targetRow - row;
    int dCol = targetCol - col;

    if (abs(dCol) > abs(dRow))
        return (dCol > 0) ? Ghost::Dir::Right : Ghost::Dir::Left;
    else
        return (dRow > 0) ? Ghost::Dir::Down : Ghost::Dir::Up;
}

// 3. Rose : harceleur indirect
Ghost::Dir chooseIndirect(GameState& g, int row, int col) {
    int pacRow = g.pacman.y / TILE_SIZE;
    int pacCol = g.pacman.x / TILE_SIZE;
    int dRow = pacRow - row;
    int dCol = pacCol - col;

    if (abs(dRow) + abs(dCol) < 4) {
        // trop proche → s’éloigne
        if (abs(dCol) > abs(dRow))
            return (dCol > 0) ? Ghost::Dir::Left : Ghost::Dir::Right;
        else
            return (dRow > 0) ? Ghost::Dir::Up : Ghost::Dir::Down;
    } else {
        // sinon se rapproche
        if (abs(dCol) > abs(dRow))
            return (dCol > 0) ? Ghost::Dir::Right : Ghost::Dir::Left;
        else
            return (dRow > 0) ? Ghost::Dir::Down : Ghost::Dir::Up;
    }
}

// 4. Orange : aléatoire
Ghost::Dir chooseRandom(GameState& g, int row, int col) {
    Ghost::Dir dirs[4] = {Ghost::Dir::Left, Ghost::Dir::Right, Ghost::Dir::Up, Ghost::Dir::Down};
    for (int tries = 0; tries < 10; tries++) {
        Ghost::Dir d = dirs[rand() % 4];
        int testRow = row + dirY(d);
        int testCol = col + dirX(d);
        TileType t = g.maze.tiles[testRow][testCol];
        if (t != TileType::Wall && t != TileType::GhostDoor) return d;
    }
    return Ghost::Dir::Left;
}

// === Collision symétrique ===
bool checkCollision(const GameState& g, int nextX, int nextY, Ghost::Dir dir) {
    auto is_blocking = [&](int r, int c) {
        TileType t = g.maze.tiles[r][c];
        return (t == TileType::Wall || t == TileType::GhostDoor);
    };

    switch (dir) {
        case Ghost::Dir::Right: {
            int right = nextX + GHOST_SIZE - 1;
            int rowTop = nextY / TILE_SIZE;
            int rowBottom = (nextY + GHOST_SIZE - 1) / TILE_SIZE;
            int colRight = right / TILE_SIZE;
            return is_blocking(rowTop, colRight) || is_blocking(rowBottom, colRight);
        }
        case Ghost::Dir::Left: {
            int left = nextX;
            int rowTop = nextY / TILE_SIZE;
            int rowBottom = (nextY + GHOST_SIZE - 1) / TILE_SIZE;
            int colLeft = left / TILE_SIZE;
            return is_blocking(rowTop, colLeft) || is_blocking(rowBottom, colLeft);
        }
        case Ghost::Dir::Down: {
            int bottom = nextY + GHOST_SIZE - 1;
            int colLeft = nextX / TILE_SIZE;
            int colRight = (nextX + GHOST_SIZE - 1) / TILE_SIZE;
            int rowBottom = bottom / TILE_SIZE;
            return is_blocking(rowBottom, colLeft) || is_blocking(rowBottom, colRight);
        }
        case Ghost::Dir::Up: {
            int top = nextY;
            int colLeft = nextX / TILE_SIZE;
            int colRight = (nextX + GHOST_SIZE - 1) / TILE_SIZE;
            int rowTop = top / TILE_SIZE;
            return is_blocking(rowTop, colLeft) || is_blocking(rowTop, colRight);
        }
        default: return false;
    }
}

// === Update ===
void Ghost::update(GameState& g) {
    int col = x / TILE_SIZE;
    int row = y / TILE_SIZE;

    int centerX = x + GHOST_SIZE/2;
    int centerY = y + GHOST_SIZE/2;
    int colCenter = col*TILE_SIZE + TILE_SIZE/2;
    int rowCenter = row*TILE_SIZE + TILE_SIZE/2;
    bool isCentered = (abs(centerX-colCenter)<=GHOST_SPEED) &&
                      (abs(centerY-rowCenter)<=GHOST_SPEED);

    if (isCentered) {
        Dir newDir = dir;
        switch (id) {
            case 0: newDir = chooseDirectChase(g, row, col); break;
            case 1: newDir = chooseInterceptor(g, row, col); break;
            case 2: newDir = chooseIndirect(g, row, col); break;
            case 3: newDir = chooseRandom(g, row, col); break;
        }
        int testRow = row + dirY(newDir);
        int testCol = col + dirX(newDir);
        TileType t = g.maze.tiles[testRow][testCol];
        if (t != TileType::Wall && t != TileType::GhostDoor)
            dir = newDir;
    }

    int nextX = x + dirX(dir)*GHOST_SPEED;
    int nextY = y + dirY(dir)*GHOST_SPEED;
    if (!checkCollision(g, nextX, nextY, dir)) {
        x = nextX;
        y = nextY;
    }

    animTick++;
}

// === Draw ===
void Ghost::draw() const {
    const uint16_t* anim[2];
    switch (id) {
        case 0: anim[0] = ghost_red_0;    anim[1] = ghost_red_1;    break;
        case 1: anim[0] = ghost_blue_0;   anim[1] = ghost_blue_1;   break;
        case 2: anim[0] = ghost_pink_0;   anim[1] = ghost_pink_1;   break;
        case 3: anim[0] = ghost_orange_0; anim[1] = ghost_orange_1; break;
        default: anim[0] = ghost_red_0;   anim[1] = ghost_red_1;    break;
    }

    int frame = (animTick / 8) % 2;
    int screen_x = x;
    int screen_y = y - (int)g_camera_y;

    gfx_drawSprite(screen_x, screen_y, anim[frame], GHOST_SIZE, GHOST_SIZE);

    // Yeux dynamiques
    uint16_t eyeColor = COLOR_LIGHTBLUE;
    int eye1x, eye1y, eye2x, eye2y;

    switch (dir) {
        case Dir::Left:
            eye1x = screen_x + 1; eye1y = screen_y + 5;
            eye2x = screen_x + 7; eye2y = screen_y + 5;
            break;
        case Dir::Right:
            eye1x = screen_x + 3; eye1y = screen_y + 5;
            eye2x = screen_x + 9; eye2y = screen_y + 5;
            break;
        case Dir::Down:
            eye1x = screen_x + 2; eye1y = screen_y + 6;
            eye2x = screen_x + 8; eye2y = screen_y + 6;
            break;
        case Dir::Up:
            eye1x = screen_x + 2; eye1y = screen_y + 3;
            eye2x = screen_x + 8; eye2y = screen_y + 3;
            break;
        case Dir::None:
        default:
            eye1x = screen_x + 3; eye1y = screen_y + 5;
            eye2x = screen_x + 9; eye2y = screen_y + 5;
            break;
    }

    // Dessin des yeux (2x2 pixels chacun)
    for (int dy = 0; dy < 2; ++dy) {
        for (int dx = 0; dx < 2; ++dx) {
            lcd_putpixel(eye1x + dx, eye1y + dy, eyeColor);
            lcd_putpixel(eye2x + dx, eye2y + dy, eyeColor);
        }
    }
}
