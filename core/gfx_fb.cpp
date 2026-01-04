	// ============================================================================
//  gfx_fb.cpp — Backend graphique FRAMEBUFFER (DMA → LCD)
// ============================================================================
//
//  Version refondue et optimisée pour pipeline DMA propre.
//  Toutes les écritures se font dans framebuffer[].
//  lcd_refresh() déclenche le DMA (via LCD_FAST_test).
//
//  Pipeline :
//      1. gfx_fb_clear() / gfx_fb_putpixel() / gfx_fb_drawSprite()...
//      2. gfx_fb_flush()
//             → lcd_wait_for_dma()
//             → lcd_wait_for_vsync()   (si activé)
//             → lcd_start_dma()
//
//  Aucun accès direct au LCD ici.
//  Aucune écriture concurrente pendant DMA.
// ============================================================================

#include "gfx_fb.h"
#include "lib/LCD.h"
#include "core/graphics.h"
#include "game/config.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <algorithm>
#include "assets/font8x8_basic.h"


// Framebuffer réel défini dans LCD.cpp
extern uint16_t framebuffer[320 * 240];

// Couleur de texte active (utilisée par lcd_draw_str)
extern uint16_t current_text_color;


// ============================================================================
//  Initialisation
// ============================================================================
void gfx_fb_init() {
    LCD_init();
}


// ============================================================================
//  Effacer l’écran
// ============================================================================
void gfx_fb_clear(uint16_t color) {
    lcd_clear(color); // remplit framebuffer[]
}


// ============================================================================
//  Pixel → framebuffer
// ============================================================================
void gfx_fb_putpixel(int x, int y, uint16_t color) {
    if ((unsigned)x >= SCREEN_W || (unsigned)y >= SCREEN_H)
        return;
    framebuffer[y * SCREEN_W + x] = color;
}


// ============================================================================
//  Sprite simple
// ============================================================================
void gfx_fb_drawSprite(int x, int y,
                       const uint16_t* data,
                       int w, int h)
{
    for (int j = 0; j < h; ++j) {
        int yy = y + j;
        if ((unsigned)yy >= SCREEN_H) continue;

        uint16_t* dst = &framebuffer[yy * SCREEN_W + x];
        const uint16_t* src = &data[j * w];

        for (int i = 0; i < w; ++i) {
            int xx = x + i;
            if ((unsigned)xx < SCREEN_W)
                dst[i] = src[i];
        }
    }
}


// ============================================================================
//  Texte
// ============================================================================
void gfx_fb_drawChar(int x, int y, char c, uint16_t color)
{
    const uint8_t* glyph = font8x8_basic[(uint8_t)c];

    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << col)) {
                gfx_fb_putpixel(x + col, y + row, color);
            }
        }
    }
}

void gfx_fb_text(int x, int y, const char* txt, uint16_t color)
{
    while (*txt) {
        gfx_fb_drawChar(x, y, *txt, color);
        x += 8;
        txt++;
    }
}


// ============================================================================
//  Flush → DMA propre
// ============================================================================
void gfx_fb_flush() {
    lcd_wait_for_dma();
    lcd_wait_for_vsync();
    lcd_start_dma();
}


// ============================================================================
//  Primitives géométriques
// ============================================================================

static inline void fb_plot(int x, int y, uint16_t color) {
    gfx_fb_putpixel(x, y, color);
}

void gfx_fb_drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        fb_plot(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_fb_drawRect(int x, int y, int w, int h, uint16_t color) {
    gfx_fb_drawLine(x, y, x + w - 1, y, color);
    gfx_fb_drawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    gfx_fb_drawLine(x, y, x, y + h - 1, color);
    gfx_fb_drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void gfx_fb_fillRect(int x, int y, int w, int h, uint16_t color) {
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx)
            fb_plot(xx, yy, color);
}

void gfx_fb_drawCircle(int cx, int cy, int r, uint16_t color) {
    int f = 1 - r, ddf_x = 1, ddf_y = -2 * r;
    int x = 0, y = r;

    fb_plot(cx, cy + r, color);
    fb_plot(cx, cy - r, color);
    fb_plot(cx + r, cy, color);
    fb_plot(cx - r, cy, color);

    while (x < y) {
        if (f >= 0) { y--; ddf_y += 2; f += ddf_y; }
        x++; ddf_x += 2; f += ddf_x;

        fb_plot(cx + x, cy + y, color);
        fb_plot(cx - x, cy + y, color);
        fb_plot(cx + x, cy - y, color);
        fb_plot(cx - x, cy - y, color);
        fb_plot(cx + y, cy + x, color);
        fb_plot(cx - y, cy + x, color);
        fb_plot(cx + y, cy - x, color);
        fb_plot(cx - y, cy - x, color);
    }
}

void gfx_fb_fillCircle(int cx, int cy, int r, uint16_t color) {
    gfx_fb_drawLine(cx, cy - r, cx, cy + r, color);

    int f = 1 - r, ddf_x = 1, ddf_y = -2 * r;
    int x = 0, y = r;

    while (x < y) {
        if (f >= 0) { y--; ddf_y += 2; f += ddf_y; }
        x++; ddf_x += 2; f += ddf_x;

        gfx_fb_drawLine(cx - x, cy - y, cx - x, cy + y, color);
        gfx_fb_drawLine(cx + x, cy - y, cx + x, cy + y, color);
        gfx_fb_drawLine(cx - y, cy - x, cx - y, cy + x, color);
        gfx_fb_drawLine(cx + y, cy - x, cx + y, cy + x, color);
    }
}

void gfx_fb_drawTriangle(int x0, int y0,
                         int x1, int y1,
                         int x2, int y2,
                         uint16_t color)
{
    gfx_fb_drawLine(x0, y0, x1, y1, color);
    gfx_fb_drawLine(x1, y1, x2, y2, color);
    gfx_fb_drawLine(x2, y2, x0, y0, color);
}

void gfx_fb_fillTriangle(int x0, int y0,
                         int x1, int y1,
                         int x2, int y2,
                         uint16_t color)
{
    // Tri rempli simple (déjà correct dans ton code)
    if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
    if (y1 > y2) { std::swap(y1, y2); std::swap(x1, x2); }
    if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }

    auto span = [&](int y, int a, int b) {
        if (a > b) std::swap(a, b);
        for (int x = a; x <= b; ++x) fb_plot(x, y, color);
    };

    int total = y2 - y0;
    if (total == 0) return;

    int h1 = y1 - y0;
    int h2 = y2 - y1;

    for (int y = 0; y <= total; ++y) {
        bool second = y > h1 || h1 == 0;
        int seg = second ? h2 : h1;
        if (seg == 0) continue;

        float alpha = (float)y / total;
        float beta  = (float)(y - (second ? h1 : 0)) / seg;

        int ax = x0 + (int)((x2 - x0) * alpha);
        int bx = second
                 ? x1 + (int)((x2 - x1) * beta)
                 : x0 + (int)((x1 - x0) * beta);

        span(y0 + y, ax, bx);
    }
}

void gfx_fb_drawRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
    gfx_fb_drawRect(x + r, y, w - 2 * r, h, color);
    gfx_fb_drawRect(x, y + r, w, h - 2 * r, color);
    gfx_fb_drawCircle(x + r, y + r, r, color);
    gfx_fb_drawCircle(x + w - r - 1, y + r, r, color);
    gfx_fb_drawCircle(x + r, y + h - r - 1, r, color);
    gfx_fb_drawCircle(x + w - r - 1, y + h - r - 1, r, color);
}

void gfx_fb_fillRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
    gfx_fb_fillRect(x + r, y, w - 2 * r, h, color);
    gfx_fb_fillCircle(x + r, y + r, r, color);
    gfx_fb_fillCircle(x + w - r - 1, y + r, r, color);
    gfx_fb_fillCircle(x + r, y + h - r - 1, r, color);
    gfx_fb_fillCircle(x + w - r - 1, y + h - r - 1, r, color);
}


// ============================================================================
//  Sprites avancés
// ============================================================================

void gfx_fb_drawSpriteTransparent(int x, int y,
                                  const uint16_t* data,
                                  int w, int h,
                                  uint16_t transparentColor)
{
    for (int j = 0; j < h; ++j) {
        int yy = y + j;
        if ((unsigned)yy >= SCREEN_H) continue;

        for (int i = 0; i < w; ++i) {
            int xx = x + i;
            if ((unsigned)xx >= SCREEN_W) continue;

            uint16_t c = data[j * w + i];
            if (c != transparentColor)
                framebuffer[yy * SCREEN_W + xx] = c;
        }
    }
}

void gfx_fb_drawSpriteFlippedH(int x, int y,
                               const uint16_t* data,
                               int w, int h)
{
    for (int j = 0; j < h; ++j) {
        int yy = y + j;
        if ((unsigned)yy >= SCREEN_H) continue;

        for (int i = 0; i < w; ++i) {
            int xx = x + (w - 1 - i);
            if ((unsigned)xx >= SCREEN_W) continue;

            framebuffer[yy * SCREEN_W + xx] = data[j * w + i];
        }
    }
}

void gfx_fb_drawSpriteFlippedV(int x, int y,
                               const uint16_t* data,
                               int w, int h)
{
    for (int j = 0; j < h; ++j) {
        int yy = y + (h - 1 - j);
        if ((unsigned)yy >= SCREEN_H) continue;

        for (int i = 0; i < w; ++i) {
            int xx = x + i;
            if ((unsigned)xx >= SCREEN_W) continue;

            framebuffer[yy * SCREEN_W + xx] = data[j * w + i];
        }
    }
}

void gfx_fb_drawSpriteRot90(int x, int y,
                            const uint16_t* data,
                            int w, int h)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            int dx = x + (h - 1 - j);
            int dy = y + i;
            if ((unsigned)dx < SCREEN_W && (unsigned)dy < SCREEN_H)
                framebuffer[dy * SCREEN_W + dx] = data[j * w + i];
        }
}

void gfx_fb_drawSpriteRot180(int x, int y,
                             const uint16_t* data,
                             int w, int h)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            int dx = x + (w - 1 - i);
            int dy = y + (h - 1 - j);
            if ((unsigned)dx < SCREEN_W && (unsigned)dy < SCREEN_H)
                framebuffer[dy * SCREEN_W + dx] = data[j * w + i];
        }
}

void gfx_fb_drawSpriteRot270(int x, int y,
                             const uint16_t* data,
                             int w, int h)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            int dx = x + j;
            int dy = y + (w - 1 - i);
            if ((unsigned)dx < SCREEN_W && (unsigned)dy < SCREEN_H)
                framebuffer[dy * SCREEN_W + dx] = data[j * w + i];
        }
}

void gfx_fb_drawSpriteScaled2x(int x, int y,
                               const uint16_t* data,
                               int w, int h)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            uint16_t c = data[j * w + i];
            int dx = x + i * 2;
            int dy = y + j * 2;

            for (int oy = 0; oy < 2; ++oy)
                for (int ox = 0; ox < 2; ++ox)
                    gfx_fb_putpixel(dx + ox, dy + oy, c);
        }
}


// ============================================================================
//  Blitting
// ============================================================================
void gfx_fb_blit(int dstX, int dstY,
                 const uint16_t* src,
                 int srcW, int srcH)
{
    gfx_fb_drawSprite(dstX, dstY, src, srcW, srcH);
}

void gfx_fb_blitRegion(int dstX, int dstY,
                       const uint16_t* src,
                       int srcW, int srcH,
                       int srcX, int srcY,
                       int blitW, int blitH)
{
    for (int j = 0; j < blitH; ++j) {
        int sy = srcY + j;
        int dy = dstY + j;
        if ((unsigned)sy >= srcH || (unsigned)dy >= SCREEN_H) continue;

        for (int i = 0; i < blitW; ++i) {
            int sx = srcX + i;
            int dx = dstX + i;
            if ((unsigned)sx >= srcW || (unsigned)dx >= SCREEN_W) continue;

            framebuffer[dy * SCREEN_W + dx] = src[sy * srcW + sx];
        }
    }
}

void gfx_fb_blitTransparent(int dstX, int dstY,
                            const uint16_t* src,
                            int srcW, int srcH,
                            uint16_t transparentColor)
{
    gfx_fb_drawSpriteTransparent(dstX, dstY, src, srcW, srcH, transparentColor);
}


// ============================================================================
//  Framebuffer utils
// ============================================================================
uint16_t* gfx_fb_getFramebuffer() {
    return framebuffer;
}

void gfx_fb_copyFramebuffer(uint16_t* dest) {
    if (!dest) return;
    memcpy(dest, framebuffer, SCREEN_W * SCREEN_H * sizeof(uint16_t));
}

void gfx_fb_fillFramebuffer(uint16_t color) {
    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i)
        framebuffer[i] = color;
}


// ============================================================================
//  Texte avancé
// ============================================================================
void gfx_fb_textRight(int x, int y, const char* txt, uint16_t color) {
    if (!txt) return;
    extern int gfx_text_width(const char* text);
    int w = gfx_text_width(txt);
    gfx_fb_text(x - w, y, txt, color);
}

void gfx_fb_textShadow(int x, int y,
                       const char* txt,
                       uint16_t textColor,
                       uint16_t shadowColor,
                       int offsetX, int offsetY)
{
    if (!txt) return;
    gfx_fb_text(x + offsetX, y + offsetY, txt, shadowColor);
    gfx_fb_text(x, y, txt, textColor);
}


// ============================================================================
//  Effets simples
// ============================================================================
void gfx_fb_flashScreen(uint16_t color, int flashes) {
    for (int i = 0; i < flashes; ++i) {
        gfx_fb_clear(color);
        gfx_fb_flush();
        // Le timing est laissé au moteur (vTaskDelay dans task_game)
    }
}

void gfx_fb_fadeToColor(uint16_t color, int steps) {
    uint16_t* fb = framebuffer;
    const int total = SCREEN_W * SCREEN_H;

    int r2 = (color >> 11) & 0x1F;
    int g2 = (color >> 5)  & 0x3F;
    int b2 =  color        & 0x1F;

    for (int s = 0; s < steps; ++s) {
        float t = (float)(s + 1) / (float)steps;

        for (int i = 0; i < total; ++i) {
            uint16_t c = fb[i];

            int r = (c >> 11) & 0x1F;
            int g = (c >> 5)  & 0x3F;
            int b =  c        & 0x1F;

            int rn = r + (int)((r2 - r) * t);
            int gn = g + (int)((g2 - g) * t);
            int bn = b + (int)((b2 - b) * t);

            fb[i] = (uint16_t)((rn << 11) | (gn << 5) | bn);
        }

        gfx_fb_flush();
        // Le timing est laissé au moteur
    }
}


// ============================================================================
//  Debug visuel
// ============================================================================
void gfx_fb_debugRect(int x, int y, int w, int h, uint16_t color) {
    gfx_fb_drawRect(x, y, w, h, color);
}

void gfx_fb_debugPoint(int x, int y, uint16_t color) {
    gfx_fb_putpixel(x, y, color);
}
