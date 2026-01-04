/*
============================================================
  tests.cpp — Module de tests (optionnel)
------------------------------------------------------------
Ce fichier regroupe toutes les fonctions de test qui étaient
dans l’ancien app_main.cpp :

 - test_sprites()      : affichage des sprites Pac-Man / fantômes
 - test_joystick()     : lecture joystick + affichage
 - test_audio()        : test WAV + bip
 - test_colors()       : affichage palette

Une tâche dédiée (task_tests) peut appeler ces fonctions
périodiquement sans polluer le moteur de jeu.
============================================================
*/

#include "tests.h"
#include "core/graphics.h"
#include "core/input.h"
#include "lib/LCD.h"
#include "lib/audio_sfx.h"
#include "lib/audio_player.h"
#include "assets/assets.h"
#include "game/config.h"
#include <stdio.h>

/*
============================================================
  TEST PALETTE COULEURS
------------------------------------------------------------
Affiche toutes les couleurs disponibles avec leur nom.
============================================================
*/
void test_colors()
{
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

/*
============================================================
  TEST SPRITES (Pac-Man + Fantômes)
------------------------------------------------------------
Affiche les sprites animés sur 4 lignes.
============================================================
*/
void test_sprites(int animTick)
{
    static const uint16_t* pacRight[3] = { pacman_right_0, pacman_right_1, pacman_right_2 };
    static const uint16_t* pacLeft [3] = { pacman_left_0,  pacman_left_1,  pacman_left_2  };
    static const uint16_t* pacUp   [3] = { pacman_up_0,    pacman_up_1,    pacman_up_2    };
    static const uint16_t* pacDown [3] = { pacman_down_0,  pacman_down_1,  pacman_down_2  };

    static const uint16_t* gRed [2] = { ghost_red_0,    ghost_red_1    };
    static const uint16_t* gBlue[2] = { ghost_blue_0,   ghost_blue_1   };
    static const uint16_t* gPink[2] = { ghost_pink_0,   ghost_pink_1   };
    static const uint16_t* gOrng[2] = { ghost_orange_0, ghost_orange_1 };

    int pacFrame   = (animTick / 12) % 3;
    int ghostFrame = (animTick / 16) % 2;

    lcd_clear(COLOR_BLACK);

    // Ligne 1 : Pac-Man RIGHT et LEFT
    gfx_drawSprite(15, 20, pacRight[0], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(40, 20, pacRight[1], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(65, 20, pacRight[2], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(90, 20, pacRight[pacFrame], PACMAN_SIZE, PACMAN_SIZE);
    gfx_text(20, 40, "DIR: RIGHT", COLOR_WHITE);

    gfx_drawSprite(135, 20, pacLeft[0], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(160, 20, pacLeft[1], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(185, 20, pacLeft[2], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(210, 20, pacLeft[pacFrame], PACMAN_SIZE, PACMAN_SIZE);
    gfx_text(135, 40, "DIR: LEFT", COLOR_WHITE);

    // Ligne 2 : UP / DOWN
    gfx_drawSprite(15, 70, pacUp[0], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(40, 70, pacUp[1], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(65, 70, pacUp[2], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(90, 70, pacUp[pacFrame], PACMAN_SIZE, PACMAN_SIZE);
    gfx_text(20, 90, "DIR: UP", COLOR_WHITE);

    gfx_drawSprite(135, 70, pacDown[0], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(160, 70, pacDown[1], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(185, 70, pacDown[2], PACMAN_SIZE, PACMAN_SIZE);
    gfx_drawSprite(210, 70, pacDown[pacFrame], PACMAN_SIZE, PACMAN_SIZE);
    gfx_text(135, 90, "DIR: DOWN", COLOR_WHITE);

    // Ligne 3 : Fantômes
    gfx_drawSprite(20, 140, gRed[ghostFrame], GHOST_SIZE, GHOST_SIZE);
    gfx_drawSprite(60, 140, gBlue[ghostFrame], GHOST_SIZE, GHOST_SIZE);
    gfx_drawSprite(100, 140, gPink[ghostFrame], GHOST_SIZE, GHOST_SIZE);
    gfx_drawSprite(140, 140, gOrng[ghostFrame], GHOST_SIZE, GHOST_SIZE);

    gfx_text(20, 160, "R", COLOR_WHITE);
    gfx_text(60, 160, "B", COLOR_WHITE);
    gfx_text(100, 160, "P", COLOR_WHITE);
    gfx_text(140, 160, "O", COLOR_WHITE);

    gfx_flush();
    lcd_refresh();
}

/*
============================================================
  TEST JOYSTICK
------------------------------------------------------------
Affiche les valeurs brutes + directions interprétées.
============================================================
*/
void test_joystick()
{
    Keys k{};
    input_poll(k);

    lcd_clear(COLOR_BLACK);

    gfx_text(10, 10, "=== Test Joystick ===", COLOR_YELLOW);

    char buf[64];
    snprintf(buf, sizeof(buf), "Joy X: %4d   Joy Y: %4d", k.joxx, k.joxy);
    gfx_text(10, 40, buf, COLOR_LIGHTBLUE);

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

/*
============================================================
  TEST AUDIO
------------------------------------------------------------
 - Lecture d’un WAV
 - Bip sinus court
============================================================
*/
void test_audio()
{
    printf("=== AUDIO TEST ===\n");

    printf("Lecture WAV...\n");
    audio_play_wav("/sdcard/PAKAMAN/Sons/PACGOMME.wav");

    vTaskDelay(pdMS_TO_TICKS(500));

    printf("Test bip...\n");
    audio_test_enable(true);
    vTaskDelay(pdMS_TO_TICKS(200));
    audio_test_enable(false);

    printf("=== FIN TEST ===\n");
}

/*
============================================================
  TÂCHE DE TESTS (optionnelle)
------------------------------------------------------------
Appelle test_sprites() en boucle.
============================================================
*/
void task_tests(void* param)
{
    int tick = 0;

    while (true)
    {
        test_sprites(tick++);
        lcd_refresh();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
