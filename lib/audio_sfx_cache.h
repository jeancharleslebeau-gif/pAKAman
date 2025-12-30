#pragma once
#include <stdint.h>

bool sfx_cache_load(const char* path, const int16_t** out_data, uint32_t* out_len);
void sfx_cache_preload_all();
