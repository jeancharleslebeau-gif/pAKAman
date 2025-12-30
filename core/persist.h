#pragma once
#include <string>
#include <vector>
#include "ui/highscores.h"

struct HighScore {
    std::string name;
    int score;
};

// Scores - sauvegardé dans le système MVS de l'ESP32
void persist_load(std::vector<HighScore>& scores);
void persist_save(const std::vector<HighScore>& scores);

// Préférences - configuration sonore stockées sur la carte SD
bool audio_settings_load(); 
bool audio_settings_save();
