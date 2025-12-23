
// Chaque sprite est un tableau 16x16 pixels (uint16_t = couleur RGB565)
// Ici on utilise des placeholders simples : remplissage uniforme ou petit motif

#pragma once
#include <stdint.h>

// Sprites 16×16 (non-const pour pouvoir les initialiser au runtime)
extern uint16_t tile_wall[16*16];
extern uint16_t tile_pacgum[16*16];
extern uint16_t tile_powerdot[16*16];

// sprites de pacman avec son animation
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

// sprties Bliky (rouge alias Shadow: le chasseur de Pac-Man)
extern uint16_t ghost_red_0[16*16];
extern uint16_t ghost_red_1[16*16];

// sprties de Pinky (rose alias Speedy: il anticipe la position de Pac-Man)
extern uint16_t ghost_pink_0[16*16];
extern uint16_t ghost_pink_1[16*16];

// sprites d'Inky (cyan alias Bashful: comportement mixte)
extern uint16_t ghost_blue_0[16*16];
extern uint16_t ghost_blue_1[16*16];

// sprites de Clyde (Orange alias Pokey: alterne entre se rapprocher et fuir Pac-Man)
extern uint16_t ghost_orange_0[16*16];
extern uint16_t ghost_orange_1[16*16];

// sprites des fantômes effrayés blancs
extern const uint16_t ghost_white_0[16 * 16]; 
extern const uint16_t ghost_white_1[16 * 16];

// sprites des fantômes effrayés bleu foncé
extern uint16_t ghost_scared_0[16*16];
extern uint16_t ghost_scared_1[16*16];

// sprites des yeux (10x5) indique la direction des fantômes 
extern const uint16_t ghost_eyes_left[10 * 5];
extern const uint16_t ghost_eyes_right[10 * 5];
extern const uint16_t ghost_eyes_up[10 * 5];
extern const uint16_t ghost_eyes_down[10 * 5];

// sprite du tunnel dans le mur
extern const uint16_t tile_tunnel_wall[];

// sprites du sol devant le tunnel permettant d'y entrer
extern const uint16_t tile_tunnel_entry_left[];
extern const uint16_t tile_tunnel_entry_right[];
extern const uint16_t tile_tunnel_entry_up[];
extern const uint16_t tile_tunnel_entry_down[];
extern const uint16_t tile_tunnel_entry_neutral[];

// sprites de la porte de la maison des fantômes
extern const uint16_t tile_ghost_door_closed[];
extern const uint16_t tile_ghost_door_opening[];
extern const uint16_t tile_ghost_door_open[];

// fonctions publiques
void assets_init();
