#pragma once
#include "stdint.h"
#include "lcd.h"

class graphics_basic {
    public:
    graphics_basic();
     ~graphics_basic();
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
	void drawFastVLine(int16_t x, int16_t y, int16_t h);
	void drawFastHLine(int16_t x, int16_t y, int16_t w);
	void drawRect(int16_t x, int16_t y, int16_t w, int16_t h);
	void fillRect(int16_t x, int16_t y, int16_t w, int16_t h);
	void drawCircle(int16_t x0, int16_t y0, int16_t r);
	void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername);
	void fillCircle(int16_t x0, int16_t y0, int16_t r);
	void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta);
	void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
	void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
	void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius);
	void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius);

    inline void drawPixel( int16_t x, int16_t y, uint16_t u16_color ) {
        lcd_putpixel( x, y, u16_color );
    }
    inline void drawPixel( int16_t x, int16_t y ) {
        lcd_putpixel( x, y, u16_color_pen );
    }
    inline void setColor( uint16_t u16_color ) {
        u16_color_pen = u16_color;
    }

    private:
        uint16_t u16_color_pen;
};


