#include "fruits.h"
#include "core/graphics.h"
#include "game/config.h"

/*
============================================================
  FruitManager — Constructeur
------------------------------------------------------------
Initialise l’état interne :
 - aucun fruit actif
 - aucun fruit déjà apparu
============================================================
*/
FruitManager::FruitManager()
{
    active = false;
    spawned1 = false;
    spawned2 = false;
}

/*
============================================================
  reset() — Nouveau niveau
------------------------------------------------------------
Réinitialise l’état des fruits :
 - aucun fruit actif
 - les deux fruits pourront réapparaître
============================================================
*/
void FruitManager::reset()
{
    active = false;
    spawned1 = false;
    spawned2 = false;
}

/*
============================================================
  spawnFruit(level)
------------------------------------------------------------
Choisit le sprite et le score selon le niveau.
Arcade-faithful :
 1 : cerise (100)
 2 : fraise (300)
 3-4 : orange (500)
 5-6 : pomme (700)
 7-8 : melon (1000)
 9-10 : galaxian (2000)
 11-12 : cloche (3000)
 13+ : clé (5000)
============================================================
*/
void FruitManager::spawnFruit(int level)
{
    active = true;
    fruit.timer = 9 * 60; // 9 secondes à 60 FPS

    // Position fixe arcade-faithful (sous la maison)
    fruit.tile_r = 17;
    fruit.tile_c = 13;

    if (level == 1) {
        fruit.sprite = fruit_cherry;
        fruit.score = 100;
    }
    else if (level == 2) {
        fruit.sprite = fruit_strawberry;
        fruit.score = 300;
    }
    else if (level <= 4) {
        fruit.sprite = fruit_orange;
        fruit.score = 500;
    }
    else if (level <= 6) {
        fruit.sprite = fruit_apple;
        fruit.score = 700;
    }
    else if (level <= 8) {
        fruit.sprite = fruit_melon;
        fruit.score = 1000;
    }
    else if (level <= 10) {
        fruit.sprite = fruit_galaxian;
        fruit.score = 2000;
    }
    else if (level <= 12) {
        fruit.sprite = fruit_bell;
        fruit.score = 3000;
    }
    else {
        fruit.sprite = fruit_key;
        fruit.score = 5000;
    }
}

/*
============================================================
  update(pellets_remaining, level)
------------------------------------------------------------
Arcade-faithful :
 - 1er fruit : quand il reste 70 pellets
 - 2e fruit : quand il reste 170 pellets

Le fruit disparaît après 9 secondes.
============================================================
*/
void FruitManager::update(int pellets_remaining, int level)
{
    // Fruit actif → décrémenter timer
    if (active)
    {
        fruit.timer--;
        if (fruit.timer <= 0)
            active = false;
        return;
    }

    // Fruit 1
    if (!spawned1 && pellets_remaining == 70)
    {
        spawned1 = true;
        spawnFruit(level);
        return;
    }

    // Fruit 2
    if (!spawned2 && pellets_remaining == 170)
    {
        spawned2 = true;
        spawnFruit(level);
        return;
    }
}

/*
============================================================
  draw(camera_y)
------------------------------------------------------------
Dessine le fruit si actif.
============================================================
*/
void FruitManager::draw(int camera_y) const
{
    if (!active)
        return;

    int px = fruit.tile_c * TILE_SIZE + (TILE_SIZE - 14) / 2;
    int py = fruit.tile_r * TILE_SIZE + (TILE_SIZE - 14) / 2 - camera_y;

    gfx_drawSprite(px, py, fruit.sprite, 14, 14);
}

/*
============================================================
  checkCollision(pac_x, pac_y)
------------------------------------------------------------
Collision pixel-based simple :
 - si Pac-Man touche le fruit → score + désactivation
============================================================
*/
bool FruitManager::checkCollision(int pac_x, int pac_y, int& out_score)
{
    if (!active)
        return false;

    int fx = fruit.tile_c * TILE_SIZE + TILE_SIZE / 2;
    int fy = fruit.tile_r * TILE_SIZE + TILE_SIZE / 2;

    int dx = pac_x - fx;
    int dy = pac_y - fy;

    if (dx*dx + dy*dy < 20*20)
    {
        active = false;
        out_score = fruit.score;
        return true;
    }

    return false;
}
