#pragma once
#include <cstdint>   
#include "pmf_player.h"

class AudioPMF {
public:
    void init(const uint8_t* data);
    void start(uint32_t sample_rate, uint16_t playlist_pos = 0);
    void stop();
    void pause(bool p);
    void setVolume(uint8_t vol);
    void render(int16_t* out, int samples);
	
	bool isPlaying() const { return player.is_playing(); }

	
private:
    pmf_player player;
    bool paused = false;
    uint8_t volume = 255;
};
