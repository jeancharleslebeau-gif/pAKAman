#pragma once
#include <cstdint>

/*
===============================================================================
  sprite_atlas.h — Gestion d’un atlas de sprites (sprite sheet)
-------------------------------------------------------------------------------
Un SpriteAtlas représente une grande image contenant plusieurs sprites
organisés en grille régulière.

Ce module NE DESSINE PAS directement sur le LCD.
Il utilise lcd_draw_partial_bitmap(), qui est routé via graphics.cpp vers :
    - gfx_fb_*()     (framebuffer + DMA)
    - gfx_direct_*() (LCD direct)

Ainsi, le moteur reste totalement indépendant du pipeline graphique réel.
===============================================================================
*/

class SpriteAtlas {
public:

    /**
     * @brief Charge un atlas de sprites.
     *
     * @param pixels   Pointeur vers les données RGB565 de la sprite sheet
     * @param sheetW   Largeur totale de la sheet (en pixels)
     * @param sheetH   Hauteur totale de la sheet (en pixels)
     * @param spriteW  Largeur d’un sprite (en pixels)
     * @param spriteH  Hauteur d’un sprite (en pixels)
     *
     * L’atlas est supposé être une grille régulière :
     *      cols = sheetW / spriteW
     *      rows = sheetH / spriteH
     */
    void load(const uint16_t* pixels,
              int sheetW, int sheetH,
              int spriteW, int spriteH);

    /**
     * @brief Dessine un sprite de l’atlas.
     *
     * @param index  Index du sprite dans la grille (0 = en haut à gauche)
     * @param dx     Position X de destination
     * @param dy     Position Y de destination
     *
     * Le calcul de la position source est automatique :
     *      sx = (index % cols) * spriteW
     *      sy = (index / cols) * spriteH
     *
     * Le dessin est effectué via lcd_draw_partial_bitmap(), donc routé
     * automatiquement vers le backend graphique actif.
     */
    void draw(int index, int dx, int dy);

private:
    const uint16_t* pixels = nullptr;  ///< Données RGB565 de la sheet
    int sheetW = 0;                    ///< Largeur totale de la sheet
    int sheetH = 0;                    ///< Hauteur totale de la sheet
    int spriteW = 0;                   ///< Largeur d’un sprite
    int spriteH = 0;                   ///< Hauteur d’un sprite
    int cols = 0;                      ///< Nombre de sprites par ligne
};
