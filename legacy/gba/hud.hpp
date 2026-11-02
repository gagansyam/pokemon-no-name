#pragma once

#include "fighter.hpp"

class HUD {
public:
    int match_intro_timer;

    void init();
    void update();
    void render(const Fighter& player, const Fighter& cpu);
    void render_text(int x, int y, const char* str, int pal_bank);
    void render_bar(int x, int y, int current, int max, int width_tiles, int fill_tile, int empty_tile, int pal_bank);
};

extern HUD g_hud;
