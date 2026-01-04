/*
============================================================
  task_audio.cpp — Tâche audio (1 kHz)
------------------------------------------------------------
Cette tâche exécute :
 - audio_update() : mixage, PMF, SFX

Elle tourne à 1 kHz pour garantir une latence minimale
et une qualité audio stable, indépendamment du framerate.
============================================================
*/

#include "task_audio.h"
#include "core/audio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"

void task_audio(void* param)
{
    const int AUDIO_US = 1000; // 1 kHz
    int64_t last = esp_timer_get_time();

    while (true)
    {
        int64_t now = esp_timer_get_time();
        int64_t dt = now - last;

        if (dt >= AUDIO_US)
        {
            last = now;
            audio_update();
        }
        else
        {
            vTaskDelay(1); // 1 ms
        }
    }
}
