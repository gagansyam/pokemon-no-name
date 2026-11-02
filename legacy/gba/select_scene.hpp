#pragma once

#include "fighter.hpp"

class SelectScene {
public:
    int cursor; // 0 = Pikachu, 1 = Charizard
    int anim_timer;
    bool confirmed;

    void init();
    bool update(); // Returns true when selection confirmed
    void render();
    PokemonType get_selected() const {
        return (cursor == 0) ? PKMN_PIKACHU : PKMN_CHARIZARD;
    }
};

extern SelectScene g_select_scene;
