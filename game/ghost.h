#pragma once
#include <stdint.h>
#include <vector>
#include "game/config.h"

struct GameState;

struct EyeOffset {
    int dx, dy; // position unique du sprite yeux (contenant les 2 yeux)
};

static const EyeOffset eyeOffsets[4] = {
    {2, 4}, // Left
    {4, 4}, // Right
    {3, 2}, // Up
    {3, 6}  // Down
};


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
	
    // --- New arcade IA helpers ---
    void getScatterTarget(const GameState& g, int& tr, int& tc) const;
    void getChaseTarget(const GameState& g, int& tr, int& tc) const;

    Dir chooseDirectionTowardsTarget(const GameState& g,
                                     int row, int col,
                                     int targetRow, int targetCol,
                                     bool is_eyes) const;

    Dir chooseRandomDir(const GameState& g,
                        int row, int col,
                        bool is_eyes) const;


HouseState houseState = HouseState::Inside;
int releaseTime_ticks = 0;


    uint16_t color;
    int id;        // 0 rouge, 1 bleu, 2 rose, 3 orange

    int x, y;      // position en pixels
    int start_col, start_row; // position de départ en cases

    Dir  dir       = Dir::Left;
    Mode mode      = Mode::Scatter;
	// --- Frightened mode timers ---
	int frightened_timer = 0;            // temps restant en frightened

    int  animTick  = 0;

    // Pathfinding pour les yeux (mode Eaten)
    std::vector<Dir> path;
	
	bool wouldCollideWithOtherGhost(const GameState& g, int row, int col) const;
	std::vector<Dir> getValidDirections(const GameState& g, int row, int col, bool is_eyes) const;

	bool collidesWithOtherGhost(const GameState& g, int nextRow, int nextCol) const;

    Ghost(uint16_t color_, int id_, int col, int row)
        : color(color_), id(id_), x(col*TILE_SIZE), y(row*TILE_SIZE),
          start_col(col), start_row(row) {}

    void update(GameState& g);
    void draw() const;

    void reverse_direction();

    void on_start_frightened();
    void on_end_frightened();
};
