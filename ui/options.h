#pragma once
#include "core/input.h"
#include "game/game.h"

extern int nav_cooldown;


void draw_options_menu(GameState::State state, int index);
void handle_audio_options_navigation(const Keys& k, int& index);
