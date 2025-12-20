# Librairie Audio pAKAman  
Moteur audio complet pour ESP32‑S3 + TAS2505  
Streaming WAV non bloquant • Mixage multi‑pistes • Pitch dynamique • SFX synthétiques

---

# 1. Architecture générale

La librairie audio repose sur quatre couches :

## ✅ 1.1. Matériel (I2S + TAS2505)
- L’ESP32S3 envoie des échantillons PCM 16 bits mono via I2S.
- Le codec TAS2505 convertit le flux numérique en audio analogique.
- Le DMA appelle un callback I2S pour demander de nouveaux échantillons.

## ✅ 1.2. FIFO audio
- Tampon circulaire d’échantillons 16 bits mono.
- Le callback I2S lit dans la FIFO.
- Le jeu écrit dans la FIFO via `audio_update()`.

## ✅ 1.3. Mixeur logiciel (`audio_player`)
- Mélange plusieurs pistes audio en un seul buffer PCM.
- Applique le volume global.
- Applique un soft‑clipping pour éviter la saturation.

## ✅ 1.4. Pistes audio (tracks)
Chaque piste est une source sonore indépendante :

| Piste 	      | Rôle				| Exemple 		|
|---------------------|---------------------------------|-----------------------|
| `audio_track_tone`  | tonalités UI 		 	| clic, validation 	|
| `audio_track_noise` | bruit blanc 		 	| pop, whoosh 		|
| `audio_track_wav`   | streaming WAV non bloquant	| waka‑waka, power‑up	|

---

# 2. Les pistes audio

Toutes les pistes héritent de :

```cpp
class audio_track_base {
public:
    virtual int16_t next_sample() = 0;
    virtual bool is_active() const = 0;
    float volume = 1.0f;
};
```

## ✅ 2.1. Piste Tone

Onde carrée
Idéal pour les sons d’interface

```cpp
g_track_tone.play_tone(1200, 40);
g_track_tone.volume = 0.8f;
```

## ✅ 2.2. Piste Noise

Bruit blanc
Parfait pour les pops, impacts, whoosh

```cpp
g_track_noise.play_noise(30, 4000);
g_track_noise.volume = 0.6f;
```

## ✅ 2.3. Piste WAV (streaming + pitch)

Lit un fichier WAV sans bloquer
Stocke un petit buffer interne (512 samples)
Supporte le pitch dynamique avec interpolation linéaire

```cpp
audio_play_wav("/sdcard/PAKAMAN/Sons/PACGOMME.wav");
g_track_wav.pitch = 1.2f;
g_track_wav.target_pitch = 0.8f;
```

# 3. Le mixeur (audio_player)

Le mixeur :
interroge chaque piste
additionne les échantillons
applique le volume global
applique un soft‑clip
renvoie un buffer PCM prêt à envoyer dans la FIFO

```cpp
s_audio_player.master_volume = 0.7f;
s_audio_player.add_track(&g_track_tone);
s_audio_player.add_track(&g_track_noise);
s_audio_player.add_track(&g_track_wav);
```

# 4. API publique

## ✅ 4.1. Jouer un WAV

```cpp
audio_play_wav("/sdcard/PAKAMAN/Sons/PACGOMME.wav");
```

## ✅ 4.2. Jouer un SFX synthétique

```cpp
audio_sfx_click();
audio_sfx_validate();
audio_sfx_cancel();
audio_sfx_pop();
audio_sfx_whoosh();
```

## ✅ 4.3. Pitch dynamique
```cpp
g_track_wav.pitch = 1.3f;        // instantané
g_track_wav.target_pitch = 0.8f; // glide
```

## ✅ 4.4. Volume global

```cpp
s_audio_player.master_volume = 0.5f;
```

## ✅ 4.5. Mise à jour audio (à appeler à chaque frame)
```cpp
audio_update();
```

# 5. Exemple complet

Voici un exemple minimal d’utilisation dans un jeu :

```cpp
#include "core/audio.h"
#include "lib/audio_sfx.h"

void game_init()
{
    audio_init();
}

void game_update(const Keys& k)
{
    // Sons de menu
    if (k.left || k.right)
        audio_sfx_click();

    if (k.A)
        audio_sfx_validate();

    // Jouer un WAV
    if (k.B)
        audio_play_wav("/sdcard/PAKAMAN/Sons/PACGOMME.wav");

    // Pitch dynamique (ex: speed-up)
    if (k.speed)
        g_track_wav.target_pitch = 1.4f;
    else
        g_track_wav.target_pitch = 1.0f;

    // Mise à jour audio
    audio_update();
}
```

# 6. Bonnes pratiques

✅ Appeler audio_update() à chaque frame
✅ Utiliser audio_play_wav() (jamais audio_push_buffer() directement)
✅ Garder les WAV en mono 16 bits 44100 Hz
✅ Ne pas dépasser volume > 1.0f  
✅ Utiliser le pitch pour varier les sons sans multiplier les fichiers
✅ Utiliser les SFX synthétiques pour les sons courts (UI, pops, impacts)

# 7. Limitations actuelles

Pas encore de musique de fond multi‑pistes
Pas encore d’ADSR pour les SFX
Pas encore de filtres (low‑pass, high‑pass)

# 8. Roadmap future

ADSR pour les SFX
Filtre passe‑bas pour les ghosts
Piste musique dédiée
Spatialisation simple (pan L/R)
Compression dynamique (limiter)

# 9. Licence
Librairie audio développée pour pAKAman (ESP32‑S3).
Réutilisable dans tout projet personnel ou open‑source.


# 10 - 🛠️ Dépendances

ESP-IDF
Driver I2S
Codec TAS2505
SDCard (pour les WAV)

