#pragma once
#include <stdint.h>
#include "audio_player.h"

struct SFXInstance {
    const int16_t* data = nullptr;
    uint32_t length = 0;

    float pos = 0.0f;      // position flottante pour pitch
    float pitch = 1.0f;    // 1.0 = normal

    float volume = 1.0f;   // 0.0–1.0 relatif à track.volume
    int priority = 0;
    bool active = false;
};

class audio_track_sfx : public audio_track_base {
public:
    audio_track_sfx();

    // Lance un SFX déjà en RAM
    void play(const int16_t* data,
              uint32_t length,
              int priority,
              float volume,
              float pitch);

    // audio_track_base
    int16_t next_sample() override;
    bool is_active() const override;

    // Pour l’auto-ducking
    bool no_high_priority_active() const;

private:
    static const int MAX_SFX = 8;
    SFXInstance sfx[MAX_SFX];
};
