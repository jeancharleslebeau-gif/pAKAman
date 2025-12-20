#pragma once
#include <vector>
#include <cstdint>

#include "maze.h"     // Maze complet
#include "pacman.h"   // Pacman complet
#include "ghost.h"    // Ghost complet

struct GameState {
    enum class State {
        Title,       // écran de titre
        Playing,     // partie en cours
        Paused,      // pause volontaire
        GameOver,    // partie terminée
        Highscores,  // affichage des scores
        Options      // menu des réglages
    };
    State state = State::Title;

    Maze maze;                   // le labyrinthe courant
    Pacman pacman;               // le joueur
    std::vector<Ghost> ghosts;   // les ennemis
    int score = 0;               // score courant
    int lives = 3;               // vies restantes
    int levelIndex = 0;          // numéro du niveau courant
};

// Fonctions principales du jeu
void game_init(GameState& g);
void game_update(GameState& g);
void game_draw(const GameState& g);
bool game_is_over(const GameState& g);
