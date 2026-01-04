// ============================================================================
//  gfx_direct.cpp — Backend graphique DIRECT LCD
// ============================================================================
//
//  Backend simple qui dessine directement sur le LCD via lcd_putpixel() et
//  lcd_draw_str(), sans passer par le framebuffer comme source unique.
//
//  Ce backend est utile pour :
//      - debug matériel
//      - tests simples
//      - comparaison avec le backend framebuffer
//
//  L’API est la même que celle de gfx_fb.*, pour rester interchangeable
//  via la façade graphics.cpp.
// ============================================================================

#include "gfx_direct.h"
#include "lib/LCD.h"
#include "core/graphics.h"
#include "game/config.h"

// Couleur de texte active
extern uint16_t current_text_color;


// ============================================================================
//  Base
// ============================================================================
void gfx_direct_init() {
    LCD_init();
}

void gfx_direct_clear(uint16_t color) {
    lcd_clear(color);   // remplit framebuffer[]
    lcd_refresh();      // pousse vers l’écran immédiatement
}

void gfx_direct_putpixel(int x, int y, uint16_t color) {
    if ((unsigned)x >= SCREEN_W || (unsigned)y >= SCREEN_H)
        return;
    lcd_putpixel((uint16_t)x, (uint16_t)y, color);
}

void gfx_direct_drawSprite(int x, int y,
                           const uint16_t* data,
                           int w, int h)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
            gfx_direct_putpixel(x + i, y + j, data[j * w + i]);
}

void gfx_direct_text(int x, int y, const char* txt, uint16_t color) {
    current_text_color = color;
    lcd_draw_str((uint16_t)x, (uint16_t)y, txt);
}

void gfx_direct_flush() {
    // En mode direct, on considère que tout est visible immédiatement.
    // On peut forcer un rafraîchissement si nécessaire :
    lcd_refresh();
}


// ============================================================================
//  Primitives
// ============================================================================

static inline void dp_plot(int x, int y, uint16_t color) {
    gfx_direct_putpixel(x, y, color);
}

void gfx_direct_drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        dp_plot(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_direct_drawRect(int x, int y, int w, int h, uint16_t color) {
    gfx_direct_drawLine(x, y, x + w - 1, y, color);
    gfx_direct_drawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    gfx_direct_drawLine(x, y, x, y + h - 1, color);
    gfx_direct_drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void gfx_direct_fillRect(int x, int y, int w, int h, uint16_t color) {
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx)
            dp_plot(xx, yy, color);
}

void gfx_direct_drawCircle(int cx, int cy, int r, uint16_t color) {
    int f = 1 - r, ddf_x = 1, ddf_y = -2 * r;
    int x = 0, y = r;

    dp_plot(cx, cy + r, color);
    dp_plot(cx, cy - r, color);
    dp_plot(cx + r, cy, color);
    dp_plot(cx - r, cy, color);

    while (x < y) {
        if (f >= 0) { y--; ddf_y += 2; f += ddf_y; }
        x++; ddf_x += 2; f += ddf_x;

        dp_plot(cx + x, cy + y, color);
        dp_plot(cx - x, cy + y, color);
        dp_plot(cx + x, cy - y, color);
        dp_plot(cx - x, cy - y, color);
        dp_plot(cx + y, cy + x, color);
        dp_plot(cx - y, cy + x, color);
        dp_plot(cx + y, cy - x, color);
        dp_plot(cx - y, cy - x, color);
    }
}

void gfx_direct_fillCircle(int cx, int cy, int r, uint16_t color) {
    gfx_direct_drawLine(cx, cy - r, cx, cy + r, color);

    int f = 1 - r, ddf_x = 1, ddf_y = -2 * r;
    int x = 0, y = r;

    while (x < y) {
        if (f >= 0) { y--; ddf_y += 2; f += ddf_y; }
        x++; ddf_x += 2; f += ddf_x;

        gfx_direct_drawLine(cx - x, cy - y, cx - x, cy + y, color);
        gfx_direct_drawLine(cx + x, cy - y, cx + x, cy + y, color);
        gfx_direct_drawLine(cx - y, cy - x, cx - y, cy + x, color);
        gfx_direct_drawLine(cx + y, cy - x, cx + y, cy + x, color);
    }
}

void gfx_direct_drawTriangle(int x0, int y0,
                             int x1, int y1,
                             int x2, int y2,
                             uint16_t color)
{
    gfx_direct_drawLine(x0, y0, x1, y1, color);
    gfx_direct_drawLine(x1, y1, x2, y2, color);
    gfx_direct_drawLine(x2, y2, x0, y0, color);
}

void gfx_direct_fillTriangle(int x0, int y0,
                             int x1, int y1,
                             int x2, int y2,
                             uint16_t color)
{
    if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
    if (y1 > y2) { std::swap(y1, y2); std::swap(x1, x2); }
    if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }

    auto span = [&](int y, int a, int b) {
        if (a > b) std::swap(a, b);
        for (int x = a; x <= b; ++x) dp_plot(x, y, color);
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

void gfx_direct_drawRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
    gfx_direct_drawRect(x + r, y, w - 2 * r, h, color);
    gfx_direct_drawRect(x, y + r, w, h - 2 * r, color);
    gfx_direct_drawCircle(x + r, y + r, r, color);
    gfx_direct_drawCircle(x + w - r - 1, y + r, r, color);
    gfx_direct_drawCircle(x + r, y + h - r - 1, r, color);
    gfx_direct_drawCircle(x + w - r - 1, y + h - r - 1, r, color);
}

void gfx_direct_fillRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
    gfx_direct_fillRect(x + r, y, w - 2 * r, h, color);
    gfx_direct_fillCircle(x + r, y + r, r, color);
    gfx_direct_fillCircle(x + w - r - 1, y + r, r, color);
    gfx_direct_fillCircle(x + r, y + h - r - 1, r, color);
    gfx_direct_fillCircle(x + w - r - 1, y + h - r - 1, r, color);
}


// ============================================================================
//  Sprites avancés
// ============================================================================

void gfx_direct_drawSpriteTransparent(int x, int y,
                                      const uint16_t* data,
                                      int w, int h,
                                      uint16_t transparentColor)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            uint16_t c = data[j * w + i];
            if (c != transparentColor)
                gfx_direct_putpixel(x + i, y + j, c);
        }
}

void gfx_direct_drawSpriteFlippedH(int x, int y,
                                   const uint16_t* data,
                                   int w, int h)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            int dx = x + (w - 1 - i);
            int dy = y + j;
            gfx_direct_putpixel(dx, dy, data[j * w + i]);
        }
}

void gfx_direct_drawSpriteFlippedV(int x, int y,
                                   const uint16_t* data,
                                   int w, int h)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            int dx = x + i;
            int dy = y + (h - 1 - j);
            gfx_direct_putpixel(dx, dy, data[j * w + i]);
        }
}

void gfx_direct_drawSpriteRot90(int x, int y,
                                const uint16_t* data,
                                int w, int h)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            int dx = x + (h - 1 - j);
            int dy = y + i;
            gfx_direct_putpixel(dx, dy, data[j * w + i]);
        }
}

void gfx_direct_drawSpriteRot180(int x, int y,
                                 const uint16_t* data,
                                 int w, int h)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            int dx = x + (w - 1 - i);
            int dy = y + (h - 1 - j);
            gfx_direct_putpixel(dx, dy, data[j * w + i]);
        }
}

void gfx_direct_drawSpriteRot270(int x, int y,
                                 const uint16_t* data,
                                 int w, int h)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            int dx = x + j;
            int dy = y + (w - 1 - i);
            gfx_direct_putpixel(dx, dy, data[j * w + i]);
        }
}

void gfx_direct_drawSpriteScaled2x(int x, int y,
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
                    gfx_direct_putpixel(dx + ox, dy + oy, c);
        }
}


// ============================================================================
//  Blitting
// ============================================================================
void gfx_direct_blit(int dstX, int dstY,
                     const uint16_t* src,
                     int srcW, int srcH)
{
    gfx_direct_drawSprite(dstX, dstY, src, srcW, srcH);
}

void gfx_direct_blitRegion(int dstX, int dstY,
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

            gfx_direct_putpixel(dx, dy, src[sy * srcW + sx]);
        }
    }
}

void gfx_direct_blitTransparent(int dstX, int dstY,
                                const uint16_t* src,
                                int srcW, int srcH,
                                uint16_t transparentColor)
{
    gfx_direct_drawSpriteTransparent(dstX, dstY, src, srcW, srcH, transparentColor);
}


// ============================================================================
//  Texte avancé
// ============================================================================
void gfx_direct_textRight(int x, int y, const char* txt, uint16_t color) {
    if (!txt) return;
    extern int gfx_text_width(const char* text);
    int w = gfx_text_width(txt);
    gfx_direct_text(x - w, y, txt, color);
}

void gfx_direct_textShadow(int x, int y,
                           const char* txt,
                           uint16_t textColor,
                           uint16_t shadowColor,
                           int offsetX, int offsetY)
{
    if (!txt) return;
    gfx_direct_text(x + offsetX, y + offsetY, txt, shadowColor);
    gfx_direct_text(x, y, txt, textColor);
}


// ============================================================================
//  Debug
// ============================================================================
void gfx_direct_debugRect(int x, int y, int w, int h, uint16_t color) {
    gfx_direct_drawRect(x, y, w, h, color);
}

void gfx_direct_debugPoint(int x, int y, uint16_t color) {
    gfx_direct_putpixel(x, y, color);
}
