#pragma once
#include <vector>
#include "pacman.h"
#include "ghost.h"
#include "maze.h"
#include "config.h"

enum class GlobalGhostMode : uint8_t {
    Scatter,
    Chase,
    Frightened
};

struct GhostModePhase {
    GlobalGhostMode mode;
    int duration_ticks; // en ticks de game_update
};

struct GhostModeSchedule {
    GhostModePhase phases[8];
    int phase_count = 0;
};

struct GameState {

    enum class State {
        Title,
        Playing,
        Paused,
        Options,
        Highscores,
        GameOver
    };
	
	State state = State::Title;
    int levelIndex = 0;

    Maze    maze;
    Pacman  pacman;
    std::vector<Ghost> ghosts;

	// Porte de la maison des fantômes
    enum class DoorState {
        Closed,
        Opening,
        Open
    };

    DoorState ghostDoorState = DoorState::Closed;
    int ghostDoorTimer_ticks = 0;

    // Sortie séquentielle des fantômes
	int ghostReleaseInterval_ticks = GHOST_RELEASE_INTERVAL_TICKS;
    int elapsed_ticks = 0;                   // temps global en ticks

	
    int score = 0;
    int lives = 3;

    // Modes temporels
    GhostModeSchedule schedule;
    int current_phase_index = 0;
    int phase_timer_ticks   = 0;
    GlobalGhostMode global_mode = GlobalGhostMode::Scatter;

    // Frightened
    int frightened_timer_ticks = 0;
    int frightened_chain = 0; // pour 200/400/800/1600
};

void game_init(GameState& g);
void game_update(GameState& g);
void game_draw(const GameState& g);
bool game_is_over(const GameState& g);
