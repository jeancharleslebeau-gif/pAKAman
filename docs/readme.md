```markdown
# 🎮 pAKAman — Librairie Audio

Ce dossier contient le moteur audio complet utilisé par pAKAman sur ESP32‑S3.

✅ Streaming WAV non bloquant  
✅ Mixage multi‑pistes  
✅ Pitch dynamique  
✅ SFX synthétiques (tone + noise)  
✅ Volume global + volume par piste  
✅ Soft‑clipping  
✅ Compatible TAS2505

---

# 📁 Structure

core/audio.cpp        → pipeline audio + FIFO + I2S + TAS2505
core/audio.h          → constantes + API publique
lib/audio_player.h    → mixeur + pistes audio
lib/audio_player.cpp
lib/audio_sfx.h       → API SFX synthétiques
lib/audio_sfx.cpp


# 🚀 Utilisation rapide

## Initialisation
```cpp
audio_init();
Mise à jour (à chaque frame)
```

```cpp
audio_update();
Jouer un WAV
```

```cpp
audio_play_wav("/sdcard/PAKAMAN/Sons/PACGOMME.wav");
SFX synthétiques
```

```cpp
audio_sfx_click();
audio_sfx_validate();
audio_sfx_pop();
Pitch dynamique
```

```cpp
g_track_wav.target_pitch = 1.3f;
Volume global
```

```cpp
s_audio_player.master_volume = 0.7f;
```

## Documentation complète

Voir : docs/audio.md


## Dépendances

ESP-IDF
Driver I2S
Codec TAS2505
SDCard (pour les WAV)


## Licence

Libre d’utilisation dans vos projets personnels ou open‑source.