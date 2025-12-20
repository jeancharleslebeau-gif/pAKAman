#pragma once
#include <string>
#include <vector>
#include "ui/highscores.h"

struct HighScore {
    std::string name;
    int score;
};

void persist_load(std::vector<HighScore>& scores);
void persist_save(const std::vector<HighScore>& scores);
