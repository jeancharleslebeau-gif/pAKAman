#pragma once

/*
============================================================
  task_audio.h — Tâche audio (1 kHz)
------------------------------------------------------------
Cette tâche exécute :
 - audio_update() : mixage, PMF, SFX

Elle tourne à 1 kHz pour garantir une latence minimale
et une qualité audio stable, indépendamment du framerate.
============================================================
*/

#ifdef __cplusplus
extern "C" {
#endif

void task_audio(void* param);

#ifdef __cplusplus
}
#endif
