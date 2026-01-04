/*
============================================================
  app_main.cpp — Version propre, commentée, multitâche
------------------------------------------------------------
Ce fichier ne contient que :
 - l'initialisation hardware
 - le lancement des tâches FreeRTOS
 - la boucle idle

Toute la logique du jeu est dans task_game.cpp
Toute la logique audio est dans task_audio.cpp
Toute la logique input est dans task_input.cpp
============================================================
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

// Hardware libs
#include "lib/expander.h"
#include "lib/LCD.h"
#include "lib/sdcard.h"
#include "lib/audio_sfx.h"
#include "lib/audio_player.h"
#include "lib/audio_sfx_cache.h"

// Core
#include "core/input.h"
#include "core/graphics.h"
#include "core/audio.h"
#include "core/persist.h"

// Game
#include "game/game.h"
#include "game/level.h"
#include "assets/assets.h"

// Tasks
#include "tasks/task_game.h"
#include "tasks/task_audio.h"
#include "tasks/task_input.h"

// Tests (optionnel)
#include "tests/tests.h"

/*
============================================================
  INITIALISATION HARDWARE
============================================================
*/
static void hardware_init()
{
    lcd_init_pwm();
    lcd_update_pwm(64);

    adc_init();
    expander_init();
    LCD_init();
    sd_init();

    audio_settings_load();
    audio_init();
    vTaskDelay(pdMS_TO_TICKS(300));
    sfx_cache_preload_all();

    input_init();
    assets_init();
    highscores_init();
}

/*
============================================================
  app_main — POINT D’ENTRÉE
============================================================
*/
extern "C" void app_main(void)
{
    hardware_init();

    // --- Création des tâches ---
    xTaskCreatePinnedToCore(task_game,  "GameTask",  8192, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(task_audio, "AudioTask", 8192, NULL, 6, NULL, 0);
    xTaskCreatePinnedToCore(task_input, "InputTask", 2048, NULL, 4, NULL, 1);

#ifdef ENABLE_TESTS
    xTaskCreatePinnedToCore(task_tests, "TestsTask", 4096, NULL, 1, NULL, 1);
#endif

    // --- Boucle idle ---
    while (true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}
