#include "graphics_basic.h"

#define abs(x) ((x)>0?(x):-(x))
#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif


graphics_basic::graphics_basic()
{
    u16_color_pen = color_white;
}

graphics_basic::~graphics_basic()
{
}

// Bresenham's algorithm - thx wikpedia
void graphics_basic::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		_swap_int16_t(x0, y0);
		_swap_int16_t(x1, y1);
	}

	if (x0 > x1) {
		_swap_int16_t(x0, x1);
		_swap_int16_t(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	} else {
		ystep = -1;
	}

	for (; x0<=x1; x0++) {
		if (steep) {
			drawPixel(y0, x0);
		} else {
			drawPixel(x0, y0);
		}
		err -= dy;
		if (err < 0) {
			y0 += ystep;
			err += dx;
		}
	}
}


void graphics_basic::drawFastVLine(int16_t x, int16_t y, int16_t h) {
	// Update in subclasses if desired!
	if (h < 0) {
		y += h + 1;
		h = abs(h);
	}
	drawLine(x, y, x, y+h-1);
}

void graphics_basic::drawFastHLine(int16_t x, int16_t y, int16_t w) {
	// Update in subclasses if desired!
	if (w < 0) {
		x += w + 1;
		w = abs(w);
	}
	drawLine(x, y, x+w-1, y);
}

// Draw a rectangle
void graphics_basic::drawRect(int16_t x, int16_t y, int16_t w, int16_t h) {
	drawFastHLine(x, y, w);
	drawFastHLine(x, y+h-1, w);
	drawFastVLine(x, y, h);
	drawFastVLine(x+w-1, y, h);
}


void graphics_basic::fillRect(int16_t x, int16_t y, int16_t w, int16_t h) {
	// Update in subclasses if desired!
	for (int16_t i=x; i<x+w; i++) {
		drawFastVLine(i, y, h);
	}
}


// Draw a circle outline
void graphics_basic::drawCircle(int16_t x0, int16_t y0, int16_t r) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	drawPixel(x0  , y0+r);
	drawPixel(x0  , y0-r);
	drawPixel(x0+r, y0  );
	drawPixel(x0-r, y0  );

	while (x<y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		drawPixel(x0 + x, y0 + y);
		drawPixel(x0 - x, y0 + y);
		drawPixel(x0 + x, y0 - y);
		drawPixel(x0 - x, y0 - y);
		drawPixel(x0 + y, y0 + x);
		drawPixel(x0 - y, y0 + x);
		drawPixel(x0 + y, y0 - x);
		drawPixel(x0 - y, y0 - x);
	}
}


void graphics_basic::drawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername) {
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	while (x<y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f     += ddF_y;
		}
		x++;
		ddF_x += 2;
		f     += ddF_x;
		if (cornername & 0x4) {
			drawPixel(x0 + x, y0 + y);
			drawPixel(x0 + y, y0 + x);
		}
		if (cornername & 0x2) {
			drawPixel(x0 + x, y0 - y);
			drawPixel(x0 + y, y0 - x);
		}
		if (cornername & 0x8) {
			drawPixel(x0 - y, y0 + x);
			drawPixel(x0 - x, y0 + y);
		}
		if (cornername & 0x1) {
			drawPixel(x0 - y, y0 - x);
			drawPixel(x0 - x, y0 - y);
		}
	}
}

void graphics_basic::fillCircle(int16_t x0, int16_t y0, int16_t r) {
	drawFastVLine(x0, y0-r, 2*r+1);
	fillCircleHelper(x0, y0, r, 3, 0);
}

// Used to do circles and roundrects
void graphics_basic::fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
 uint8_t cornername, int16_t delta) {

	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	while (x<y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f     += ddF_y;
		}
		x++;
		ddF_x += 2;
		f     += ddF_x;

		if (cornername & 0x1) {
			drawFastVLine(x0+x, y0-y, 2*y+1+delta);
			drawFastVLine(x0+y, y0-x, 2*x+1+delta);
		}
		if (cornername & 0x2) {
			drawFastVLine(x0-x, y0-y, 2*y+1+delta);
			drawFastVLine(x0-y, y0-x, 2*x+1+delta);
		}
	}
}

// Draw a triangle
void graphics_basic::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
	drawLine(x0, y0, x1, y1);
	drawLine(x1, y1, x2, y2);
	drawLine(x2, y2, x0, y0);
}

// Fill a triangle
void graphics_basic::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {

	int16_t a, b, y, last;

	// Sort coordinates by Y order (y2 >= y1 >= y0)
	if (y0 > y1) {
		_swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
	}
	if (y1 > y2) {
		_swap_int16_t(y2, y1); _swap_int16_t(x2, x1);
	}
	if (y0 > y1) {
		_swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
	}

	if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
		a = b = x0;
		if(x1 < a)      a = x1;
		else if(x1 > b) b = x1;
		if(x2 < a)      a = x2;
		else if(x2 > b) b = x2;
		drawFastHLine(a, y0, b-a+1);
		return;
	}

	int16_t
		dx01 = x1 - x0,
		dy01 = y1 - y0,
		dx02 = x2 - x0,
		dy02 = y2 - y0,
		dx12 = x2 - x1,
		dy12 = y2 - y1;
	int32_t
		sa   = 0,
		sb   = 0;

	// For upper part of triangle, find scanline crossings for segments
	// 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	// is included here (and second loop will be skipped, avoiding a /0
	// error there), otherwise scanline y1 is skipped here and handled
	// in the second loop...which also avoids a /0 error here if y0=y1
	// (flat-topped triangle).
	if(y1 == y2) last = y1;   // Include y1 scanline
	else         last = y1-1; // Skip it

	for(y=y0; y<=last; y++) {
		a   = x0 + sa / dy01;
		b   = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;
		/* longhand:
		a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
		b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		*/
		if(a > b) _swap_int16_t(a,b);
		drawFastHLine(a, y, b-a+1);
	}

	// For lower part of triangle, find scanline crossings for segments
	// 0-2 and 1-2.  This loop is skipped if y1=y2.
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for(; y<=y2; y++) {
		a   = x1 + sa / dy12;
		b   = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;
		/* longhand:
		a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
		b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		*/
		if(a > b) _swap_int16_t(a,b);
		drawFastHLine(a, y, b-a+1);
	}
}



// Draw a rounded rectangle
void graphics_basic::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r) {
	// smarter version
	drawFastHLine(x+r  , y    , w-2*r); // Top
	drawFastHLine(x+r  , y+h-1, w-2*r); // Bottom
	drawFastVLine(x    , y+r  , h-2*r); // Left
	drawFastVLine(x+w-1, y+r  , h-2*r); // Right
	// draw four corners
	drawCircleHelper(x+r    , y+r    , r, 1);
	drawCircleHelper(x+w-r-1, y+r    , r, 2);
	drawCircleHelper(x+w-r-1, y+h-r-1, r, 4);
	drawCircleHelper(x+r    , y+h-r-1, r, 8);
}

// Fill a rounded rectangle
void graphics_basic::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r) {
	// smarter version
	fillRect(x+r, y, w-2*r, h);

	// draw four corners
	fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1);
	fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1);
}
/*
void graphics_basic::drawChar(int16_t x, int16_t y, unsigned char c, uint8_t size) {
	if(!gfxFont) { // 'Classic' built-in font

		if((x >= _width)            || // Clip right
			 (y >= _height)           || // Clip bottom
			 ((x + fontWidth * size - 1) < 0) || // Clip left
			 ((y + fontHeight * size - 1) < 0))   // Clip top
			return;

		if(!_cp437 && (c >= 176)) c++; // Handle 'classic' charset behavior

		if (c >= 0x80) {
			c -= 0x20;
		}

		for (int8_t i = 0; i < fontWidth; i++) {
			uint8_t line;
			if (i == (fontWidth - 1))
				line = 0x0;
			else
				line = pgm_read_byte(font + (c * (fontWidth - 1)) + i);
			for (int8_t j = 0; j < fontHeight; j++) {
				if (line & 0x1) {
					if (size == 1) // default size
						drawPixel(x + i, y + j);
					else { // big size
						fillRect(x + (i * size), y + (j * size), size, size);
					}
				}
				else if (bgcolor.c != color.c) {
					Color tempcolor = color.c;
					color.c = bgcolor.c;
					if (size == 1) // default size
						drawPixel(x + i, y + j);
					else { // big size
						fillRect(x + i*size, y + j*size, size, size);
					}
					color.c = tempcolor; //restore color to its initial value
				}
				line >>= 1;
			}
		}

	} else { // Custom font

		// Character is assumed previously filtered by write() to eliminate
		// newlines, returns, non-printable characters, etc.  Calling drawChar()
		// directly with 'bad' characters of font may cause mayhem!

		c -= pgm_read_byte(&gfxFont->first);
		GFXglyph *glyph  = &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
		uint8_t  *bitmap = (uint8_t *)pgm_read_pointer(&gfxFont->bitmap);

		uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
		uint8_t  w  = pgm_read_byte(&glyph->width),
					 h  = pgm_read_byte(&glyph->height),
					 xa = pgm_read_byte(&glyph->xAdvance);
		int8_t   xo = pgm_read_byte(&glyph->xOffset),
					 yo = pgm_read_byte(&glyph->yOffset);
		uint8_t  xx, yy, bits, bit = 0;
		int16_t  xo16, yo16;

		if(size > 1) {
			xo16 = xo;
			yo16 = yo;
		}

		// Todo: Add character clipping here

		// NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
		// THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
		// has typically been used with the 'classic' font to overwrite old
		// screen contents with new data.  This ONLY works because the
		// characters are a uniform size; it's not a sensible thing to do with
		// proportionally-spaced fonts with glyphs of varying sizes (and that
		// may overlap).  To replace previously-drawn text when using a custom
		// font, use the getTextBounds() function to determine the smallest
		// rectangle encompassing a string, erase the area with fillRect(),
		// then draw new text.  This WILL infortunately 'blink' the text, but
		// is unavoidable.  Drawing 'background' pixels will NOT fix this,
		// only creates a new set of problems.  Have an idea to work around
		// this (a canvas object type for MCUs that can afford the RAM and
		// displays supporting setAddrWindow() and pushColors()), but haven't
		// implemented this yet.

		for(yy=0; yy<h; yy++) {
			for(xx=0; xx<w; xx++) {
				if(!(bit++ & 7)) {
					bits = pgm_read_byte(&bitmap[bo++]);
				}
				if(bits & 0x80) {
					if(size == 1) {
						drawPixel(x+xo+xx, y+yo+yy);
					} else {
						fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size);
					}
				}
				bits <<= 1;
			}
		}

	} // End classic vs custom font
}
	*/