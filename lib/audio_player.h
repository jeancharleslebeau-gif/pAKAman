#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cstdio>   // pour FILE*

// -------------------------------------------------------------
// Taille locale du buffer WAV pour le streaming
// -------------------------------------------------------------
static constexpr int WAV_BUFFER_SAMPLES = 512;

// -------------------------------------------------------------
// Classe de base pour toutes les pistes audio
// -------------------------------------------------------------
class audio_track_base {
public:
    virtual ~audio_track_base() {}
    virtual int16_t next_sample() = 0;
    virtual bool is_active() const = 0;

    float volume = 1.0f; // volume par piste (0.0 à 1.0)
};

// -------------------------------------------------------------
// Générateur de tonalité (square-like)
// -------------------------------------------------------------
class audio_track_tone : public audio_track_base {
public:
    audio_track_tone();

    void play_tone(int freq_hz, int duration_ms);
    int16_t next_sample() override;
    bool is_active() const override { return remaining_samples > 0; }

private:
    int phase;
    int phase_inc;
    int remaining_samples;
};

// -------------------------------------------------------------
// Générateur de bruit blanc
// -------------------------------------------------------------
class audio_track_noise : public audio_track_base {
public:
    audio_track_noise();

    void play_noise(int duration_ms, int volume = 2000);
    int16_t next_sample() override;
    bool is_active() const override { return remaining_samples > 0; }

private:
    int remaining_samples;
    int volume_int;
};

// -------------------------------------------------------------
// Piste WAV avec streaming + pitch dynamique
// -------------------------------------------------------------
class audio_track_wav : public audio_track_base {
public:
    audio_track_wav();

    void start(FILE* f, uint32_t sample_count);

    int16_t next_sample() override;
    bool is_active() const override { return active; }

    float pitch = 1.0f;         // pitch actuel
    float target_pitch = 1.0f;  // pitch cible (glide)
    float pitch_smooth = 0.15f; // vitesse du glide

private:
    FILE*      file;
    uint32_t   samples_remaining;
    int16_t    buffer[WAV_BUFFER_SAMPLES];
    uint32_t   buffer_len;
    float      pos;             // position flottante dans le buffer
    bool       active;

    void fill_buffer();
};

// -------------------------------------------------------------
// Mixeur audio global
// -------------------------------------------------------------
class audio_player {
public:
    audio_player();

    void add_track(audio_track_base* t);
    int mix(int16_t* out, int count);

    float master_volume = 1.0f; // volume global

private:
    static const int MAX_TRACKS = 8;
    audio_track_base* tracks[MAX_TRACKS];
};
