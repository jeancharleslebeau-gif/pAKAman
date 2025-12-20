#pragma once
#ifndef __SDCARD_H_
#define __SDCARD_H_

#include <stdint.h>
#include "esp_err.h"

// Initialisation de la carte SD et montage du FS
void sd_init(void);

// Ouvre un fichier audio (WAV) sur la carte SD
esp_err_t open_file(const char *path);

// Lit un bloc de données audio depuis le fichier ouvert
void read_audio_file(uint8_t *ptr, uint32_t u32_length);

#endif // __SDCARD_H_
