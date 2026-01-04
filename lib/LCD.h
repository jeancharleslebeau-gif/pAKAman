#pragma once
#ifndef __LCD_H_
#define __LCD_H_

#include "common.h"
#include <stdint.h>

/*
===============================================================================
  LCD.h — Interface bas niveau du contrôleur LCD ST7789 (bus i80 + DMA)
-------------------------------------------------------------------------------
Ce module fournit :

  - L'initialisation complète du LCD (bus i80, registres ST7789, rotation…)
  - L'accès au framebuffer 320×240 (RGB565)
  - Les primitives bas niveau : lcd_putpixel(), lcd_draw_char(), lcd_draw_str()
  - Le pipeline DMA propre :
        lcd_wait_for_dma()
        lcd_wait_for_vsync()
        lcd_start_dma()
        lcd_refresh()

  Ce module ne contient aucune logique de rendu haut niveau.
  Il est utilisé par les backends graphiques (gfx_fb / gfx_direct).
===============================================================================
*/


// ============================================================================
//  Commandes ST7789
// ============================================================================
#define ST7789V_CMD_RESET         0x01
#define ST7789V_CMD_GET_SCANLINE  0x45
#define ST7789V_CMD_SLPOUT        0x11
#define ST7789V_CMD_COLMOD        0x3A
#define COLOR_MODE_65K            0x50
#define COLOR_MODE_16BIT          0x05
#define ST7789V_CMD_INVOFF        0x20
#define ST7789V_CMD_INVON         0x21
#define ST7789V_CMD_NORON         0x13
#define ST7789V_CMD_DISPON        0x29
#define ST7789V_CMD_RAMWR         0x2C
#define ST7789V_CMD_RDID          0x04
#define ST7789V_CMD_MADCTL        0x36

// Bits du registre MADCTL
#define ST7789V_MADCTRL_MY        0x80  // flip vertical
#define ST7789V_MADCTRL_MX        0x40  // flip horizontal
#define ST7789V_MADCTRL_MV        0x20  // swap X/Y
#define ST7789V_MADCTRL_ML        0x10  // refresh order
#define ST7789V_MADCTRL_RGB       0x08  // RGB/BGR
#define ST7789V_MADCTRL_MH        0x04  // refresh direction

#define ST7789V_CMD_CASET         0x2A
#define ST7789V_CMD_RASET         0x2B


// ============================================================================
//  Framebuffer global (défini dans LCD.cpp)
// ============================================================================
extern uint16_t framebuffer[320 * 240];


// ============================================================================
//  Initialisation LCD
// ============================================================================
void LCD_init();   // Initialise bus i80 + ST7789 + rotation + DMA


// ============================================================================
//  Pipeline DMA propre
// ============================================================================
//
//  lcd_wait_for_dma()   → attend la fin du DMA précédent (non bloquant)
//  lcd_wait_for_vsync() → optionnel, synchronisation FMARK (anti-tearing)
//  lcd_start_dma()      → déclenche un transfert DMA depuis framebuffer[]
//  lcd_refresh()        → pipeline complet (attend + vsync + start DMA)
//
void lcd_wait_for_dma();
void lcd_wait_for_vsync();
void lcd_start_dma();
void lcd_refresh();

uint8_t  lcd_refresh_completed();   // indique si le DMA précédent est terminé
uint32_t LCD_last_refresh_delay();  // timing du dernier DMA (debug)


// ============================================================================
//  Primitives bas niveau (écriture directe dans framebuffer)
// ============================================================================
void lcd_putpixel(uint16_t x, uint16_t y, uint16_t color);

void lcd_clear(uint16_t color);     // remplit framebuffer[]

void lcd_draw_char(uint16_t x, uint16_t y, char c);
void lcd_draw_char_bg(uint16_t x, uint16_t y, char c, uint16_t bgColor);

void lcd_draw_str(uint16_t x, uint16_t y, const char* text);
void lcd_draw_str_bg(uint16_t x, uint16_t y, const char* text, uint16_t bgColor);

void lcd_draw_text(uint16_t x, uint16_t y, const char* text); // alias pratique

void lcd_move_cursor(uint16_t x, uint16_t y);
void lcd_printf(const char* fmt, ...);


// ============================================================================
//  Utilitaires
// ============================================================================

// Conversion RGB888 → RGB565
static inline uint16_t lcd_color_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (r >> 3) | ((g >> 2) << 5) | ((b >> 3) << 11);
}

void lcd_set_fps(uint8_t fps);   // configuration du framerate ST7789


// ============================================================================
//  Palette Gamebuino (héritage)
// ============================================================================
typedef enum gamebuino_color {
    color_white     = 0xFFFF,
    color_gray      = 0x84D5,
    color_darkgray  = 0x426A,
    color_black     = 0x0000,
    color_purple    = 0x4012,
    color_pink      = 0x8239,
    color_red       = 0x20FB,
    color_orange    = 0x155F,
    color_brown     = 0x4479,
    color_beige     = 0x96BF,
    color_yellow    = 0x073E,
    color_lightgreen= 0x4670,
    color_green     = 0x5440,
    color_darkblue  = 0x8200,
    color_blue      = 0xCC28,
    color_lightblue = 0xFDCF
} gamebuino_color_e;

#endif // __LCD_H_
