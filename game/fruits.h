#pragma once
#include <stdint.h>
#include "assets/assets.h"

/*
============================================================
  fruits.h — Gestion des fruits (arcade-faithful)
------------------------------------------------------------
Ce module gère :
 - l’apparition des fruits selon le niveau
 - leur durée d’affichage
 - leur collision avec Pac-Man
 - leur score
 - leur sprite (défini dans assets)

Arcade-faithful :
 - 1er fruit apparaît après 70 pellets mangés
 - 2e fruit après 170 pellets
 - chaque fruit reste ~9 secondes
 - score dépend du niveau
============================================================
*/

struct Fruit {
    int tile_r;                 // position en tuiles
    int tile_c;
    int timer;                  // durée restante avant disparition
    int score;                  // score associé
    const uint16_t* sprite;     // sprite 14x14
};

class FruitManager {
public:
    FruitManager();

    // Appelé chaque frame (60 Hz)
    void update(int pellets_remaining, int level);

    // Dessine le fruit si actif
    void draw(int camera_y) const;

    // Collision Pac-Man → renvoie true si fruit mangé
    bool checkCollision(int pac_x, int pac_y, int& out_score);

    // Reset complet (nouveau niveau)
    void reset();

private:
    bool active;        // fruit visible ?
    bool spawned1;      // premier fruit déjà apparu ?
    bool spawned2;      // deuxième fruit déjà apparu ?
    Fruit fruit;

    void spawnFruit(int level);
};
