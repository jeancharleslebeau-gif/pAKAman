#pragma once
#include <cstdint> 

struct Keys {
    uint32_t raw;       // état brut des touches (bitmask)
    uint32_t pressed;   // touches actuellement pressées
    uint32_t released;  // touches relâchées

    int joxx, joxy;
	bool up, down, left, right;
    bool A, B, C, D, RUN, MENU, R1, L1;
};

void input_init();
void input_poll(Keys& k);
bool isLongPress(const Keys& k, int key);
