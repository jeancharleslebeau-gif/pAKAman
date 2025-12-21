
// Chaque sprite est un tableau 16x16 pixels (uint16_t = couleur RGB565)
// Ici on utilise des placeholders simples : remplissage uniforme ou petit motif

#pragma once
#include <stdint.h>

// Sprites 16×16 (non-const pour pouvoir les initialiser au runtime)
extern uint16_t tile_wall[16*16];
extern uint16_t tile_pacgum[16*16];
extern uint16_t tile_powerdot[16*16];

extern uint16_t pacman_right_0[16*16];
extern uint16_t pacman_right_1[16*16];
extern uint16_t pacman_right_2[16*16];
extern uint16_t pacman_left_0[16*16];
extern uint16_t pacman_left_1[16*16];
extern uint16_t pacman_left_2[16*16];
extern uint16_t pacman_up_0[16*16];
extern uint16_t pacman_up_1[16*16];
extern uint16_t pacman_up_2[16*16];
extern uint16_t pacman_down_0[16*16];
extern uint16_t pacman_down_1[16*16];
extern uint16_t pacman_down_2[16*16];

extern uint16_t ghost_red_0[16*16];
extern uint16_t ghost_red_1[16*16];
extern uint16_t ghost_pink_0[16*16];
extern uint16_t ghost_pink_1[16*16];
extern uint16_t ghost_blue_0[16*16];
extern uint16_t ghost_blue_1[16*16];
extern uint16_t ghost_orange_0[16*16];
extern uint16_t ghost_orange_1[16*16];
extern uint16_t ghost_scared_0[16*16];
extern uint16_t ghost_scared_1[16*16];

extern const uint16_t tile_tunnel_wall[];
extern const uint16_t tile_tunnel_entry_left[];
extern const uint16_t tile_tunnel_entry_right[];
extern const uint16_t tile_tunnel_entry_up[];
extern const uint16_t tile_tunnel_entry_down[];
extern const uint16_t tile_tunnel_entry_neutral[];

extern const uint16_t tile_ghost_door_closed[];
extern const uint16_t tile_ghost_door_opening[];
extern const uint16_t tile_ghost_door_open[];

void assets_init();
