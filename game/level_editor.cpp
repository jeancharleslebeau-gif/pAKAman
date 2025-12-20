#include "config.h"
#include "brick.h"
#include "level.h"
#include "lcd.h"
#include "input.h"
#include "expander.h"
#include "graphics_basic.h"
#include "core/graphics.h"
#include "assets/spritesheet_bricks.h"   

struct Cursor {
    int gx, gy; // position en grille
};

// Instance globale pour dessiner
static graphics_basic gfx;

void draw_cursor(const Cursor& c) {
    gfx.setColor(COLOR_WHITE); // curseur blanc
    gfx.drawRect(
        c.gx * BRICK_W,
        c.gy * BRICK_H,
        BRICK_W,
        BRICK_H
    );
}

void level_editor_run(Level& lvl) {
    Cursor cursor{0, 0};
    bool running = true;

    while (running) {
        lcd_clear(COLOR_BLACK);

        // Affiche toutes les briques
        for (auto& b : lvl.bricks) {
            if (b.alive) {
                const uint16_t* sprite = get_brick_sprite_pixels(b.color_index, b.hp, b.indestructible);
                lcd_draw_bitmap(sprite, BRICK_W, BRICK_H, b.x, b.y);
            }
        }

        // Affiche le curseur
        draw_cursor(cursor);
        lcd_refresh();

        // Lecture des touches
        Keys k;
        input_poll(k);

        // Déplacement du curseur
 		if (k.raw & EXPANDER_KEY_LEFT)  cursor.gx = (cursor.gx - 1 + BRICK_COLS) % BRICK_COLS;
		if (k.raw & EXPANDER_KEY_RIGHT) cursor.gx = (cursor.gx + 1) % BRICK_COLS;
		if (k.raw & EXPANDER_KEY_UP)    cursor.gy = (cursor.gy - 1 + BRICK_ROWS) % BRICK_ROWS;
		if (k.raw & EXPANDER_KEY_DOWN)  cursor.gy = (cursor.gy + 1) % BRICK_ROWS;		
		
        // Trouve la brique sous le curseur
        Brick* selected = nullptr;
        for (auto& b : lvl.bricks) {
            if (b.x == cursor.gx * BRICK_W && b.y == cursor.gy * BRICK_H && b.alive) {
                selected = &b;
                break;
            }
        }

        // Placer une brique si vide
        if (k.raw & EXPANDER_KEY_A && !selected) {
            Brick newBrick;
            newBrick.x = cursor.gx * BRICK_W;
            newBrick.y = cursor.gy * BRICK_H;
            newBrick.hp = 1;
            newBrick.alive = true;
            newBrick.indestructible = false;
            newBrick.color_index = 0;
            lvl.bricks.push_back(newBrick);
        }

        // Supprimer une brique
        if ((k.raw & EXPANDER_KEY_D) && selected) {
            selected->alive = false;
        }

        // Changer couleur avec C/B
		if ((k.raw & EXPANDER_KEY_C) && selected) {
			selected->color_index = (selected->color_index + 1) % BRICK_COLOR_COUNT;
		}
		if ((k.raw & EXPANDER_KEY_B) && selected) {
			selected->color_index = (selected->color_index - 1 + BRICK_COLOR_COUNT) % BRICK_COLOR_COUNT;
		}

        // Quitter
        if (k.raw & EXPANDER_KEY_RUN) {
            running = false;
        }
    }
}
