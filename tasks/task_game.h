	#pragma once

/*
============================================================
  task_game.h — Tâche principale du moteur de jeu (60 Hz)
------------------------------------------------------------
Cette tâche exécute :
 - game_update() : logique du jeu
 - game_draw()   : rendu graphique
 - gfx_flush()   : transfert vers le framebuffer
 - lcd_refresh() : rafraîchissement de l’écran

Elle tourne à 60 FPS fixes grâce à un timer haute précision.
============================================================
*/

#ifdef __cplusplus
extern "C" {
#endif

void task_game(void* param);

#ifdef __cplusplus
}
#endif
