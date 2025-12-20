#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_chip_info.h"	
#include "esp_flash.h"
#include "esp_psram.h"
#include "common.h"
#include "driver/gpio.h"

// Librairies matérielles
#include "lib/expander.h"
#include "lib/LCD.h"
#include "lib/graphics_basic.h"
#include "lib/sdcard.h"
#include "lib/audio_sfx.h"
#include "lib/audio_player.h"

// Core
#include "core/input.h"
#include "core/graphics.h"
#include "core/audio.h"
#include "core/persist.h"
#include "core/sprite.h"

// Composants du jeu
#include "game/config.h"
#include "ui/title_screen.h"
#include "ui/menu.h"
#include "ui/highscores.h"
#include "game/game.h"
#include "game/level.h"
#include "assets/assets.h"

#include "esp_adc/adc_oneshot.h" 
#include "esp_adc/adc_cali.h"

int volume = 128; // valeur initiale

// Vérification de la palette avec du texte coloré
void lcd_test_colors() {
    struct ColorDef {
        const char* name;
        uint16_t value;
    };

    ColorDef colors[] = {
        {"WHITE", COLOR_WHITE}, {"GRAY", COLOR_GRAY}, {"DARKGRAY", COLOR_DARKGRAY},
        {"BLACK", COLOR_BLACK}, {"PURPLE", COLOR_PURPLE}, {"PINK", COLOR_PINK},
        {"RED", COLOR_RED}, {"ORANGE", COLOR_ORANGE}, {"BROWN", COLOR_BROWN},
        {"BEIGE", COLOR_BEIGE}, {"YELLOW", COLOR_YELLOW}, {"LIGHTGREEN", COLOR_LIGHTGREEN},
        {"GREEN", COLOR_GREEN}, {"DARKBLUE", COLOR_DARKBLUE}, {"BLUE", COLOR_BLUE},
        {"LIGHTBLUE", COLOR_LIGHTBLUE}, {"SILVER", COLOR_SILVER}, {"GOLD", COLOR_GOLD}
    };

    lcd_clear(COLOR_BLACK);

    int spacing_y = 20;
    int col_x[2] = {10, 160};
    int col = 0, row = 0;

    for (int i = 0; i < sizeof(colors)/sizeof(colors[0]); i++) {
        int x = col_x[col];
        int y = 20 + row * spacing_y;

        gfx_text(x, y, colors[i].name, colors[i].value);

        row++;
        if (row >= 9) {
            row = 0;
            col++;
        }
    }
	
	// Après la boucle
    const char* modeText =
    #ifdef LCD_COLOR_ORDER_RGB
        "Mode: RGB565";
    #else
        "Mode: BGR565";
    #endif

    gfx_text(10, 220, modeText, COLOR_WHITE);

    gfx_flush();
    lcd_refresh();
}


// Call this once per frame with a monotonically increasing counter (e.g., animTick++)
void testSprites_4lines(int animTick) {
    // Sprites
    static const uint16_t* pacRight[3] = { pacman_right_0, pacman_right_1, pacman_right_2 };
    static const uint16_t* pacLeft [3] = { pacman_left_0,  pacman_left_1,  pacman_left_2  };
    static const uint16_t* pacUp   [3] = { pacman_up_0,    pacman_up_1,    pacman_up_2    };
    static const uint16_t* pacDown [3] = { pacman_down_0,  pacman_down_1,  pacman_down_2  };

    static const uint16_t* gRed [2] = { ghost_red_0,    ghost_red_1    };
    static const uint16_t* gBlue[2] = { ghost_blue_0,   ghost_blue_1   };
    static const uint16_t* gPink[2] = { ghost_pink_0,   ghost_pink_1   };
    static const uint16_t* gOrng[2] = { ghost_orange_0, ghost_orange_1 };

    // Slower animation: divide tick to hold frames longer
    int pacFrame   = (animTick / 12) % 3; // slower Pac-Man
    int ghostFrame = (animTick / 16) % 2; // even slower ghosts

    lcd_clear(COLOR_BLACK);

    // Line 1: Pac-Man RIGHT and LEFT
	gfx_drawSprite(15, 20, pacRight[0], PACMAN_SIZE, PACMAN_SIZE);
	gfx_drawSprite(40, 20, pacRight[1], PACMAN_SIZE, PACMAN_SIZE);
	gfx_drawSprite(65, 20, pacRight[2], PACMAN_SIZE, PACMAN_SIZE);
    for (int i = 0; i < 3; ++i)
        gfx_drawSprite(90, 20, pacRight[pacFrame], PACMAN_SIZE, PACMAN_SIZE);
    gfx_text(20,  40, "DIR: RIGHT", COLOR_WHITE);

	gfx_drawSprite(135, 20, pacLeft[0], PACMAN_SIZE, PACMAN_SIZE);
	gfx_drawSprite(160, 20, pacLeft[1], PACMAN_SIZE, PACMAN_SIZE);
	gfx_drawSprite(185, 20, pacLeft[2], PACMAN_SIZE, PACMAN_SIZE);
	for (int i = 0; i < 3; ++i)
        gfx_drawSprite(210, 20, pacLeft[pacFrame], PACMAN_SIZE, PACMAN_SIZE);
    gfx_text(135,  40, "DIR: LEFT", COLOR_WHITE);

    // Line 2: Pac-Man UP and DOWN (two blocks)
    gfx_drawSprite(15, 70, pacUp[0], PACMAN_SIZE, PACMAN_SIZE);
	gfx_drawSprite(40, 70, pacUp[1], PACMAN_SIZE, PACMAN_SIZE);
	gfx_drawSprite(65, 70, pacUp[2], PACMAN_SIZE, PACMAN_SIZE);
	for (int i = 0; i < 3; ++i)
        gfx_drawSprite(90, 70, pacUp[pacFrame], PACMAN_SIZE, PACMAN_SIZE);
    gfx_text(20,  90, "DIR: UP", COLOR_WHITE);

	gfx_drawSprite(135, 70, pacDown[0], PACMAN_SIZE, PACMAN_SIZE);
	gfx_drawSprite(160, 70, pacDown[1], PACMAN_SIZE, PACMAN_SIZE);
	gfx_drawSprite(185, 70, pacDown[2], PACMAN_SIZE, PACMAN_SIZE);
		for (int i = 0; i < 3; ++i)
        gfx_drawSprite(210, 70, pacDown[pacFrame], PACMAN_SIZE, PACMAN_SIZE);
    gfx_text(135,  90, "DIR: DOWN", COLOR_WHITE);
		
    // Line 4: Ghosts (two rows for clarity)
    gfx_drawSprite(20,  140, gRed [ghostFrame], GHOST_SIZE, GHOST_SIZE);
    gfx_drawSprite(60,  140, gBlue[ghostFrame], GHOST_SIZE, GHOST_SIZE);
    gfx_drawSprite(100, 140, gPink[ghostFrame], GHOST_SIZE, GHOST_SIZE);
    gfx_drawSprite(140, 140, gOrng[ghostFrame], GHOST_SIZE, GHOST_SIZE);

    // Tiny labels of ghosts instead of long names
    gfx_text(20,  160, "R", COLOR_WHITE);
    gfx_text(60,  160, "B", COLOR_WHITE);
    gfx_text(100, 160, "P", COLOR_WHITE);
    gfx_text(140, 160, "O", COLOR_WHITE);
}


// === Fonction de test joystick ===
void lcd_test_joystick()
{
    Keys k{};
    input_poll(k);

    lcd_clear(COLOR_BLACK);

    gfx_text(10, 10, "=== Test Joystick ===", COLOR_YELLOW);

    // Valeurs brutes
    char buf[64];
    snprintf(buf, sizeof(buf), "Joy X: %4d   Joy Y: %4d", k.joxx, k.joxy);
    gfx_text(10, 40, buf, COLOR_LIGHTBLUE);

    // Directions interprétées
    const int DEADZONE = 500;
    bool joyLeft  = (k.joxx < 2048 - DEADZONE);
    bool joyRight = (k.joxx > 2048 + DEADZONE);
    bool joyUp    = (k.joxy > 2048 + DEADZONE);
    bool joyDown  = (k.joxy < 2048 - DEADZONE);

    snprintf(buf, sizeof(buf), "Dir L:%s R:%s U:%s D:%s",
             joyLeft  ? "ON" : "OFF",
             joyRight ? "ON" : "OFF",
             joyUp    ? "ON" : "OFF",
             joyDown  ? "ON" : "OFF");
    gfx_text(10, 70, buf, COLOR_GREEN);

    gfx_flush();
    lcd_refresh();
}


void audio_test()
{
    printf("=== AUDIO TEST ===\n");

    // 1) Test WAV
    printf("Lecture WAV...\n");
    audio_play_wav("/sdcard/PAKAMAN/Sons/PACGOMME.wav");

    // Attendre un peu
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // 2) Test bip (sinus court)
    printf("Test bip...\n");
    audio_test_enable(true);   // active cos44100
    vTaskDelay(200 / portTICK_PERIOD_MS);
    audio_test_enable(false);  // retour mode normal

    printf("=== FIN TEST ===\n");
}



extern "C" void app_main(void) {
	
	// Activer ci dessous pour tester les sprites si on active la fonction de test
	// int animTick = 0; // compteur global pour l’animation pour le test des sprites
	
    // === Initialisation matériel ===
    lcd_init_pwm();
    lcd_update_pwm(64);
    adc_init();
    expander_init();
    LCD_init();
    sd_init();
    audio_init();
    audio_set_volume(volume);
	// audio_play_wav("/sdcard/PAKAMAN/Sons/PACGOMME.wav");
	// audio_test();
    input_init();
    assets_init();
    highscores_init();   // crée le fichier pAKAman.sco si absent

    GameState g;
    g.state = GameState::State::Title;
    g.score = 0;
    g.lives = 3;
    g.levelIndex = 0;

    while (true) {
        Keys k;
        input_poll(k);   // lecture des touches
 
        switch (g.state) {
            case GameState::State::Title:
                title_screen_show();
                if (k.A) {
					audio_sfx_validate();
                    game_init(g);   // initialise le niveau (via level_init)
                    g.state = GameState::State::Playing;
                    gfx_clear(COLOR_BLACK);
                    gfx_flush();
					audio_update();
                }
                if (k.MENU) {
					audio_sfx_click();
                    g.state = GameState::State::Options;
                }
                break;

            case GameState::State::Playing:
                game_update(g);   // Pac-Man + fantômes
                game_draw(g);     // affichage
                if (k.RUN) {
                    g.state = GameState::State::Paused;
                }
				
				if (k.A) {
					audio_sfx_pop();   // bruit blanc court (test sons)
				}

                if (game_is_over(g)) {
                    highscores_submit(g.score);
                    g.state = GameState::State::Highscores;
                }
                break;

            case GameState::State::Options:
                gfx_clear(COLOR_BLACK);
				
                gfx_text(20, 80,  "=== Options ===", COLOR_YELLOW);
                gfx_text(20, 100, ("Volume: " + std::to_string(volume)).c_str(), COLOR_WHITE);
                gfx_text(20, 120, "LEFT/RIGHT pour regler", COLOR_WHITE);
                gfx_text(20, 140, "A pour consulter les scores", COLOR_WHITE);
                gfx_text(20, 160, "B pour retour", COLOR_YELLOW);
                gfx_flush();

                if (k.left && volume > 0) { 
					audio_sfx_click();
					volume -= 8;
				}
				if (volume < 1) volume = 0;
				
                if (k.right && volume < 255) {
					audio_sfx_click();
					volume += 8;
				}
				if (volume > 254) volume = 255;	
                audio_set_volume(volume);

                if (k.A) {
					audio_sfx_validate();
                    highscores_show();
                    while (true) {
                        Keys ks;
                        input_poll(ks);
                        if (ks.B) {
							audio_sfx_cancel(); 
							break;
						}	
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                } 

				// testSprites_4lines(animTick); 
				// animTick++; 
				// Utilise le menu option pour tester certains éléments
				// lcd_show_inputs();
				
                if (k.B) {
					g.state = GameState::State::Title;
					audio_sfx_cancel(); 
				}	
                break;

            case GameState::State::Paused:
                gfx_text(20, 160, "Pause - Appuyez sur A pour reprendre", COLOR_WHITE);
                gfx_flush();
                if (k.A) {
					audio_sfx_validate();
                    g.state = GameState::State::Playing;
                }
                break;

            case GameState::State::Highscores:
                highscores_show();
                if (k.B) {
					audio_sfx_cancel();
                    g.state = GameState::State::Title;
                }
                break;

            case GameState::State::GameOver:
                gfx_text(20, 160, "Game Over - Appuyez sur A pour recommencer", COLOR_RED);
                if (k.A || k.B) {
					audio_sfx_validate();
                    g.state = GameState::State::Title;
                }
                break;

            default:
                g.state = GameState::State::Title;
                break;
        }
		gfx_flush();
		lcd_refresh();
		audio_update();
		vTaskDelay(pdMS_TO_TICKS(20));
    }
}