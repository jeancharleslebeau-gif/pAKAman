#include "audio_track_sfx.h"
#include <algorithm>

audio_track_sfx::audio_track_sfx()
{
    for (int i = 0; i < MAX_SFX; i++) {
        sfx[i].active = false;
        sfx[i].data = nullptr;
        sfx[i].length = 0;
        sfx[i].pos = 0.0f;
        sfx[i].pitch = 1.0f;
        sfx[i].volume = 1.0f;
        sfx[i].priority = 0;
    }
}


void audio_track_sfx::play(const int16_t* data,
                           uint32_t length,
                           int priority,
                           float volume,
                           float pitch)
{
    int best = -1;
	if (data == nullptr || length == 0)
		return;

    for (int i = 0; i < MAX_SFX; i++) {
        if (!sfx[i].active) {
            best = i;
            break;
        }
        if (priority > sfx[i].priority)
            best = i;
    }

    if (best < 0) return;

    sfx[best].data = data;
    sfx[best].length = length;
    sfx[best].pos = 0.0f;
    sfx[best].pitch = pitch;
    sfx[best].volume = volume;
    sfx[best].priority = priority;
    sfx[best].active = true;
}

bool audio_track_sfx::no_high_priority_active() const
{
    for (int i = 0; i < MAX_SFX; i++)
        if (sfx[i].active && sfx[i].priority >= 60)
            return false;
    return true;
}

bool audio_track_sfx::is_active() const
{
    for (int i = 0; i < MAX_SFX; i++)
        if (sfx[i].active)
            return true;
    return false;
}

int16_t audio_track_sfx::next_sample()
{
    int32_t mix = 0;

    for (int c = 0; c < MAX_SFX; c++) {

        // Sécurité absolue
        if (!sfx[c].active || sfx[c].data == nullptr)
            continue;

        int idx = (int)sfx[c].pos;

        // Sécurité hors limites
        if (idx < 0 || idx >= (int)sfx[c].length) {
            sfx[c].active = false;
            continue;
        }

        int32_t s = sfx[c].data[idx];
        s = (int32_t)(s * sfx[c].volume);
        mix += s;

        sfx[c].pos += sfx[c].pitch;
    }

    // Volume de la piste
    mix = (int32_t)(mix * volume);

    if (mix > 32767) mix = 32767;
    if (mix < -32768) mix = -32768;

    return (int16_t)mix;
}

