#include "persist.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <cstring>
#include <cstdio>
#include "core/audio.h"

static const char* NVS_NAMESPACE = "storage";
static const char* NVS_KEY = "highscores";
static const char* SETTINGS_PATH = "/sdcard/PAKAMAN/settings.cfg";

void persist_load(std::vector<HighScore>& scores) {
    scores.clear();

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        printf("NVS open failed: %s\n", esp_err_to_name(err));
        return;
    }

    size_t required_size = 0;
    err = nvs_get_blob(handle, NVS_KEY, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return; // rien stocké
    }

    std::vector<uint8_t> buffer(required_size);
    err = nvs_get_blob(handle, NVS_KEY, buffer.data(), &required_size);
    if (err == ESP_OK) {
        // décoder le blob en HighScore
        size_t count = required_size / sizeof(HighScore);
        scores.resize(count);
        memcpy(scores.data(), buffer.data(), required_size);
    }

    nvs_close(handle);
}

void persist_save(const std::vector<HighScore>& scores) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        printf("NVS open failed: %s\n", esp_err_to_name(err));
        return;
    }

    size_t size = scores.size() * sizeof(HighScore);
    err = nvs_set_blob(handle, NVS_KEY, scores.data(), size);
    if (err != ESP_OK) {
        printf("NVS set_blob failed: %s\n", esp_err_to_name(err));
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        printf("NVS commit failed: %s\n", esp_err_to_name(err));
    }

    nvs_close(handle);
}


// ------------------------------------------------------------
// Charger les réglages audio depuis la carte SD
// ------------------------------------------------------------
bool audio_settings_load()
{
    FILE* f = fopen(SETTINGS_PATH, "r");
    if (!f)
        return false; // fichier absent → valeurs par défaut

    char key[64];
    char value[64];

    while (fscanf(f, "%63[^=]=%63s\n", key, value) == 2)
    {
        if (strcmp(key, "music_enabled") == 0)
            g_audio_settings.music_enabled = (atoi(value) != 0);

        else if (strcmp(key, "music_volume") == 0)
            g_audio_settings.music_volume = atoi(value);

        else if (strcmp(key, "sfx_volume") == 0)
            g_audio_settings.sfx_volume = atoi(value);

        else if (strcmp(key, "master_volume") == 0)
            g_audio_settings.master_volume = atoi(value);
    }

    fclose(f);
    return true;
}

// ------------------------------------------------------------
// Sauvegarder les réglages audio sur la carte SD
// ------------------------------------------------------------
bool audio_settings_save()
{
    FILE* f = fopen(SETTINGS_PATH, "w");
    if (!f)
        return false;

    fprintf(f, "music_enabled=%d\n", g_audio_settings.music_enabled ? 1 : 0);
    fprintf(f, "music_volume=%d\n",  g_audio_settings.music_volume);
    fprintf(f, "sfx_volume=%d\n",    g_audio_settings.sfx_volume);
    fprintf(f, "master_volume=%d\n", g_audio_settings.master_volume);

    fclose(f);
    return true;
}
