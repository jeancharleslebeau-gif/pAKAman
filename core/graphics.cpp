#include "graphics.h"
#include "lib/LCD.h"
#include "game/config.h"
#include "lib/graphics_basic.h"
#include <cstring>

uint16_t current_text_color = COLOR_WHITE; //couleur d'écriture par défaut	

graphics_basic gfx;   // instance globale

// Initialisation
void gfx_init() {
    LCD_init();  
}

// Effacer l’écran
void gfx_clear(uint16_t color) {
    lcd_clear(color);   
}

// Rafraîchir l’écran
void gfx_flush() {
    lcd_refresh();      
}

// Changement de la couleur d'écriture
void gfx_set_text_color(uint16_t color) {
    current_text_color = color;
}

// Afficher du texte
void gfx_text(int x, int y, const char* txt, uint16_t color) {
    gfx_set_text_color(color);   // on change la couleur active
    lcd_draw_str(x, y, txt);     // lcd_draw_str utilise current_text_color
}

const int FONT_CHAR_WIDTH = 6; // largeur fixe en pixels

int gfx_char_width(char c) {
    (void)c; // inutilisé si police monospaced
    return FONT_CHAR_WIDTH;
}

int gfx_text_width(const char* text) {
    if (!text) return 0;
    return strlen(text) * FONT_CHAR_WIDTH;
}

void gfx_text_center(int y, const char* text, uint16_t color) {
    int textWidth = gfx_text_width(text);
    int x = (SCREEN_W - textWidth) / 2;
    gfx_text(x, y, text, color);
}

// Dessiner un bitmap complet
void lcd_draw_bitmap(const uint16_t* pixels, int w, int h, int dx, int dy) {
    // Version simple : copie pixel par pixel
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            uint16_t color = pixels[j * w + i];
            lcd_putpixel(dx + i, dy + j, color);
        }
    }
}

void lcd_draw_partial_bitmap(const uint16_t* pixels,
                             int sheetW, int sheetH,
                             int sx, int sy,
                             int spriteW, int spriteH,
                             int dx, int dy) {
    for (int y = 0; y < spriteH; ++y) {
        for (int x = 0; x < spriteW; ++x) {
            uint16_t color = pixels[(sy + y) * sheetW + (sx + x)];
            lcd_putpixel(dx + x, dy + y, color);
        }
    }
}

void gfx_putpixel16(int x, int y, uint16_t color) {
    // sécurité : éviter d’écrire hors écran
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H) return;
    lcd_putpixel((uint16_t)x, (uint16_t)y, color);
}

