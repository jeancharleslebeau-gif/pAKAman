#pragma once
#ifndef __LCD_H_
#define __LCD_H_

#include "common.h"

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
#define ST7789V_CMD_RAMWR         0x2c
#define ST7789V_CMD_RDID          0x04
#define ST7789V_CMD_MADCTL        0x36
    #define ST7789V_MADCTRL_MY        0x80  // y flip
    #define ST7789V_MADCTRL_MX        0x40  // x flip
    #define ST7789V_MADCTRL_MV        0x20  // x/y swap
    #define ST7789V_MADCTRL_ML        0x10  // refresh top to bottom / bottom to top
    #define ST7789V_MADCTRL_RGB       0x08  // RGB/BGR
    #define ST7789V_MADCTRL_MH        0x04  // refresh left to right / right to left 

#define ST7789V_CMD_CASET       0x2A
#define ST7789V_CMD_RASET       0x2B

extern uint16_t framebuffer[];

void LCD_init();
void LCD_FAST_test(const uint16_t* u16_pframe_buffer);
uint32_t LCD_last_refresh_delay();

void lcd_draw_char(uint16_t x, uint16_t y, char c);
void lcd_draw_char_bg(uint16_t x, uint16_t y, char c, uint16_t bgColor);
void lcd_draw_str(uint16_t x, uint16_t y, const char* pc);
void lcd_draw_str_bg(uint16_t x, uint16_t y, const char* pc, uint16_t bgColor);
void lcd_draw_text(uint16_t x, uint16_t y, const char* pc);

void lcd_refresh();
void lcd_clear(uint16_t u16_pix_color);
void lcd_printf(const char *pc_format, ...);
uint8_t lcd_refresh_completed();
void lcd_move_cursor(uint16_t x, uint16_t y);
void lcd_putpixel(uint16_t x, uint16_t y, uint16_t u16_color);

// RGB 888 to RGB 565
inline uint16_t lcd_colopr_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    return (red >> 3) | ((green >> 2) << 5) | ((blue >> 3) << (5 + 6));
}

void lcd_set_fps(uint8_t u8_fps);

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
