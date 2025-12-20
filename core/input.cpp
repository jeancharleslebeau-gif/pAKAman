#include "input.h"
#include "lib/expander.h"
#include "driver/gpio.h"  

static uint16_t prev=0;

void input_init(){ prev=0; }

void input_poll(Keys& k) {
    // lecture brute des touches
    uint16_t raw = expander_read();
    k.raw = raw;

    k.pressed  = raw & ~prev;   // nouvelles touches pressées
    k.released = prev & ~raw;   // touches relâchées
    prev       = raw;

    // mapping des boutons
    k.up    = raw & EXPANDER_KEY_UP;
    k.down  = raw & EXPANDER_KEY_DOWN;
    k.left  = raw & EXPANDER_KEY_LEFT;
    k.right = raw & EXPANDER_KEY_RIGHT;

    k.A   = raw & EXPANDER_KEY_A;
    k.B   = raw & EXPANDER_KEY_B;
    k.C   = raw & EXPANDER_KEY_C;
    k.D   = raw & EXPANDER_KEY_D;
    k.RUN = raw & EXPANDER_KEY_RUN;
	k.MENU= raw & EXPANDER_KEY_MENU;
	k.R1  = raw & EXPANDER_KEY_R1;
	k.L1  = raw & EXPANDER_KEY_L1;
	k.joxx = adc_read_joyx();
    k.joxy = adc_read_joyy();
}

bool isLongPress(const Keys& k, int key) {
    static int pressDuration = 0;

    if (k.raw & key) {
        pressDuration++;
        if (pressDuration > 60) { // ~1 seconde si vTaskDelay(16) = 60 FPS
            pressDuration = 0;
            return true;
        }
    } else {
        pressDuration = 0;
    }
    return false;
}