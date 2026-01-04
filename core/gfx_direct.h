#pragma once
#include <stdint.h>

/*
===============================================================================
  gfx_direct.h — Backend graphique DIRECT LCD
-------------------------------------------------------------------------------
Ce backend écrit directement sur le LCD via lcd_putpixel(), sans utiliser
le framebuffer comme référence principale.

Ce mode est utile pour :
    - du debug très simple
    - des tests matériels
    - des projets minimaux

Pour pAKAman, le backend FRAMEBUFFER est recommandé, mais ce backend reste
disponible et compatible avec l’API façade gfx_*().
===============================================================================
*/

// Base
void gfx_direct_init();
void gfx_direct_clear(uint16_t color);
void gfx_direct_putpixel(int x, int y, uint16_t color);
void gfx_direct_drawSprite(int x, int y,
                           const uint16_t* data,
                           int w, int h);
void gfx_direct_text(int x, int y, const char* txt, uint16_t color);
void gfx_direct_flush(); // No-op en général

// Primitives
void gfx_direct_drawLine(int x0, int y0, int x1, int y1, uint16_t color);
void gfx_direct_drawRect(int x, int y, int w, int h, uint16_t color);
void gfx_direct_fillRect(int x, int y, int w, int h, uint16_t color);
void gfx_direct_drawCircle(int cx, int cy, int r, uint16_t color);
void gfx_direct_fillCircle(int cx, int cy, int r, uint16_t color);
void gfx_direct_drawTriangle(int x0, int y0,
                             int x1, int y1,
                             int x2, int y2,
                             uint16_t color);
void gfx_direct_fillTriangle(int x0, int y0,
                             int x1, int y1,
                             int x2, int y2,
                             uint16_t color);
void gfx_direct_drawRoundRect(int x, int y, int w, int h, int r, uint16_t color);
void gfx_direct_fillRoundRect(int x, int y, int w, int h, int r, uint16_t color);

// Sprites avancés
void gfx_direct_drawSpriteTransparent(int x, int y,
                                      const uint16_t* data,
                                      int w, int h,
                                      uint16_t transparentColor);

void gfx_direct_drawSpriteFlippedH(int x, int y,
                                   const uint16_t* data,
                                   int w, int h);

void gfx_direct_drawSpriteFlippedV(int x, int y,
                                   const uint16_t* data,
                                   int w, int h);

void gfx_direct_drawSpriteRot90(int x, int y,
                                const uint16_t* data,
                                int w, int h);

void gfx_direct_drawSpriteRot180(int x, int y,
                                 const uint16_t* data,
                                 int w, int h);

void gfx_direct_drawSpriteRot270(int x, int y,
                                 const uint16_t* data,
                                 int w, int h);

void gfx_direct_drawSpriteScaled2x(int x, int y,
                                   const uint16_t* data,
                                   int w, int h);

// Blitting
void gfx_direct_blit(int dstX, int dstY,
                     const uint16_t* src,
                     int srcW, int srcH);

void gfx_direct_blitRegion(int dstX, int dstY,
                           const uint16_t* src,
                           int srcW, int srcH,
                           int srcX, int srcY,
                           int blitW, int blitH);

void gfx_direct_blitTransparent(int dstX, int dstY,
                                const uint16_t* src,
                                int srcW, int srcH,
                                uint16_t transparentColor);

// Texte avancé
void gfx_direct_textRight(int x, int y, const char* txt, uint16_t color);
void gfx_direct_textShadow(int x, int y,
                           const char* txt,
                           uint16_t textColor,
                           uint16_t shadowColor,
                           int offsetX, int offsetY);

// Debug
void gfx_direct_debugRect(int x, int y, int w, int h, uint16_t color);
void gfx_direct_debugPoint(int x, int y, uint16_t color);
