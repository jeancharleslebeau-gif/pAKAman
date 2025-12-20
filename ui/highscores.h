#pragma once
#include <vector>
#include <string>
#include <cstdint>   // pour int32_t
#include "core/persist.h"

struct HighscoreEntry {
    char name[9];       // 8 caractères + '\0'
    int32_t score;      // taille garantie 32 bits
};

constexpr int MAX_SCORES = 6;

void highscores_init();
std::vector<HighscoreEntry> highscores_load();
std::string highscores_input_name();
void highscores_submit(int32_t score);
void highscores_show();