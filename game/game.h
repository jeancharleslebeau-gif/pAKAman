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

struct EyeOffset {
    int dx;
    int dy;
};

struct GhostModePhase {
    GlobalGhostMode mode;
    int duration_ticks;
};

struct GhostModeSchedule {
    GhostModePhase phases[8];
    int phase_count = 0;
};

struct FloatingScore {
    int x;
    int y;
    int value;
    int timer;
};

struct GameState {

    enum class State {
        TitleScreen,
        StartingLevel,
        Playing,
        PacmanDying,
        LevelComplete,
        Paused,
        Options,
        OptionsMenu,
        Highscores,
        GameOver
    };

    struct PortalPair {
        int T0_r = -1, T0_c = -1;
        int E0_r = -1, E0_c = -1;
        int T1_r = -1, T1_c = -1;
        int E1_r = -1, E1_c = -1;
        bool exists = false;
    };

    PortalPair portalH;
    PortalPair portalV;

    State state = State::TitleScreen;
    int levelIndex = 0;

    bool ready_waiting_for_input = false;
    int ready_timer = 0;

    bool gameover_waiting_for_input = false;
    int gameover_timer = 0;

    Maze maze;
    Pacman pacman;
    int pacman_start_r;
    int pacman_start_c;
    int level = 1;

    std::vector<Ghost> ghosts;
    std::vector<FloatingScore> floatingScores;

    enum class DoorState {
        Closed,
        Opening,
        Open
    };

    DoorState ghostDoorState = DoorState::Closed;
    int ghostDoorTimer_ticks = 0;

    int ghostReleaseInterval_ticks = GHOST_RELEASE_INTERVAL_TICKS;
    int elapsed_ticks = 0;

    int score = 0;
    int lives = 3;

    int ghostEatScore = 200;
    int pacman_death_timer = 0;

    GhostModeSchedule schedule;
    int current_phase_index = 0;
    int phase_timer_ticks = 0;
    GlobalGhostMode global_mode = GlobalGhostMode::Scatter;

    int frightened_timer_ticks = 0;
    int frightened_chain = 0;
};

// ------------------------------------------------------------
//  FONCTION TEMPLATE APRÈS GameState
// ------------------------------------------------------------
template<typename Actor>
bool try_portal_wrap(const GameState& g,
                     const GameState::PortalPair& P,
                     Actor& a)
{
    if (!P.exists)
        return false;

    if (!a.isCentered())
        return false;

    // --- Côté 1 -> Côté 0 ---
    // E1 -> T1
    bool from_E1_to_T1 =
        (a.prev_tile_r == P.E1_r && a.prev_tile_c == P.E1_c &&
         a.tile_r      == P.T1_r && a.tile_c      == P.T1_c);

    // T1 -> E1 (demi-tour vers EXT1)
    bool from_T1_to_E1 =
        (a.prev_tile_r == P.T1_r && a.prev_tile_c == P.T1_c &&
         a.tile_r      == P.E1_r && a.tile_c      == P.E1_c);

    if (from_E1_to_T1 || from_T1_to_E1)
    {
        a.tile_r = P.T0_r;
        a.tile_c = P.T0_c;
        a.pixel_offset = 0;
        a.dir = Actor::Dir::Left;   // vers E0
        return true;
    }

    // --- Côté 0 -> Côté 1 ---
    // E0 -> T0
    bool from_E0_to_T0 =
        (a.prev_tile_r == P.E0_r && a.prev_tile_c == P.E0_c &&
         a.tile_r      == P.T0_r && a.tile_c      == P.T0_c);

    // T0 -> E0 (demi-tour vers EXT0)
    bool from_T0_to_E0 =
        (a.prev_tile_r == P.T0_r && a.prev_tile_c == P.T0_c &&
         a.tile_r      == P.E0_r && a.tile_c      == P.E0_c);

    if (from_E0_to_T0 || from_T0_to_E0)
    {
        a.tile_r = P.T1_r;
        a.tile_c = P.T1_c;
        a.pixel_offset = 0;
        a.dir = Actor::Dir::Right;  // vers E1
        return true;
    }

    return false;
}


// ------------------------------------------------------------
//  Prototypes
// ------------------------------------------------------------

void game_init(GameState& g);
void game_update(GameState& g);
void game_draw(const GameState& g);
bool game_is_over(const GameState& g);
bool ghost_can_kill(const Ghost& gh);
bool ghost_can_be_eaten(const Ghost& gh);
void check_pacman_ghost_collision(GameState& g);
void game_trigger_frightened(GameState& g);
void update_floating_scores(GameState& g);
void detect_portals(GameState& g);
