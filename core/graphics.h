#pragma once
#include <stdint.h>
#include "lib/graphics_basic.h"  

void gfx_init();
void gfx_clear(uint16_t color);
void gfx_flush();
void gfx_text(int x, int y, const char* txt, uint16_t color);
void gfx_set_text_color(uint16_t color);
void gfx_putpixel16(int x, int y, uint16_t color);

extern graphics_basic gfx;

#ifdef __cplusplus
extern "C" {
#endif

// Bas niveau
// Dessine une sous partie d’un bitmap (sprite dans une sprite sheet)
void lcd_draw_partial_bitmap(const uint16_t* pixels,
                             int sheetW, int sheetH,
                             int sx, int sy,
                             int spriteW, int spriteH,
                             int dx, int dy);

// Dessine une image
void lcd_draw_bitmap(const uint16_t* pixels,
                     int w, int h,
                     int x, int y);

	
	
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

extern "C" {
#endif

extern uint16_t current_text_color;

#ifdef __cplusplus
}
#endif

// Couleurs usuelles
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


void gfx_text(int x, int y, const char* str, unsigned short color);
void gfx_flush();


// Calcule la largeur en pixels d'un texte
int gfx_text_width(const char* text);

// Centre un texte horizontalement sur l'écran
void gfx_text_center(int y, const char* text, uint16_t color);

// Largeur d'un caractère de taille fixe
int gfx_char_width(char c);
