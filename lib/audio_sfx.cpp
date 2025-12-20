#include "audio_sfx.h"
#include "audio_player.h"

// These are defined in core/audio.cpp
extern audio_track_tone  g_track_tone;
extern audio_track_noise g_track_noise;

// -------------------------------------------------------------
// Simple UI sound effects
// -------------------------------------------------------------

void audio_sfx_click()     { g_track_tone.volume = 0.6f; g_track_tone.play_tone(1200, 20); }
void audio_sfx_validate()  { g_track_tone.volume = 0.7f; g_track_tone.play_tone(1500, 40); }
void audio_sfx_cancel()    { g_track_tone.volume = 0.5f; g_track_tone.play_tone(600, 40); }
void audio_sfx_error()     { g_track_tone.volume = 0.5f; g_track_tone.play_tone(300, 60); }
void audio_sfx_pop()       { g_track_noise.volume = 0.6f; g_track_noise.play_noise(30, 4000); }
void audio_sfx_whoosh()    { g_track_noise.volume = 0.7f; g_track_noise.play_noise(80, 2000); }
