#pragma once
#ifndef __CORE_AUDIO_H_
#define __CORE_AUDIO_H_

#include "common.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Configuration audio globale
// -----------------------------------------------------------------------------
#define GB_AUDIO_BUFFER_FIFO_COUNT      4      // nombre de buffers FIFO
#define GB_AUDIO_BUFFER_SAMPLE_COUNT    512    // taille d’un buffer (en samples 16 bits)
#define GB_AUDIO_SAMPLE_RATE            44100  // fréquence d’échantillonnage

// -----------------------------------------------------------------------------
// Initialisation complète du système audio (I2S + TAS2505 + FIFO)
// Retourne 0 si OK, <0 si erreur.
// -----------------------------------------------------------------------------
int audio_init(void);

// -----------------------------------------------------------------------------
// Activation des fonctions de tests internes (sinus / bip, etc.)
// -----------------------------------------------------------------------------
void audio_test_enable(bool enable);

// -----------------------------------------------------------------------------
// Gestion de la FIFO audio (pour la lecture WAV existante)
// -----------------------------------------------------------------------------
void wav_pool_update(void);                        // remplit la FIFO depuis read_audio_file()
void audio_push_buffer(const int16_t* buffer);     // pousse un bloc PCM dans la FIFO

uint32_t audio_fifo_buffer_count(void);            // nombre total de buffers FIFO
uint32_t audio_fifo_buffer_used(void);             // nombre de buffers utilisés
uint32_t audio_fifo_buffer_free(void);             // nombre de buffers libres

// -----------------------------------------------------------------------------
// Lecture WAV (robuste, avec parsing d’entête)
// -----------------------------------------------------------------------------
void audio_play_wav(const char* path);

// -----------------------------------------------------------------------------
// Fonctions utilitaires pour Pac-Man (s'appuient sur audio_play_wav)
// -----------------------------------------------------------------------------
void audio_play_pacgomme(void);
void audio_play_power(void);
void audio_play_gameover(void);

// -----------------------------------------------------------------------------
// Phase 2 : mixage temps réel et SFX synthétiques
// -----------------------------------------------------------------------------

// À appeler dans la boucle principale, après gfx_flush() par exemple.
// Cette fonction mettra à jour le mixeur et poussera, si besoin, des buffers
// dans la FIFO via audio_push_buffer().
void audio_update(void);

#ifdef __cplusplus
}
#endif

#endif // __CORE_AUDIO_H_
