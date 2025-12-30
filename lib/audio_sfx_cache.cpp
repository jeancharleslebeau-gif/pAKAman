#include "audio_sfx_cache.h"
#include <vector>
#include <string>
#include <stdio.h>
#include <string.h>
#include <new>   // pour std::nothrow

struct CacheEntry {
    std::string path;
    int16_t* data;
    uint32_t length;
};

static std::vector<CacheEntry> cache;

bool sfx_cache_load(const char* path, const int16_t** out_data, uint32_t* out_len)
{
    // 1) Déjà en cache ?
    for (auto& e : cache) {
        if (e.path == path) {
            if (out_data) *out_data = e.data;
            if (out_len)  *out_len  = e.length;
            return true;
        }
    }

    // 2) Ouvrir le fichier
    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("SFX preload: missing file %s\n", path);
        return false;
    }

    // 3) Taille du fichier
    if (fseek(f, 0, SEEK_END) != 0) {
        printf("SFX preload: fseek end failed %s\n", path);
        fclose(f);
        return false;
    }

    long file_size = ftell(f);
    if (file_size < 44) {
        printf("SFX preload: WAV too small %s\n", path);
        fclose(f);
        return false;
    }

    long data_size_bytes = file_size - 44;
    uint32_t sample_count = (uint32_t)(data_size_bytes / sizeof(int16_t));

    if (sample_count == 0) {
        printf("SFX preload: no PCM data %s\n", path);
        fclose(f);
        return false;
    }

    // 4) Allocation unique (sans exceptions)
    int16_t* data = new (std::nothrow) int16_t[sample_count];
    if (!data) {
        printf("SFX preload: out of memory for %s (%lu samples)\n", path, sample_count);
        fclose(f);
        return false;
    }

    // 5) Lire les données PCM
    if (fseek(f, 44, SEEK_SET) != 0) {
        printf("SFX preload: fseek data failed %s\n", path);
        fclose(f);
        delete[] data;
        return false;
    }

    size_t read_count = fread(data, sizeof(int16_t), sample_count, f);
    fclose(f);

    if (read_count != sample_count) {
        printf("SFX preload: fread truncated %s (%zu/%lu)\n", path, read_count, sample_count);
        delete[] data;
        return false;
    }

    // 6) Ajouter au cache
    cache.push_back({path, data, sample_count});

    if (out_data) *out_data = data;
    if (out_len)  *out_len  = sample_count;

    printf("SFX preload: ok %s (%lu samples)\n", path, sample_count);
    return true;
}

void sfx_cache_preload_all()
{
    const char* list[] = {
        "/sdcard/PAKAMAN/Sons/PACGOMME.wav",
        "/sdcard/PAKAMAN/Sons/BONUS.wav",
        "/sdcard/PAKAMAN/Sons/G_EATEN.wav",  
    };

    for (auto path : list)
        sfx_cache_load(path, nullptr, nullptr);
}
