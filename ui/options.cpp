#include "options.h"
#include "core/graphics.h"
#include "core/audio.h"
#include "lib/audio_sfx.h"
#include "game/config.h"

extern AudioSettings g_audio_settings;   // déjà global dans ton moteur
int nav_cooldown = 0;

void handle_audio_options_navigation(const Keys& k, int& index)
{
    if (nav_cooldown > 0)
        nav_cooldown--;

    if (nav_cooldown == 0)
    {
        if (k.up) {
            index--;
            if (index < 0) index = 5;
            audio_sfx_click();
            nav_cooldown = 8;
        }

        if (k.down) {
            index++;
            if (index > 5) index = 0;
            audio_sfx_click();
            nav_cooldown = 8;
        }

        if (k.left) {
            audio_sfx_click();
            if (index == 0) g_audio_settings.music_enabled = !g_audio_settings.music_enabled;
            if (index == 1 && g_audio_settings.music_volume > 0) g_audio_settings.music_volume -= 8;
            if (index == 2 && g_audio_settings.sfx_volume > 0)   g_audio_settings.sfx_volume   -= 8;
            if (index == 3 && g_audio_settings.master_volume > 0) g_audio_settings.master_volume -= 8;
            nav_cooldown = 8;
        }

        if (k.right) {
            audio_sfx_click();
            if (index == 0) g_audio_settings.music_enabled = !g_audio_settings.music_enabled;
            if (index == 1 && g_audio_settings.music_volume < 255) g_audio_settings.music_volume += 8;
            if (index == 2 && g_audio_settings.sfx_volume   < 255) g_audio_settings.sfx_volume   += 8;
            if (index == 3 && g_audio_settings.master_volume < 255) g_audio_settings.master_volume += 8;
            nav_cooldown = 8;
        }
    }
}

void draw_options_menu(GameState::State state, int index)
{
    auto draw_item = [&](int y, const char* text, bool selected) {
        uint16_t color = selected ? COLOR_YELLOW : COLOR_WHITE;
        gfx_text(20, y, text, color);
    };

    gfx_text(20, 20, "OPTIONS", COLOR_WHITE);

    char buf[64];

    draw_item(60, g_audio_settings.music_enabled ? "Music: ON" : "Music: OFF", index == 0);

    snprintf(buf, sizeof(buf), "Music Volume: %d", g_audio_settings.music_volume);
    draw_item(90, buf, index == 1);

    snprintf(buf, sizeof(buf), "SFX Volume: %d", g_audio_settings.sfx_volume);
    draw_item(120, buf, index == 2);

    snprintf(buf, sizeof(buf), "Master Volume: %d", g_audio_settings.master_volume);
    draw_item(150, buf, index == 3);

    if (state == GameState::State::OptionsMenu)
        draw_item(180, "Quitter la partie", index == 4);
    else
        draw_item(180, "Highscores", index == 4);

    draw_item(210, "Retour", index == 5);
}
