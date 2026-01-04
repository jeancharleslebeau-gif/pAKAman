#include "game.h"
#include "level.h"
#include "maze.h"
#include "core/graphics.h"
#include <string>
#include "assets/pacman_pmf.h"
#include "lib/audio_pmf.h"
#include "core/audio.h"
#include "config.h"
#include <algorithm>
#include "assets/assets.h"
#include "core/sprite.h"
#include "core/input.h"

extern AudioPMF audioPMF;
extern int debug;

float g_camera_y = 0.0f;

#define DBG(code) do { if (debug) { code; } } while(0)

/*
============================================================
  SCHEDULE SCATTER / CHASE (NIVEAU 1)
============================================================
Arcade-like simplifié :
  - 7s Scatter
  - 20s Chase
  - 7s Scatter
  - Chase "infini"
============================================================
*/
static void init_ghost_schedule(GameState& g)
{
    g.schedule.phase_count = 4;
    g.schedule.phases[0] = { GlobalGhostMode::Scatter, 7 * 60 };
    g.schedule.phases[1] = { GlobalGhostMode::Chase,   20 * 60 };
    g.schedule.phases[2] = { GlobalGhostMode::Scatter, 7 * 60 };
    g.schedule.phases[3] = { GlobalGhostMode::Chase,   9999 * 60 };

    g.current_phase_index = 0;
    g.phase_timer_ticks   = g.schedule.phases[0].duration_ticks;
    g.global_mode         = g.schedule.phases[0].mode;
}

/*
============================================================
  INVERSION DES DIRECTIONS DES FANTÔMES
============================================================
Utilisé lors des changements Scatter/Chase, et au début du
mode Frightened (via Ghost::on_start_frightened).
============================================================
*/
static void reverse_ghost_directions(GameState& g)
{
    for (auto& ghost : g.ghosts)
        ghost.reverse_direction();
}

/*
============================================================
  CONDITIONS DE KILL / EAT
============================================================
*/
bool ghost_can_kill(const Ghost& gh)
{
    if (gh.mode == Ghost::Mode::Eaten) return false;
    if (gh.houseState != Ghost::HouseState::Outside) return false;
    if (gh.mode == Ghost::Mode::Frightened) return false;
    return true;
}

bool ghost_can_be_eaten(const Ghost& gh)
{
    return (gh.mode == Ghost::Mode::Frightened &&
            gh.houseState == Ghost::HouseState::Outside);
}

/*
============================================================
  SCORES FLOTTANTS
============================================================
*/
void update_floating_scores(GameState& g)
{
    for (auto& fs : g.floatingScores)
        fs.timer--;

    g.floatingScores.erase(
        std::remove_if(g.floatingScores.begin(), g.floatingScores.end(),
                       [](const FloatingScore& fs){ return fs.timer <= 0; }),
        g.floatingScores.end());
}

/*
============================================================
  FRIGHTENED : DÉCLENCHEMENT
============================================================
*/
void game_trigger_frightened(GameState& g)
{
    g.frightened_timer_ticks = g.frightened_duration_ticks;
    g.frightened_chain = 0;

    for (auto& gh : g.ghosts)
        gh.on_start_frightened();
}

/*
============================================================
  FRIGHTENED : MISE À JOUR GLOBALE
============================================================
*/
static void update_frightened(GameState& g)
{
    if (g.frightened_timer_ticks > 0)
    {
        g.frightened_timer_ticks--;

        if (g.frightened_timer_ticks == 0)
        {
            for (auto& gh : g.ghosts)
                gh.on_end_frightened();
        }
    }
}

/*
============================================================
  COLLISION PAC-MAN / FANTÔMES
============================================================
*/
void check_pacman_ghost_collision(GameState& g)
{
    int px = g.pacman.x + PACMAN_SIZE / 2;
    int py = g.pacman.y + PACMAN_SIZE / 2;

    for (auto& gh : g.ghosts)
    {
        int gx = gh.x + GHOST_SIZE / 2;
        int gy = gh.y + GHOST_SIZE / 2;

        int dx = px - gx;
        int dy = py - gy;
        int dist2 = dx*dx + dy*dy;

        const int COLLISION_RADIUS = 12;

        if (dist2 < COLLISION_RADIUS * COLLISION_RADIUS)
        {
            // --- Fantôme mangeable ---
            if (ghost_can_be_eaten(gh))
            {
                gh.mode = Ghost::Mode::Eaten;
                gh.path.clear();

                g.score += g.ghostEatScore;

                FloatingScore fs;
                fs.x = gh.x + GHOST_SIZE / 2;
                fs.y = gh.y;
                fs.value = g.ghostEatScore;
                fs.timer = 60;
                g.floatingScores.push_back(fs);

                g.ghostEatScore *= 2;   // 200 → 400 → 800 → 1600
                audio_play_eatghost();
                continue;
            }

            // --- Fantôme létal pour Pac-Man ---
            if (ghost_can_kill(gh))
            {
                g.state = GameState::State::PacmanDying;
                g.pacman_death_timer = 0;

                audioPMF.stop();
                audio_play_death();
                return;
            }
        }
    }
}

/*
============================================================
  DÉTECTION AUTOMATIQUE DES TUNNELS
============================================================
*/
void detect_portals(GameState& g)
{
    const Maze& m = g.maze;

    g.portalH = GameState::PortalPair{};
    g.portalV = GameState::PortalPair{};

    // --- Horizontal ---
    for (int r = 0; r < MAZE_HEIGHT; r++)
        if (m.tiles[r][0] == TileType::Tunnel)
        {
            g.portalH.T0_r = r;
            g.portalH.T0_c = 0;
            g.portalH.E0_r = r;
            g.portalH.E0_c = 1;
            break;
        }

    for (int r = 0; r < MAZE_HEIGHT; r++)
        if (m.tiles[r][MAZE_WIDTH-1] == TileType::Tunnel)
        {
            g.portalH.T1_r = r;
            g.portalH.T1_c = MAZE_WIDTH-1;
            g.portalH.E1_r = r;
            g.portalH.E1_c = MAZE_WIDTH-2;
            break;
        }

    if ((g.portalH.T0_r != -1) != (g.portalH.T1_r != -1))
        printf("ERREUR MAP: Portail horizontal incohérent (T0 sans T1 ou inverse)\n");
    else if (g.portalH.T0_r != -1)
        g.portalH.exists = true;

    // --- Vertical ---
    for (int c = 0; c < MAZE_WIDTH; c++)
        if (m.tiles[0][c] == TileType::Tunnel)
        {
            g.portalV.T0_r = 0;
            g.portalV.T0_c = c;
            g.portalV.E0_r = 1;
            g.portalV.E0_c = c;
            break;
        }

    for (int c = 0; c < MAZE_WIDTH; c++)
        if (m.tiles[MAZE_HEIGHT-1][c] == TileType::Tunnel)
        {
            g.portalV.T1_r = MAZE_HEIGHT-1;
            g.portalV.T1_c = c;
            g.portalV.E1_r = MAZE_HEIGHT-2;
            g.portalV.E1_c = c;
            break;
        }

    if ((g.portalV.T0_r != -1) != (g.portalV.T1_r != -1))
        printf("ERREUR MAP: Portail vertical incohérent (T0 sans T1 ou inverse)\n");
    else if (g.portalV.T0_r != -1)
        g.portalV.exists = true;
}

/*
============================================================
  RESET PARTIEL DU NIVEAU (APRÈS MORT)
============================================================
*/
static void reset_level(GameState& g)
{
    // Reset Pac-Man
    g.pacman.tile_r = g.pacman_start_r;
    g.pacman.tile_c = g.pacman_start_c;
    g.pacman.pixel_offset = 0;
    g.pacman.dir = Pacman::Dir::Left;
    g.pacman.next_dir = Pacman::Dir::Left;

    // Reset ghosts
    for (auto& gh : g.ghosts)
    {
        gh.start_row = g.maze.ghost_spawn_row[gh.id];
        gh.start_col = g.maze.ghost_spawn_col[gh.id];
        gh.reset_to_start();

        gh.houseState     = Ghost::HouseState::Inside;
        gh.mode           = Ghost::Mode::Scatter;
        gh.previous_mode  = Ghost::Mode::Scatter;
        gh.eaten_timer    = 0;
        gh.path.clear();

        gh.pixel_offset = 0;
        gh.x = gh.tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;
        gh.y = gh.tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;
    }

    // Reset ghost house
    g.ghostDoorState = GameState::DoorState::Closed;

    // Reset séquence Scatter/Chase (arcade-like)
    init_ghost_schedule(g);

    // Reset timers
    g.elapsed_ticks = 0;
    g.ready_waiting_for_input = true;
    g.ready_timer = 30;

    if (g_audio_settings.music_enabled) {
        audioPMF.stop();
        // La musique PMF sera relancée après READY!
    } else {
        audioPMF.stop();
    }
}

/*
============================================================
  RESET COMPLET DU NIVEAU (NIVEAU SUIVANT)
============================================================
*/
static void reset_level_full(GameState& g)
{
    level_init(g);
    detect_portals(g);

    g.pacman = Pacman(
        g.maze.pac_spawn_col,
        g.maze.pac_spawn_row
    );
    g.pacman_start_r = g.maze.pac_spawn_row;
    g.pacman_start_c = g.maze.pac_spawn_col;

    for (auto& gh : g.ghosts)
    {
        gh.start_row = g.maze.ghost_spawn_row[gh.id];
        gh.start_col = g.maze.ghost_spawn_col[gh.id];
        gh.reset_to_start();

        gh.houseState     = Ghost::HouseState::Inside;
        gh.mode           = Ghost::Mode::Scatter;
        gh.previous_mode  = Ghost::Mode::Scatter;
        gh.eaten_timer    = 0;
        gh.path.clear();

        gh.pixel_offset = 0;
        gh.x = gh.tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;
        gh.y = gh.tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;
    }

    init_ghost_schedule(g);
    int t = FIRST_GHOST_RELEASE_TICKS;
    for (auto& ghost : g.ghosts) {
        ghost.releaseTime_ticks = t;
        t += g.ghostReleaseInterval_ticks;
    }

    g.frightened_timer_ticks = 0;
    g.frightened_chain = 0;
    g.ghostDoorState = GameState::DoorState::Closed;
    g.ghostDoorTimer_ticks = 0;
    g.elapsed_ticks = 0;
    g.ghostEatScore = 200;

    audio_play_begin();

    if (g_audio_settings.music_enabled) {
        audioPMF.stop();
        // La musique PMF sera relancée après READY!
    } else {
        audioPMF.stop();
    }

    g.state = GameState::State::StartingLevel;
}

/*
============================================================
  INITIALISATION GLOBALE DU JEU
============================================================
*/
void game_init(GameState& g) {
    g.score = 0;
    g.lives = 3;

    level_init(g);
    detect_portals(g);

    g.pacman = Pacman(
        g.maze.pac_spawn_col,
        g.maze.pac_spawn_row
    );
    g.pacman_start_r = g.maze.pac_spawn_row;
    g.pacman_start_c = g.maze.pac_spawn_col;

    // IMPORTANT : dimensionner le vecteur de fantômes
    g.ghosts.clear();
    g.ghosts.resize(4);

    for (int i = 0; i < 4; i++) {
        g.ghosts[i] = Ghost(
            i,
            g.maze.ghost_spawn_col[i],
            g.maze.ghost_spawn_row[i]
        );
        g.ghosts[i].houseState = Ghost::HouseState::Inside;
    }

    init_ghost_schedule(g);

    int t = FIRST_GHOST_RELEASE_TICKS;
    for (auto& ghost : g.ghosts) {
        ghost.releaseTime_ticks = t;
        t += g.ghostReleaseInterval_ticks;
    }

    g.frightened_timer_ticks = 0;
    g.frightened_chain = 0;
    g.ghostDoorState = GameState::DoorState::Closed;
    g.ghostDoorTimer_ticks = 0;
    g.elapsed_ticks = 0;
    g.ghostEatScore = 200;

    g.state = GameState::State::TitleScreen;

    audioPMF.stop();
    audioPMF.init(pacman_pmf);
    // La musique PMF sera lancée au bon moment
    audio_play_begin();
}

/*
============================================================
  CAMÉRA VERTICALE
============================================================
*/
static void update_camera(const GameState& g)
{
    int maze_height_px = MAZE_HEIGHT * TILE_SIZE;
    int max_scroll = maze_height_px - SCREEN_H;
    if (max_scroll < 0) max_scroll = 0;

    int pac_center_y = g.pacman.y + PACMAN_SIZE / 2;
    int target_y = pac_center_y - SCREEN_H / 2;

    if (target_y < 0) target_y = 0;
    if (target_y > max_scroll) target_y = max_scroll;

    g_camera_y = (float)target_y;
}

/*
============================================================
  MISE À JOUR DES MODES SCATTER / CHASE
============================================================
*/
static void update_modes(GameState& g)
{
    if (g.phase_timer_ticks > 0) {
        g.phase_timer_ticks--;
        if (g.phase_timer_ticks == 0) {
            if (g.current_phase_index + 1 < g.schedule.phase_count) {
                g.current_phase_index++;
                g.global_mode = g.schedule.phases[g.current_phase_index].mode;
                g.phase_timer_ticks = g.schedule.phases[g.current_phase_index].duration_ticks;
                reverse_ghost_directions(g);
            }
        }
    }
}

/*
============================================================
  MISE À JOUR DE LA PORTE DES FANTÔMES
============================================================
*/
static void update_ghost_door(GameState& g)
{
    switch (g.ghostDoorState)
    {
        case GameState::DoorState::Closed:
            if (g.elapsed_ticks >= 2 * 60) {
                g.ghostDoorState = GameState::DoorState::Opening;
                g.maze.setGhostDoor(TileType::GhostDoorOpening);
                g.ghostDoorTimer_ticks = g.elapsed_ticks + 30;
            }
            break;

        case GameState::DoorState::Opening:
            if (g.elapsed_ticks >= g.ghostDoorTimer_ticks) {
                g.ghostDoorState = GameState::DoorState::Open;
                g.maze.setGhostDoor(TileType::GhostDoorOpen);
            }
            break;

        case GameState::DoorState::Open:
        {
            bool all_outside = true;
            for (auto& gh : g.ghosts)
            {
                if (gh.houseState != Ghost::HouseState::Outside &&
                    gh.mode != Ghost::Mode::Eaten)
                {
                    all_outside = false;
                    break;
                }
            }

            if (all_outside)
            {
                g.ghostDoorState = GameState::DoorState::Closed;
                g.maze.setGhostDoor(TileType::GhostDoorClosed);
            }
            break;
        }
    }
}

/*
============================================================
  MISE À JOUR PAR ÉTAT : StartingLevel
============================================================
*/
static void update_state_starting_level(GameState& g)
{
    // Animation de mort de Pac-Man non concernée ici

    // Tant que BEGIN.wav joue, on ne bouge rien
    if (audio_wav_is_playing()) {
        update_floating_scores(g);
        update_camera(g);  // garder la caméra cohérente
        return;
    }

    // Lancer la musique PMF après BEGIN + READY
    if (!audioPMF.isPlaying() && g_audio_settings.music_enabled) {
        audioPMF.init(pacman_pmf);
        audioPMF.start(GB_AUDIO_SAMPLE_RATE);
    }

    if (g.ready_timer > 0) {
        g.ready_timer--;
        update_floating_scores(g);
        update_camera(g);
        return;
    }

    if (g.ready_waiting_for_input)
    {
        update_floating_scores(g);

        if (g_keys.A) {
            g.ready_waiting_for_input = false;
            g.state = GameState::State::Playing;
        }
        update_camera(g);
        return;
    }

    // Pac-Man et fantômes figés sur la position de départ
    g.pacman.pixel_offset = 0;
    for (auto& gh : g.ghosts)
        gh.pixel_offset = 0;

    g.state = GameState::State::Playing;
    update_camera(g);
}

/*
============================================================
  MISE À JOUR PAR ÉTAT : PacmanDying
============================================================
*/
static void update_state_pacman_dying(GameState& g)
{
    // Incrémenter le timer d'animation de mort
    g.pacman_death_timer++;

    // Tant que le son de mort joue, on attend
    if (audio_wav_is_playing()) {
        update_floating_scores(g);
        return;
    }

    g.lives--;

    if (g.lives <= 0)
    {
        g.gameover_timer = 60;
        g.gameover_waiting_for_input = true;
        g.state = GameState::State::GameOver;
        return;
    }

    reset_level(g);
    g.state = GameState::State::StartingLevel;
}

/*
============================================================
  MISE À JOUR PAR ÉTAT : Playing
============================================================
*/
static void update_state_playing(GameState& g)
{
    g.pacman.update(g);

    for (auto& gh : g.ghosts)
        gh.update(g);

    check_pacman_ghost_collision(g);

    update_modes(g);
    update_camera(g);
    update_floating_scores(g);

    if (g.maze.pellet_count == 0)
    {
        g.level++;
        reset_level_full(g);
    }
}

/*
============================================================
  MISE À JOUR PAR ÉTAT : GameOver
============================================================
*/
static void update_state_gameover(GameState& g)
{
    if (g.gameover_timer > 0)
    {
        g.gameover_timer--;
        return;
    }

    if (g.gameover_waiting_for_input)
    {
        if (g_keys.A)
        {
            g.gameover_waiting_for_input = false;
            g.state = GameState::State::TitleScreen;
        }
        return;
    }
}

/*
============================================================
  MISE À JOUR PRINCIPALE DU JEU (GAME LOOP)
============================================================
*/
void game_update(GameState& g) {
    g.elapsed_ticks++;

    update_frightened(g);
    update_ghost_door(g);

    switch (g.state)
    {
        case GameState::State::StartingLevel:
            update_state_starting_level(g);
            break;

        case GameState::State::PacmanDying:
            update_state_pacman_dying(g);
            break;

        case GameState::State::Playing:
            update_state_playing(g);
            break;

        case GameState::State::GameOver:
            update_state_gameover(g);
            break;

        case GameState::State::TitleScreen:
        case GameState::State::LevelComplete:
        case GameState::State::Paused:
        case GameState::State::Options:
        case GameState::State::OptionsMenu:
        case GameState::State::Highscores:
            // Pas de logique de gameplay ici pour l’instant
            break;
    }
}

/*
============================================================
  RENDU GLOBAL (game_draw)
============================================================
*/
void game_draw(const GameState& g) {
    gfx_clear(COLOR_BLACK);

    // Labyrinthe
    g.maze.draw();

    switch (g.state)
    {
        case GameState::State::StartingLevel:
        {
            g.pacman.draw(g);

            gfx_text(150, 120, "READY!", COLOR_YELLOW);

            for (const auto& fs : g.floatingScores)
            {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", fs.value);
                int screen_x = fs.x;
                int screen_y = fs.y - (int)g_camera_y;
                gfx_text(screen_x, screen_y, buf, COLOR_YELLOW);
            }

            gfx_text(4,   4, ("SCORE: " + std::to_string(g.score)).c_str(), COLOR_WHITE);
            gfx_text(180, 4, ("LIVES: " + std::to_string(g.lives)).c_str(), COLOR_YELLOW);
            return;
        }

        case GameState::State::PacmanDying:
        {
            int frame = g.pacman_death_timer / 6;
            if (frame < 0)   frame = 0;
            if (frame > 11)  frame = 11;

            const uint16_t* sprite = pacman_death_anim[frame];

            int screen_x = g.pacman.x + 1;
            int screen_y = g.pacman.y + 1 - (int)g_camera_y;

            gfx_drawSprite(screen_x, screen_y, sprite, 14, 14);

            for (const auto& fs : g.floatingScores)
            {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", fs.value);
                int screen_x = fs.x;
                int screen_y = fs.y - (int)g_camera_y;
                gfx_text(screen_x, screen_y, buf, COLOR_YELLOW);
            }

            gfx_text(4,   4, ("SCORE: " + std::to_string(g.score)).c_str(), COLOR_WHITE);
            gfx_text(180, 4, ("LIVES: " + std::to_string(g.lives)).c_str(), COLOR_YELLOW);
            return;
        }

        case GameState::State::GameOver:
        {
            gfx_text(100, 120, "GAME OVER", COLOR_RED);
            gfx_text(110, 140, "PRESS A",   COLOR_WHITE);

            gfx_text(4,   4, ("SCORE: " + std::to_string(g.score)).c_str(), COLOR_WHITE);
            gfx_text(180, 4, ("LIVES: " + std::to_string(g.lives)).c_str(), COLOR_YELLOW);
            return;
        }

        case GameState::State::Playing:
        default:
            break;
    }

    // État normal : Pac-Man + fantômes
    g.pacman.draw(g);

    for (const auto& ghost : g.ghosts)
        ghost.draw(g);

    for (const auto& fs : g.floatingScores)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", fs.value);
        int screen_x = fs.x;
        int screen_y = fs.y - (int)g_camera_y;
        gfx_text(screen_x, screen_y, buf, COLOR_YELLOW);
    }

    gfx_text(4,   4, ("SCORE: " + std::to_string(g.score)).c_str(), COLOR_WHITE);
    gfx_text(180, 4, ("LIVES: " + std::to_string(g.lives)).c_str(), COLOR_YELLOW);
}

/*
============================================================
  ÉTAT DE FIN DE PARTIE
============================================================
*/
bool game_is_over(const GameState& g) {
    return (g.state == GameState::State::GameOver);
}
