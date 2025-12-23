#include "audio_pmf.h"
#include <string.h>

// ---------------------------------------
/* Utilisation:
// ---------------------------------------

* Pour lancer la musique :
  audioPMF.init(maze_rush_pmf);
  audioPMF.start(GB_AUDIO_SAMPLE_RATE);

* Pour changer de musique :
  audioPMF.stop();
  audioPMF.init(new_music_pmf);
  audioPMF.start(GB_AUDIO_SAMPLE_RATE);

* Pour pause :
  audioPMF.pause(true);

* Pour reprise :
  audioPMF.pause(false);

* Pour volume :
  audioPMF.setVolume(128);

-------------------------------------*/  


void AudioPMF::init(const uint8_t* data)
{
    player.load(data);
}

void AudioPMF::start(uint32_t sample_rate, uint16_t playlist_pos)
{
    player.start(sample_rate, playlist_pos);
    paused = false;
}

void AudioPMF::stop()
{
    player.stop();
}

void AudioPMF::pause(bool p)
{
    paused = p;
}

void AudioPMF::setVolume(uint8_t vol)
{
    volume = vol;
}

void AudioPMF::render(int16_t* out, int samples)
{
    if (paused || volume == 0)
        return;

    static int16_t temp[512];
    if (samples > 512)
        samples = 512;

    memset(temp, 0, samples * sizeof(int16_t));

    // mélange PMF → temp
    player.mix(temp, samples);

    // mixage temp → out
    for (int i = 0; i < samples; i++)
    {
        int32_t s = out[i] + ((temp[i] * volume) >> 8);
        if (s < -32768) s = -32768;
        if (s >  32767) s =  32767;
        out[i] = int16_t(s);
    }
}