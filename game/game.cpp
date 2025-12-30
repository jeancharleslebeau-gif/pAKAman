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

// === Initialisation des phases Scatter/Chase niveau 1 ===
void init_ghost_schedule(GameState& g)
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

void reverse_ghost_directions(GameState& g)
{
    for (auto& ghost : g.ghosts)
        ghost.reverse_direction();
}

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

void update_floating_scores(GameState& g)
{
    for (auto& fs : g.floatingScores)
        fs.timer--;

    g.floatingScores.erase(
        std::remove_if(g.floatingScores.begin(), g.floatingScores.end(),
                       [](const FloatingScore& fs){ return fs.timer <= 0; }),
        g.floatingScores.end());
}

// Collision Pac-Man / fantômes (pixel-based mais robuste)
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

                g.ghostEatScore *= 2;
                audio_play_eatghost();
                continue;
            }

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

void detect_portals(GameState& g)
{
    const Maze& m = g.maze;

    // Reset
    g.portalH = GameState::PortalPair{};
    g.portalV = GameState::PortalPair{};

    // --- Horizontal ---
    // T0 = Tunnel en colonne 0
    for (int r = 0; r < MAZE_HEIGHT; r++)
        if (m.tiles[r][0] == TileType::Tunnel)
        {
            g.portalH.T0_r = r;
            g.portalH.T0_c = 0;
            g.portalH.E0_r = r;
            g.portalH.E0_c = 1;
            break;
        }

    // T1 = Tunnel en colonne WIDTH-1
    for (int r = 0; r < MAZE_HEIGHT; r++)
        if (m.tiles[r][MAZE_WIDTH-1] == TileType::Tunnel)
        {
            g.portalH.T1_r = r;
            g.portalH.T1_c = MAZE_WIDTH-1;
            g.portalH.E1_r = r;
            g.portalH.E1_c = MAZE_WIDTH-2;
            break;
        }

    // Cohérence horizontale
    if ((g.portalH.T0_r != -1) != (g.portalH.T1_r != -1))
        printf("ERREUR MAP: Portail horizontal incohérent (T0 sans T1 ou inverse)\n");
    else if (g.portalH.T0_r != -1)
        g.portalH.exists = true;

    // --- Vertical ---
    // T2 = Tunnel en ligne 0
    for (int c = 0; c < MAZE_WIDTH; c++)
        if (m.tiles[0][c] == TileType::Tunnel)
        {
            g.portalV.T0_r = 0;
            g.portalV.T0_c = c;
            g.portalV.E0_r = 1;
            g.portalV.E0_c = c;
            break;
        }

    // T3 = Tunnel en ligne HEIGHT-1
    for (int c = 0; c < MAZE_WIDTH; c++)
        if (m.tiles[MAZE_HEIGHT-1][c] == TileType::Tunnel)
        {
            g.portalV.T1_r = MAZE_HEIGHT-1;
            g.portalV.T1_c = c;
            g.portalV.E1_r = MAZE_HEIGHT-2;
            g.portalV.E1_c = c;
            break;
        }

    // Cohérence verticale
    if ((g.portalV.T0_r != -1) != (g.portalV.T1_r != -1))
        printf("ERREUR MAP: Portail vertical incohérent (T2 sans T3 ou inverse)\n");
    else if (g.portalV.T0_r != -1)
        g.portalV.exists = true;
}


// Réinitialisation du niveau (après mort, si vies restantes) mais pas de tout
void reset_level(GameState& g)
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

		gh.houseState = Ghost::HouseState::Inside;
		gh.mode = Ghost::Mode::Scatter;
		gh.previous_mode = Ghost::Mode::Scatter;
		gh.eaten_timer = 0;
		gh.path.clear();

		gh.pixel_offset = 0;
		gh.x = gh.tile_c * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;
		gh.y = gh.tile_r * TILE_SIZE + (TILE_SIZE - GHOST_SIZE) / 2;
	}



    // Reset ghost house
    g.ghostDoorState = GameState::DoorState::Closed;

    // Reset timers
    g.global_mode = GlobalGhostMode::Scatter;
    g.elapsed_ticks = 0;
	g.ready_waiting_for_input = true;
	g.ready_timer = 30;
	
	if (g_audio_settings.music_enabled) {
		audioPMF.stop();
		audioPMF.init(pacman_pmf);
		audioPMF.start(GB_AUDIO_SAMPLE_RATE);
	} else {
		audioPMF.stop();  
	}
}


// Initialisation du niveau lors d'un nouveau niveau
static void reset_level_full(GameState& g)
{
    // Recharger la map (pellets, power pellets, murs, tunnels…)
    level_init(g);
	detect_portals(g);

    // Reset Pac-Man
    g.pacman = Pacman(
        g.maze.pac_spawn_col,
        g.maze.pac_spawn_row
    );
	g.pacman_start_r = g.maze.pac_spawn_row; 
	g.pacman_start_c = g.maze.pac_spawn_col;

    // Reset fantômes
	for (auto& gh : g.ghosts)
	{
		gh.start_row = g.maze.ghost_spawn_row[gh.id];
		gh.start_col = g.maze.ghost_spawn_col[gh.id];
		gh.reset_to_start();

		gh.houseState = Ghost::HouseState::Inside;
		gh.mode = Ghost::Mode::Scatter;
		gh.previous_mode = Ghost::Mode::Scatter;
		gh.eaten_timer = 0;
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

    // Reset timers
    g.frightened_timer_ticks = 0;
    g.frightened_chain = 0;
    g.ghostDoorState = GameState::DoorState::Closed;
    g.ghostDoorTimer_ticks = 0;
    g.elapsed_ticks = 0;
    g.ghostEatScore = 200;

    // Relancer la musique BEGIN
    audio_play_begin();

    // Relancer la musique PMF
    if (g_audio_settings.music_enabled) {
        audioPMF.stop();
        audioPMF.init(pacman_pmf);
        audioPMF.start(GB_AUDIO_SAMPLE_RATE);
	} else {
		audioPMF.stop();  
	}
	
    g.state = GameState::State::StartingLevel;
}


// === Initialisation globale du jeu ===
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
    audioPMF.start(GB_AUDIO_SAMPLE_RATE);

    // BEGIN.wav au début de niveau si tu veux
    audio_play_begin();
}

// Caméra
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

// Modes global (scatter/chase + frightened)
static void update_modes(GameState& g)
{
    if (g.global_mode == GlobalGhostMode::Frightened) {
        if (g.frightened_timer_ticks > 0) {
            g.frightened_timer_ticks--;
            if (g.frightened_timer_ticks == 0) {
                g.current_phase_index = (g.current_phase_index < g.schedule.phase_count)
                                        ? g.current_phase_index
                                        : (g.schedule.phase_count - 1);
                g.global_mode = g.schedule.phases[g.current_phase_index].mode;
                reverse_ghost_directions(g);
                for (auto& ghost : g.ghosts)
                    ghost.on_end_frightened();
            }
        }
        return;
    }

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

// Power pellet
void game_trigger_frightened(GameState& g)
{
    g.global_mode = GlobalGhostMode::Frightened;
    g.frightened_timer_ticks = 6 * 60;
    g.frightened_chain = 0;
    for (auto& ghost : g.ghosts)
        ghost.on_start_frightened();
}

// === game_update ===
void game_update(GameState& g) {
    g.elapsed_ticks++;

    // Porte des fantômes (comme avant)
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
            break;
    }

    switch (g.state)
    {
		case GameState::State::StartingLevel:
		{
			if (audio_wav_is_playing()) {
				update_floating_scores(g);
				return;
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
			
			// Pac-Man ne bouge pas 
			g.pacman.pixel_offset = 0; 
			// Fantômes ne bougent pas 
			for (auto& gh : g.ghosts) 
				gh.pixel_offset = 0;

			g.state = GameState::State::Playing;
			update_camera(g);
			return;
		}


		case GameState::State::PacmanDying:
		{
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
			return;
		}

		
        case GameState::State::Playing:
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
				return;
			}


            break;
        }

		case GameState::State::GameOver:
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
					g.state = GameState::State::TitleScreen; // ou game_init(g)
				}
				return;
			}

			return;
		}
		case GameState::State::TitleScreen:
		case GameState::State::LevelComplete:
		case GameState::State::Paused:
		case GameState::State::Options:
		case GameState::State::OptionsMenu:
		case GameState::State::Highscores:
			return;
    }
}

// === Draw ===
void game_draw(const GameState& g) {
    gfx_clear(COLOR_BLACK);

    // 1) Toujours dessiner le labyrinthe
    g.maze.draw();

    // 2) Gestion des états
    switch (g.state)
    {
        case GameState::State::StartingLevel:
        {
            // Pac-Man visible (arcade-faithful)
            g.pacman.draw(g);

            // Fantômes invisibles → NE PAS les dessiner

            // READY!
            gfx_text(150, 120, "READY!", COLOR_YELLOW);

            // HUD + floating scores
            for (const auto& fs : g.floatingScores)
            {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", fs.value);
                int screen_x = fs.x;
                int screen_y = fs.y - (int)g_camera_y;
                gfx_text(screen_x, screen_y, buf, COLOR_YELLOW);
            }

            gfx_text(4, 4, ("SCORE: " + std::to_string(g.score)).c_str(), COLOR_WHITE);
            gfx_text(180, 4, ("LIVES: " + std::to_string(g.lives)).c_str(), COLOR_YELLOW);

            return; // IMPORTANT : ne pas dessiner les fantômes
        }

        case GameState::State::PacmanDying:
        {
            // Animation de mort
            int frame = g.pacman_death_timer / 6;
            if (frame < 0)   frame = 0;
            if (frame > 11)  frame = 11;

            const uint16_t* sprite = pacman_death_anim[frame];

            int screen_x = g.pacman.x + 1;
            int screen_y = g.pacman.y + 1 - (int)g_camera_y;

            gfx_drawSprite(screen_x, screen_y, sprite, 14, 14);

            // HUD + floating scores
            for (const auto& fs : g.floatingScores)
            {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", fs.value);
                int screen_x = fs.x;
                int screen_y = fs.y - (int)g_camera_y;
                gfx_text(screen_x, screen_y, buf, COLOR_YELLOW);
            }

            gfx_text(4, 4, ("SCORE: " + std::to_string(g.score)).c_str(), COLOR_WHITE);
            gfx_text(180, 4, ("LIVES: " + std::to_string(g.lives)).c_str(), COLOR_YELLOW);

            return;
        }

        case GameState::State::GameOver:
        {
            gfx_text(100, 120, "GAME OVER", COLOR_RED);
            gfx_text(110, 140, "PRESS A", COLOR_WHITE);

            // HUD
            gfx_text(4, 4, ("SCORE: " + std::to_string(g.score)).c_str(), COLOR_WHITE);
            gfx_text(180, 4, ("LIVES: " + std::to_string(g.lives)).c_str(), COLOR_YELLOW);

            return;
        }

        case GameState::State::Playing:
        default:
            break;
    }

    // 3) État normal : Pac-Man + fantômes
    g.pacman.draw(g);

    for (const auto& ghost : g.ghosts)
        ghost.draw();

    // Floating scores
    for (const auto& fs : g.floatingScores)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", fs.value);
        int screen_x = fs.x;
        int screen_y = fs.y - (int)g_camera_y;
        gfx_text(screen_x, screen_y, buf, COLOR_YELLOW);
    }

    // HUD
    gfx_text(4, 4, ("SCORE: " + std::to_string(g.score)).c_str(), COLOR_WHITE);
    gfx_text(180, 4, ("LIVES: " + std::to_string(g.lives)).c_str(), COLOR_YELLOW);
}


bool game_is_over(const GameState& g) {
    return (g.state == GameState::State::GameOver);
}
