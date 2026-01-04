/*
============================================================
  task_game.cpp — Tâche principale du moteur de jeu (60 Hz)
------------------------------------------------------------
Cette tâche exécute :
 - game_update() : logique du jeu
 - game_draw()   : rendu graphique
 - gfx_flush()   : transfert vers le framebuffer
 - lcd_refresh() : rafraîchissement de l’écran

Elle contient désormais la machine à états complète du jeu.
============================================================
*/

#include "task_game.h"
#include "game/game.h"
#include "core/graphics.h"
#include "core/input.h"
#include "lib/LCD.h"
#include "ui/title_screen.h"
#include "ui/menu.h"
#include "ui/highscores.h"
#include "game/level.h"
#include "assets/assets.h"
#include "core/audio.h"
#include "lib/audio_sfx.h"
#include "core/persist.h"
#include "ui/options.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"

// ------------------------------------------------------------
// État global du jeu
// ------------------------------------------------------------
static GameState g;
static int options_index = 0;


// ------------------------------------------------------------
// Fonctions d’états (une par écran)
// ------------------------------------------------------------

static void state_title_screen(const Keys& k)
{
    title_screen_show();

    if (k.A) {
        audio_sfx_validate();
        game_init(g);
        g.state = GameState::State::StartingLevel;
        gfx_clear(COLOR_BLACK);
    }

    if (k.MENU) {
        audio_sfx_click();
        g.state = GameState::State::Options;
    }
}

static void state_starting_level(const Keys& k)
{
    // Logique + rendu du niveau (affichage READY!, etc.)
    game_update(g);
    game_draw(g);
}


static void state_playing(const Keys& k)
{
    game_update(g);
    game_draw(g);

    if (k.RUN) {
        g.state = GameState::State::Paused;
    }

    if (k.A) {
        audio_sfx_pop();
    }

    if (k.MENU) {
        audio_sfx_click();
        g.state = GameState::State::OptionsMenu;
    }

    if (game_is_over(g)) {
        highscores_submit(g.score);
        g.state = GameState::State::Highscores;
    }
}

static void state_pacman_dying(const Keys& k)
{
    // game_update() gère :
    // - l’animation de mort
    // - la décrémentation des vies
    // - la transition vers StartingLevel ou GameOver
    game_update(g);
    game_draw(g);

}


static void state_options(const Keys& k)
{
    gfx_clear(COLOR_BLACK);
    draw_options_menu(g.state, options_index);
    handle_audio_options_navigation(k, options_index);

    if (k.A && options_index == 4) {
        audio_sfx_validate();
        highscores_show();
    }

    if ((k.A && options_index == 5) || k.B) {
        audio_sfx_cancel();
        g.state = GameState::State::TitleScreen;
        audio_settings_save();
    }
}

static void state_options_menu(const Keys& k)
{
    gfx_clear(COLOR_BLACK);
    draw_options_menu(g.state, options_index);
    handle_audio_options_navigation(k, options_index);

    if (k.A && options_index == 4) {
        audio_sfx_cancel();
        g.state = GameState::State::TitleScreen;
        return;
    }

    if ((k.A && options_index == 5) || k.B) {
        audio_sfx_validate();
        g.state = GameState::State::Playing;
        audio_settings_save();
    }
}

static void state_paused(const Keys& k)
{
    gfx_text(70, 140, "PRESS A TO PLAY", COLOR_WHITE);
    gfx_text(20, 160, "Pause - Appuyez sur A pour reprendre", COLOR_WHITE);

    if (k.A) {
        audio_sfx_validate();
        g.state = GameState::State::Playing;
    }
}

static void state_highscores(const Keys& k)
{
    highscores_show();

    if (k.B) {
        audio_sfx_cancel();
        g.state = GameState::State::TitleScreen;
    }
}

static void state_gameover(const Keys& k)
{
    gfx_text(70, 140, "PRESS A TO START", COLOR_WHITE);
    gfx_text(50, 160, "Appuyez sur A pour recommencer", COLOR_YELLOW);

    if (k.A || k.B) {
        audio_sfx_validate();
        g.state = GameState::State::TitleScreen;
    }
}


// ------------------------------------------------------------
// Tâche principale du moteur de jeu (60 FPS)
// ------------------------------------------------------------
void task_game(void* param)
{
    // Initialisation du moteur
    game_init(g);

    const int FRAME_US = 16666; // 60 FPS
    int64_t last = esp_timer_get_time();

    while (true)
    {
        int64_t now = esp_timer_get_time();
        int64_t dt = now - last;

		if (dt >= FRAME_US)
		{
			last = now;

			Keys k;
			input_poll(k);

			switch (g.state)
			{
				case GameState::State::TitleScreen:   state_title_screen(k);   break;
				case GameState::State::StartingLevel: state_starting_level(k); break;
				case GameState::State::Playing:       state_playing(k);        break;
				case GameState::State::PacmanDying:   state_pacman_dying(k);   break;
				case GameState::State::Options:       state_options(k);        break;
				case GameState::State::OptionsMenu:   state_options_menu(k);   break;
				case GameState::State::Paused:        state_paused(k);         break;
				case GameState::State::Highscores:    state_highscores(k);     break;
				case GameState::State::GameOver:      state_gameover(k);       break;
				default:
					g.state = GameState::State::TitleScreen;
					break;
			}

			gfx_flush();
			lcd_refresh();
		}
		else
		{
			// Remplace déjà ça:
			// esp_rom_delay_us(sleep);
			vTaskDelay(1);
		}

    }
}

