#pragma once
#include <stdint.h>

// Dessine un sprite 16x16 en RGB565
void draw_sprite16(int x, int y, const uint16_t* pixels);
void draw_sprite16_transparent(int x, int y, const uint16_t* pixels, uint16_t transparentColor);

void draw_sprite_grid(int gridX, int gridY,
                      int tileSizeX, int tileSizeY,
                      int spriteW, int spriteH,
                      int offsetX, int offsetY,
                      const uint16_t* pixels);
					  
// Variante avec couleur de transparence
void draw_sprite_grid_transparent(int gridX, int gridY,
                                  int tileSizeX, int tileSizeY,
                                  int spriteW, int spriteH,
                                  int offsetX, int offsetY,
                                  const uint16_t* pixels,
                                  uint16_t transparentColor);

void gfx_drawSprite(int x, int y, const uint16_t* sprite, int w, int h);
								  
								  
								  // Structure générique d’animation
struct SpriteAnim {
    const uint16_t** frames;   // tableau de pointeurs vers les frames
    int frameCount;            // nombre de frames
    int tick;                  // compteur interne
    int speed;                 // vitesse (nombre de ticks par frame)
    int currentFrame;          // frame courante

    SpriteAnim(const uint16_t** f, int count, int spd)
        : frames(f), frameCount(count), tick(0), speed(spd), currentFrame(0) {}

    // Avance l’animation
    void update() {
        tick++;
        if (tick >= speed) {
            tick = 0;
            currentFrame = (currentFrame + 1) % frameCount;
        }
    }

    // Récupère la frame courante
    const uint16_t* getFrame() const {
        return frames[currentFrame];
    }
};
