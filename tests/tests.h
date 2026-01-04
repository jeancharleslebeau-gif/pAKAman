#pragma once

/*
============================================================
  tests.h — Tâche de tests (optionnelle)
------------------------------------------------------------
Contient :
 - task_tests() : boucle dédiée aux tests
 - test_sprites()
 - test_joystick()
 - test_audio()

Ces fonctions sont isolées dans un module séparé pour garder
app_main.cpp propre et éviter de polluer le moteur de jeu.
============================================================
*/

#ifdef __cplusplus
extern "C" {
#endif

void task_tests(void* param);

void test_sprites(int tick);
void test_joystick();
void test_audio();

#ifdef __cplusplus
}
#endif
