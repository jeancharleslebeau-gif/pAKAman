#pragma once
#include <stdint.h>

/*
===============================================================================
  gfx_fb.h — Backend graphique FRAMEBUFFER (DMA → LCD)
-------------------------------------------------------------------------------
Ce backend implémente le pipeline graphique moderne basé sur un framebuffer
unique (320×240 RGB565), envoyé au LCD via DMA (esp_lcd_panel_io_tx_color).

Il fournit :
    - Toutes les primitives 2D (lignes, rectangles, cercles…)
    - Le dessin de sprites (opaque, transparent, flip, rotation…)
    - Le blitting (copie de zones)
    - Le texte
    - Les effets simples (fade, flash)
    - Les outils debug

Aucune fonction ici n’écrit directement sur le LCD :
    → Toutes les écritures se font dans framebuffer[]
    → lcd_refresh() déclenche le DMA
===============================================================================
*/

// ============================================================================
//  INITIALISATION
// ============================================================================
void gfx_fb_init();                 // Initialise LCD + DMA + framebuffer
void gfx_fb_clear(uint16_t color);  // Efface framebuffer[]
void gfx_fb_flush();                // Envoie framebuffer → LCD (DMA)


// ============================================================================
//  PIXELS
// ============================================================================
void gfx_fb_putpixel(int x, int y, uint16_t color);


// ============================================================================
//  SPRITES (w × h)
// ============================================================================
void gfx_fb_drawSprite(int x, int y,
                       const uint16_t* data,
                       int w, int h);

void gfx_fb_drawSpriteTransparent(int x, int y,
                                  const uint16_t* data,
                                  int w, int h,
                                  uint16_t transparentColor);

void gfx_fb_drawSpriteFlippedH(int x, int y,
                               const uint16_t* data,
                               int w, int h);

void gfx_fb_drawSpriteFlippedV(int x, int y,
                               const uint16_t* data,
                               int w, int h);

void gfx_fb_drawSpriteRot90(int x, int y,
                            const uint16_t* data,
                            int w, int h);

void gfx_fb_drawSpriteRot180(int x, int y,
                             const uint16_t* data,
                             int w, int h);

void gfx_fb_drawSpriteRot270(int x, int y,
                             const uint16_t* data,
                             int w, int h);

void gfx_fb_drawSpriteScaled2x(int x, int y,
                               const uint16_t* data,
                               int w, int h);


// ============================================================================
//  PRIMITIVES GÉOMÉTRIQUES
// ============================================================================
void gfx_fb_drawLine(int x0, int y0, int x1, int y1, uint16_t color);
void gfx_fb_drawRect(int x, int y, int w, int h, uint16_t color);
void gfx_fb_fillRect(int x, int y, int w, int h, uint16_t color);

void gfx_fb_drawCircle(int cx, int cy, int r, uint16_t color);
void gfx_fb_fillCircle(int cx, int cy, int r, uint16_t color);

void gfx_fb_drawTriangle(int x0, int y0,
                         int x1, int y1,
                         int x2, int y2,
                         uint16_t color);

void gfx_fb_fillTriangle(int x0, int y0,
                         int x1, int y1,
                         int x2, int y2,
                         uint16_t color);

void gfx_fb_drawRoundRect(int x, int y, int w, int h, int r, uint16_t color);
void gfx_fb_fillRoundRect(int x, int y, int w, int h, int r, uint16_t color);


// ============================================================================
//  BLITTING
// ============================================================================
void gfx_fb_blit(int dstX, int dstY,
                 const uint16_t* src,
                 int srcW, int srcH);

void gfx_fb_blitRegion(int dstX, int dstY,
                       const uint16_t* src,
                       int srcW, int srcH,
                       int srcX, int srcY,
                       int blitW, int blitH);

void gfx_fb_blitTransparent(int dstX, int dstY,
                            const uint16_t* src,
                            int srcW, int srcH,
                            uint16_t transparentColor);


// ============================================================================
//  TEXTE
// ============================================================================
void gfx_fb_text(int x, int y, const char* txt, uint16_t color);
void gfx_fb_textRight(int x, int y, const char* txt, uint16_t color);
void gfx_fb_textShadow(int x, int y,
                       const char* txt,
                       uint16_t textColor,
                       uint16_t shadowColor,
                       int offsetX, int offsetY);


// ============================================================================
//  FRAMEBUFFER UTILS
// ============================================================================
uint16_t* gfx_fb_getFramebuffer();      // Retourne framebuffer[]
void gfx_fb_copyFramebuffer(uint16_t* dest);
void gfx_fb_fillFramebuffer(uint16_t color);


// ============================================================================
//  EFFETS
// ============================================================================
void gfx_fb_flashScreen(uint16_t color, int flashes);
void gfx_fb_fadeToColor(uint16_t color, int steps);


// ============================================================================
//  DEBUG
// ============================================================================
void gfx_fb_debugRect(int x, int y, int w, int h, uint16_t color);
void gfx_fb_debugPoint(int x, int y, uint16_t color);
