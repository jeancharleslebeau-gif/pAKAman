// ============================================================================
//  sprite_atlas.cpp — Gestion d’une sprite sheet (atlas)
// ============================================================================
//
//  Ce module utilise lcd_draw_partial_bitmap(), qui est routé via graphics.cpp
//  vers le backend framebuffer ou direct LCD.
//
//  Aucun accès direct au LCD ici.
// ============================================================================

#include "sprite_atlas.h"
#include "core/graphics.h"
#include <cstdio>

void SpriteAtlas::load(const uint16_t* p, int sw, int sh, int w, int h) {
    pixels  = p;
    sheetW  = sw;
    sheetH  = sh;
    spriteW = w;
    spriteH = h;
    cols    = sheetW / spriteW;
}

void SpriteAtlas::draw(int index, int dx, int dy) {
    if (!pixels) return;

    int sx = (index % cols) * spriteW;
    int sy = (index / cols) * spriteH;

    // Appel backend-agnostique
    lcd_draw_partial_bitmap(
        pixels,
        sheetW, sheetH,
        sx, sy,
        spriteW, spriteH,
        dx, dy
    );
}
