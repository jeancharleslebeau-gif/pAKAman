	#pragma once

	// Dimensions de l’écran
	constexpr int SCREEN_W = 320;
	constexpr int SCREEN_H = 240;

	// Dimensions du labyrinthe
	constexpr int TILE_SIZE = 16;          // chaque case du labyrinthe
	constexpr int MAZE_COLS = SCREEN_W / TILE_SIZE;   // 20 colonnes
	constexpr int MAZE_ROWS = SCREEN_H / TILE_SIZE;   // 15 lignes
	
	// Donner d'alignement de la grille / déplacement dans la grille
	constexpr int GRID_X0 = 0;      // origine X de la grille (pixels)
	constexpr int GRID_Y0 = 0;      // origine Y de la grille (pixels)
	constexpr int CENTER_EPS = 2;   // tolérance d’alignement au centre (pixels)
	constexpr int SNAP_EPS   = 3;   // distance sous laquelle on "snap" au centre

	// Pacman
	constexpr int PACMAN_SIZE = 14;        // sprite ~ taille d’une tile
	constexpr float PACMAN_SPEED = 2.0f;   // vitesse de déplacement (pixels/frame)
	constexpr int PACMAN_OFFSET = (TILE_SIZE - PACMAN_SIZE) / 2;
	

	// Fantômes
	constexpr int GHOST_SIZE = 14;
	constexpr float GHOST_SPEED = 1.8f;    // vitesse légèrement inférieure
	constexpr int GHOST_OFFSET  = (TILE_SIZE - GHOST_SIZE) / 2;
	constexpr int NUM_GHOSTS = 4;          // classique: Blinky, Pinky, Inky, Clyde
	
	#define FRIGHTENED_DURATION_TICKS 360      // exemple : 6 secondes
	#define FRIGHTENED_BLINK_START_TICKS 120   // exemple : clignotement sur les 2 dernières secondes
	
	// Temps entre les sorties des fantômes (en ticks)
	static const int GHOST_RELEASE_INTERVAL_TICKS = 3 * 60; // 3 secondes

	// Temps avant la première sortie
	static const int FIRST_GHOST_RELEASE_TICKS = 5 * 60; // 5 secondes

	// Points et bonus
	constexpr int DOT_SCORE = 10;          // pac-gum
	constexpr int POWERDOT_SCORE = 50;     // super pac-gum
	constexpr int GHOST_SCORE = 200;       // fantôme mangé
	constexpr int FRUIT_SCORE = 500;       // bonus fruit

	// Mode debug
	extern int debug;   // 0 = off, 1 = on
