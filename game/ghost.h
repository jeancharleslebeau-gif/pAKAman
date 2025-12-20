#pragma once
#include <cstdint>
#include "game/config.h"

struct GameState; // forward

class Ghost {
public:
    enum class Dir { Left, Right, Up, Down, None };

    int x, y;          // position (coin haut-gauche du sprite)
    Dir dir;           // direction actuelle
    int animTick;      // compteur animation
    int id;            // identifiant du fantôme (0=rouge,1=bleu,2=rose,3=orange)
    uint16_t color;    // couleur principale (optionnel)

    Ghost(uint16_t c, int ghostId, int startX=0, int startY=0)
        : x(startX), y(startY), dir(Dir::Left), animTick(0), id(ghostId), color(c) {}

    void update(GameState& g);
    void draw() const;
};
