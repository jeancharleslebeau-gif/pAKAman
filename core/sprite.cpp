#include "sprite.h"
#include "core/graphics.h"

void draw_sprite16(int x, int y, const uint16_t* pixels) {
    for (int j = 0; j < 16; ++j) {
        for (int i = 0; i < 16; ++i) {
            uint16_t c = pixels[j * 16 + i];
            gfx_putpixel16(x + i, y + j, c);
        }
    }
}


void draw_sprite16_transparent(int x, int y, const uint16_t* pixels, uint16_t transparentColor) {
    for (int j = 0; j < 16; ++j) {
        for (int i = 0; i < 16; ++i) {
            uint16_t c = pixels[j * 16 + i];
            if (c != transparentColor) {
                gfx_putpixel16(x + i, y + j, c);
            }
        }
    }
}


void gfx_drawSprite(int x, int y, const uint16_t* sprite, int w, int h) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            uint16_t color = sprite[dy * w + dx];
            lcd_putpixel(x + dx, y + dy, color);
        }
    }
}



// Dessine un sprite de taille (spriteW, spriteH) dans une grille (tileSizeX, tileSizeY)
// gridX, gridY = position en cases de la grille
// offsetX, offsetY = décalage du sprite dans la case (pour le centrer ou ajuster)
// pixels = pointeur sur les données du sprite
void draw_sprite_grid(int gridX, int gridY,
                      int tileSizeX, int tileSizeY,
                      int spriteW, int spriteH,
                      int offsetX, int offsetY,
                      const uint16_t* pixels) {
    int px = gridX * tileSizeX + offsetX;
    int py = gridY * tileSizeY + offsetY;

    for (int j = 0; j < spriteH; ++j) {
        for (int i = 0; i < spriteW; ++i) {
            uint16_t c = pixels[j * spriteW + i];
            gfx_putpixel16(px + i, py + j, c);
        }
    }
}


void draw_sprite_grid_transparent(int gridX, int gridY,
                                  int tileSizeX, int tileSizeY,
                                  int spriteW, int spriteH,
                                  int offsetX, int offsetY,
                                  const uint16_t* pixels,
                                  uint16_t transparentColor) {
    int px = gridX * tileSizeX + offsetX;
    int py = gridY * tileSizeY + offsetY;

    for (int j = 0; j < spriteH; ++j) {
        for (int i = 0; i < spriteW; ++i) {
            uint16_t c = pixels[j * spriteW + i];
            if (c != transparentColor) {
                gfx_putpixel16(px + i, py + j, c);
            }
        }
    }
}
