#include "game.h"
#include "level.h"
#include "maze.h"
#include "core/graphics.h"
#include <string>
#include "assets/pacman_pmf.h" 
#include "lib/audio_pmf.h"
#include "core/audio.h"


extern AudioPMF audioPMF;

float g_camera_y = 0.0f;

// === Initialisation des phases Scatter/Chase niveau 1 ===
static void init_ghost_schedule(GameState& g)
{
    // Timings approximatifs arcade (en ticks de 1/60s si tu veux)
    // Tu peux adapter la durée réelle plus tard.
    g.schedule.phase_count = 4;
    g.schedule.phases[0] = { GlobalGhostMode::Scatter, 7 * 60 };
    g.schedule.phases[1] = { GlobalGhostMode::Chase,   20 * 60 };
    g.schedule.phases[2] = { GlobalGhostMode::Scatter, 7 * 60 };
    g.schedule.phases[3] = { GlobalGhostMode::Chase,   9999 * 60 }; // "infini"

    g.current_phase_index = 0;
    g.phase_timer_ticks   = g.schedule.phases[0].duration_ticks;
    g.global_mode         = g.schedule.phases[0].mode;
}

// Inversion de direction au changement de Scatter/Chase
static void reverse_ghost_directions(GameState& g)
{
    for (auto& ghost : g.ghosts) {
        ghost.reverse_direction();
    }
}

void game_init(GameState& g) {
    g.score = 0;
    g.lives = 3;
    level_init(g);
    init_ghost_schedule(g);
    g.frightened_timer_ticks = 0;
    g.frightened_chain = 0;
	g.ghostDoorState = GameState::DoorState::Closed;
	g.ghostDoorTimer_ticks = 0;
	g.elapsed_ticks = 0;
	
	// initialisation du temps de libération des fantômes
	int t = FIRST_GHOST_RELEASE_TICKS;

	for (auto& ghost : g.ghosts) {
		ghost.houseState = Ghost::HouseState::Inside;
		ghost.releaseTime_ticks = t;
		t += g.ghostReleaseInterval_ticks;
	}
	
	// === LANCEMENT DE LA MUSIQUE PMF === 
	audioPMF.stop(); 						// au cas où 
	audioPMF.init(pacman_pmf); 				// charge le PMF 
	audioPMF.start(GB_AUDIO_SAMPLE_RATE); 	// démarre la lecture


}

// Mise à jour caméra (suivi vertical de Pac-Man)
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

// Mise à jour du scheduler global (Scatter/Chase) + Frightened
static void update_modes(GameState& g)
{
    // Frightened
    if (g.global_mode == GlobalGhostMode::Frightened) {
        if (g.frightened_timer_ticks > 0) {
            g.frightened_timer_ticks--;
            if (g.frightened_timer_ticks == 0) {
                // Retour au mode planifié
                g.current_phase_index = (g.current_phase_index < g.schedule.phase_count)
                                        ? g.current_phase_index
                                        : (g.schedule.phase_count - 1);
                g.global_mode = g.schedule.phases[g.current_phase_index].mode;
                reverse_ghost_directions(g);
                for (auto& ghost : g.ghosts) {
                    ghost.on_end_frightened();
                }
            }
        }
        return;
    }

    // Scatter/Chase
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

// Appelé par Pacman lorsqu'il mange un power pellet
void game_trigger_frightened(GameState& g)
{
    g.global_mode = GlobalGhostMode::Frightened;
    g.frightened_timer_ticks = 6 * 60; // 6 secondes, à ajuster par niveau
    g.frightened_chain = 0;
    for (auto& ghost : g.ghosts) {
        ghost.on_start_frightened();
    }
}

// === Mise à jour du jeu ===
void game_update(GameState& g) {
    // Temps global
    g.elapsed_ticks++;

    // Animation de la porte des fantômes
    switch (g.ghostDoorState)
    {
        case GameState::DoorState::Closed:
            // Exemple : ouverture après 2 secondes
            if (g.elapsed_ticks >= 2 * 60) {
                g.ghostDoorState = GameState::DoorState::Opening;
                g.maze.setGhostDoor(TileType::GhostDoorOpening);
                g.ghostDoorTimer_ticks = g.elapsed_ticks + 30; // 0.5 s d’animation
            }
            break;

        case GameState::DoorState::Opening:
            if (g.elapsed_ticks >= g.ghostDoorTimer_ticks) {
                g.ghostDoorState = GameState::DoorState::Open;
                g.maze.setGhostDoor(TileType::GhostDoorOpen);
            }
            break;

        case GameState::DoorState::Open:
            // reste ouvert
            break;
    }

    // Pac-Man
    g.pacman.update(g);

    // Fantômes
    for (auto& ghost : g.ghosts) {
        ghost.update(g);
    }

    // Scheduler modes
    update_modes(g);

    // Caméra
    update_camera(g);
}


// === Dessin du jeu ===
void game_draw(const GameState& g) {
    gfx_clear(COLOR_BLACK);

    g.maze.draw();
    g.pacman.draw();

    for (const auto& ghost : g.ghosts) {
        ghost.draw();
    }

    gfx_text(4, 4, ("SCORE: " + std::to_string(g.score)).c_str(), COLOR_WHITE);
    gfx_text(180, 4, ("LIVES: " + std::to_string(g.lives)).c_str(), COLOR_YELLOW);
}

bool game_is_over(const GameState& g) {
    return false;
}
