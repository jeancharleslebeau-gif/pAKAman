#pragma once
#include <stdint.h>
#include <vector>
#include "game/config.h"

struct GameState;

struct Ghost {
    enum class Dir : uint8_t {
        None,
        Left,
        Right,
        Up,
        Down
    };

    enum class Mode : uint8_t {
        Scatter,
        Chase,
        Frightened,
        Eaten
    };
	
	enum class HouseState {
		Inside,
		Leaving,
		Outside
	};

HouseState houseState = HouseState::Inside;
int releaseTime_ticks = 0;


    uint16_t color;
    int id;        // 0 rouge, 1 bleu, 2 rose, 3 orange

    int x, y;      // position en pixels
    int start_col, start_row; // position de départ en cases

    Dir  dir       = Dir::Left;
    Mode mode      = Mode::Scatter;
    int  animTick  = 0;

    // Pathfinding pour les yeux (mode Eaten)
    std::vector<Dir> path;

    Ghost(uint16_t color_, int id_, int col, int row)
        : color(color_), id(id_), x(col*TILE_SIZE), y(row*TILE_SIZE),
          start_col(col), start_row(row) {}

    void update(GameState& g);
    void draw() const;

    void reverse_direction();

    void on_start_frightened();
    void on_end_frightened();
};
