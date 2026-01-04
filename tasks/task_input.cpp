/*
============================================================
  task_input.cpp — Tâche input (100 Hz)
------------------------------------------------------------
Cette tâche exécute :
 - input_poll(g_keys)

Elle met à jour l’état global des touches 100 fois par seconde,
ce qui garantit une excellente réactivité même si le jeu tourne
à 60 FPS.
============================================================
*/

#include "task_input.h"
#include "core/input.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

void task_input(void* param)
{
    const int INPUT_US = 10000; // 100 Hz
    int64_t last = esp_timer_get_time();

    while (true)
    {
        int64_t now = esp_timer_get_time();
        int64_t dt = now - last;

        if (dt >= INPUT_US)
        {
            last = now;
            input_poll(g_keys);
        }
        else
        {
            esp_rom_delay_us(2000);
        }
    }
}
