#pragma once
#include "config.h"
#include "maze.h"
#include <vector>

struct GameState;

bool is_walkable_for_eyes(TileType t);

struct Ghost
{
    enum class Mode {
        Scatter,
        Chase,
        Frightened,
        Eaten
    };
	
	struct EyeOffset { 
		int dx; 
		int dy; 
	};

    enum class HouseState {
        Inside,
        Leaving,
        Outside,
        Returning
    };

    enum class Dir {
        None, Left, Right, Up, Down
    };

    // Position logique
    int tile_r, tile_c;
	int prev_tile_r = 0;
	int prev_tile_c = 0;

    int pixel_offset;

    // Position pixel
    int x, y;

    // Direction
    Dir dir;
    Dir next_dir;

    // Mode
    Mode mode;
    Mode previous_mode;

    // Maison
    HouseState houseState;
    int eaten_timer;

    // Pathfinding
    std::vector<std::pair<int,int>> path;

    // Vitesse
    int speed_normal;
    int speed_frightened;
    int speed_tunnel;
    int speed_eyes;

    // Identité
    int id;

    // Animation
    int animTick = 0;

    // Spawn
    int start_row;
    int start_col;

    // Release timer
    int releaseTime_ticks = 0;

    // Constructeur
    Ghost(int id_, int start_c, int start_r);

    // API
    void update(GameState& g);
    void draw() const;
    void reset_to_start();

    // Frightened
    void on_start_frightened();
    void on_end_frightened();

    // Utilitaires
    static int dirX(Dir d);
    static int dirY(Dir d);
    bool isCentered() const;

    // IA
    std::vector<Dir> getValidDirections(const GameState& g,
                                        int row, int col,
                                        bool is_eyes) const;

    Dir chooseDirectionTowardsTarget(const GameState& g,
                                     int row, int col,
                                     int tr, int tc,
                                     bool is_eyes) const;

    Dir chooseRandomDir(const GameState& g,
                        int row, int col,
                        bool is_eyes) const;

    void getScatterTarget(const GameState& g, int& tr, int& tc) const;
    void getChaseTarget(const GameState& g, int& tr, int& tc) const;
	Dir chooseDirectionInsideHouse(const GameState& g, int row, int col);

    void reverse_direction();
};
