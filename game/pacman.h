#pragma once
#include <cstdint>
#include "game/config.h"

struct GameState; // forward

class Pacman {
public:
    enum class Dir { None=0, Left=1, Right=2, Up=3, Down=4 };;

    int x, y;
    Dir dir;
    int animTick;

    Pacman(int startX=0, int startY=0)
        : x(startX), y(startY), dir(Dir::None), animTick(0) {}

    void update(GameState& g);
    void draw() const;

    int dirX() const;
    int dirY() const;
};
