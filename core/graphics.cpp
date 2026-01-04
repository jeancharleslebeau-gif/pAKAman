// ============================================================================
//  graphics.cpp — Façade graphique backend-agnostique
// ============================================================================
//
//  Rôle de ce fichier
//  -------------------
//  Ce module ne contient *pas* de logique graphique proprement dite.
//  Il sert d’interface entre le moteur de jeu (qui appelle gfx_*) et
//  le backend graphique sélectionné (framebuffer ou direct LCD).
//
//  Pourquoi une façade ?
//  ----------------------
//  - Séparer l’API publique du moteur de l’implémentation matérielle.
//  - Permettre de basculer entre framebuffer et direct LCD sans toucher au jeu.
//  - Éviter les conflits DMA / écritures directes.
//  - Documenter clairement chaque couche.
//
//  Backends disponibles
//  ---------------------
//    USE_FRAMEBUFFER = 1  → core/gfx_fb.*
//        Pipeline moderne, propre, rapide, basé sur un framebuffer
//        + DMA via lcd_refresh(). Recommandé.
//
//    USE_FRAMEBUFFER = 0  → core/gfx_direct.*
//        Pipeline direct LCD. Utile pour debug matériel.
//
//  Le moteur de jeu utilise uniquement :
//      gfx_init(), gfx_clear(), gfx_flush(), gfx_putpixel16(),
//      gfx_text(), gfx_text_center(), etc.
// ============================================================================

#include "graphics.h"
#include "game/config.h"

// Sélection du backend selon USE_FRAMEBUFFER (défini via CMake)
#if USE_FRAMEBUFFER
    #include "gfx_fb.h"
#else
    #include "gfx_direct.h"
#endif

// ---------------------------------------------------------------------------
//  Variables globales héritées
// ---------------------------------------------------------------------------

// Couleur de texte active (utilisée par lcd_draw_str / lcd_draw_char)
uint16_t current_text_color = COLOR_WHITE;

// Ancien système graphique (compat éventuelle)
graphics_basic gfx;


// ============================================================================
//  INITIALISATION
// ============================================================================
/*
    Initialise le système graphique.

    Backend framebuffer :
        - LCD_init() via gfx_fb_init()
        - configuration i80 + DMA
        - framebuffer prêt

    Backend direct :
        - LCD_init() via gfx_direct_init()
*/
void gfx_init() {
#if USE_FRAMEBUFFER
    gfx_fb_init();
#else
    gfx_direct_init();
#endif
}


// ============================================================================
//  EFFACER L’ÉCRAN
// ============================================================================
/*
    Efface l’écran avec une couleur.

    Backend framebuffer :
        - Remplit framebuffer[]
        - L’affichage réel se fait à gfx_flush()

    Backend direct :
        - Efface directement le LCD + lcd_refresh()
*/
void gfx_clear(uint16_t color) {
#if USE_FRAMEBUFFER
    gfx_fb_clear(color);
#else
    gfx_direct_clear(color);
#endif
}


// ============================================================================
//  RAFRAÎCHISSEMENT DE L’ÉCRAN
// ============================================================================
/*
    Envoie le contenu du framebuffer vers le LCD (DMA) ou force un
    rafraîchissement en mode direct.

    Backend framebuffer :
        - lcd_wait_for_dma()
        - lcd_wait_for_vsync()
        - LCD_FAST_test(framebuffer)

    Backend direct :
        - lcd_refresh()
*/
void gfx_flush() {
#if USE_FRAMEBUFFER
    gfx_fb_flush();
#else
    gfx_direct_flush();
#endif
}


// ============================================================================
//  TEXTE
// ============================================================================
/*
    Le texte utilise la police 8x8 (font8x8_basic) côté LCD.cpp.

    - En mode framebuffer, lcd_draw_str() écrit dans framebuffer[] via
      lcd_putpixel(), donc tout reste cohérent.

    - En mode direct, lcd_draw_str() écrit directement sur l’écran.
*/

// Change la couleur de texte active
void gfx_set_text_color(uint16_t color) {
    current_text_color = color;
}

// Affiche une chaîne de caractères
void gfx_text(int x, int y, const char* txt, uint16_t color) {
#if USE_FRAMEBUFFER
    gfx_fb_text(x, y, txt, color);
#else
    gfx_direct_text(x, y, txt, color);
#endif
}

// Largeur fixe d’un caractère (police monospaced 6 px de large)
static const int FONT_CHAR_WIDTH = 6;

// Largeur d’un caractère
int gfx_char_width(char) {
    return FONT_CHAR_WIDTH;
}

// Largeur d’un texte
int gfx_text_width(const char* text) {
    if (!text) return 0;
    int len = 0;
    for (const char* p = text; *p; ++p) ++len;
    return len * FONT_CHAR_WIDTH;
}

// Centre un texte horizontalement
void gfx_text_center(int y, const char* text, uint16_t color) {
    int w = gfx_text_width(text);
    int x = (SCREEN_W - w) / 2;
    gfx_text(x, y, text, color);
}


// ============================================================================
//  PIXELS
// ============================================================================
/*
    Dessine un pixel à l’écran, route vers le backend.

    Backend framebuffer :
        - framebuffer[y * SCREEN_W + x] = color

    Backend direct :
        - lcd_putpixel(x, y, color)
*/
void gfx_putpixel16(int x, int y, uint16_t color) {
#if USE_FRAMEBUFFER
    gfx_fb_putpixel(x, y, color);
#else
    gfx_direct_putpixel(x, y, color);
#endif
}


// ============================================================================
//  BITMAPS / SPRITE SHEETS (bas niveau)
// ============================================================================
/*
    Ces fonctions restent nommées lcd_* pour compat avec ton ancien code,
    mais elles ne parlent plus directement au LCD : elles routent vers
    le backend choisi.

    Elles sont utilisées par SpriteAtlas et divers modules de rendu.
*/

// Dessine un bitmap complet (pixels RGB565) à l’écran
void lcd_draw_bitmap(const uint16_t* pixels, int w, int h, int dx, int dy) {
#if USE_FRAMEBUFFER
    // Route vers le backend framebuffer
    gfx_fb_drawSprite(dx, dy, pixels, w, h);
#else
    // Route vers le backend direct
    gfx_direct_drawSprite(dx, dy, pixels, w, h);
#endif
}

// Dessine une sous-partie d’un bitmap (sprite sheet)
void lcd_draw_partial_bitmap(const uint16_t* pixels,
                             int sheetW, int sheetH,
                             int sx, int sy,
                             int spriteW, int spriteH,
                             int dx, int dy)
{
    // Sécurité basique sur les bornes source
    if (sx < 0 || sy < 0 ||
        sx + spriteW > sheetW ||
        sy + spriteH > sheetH)
    {
        // On tronque plutôt que planter, mais tu peux décider de retourner
        // directement si tu veux un comportement strict.
        if (sx < 0) { spriteW += sx; dx -= sx; sx = 0; }
        if (sy < 0) { spriteH += sy; dy -= sy; sy = 0; }
        if (sx + spriteW > sheetW) spriteW = sheetW - sx;
        if (sy + spriteH > sheetH) spriteH = sheetH - sy;
        if (spriteW <= 0 || spriteH <= 0) return;
    }

    // Pointeur de départ dans la sheet
    const uint16_t* srcBase = pixels + sy * sheetW + sx;

#if USE_FRAMEBUFFER
    // Ecriture dans le framebuffer via gfx_fb_putpixel() (cohérence totale)
    for (int j = 0; j < spriteH; ++j) {
        int yy = dy + j;
        if ((unsigned)yy >= SCREEN_H) continue;

        const uint16_t* src = srcBase + j * sheetW;
        for (int i = 0; i < spriteW; ++i) {
            int xx = dx + i;
            if ((unsigned)xx >= SCREEN_W) continue;

            gfx_fb_putpixel(xx, yy, src[i]);
        }
    }
#else
    // Mode direct : on écrit directement avec gfx_direct_putpixel()
    for (int j = 0; j < spriteH; ++j) {
        int yy = dy + j;
        if ((unsigned)yy >= SCREEN_H) continue;

        const uint16_t* src = srcBase + j * sheetW;
        for (int i = 0; i < spriteW; ++i) {
            int xx = dx + i;
            if ((unsigned)xx >= SCREEN_W) continue;

            gfx_direct_putpixel(xx, yy, src[i]);
        }
    }
#endif
}
