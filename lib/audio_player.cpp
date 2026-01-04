#include "audio_player.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "core/audio.h"  // pour GB_AUDIO_SAMPLE_RATE

static constexpr int SAMPLE_RATE = GB_AUDIO_SAMPLE_RATE;

// -------------------------------------------------------------
// Tone track
// -------------------------------------------------------------
audio_track_tone::audio_track_tone()
{
    phase = 0;
    phase_inc = 0;
    remaining_samples = 0;
}

void audio_track_tone::play_tone(int freq_hz, int duration_ms)
{
    phase = 0;
    phase_inc = (int)(freq_hz * 65536.0f / SAMPLE_RATE);
    remaining_samples = (duration_ms * SAMPLE_RATE) / 1000;
}

int16_t audio_track_tone::next_sample()
{
    if (remaining_samples <= 0)
        return 0;

    remaining_samples--;

    phase += phase_inc;
    int16_t s = (int16_t)((phase >> 8) & 0xFF);
    s = (s - 128) * 256;

    return (int16_t)(s * volume);
}

// -------------------------------------------------------------
// Noise track
// -------------------------------------------------------------
audio_track_noise::audio_track_noise()
{
    remaining_samples = 0;
    volume_int = 2000;
}

void audio_track_noise::play_noise(int duration_ms, int vol)
{
    remaining_samples = (duration_ms * SAMPLE_RATE) / 1000;
    volume_int = vol;
}

int16_t audio_track_noise::next_sample()
{
    if (remaining_samples <= 0)
        return 0;

    remaining_samples--;

    int r = (rand() & 0xFFFF) - 32768;
    float s = r * (volume_int / 32768.0f);

    return (int16_t)(s * volume);
}

// -------------------------------------------------------------
// WAV track
// -------------------------------------------------------------
audio_track_wav::audio_track_wav()
{
    file = nullptr;
    samples_remaining = 0;
    buffer_len = 0;
    pos = 0.0f;
    active = false;
}

void audio_track_wav::start(FILE* f, uint32_t sample_count)
{
    if (file) {
        fclose(file);
        file = nullptr;
    }

    file = f;
    samples_remaining = sample_count;
    buffer_len = 0;
    pos = 0.0f;
    active = (file != nullptr && samples_remaining > 0);
}

void audio_track_wav::fill_buffer()
{
    if (!file || samples_remaining == 0) {
        buffer_len = 0;
        pos = 0.0f;
        active = false;
        return;
    }

    uint32_t to_read = WAV_BUFFER_SAMPLES;
    if (samples_remaining < to_read)
        to_read = samples_remaining;

    size_t n = fread(buffer, sizeof(int16_t), to_read, file);
    buffer_len = (uint32_t)n;
    pos = 0.0f;

    if (n == 0) {
        fclose(file);
        file = nullptr;
        samples_remaining = 0;
        active = false;
        return;
    }

    samples_remaining -= buffer_len;

    if (samples_remaining == 0) {
        fclose(file);
        file = nullptr;
    }

    if (buffer_len == 0)
        active = false;
}

int16_t audio_track_wav::next_sample()
{
    if (!active)
        return 0;

    // Pitch glide
    pitch += (target_pitch - pitch) * pitch_smooth;

    // Refill si nécessaire
    if (pos >= buffer_len) {
        fill_buffer();
        if (!active || buffer_len == 0)
            return 0;
    }

    // Interpolation linéaire
    int i0 = (int)pos;
    int i1 = i0 + 1;
    if (i1 >= buffer_len) i1 = buffer_len - 1;

    float frac = pos - i0;
    float s = buffer[i0] * (1.0f - frac) + buffer[i1] * frac;

    pos += pitch;

    return (int16_t)(s * volume);
}

// -------------------------------------------------------------
// Audio player (mixing engine)
// -------------------------------------------------------------
audio_player::audio_player()
{
    for (int i = 0; i < MAX_TRACKS; i++)
        tracks[i] = nullptr;
}

void audio_player::add_track(audio_track_base* t)
{
    for (int i = 0; i < MAX_TRACKS; i++) {
        if (tracks[i] == nullptr) {
            tracks[i] = t;
            return;
        }
    }
}

int audio_player::mix(int16_t* out, int count)
{
    for (int i = 0; i < count; i++) {
        int mix = 0;

        for (int t = 0; t < MAX_TRACKS; t++) {
            if (tracks[t] && tracks[t]->is_active())
                mix += tracks[t]->next_sample();
        }

        // Volume global
        mix = (int)(mix * master_volume);

        // Soft clipping
        if (mix > 32767) mix = 32767 - (mix - 32767) / 4;
        if (mix < -32768) mix = -32768 - (mix + 32768) / 4;

        out[i] = (int16_t)mix;
    }

    return count;
}
