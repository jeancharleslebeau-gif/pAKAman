#pragma once
#include <stdint.h>
#include "lib/graphics_basic.h"   // Pour compatibilité avec ton ancien système si nécessaire

/*
============================================================
  graphics.h — API graphique unifiée (backend-agnostique)
------------------------------------------------------------
Ce fichier expose une API unique utilisée par tout le moteur
de jeu (sprites, texte, UI, maze, etc.).

Le rendu réel est délégué à un backend sélectionné via
USE_FRAMEBUFFER :

    USE_FRAMEBUFFER = 1 → backend framebuffer (DMA)
    USE_FRAMEBUFFER = 0 → backend direct LCD (pixel par pixel)

Les backends sont implémentés dans :
    lib/gfx/gfx_fb.*      (framebuffer)
    lib/gfx/gfx_direct.*  (direct LCD)

============================================================
*/

// ------------------------------------------------------------
// Choix du backend graphique
// ------------------------------------------------------------
#ifndef USE_FRAMEBUFFER
#define USE_FRAMEBUFFER 1   // 1 = framebuffer (recommandé), 0 = direct LCD
#endif

// ------------------------------------------------------------
// API PUBLIQUE — utilisée par le moteur de jeu
// ------------------------------------------------------------

// Initialise le système graphique (LCD + backend)
void gfx_init();

// Efface l'écran avec une couleur
void gfx_clear(uint16_t color);

// Rafraîchit l'écran (push framebuffer → LCD ou no-op selon backend)
void gfx_flush();

// Dessine un pixel (coordonnées écran)
void gfx_putpixel16(int x, int y, uint16_t color);

// Affiche du texte (police 8x8)
void gfx_text(int x, int y, const char* txt, uint16_t color);

// Change la couleur de texte active
void gfx_set_text_color(uint16_t color);

// Calcule la largeur d’un texte en pixels
int gfx_text_width(const char* text);

// Centre un texte horizontalement
void gfx_text_center(int y, const char* text, uint16_t color);

// Largeur d’un caractère (police monospaced)
int gfx_char_width(char c);

// Instance héritée (compatibilité)
extern graphics_basic gfx;

// ------------------------------------------------------------
// API BAS NIVEAU — utilisée par certains modules internes
// ------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

// Dessine une sous-partie d’un bitmap (sprite sheet)
void lcd_draw_partial_bitmap(const uint16_t* pixels,
                             int sheetW, int sheetH,
                             int sx, int sy,
                             int spriteW, int spriteH,
                             int dx, int dy);

// Dessine un bitmap complet
void lcd_draw_bitmap(const uint16_t* pixels,
                     int w, int h,
                     int x, int y);

// Couleur de texte active (utilisée par lcd_draw_str)
extern uint16_t current_text_color;

#ifdef __cplusplus
}
#endif

// ------------------------------------------------------------
// Palette de couleurs standard (BGR565)
// ------------------------------------------------------------
#define COLOR_BLACK      0x0000
#define COLOR_WHITE      0xFFFF
#define COLOR_RED        0x001F
#define COLOR_GREEN      0x03E0 
#define COLOR_BLUE       0xF800
#define COLOR_GRAY       0x84D5
#define COLOR_DARKGRAY   0x426A
#define COLOR_PURPLE     0x4012
#define COLOR_PINK       0x8239
#define COLOR_ORANGE     0x155F
#define COLOR_BROWN      0x4479
#define COLOR_BEIGE      0x96BF
#define COLOR_YELLOW     0x073E
#define COLOR_LIGHTGREEN 0x07E0  
#define COLOR_DARKBLUE   0x8200
#define COLOR_LIGHTBLUE  0xFDCF
#define COLOR_SILVER     0xBDF7
#define COLOR_GOLD       0x159C

