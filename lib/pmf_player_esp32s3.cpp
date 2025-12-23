#include "pmf_player.h"

// Ici, tu décides si tu veux mono ou stéréo.
// Pour pAKAman, probablement mono 16 bits suffit.
static const bool kStereoOutput = false;

//---------------------------------------------------------------------------
// plateforme: fréquence
//---------------------------------------------------------------------------

uint32_t pmf_player::get_sampling_freq(uint32_t sampling_freq_) const
{
  // Tu peux éventuellement forcer à 44100 / 32000 / 22050
  // selon ta config I2S. Si ton moteur tourne déjà à sample_rate fixe,
  // tu peux simplement retourner sampling_freq_.
  return sampling_freq_;
}

//---------------------------------------------------------------------------
// plateforme: démarrage / arrêt (ne font rien ici)
//---------------------------------------------------------------------------

void pmf_player::start_playback(uint32_t sampling_freq_)
{
  // Sur ESP32S3, c'est ton moteur audio qui contrôle l'I2S.
  // On ne fait rien ici.
  (void)sampling_freq_;
}

void pmf_player::stop_playback()
{
  // Rien à faire non plus.
}

//---------------------------------------------------------------------------
// plateforme: buffer audio (non utilisé dans notre intégration)
//---------------------------------------------------------------------------

pmf_mixer_buffer pmf_player::get_mixer_buffer()
{
  // On n'utilise pas update()+get_mixer_buffer dans ce backend,
  // car on passe par pmf_player::mix().
  pmf_mixer_buffer buf = { 0, 0 };
  return buf;
}

//---------------------------------------------------------------------------
// plateforme: mix_buffer → appelle mix_buffer_impl avec nos paramètres
//---------------------------------------------------------------------------

void pmf_player::mix_buffer(pmf_mixer_buffer &buf_, unsigned num_samples_)
{
  if (kStereoOutput)
  {
    // stéréo 16 bits
    mix_buffer_impl<int16_t, true, 16>(buf_, num_samples_);
  }
  else
  {
    // mono 16 bits
    mix_buffer_impl<int16_t, false, 16>(buf_, num_samples_);
  }
}
