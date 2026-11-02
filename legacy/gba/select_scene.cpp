#include "select_scene.hpp"
#include "input.hpp"
#include "display.hpp"
#include "sprites.hpp"
#include "hud.hpp"

SelectScene g_select_scene;

void SelectScene::init() {
    cursor = 0;
    anim_timer = 0;
    confirmed = false;
}

bool SelectScene::update() {
    anim_timer++;
    if (g_input.is_just_pressed(KEY_LEFT) || g_input.is_just_pressed(KEY_RIGHT)) {
        cursor = 1 - cursor;
    }

    if (g_input.is_just_pressed(KEY_A) || g_input.is_just_pressed(KEY_START)) {
        confirmed = true;
        return true;
    }

    // Auto-start battle after 10 seconds if idle
    if (anim_timer >= 600) {
        confirmed = true;
        return true;
    }
    return false;
}

void SelectScene::render() {
    // Title
    g_hud.render_text(44, 16, "CHOOSE YOUR POKEMON", PAL_UI);

    // Left: Pikachu (x=44, y=48)
    int pika_y = 48 + ((cursor == 0 && (anim_timer & 8)) ? -2 : 0);
    g_display.alloc_sprite(44, pika_y, ATTR0_SQUARE, ATTR1_SIZE_32, TILE_PIKACHU_FRONT, PAL_PIKACHU, false, false);
    g_hud.render_text(36, 88, "PIKACHU", PAL_UI);
    g_hud.render_text(40, 100, "SPEED", PAL_UI);

    // Right: Charizard (x=164, y=48)
    int char_y = 48 + ((cursor == 1 && (anim_timer & 8)) ? -2 : 0);
    g_display.alloc_sprite(164, char_y, ATTR0_SQUARE, ATTR1_SIZE_32, TILE_CHARIZARD_FRONT, PAL_CHARIZARD, false, false);
    g_hud.render_text(152, 88, "CHARIZARD", PAL_UI);
    g_hud.render_text(168, 100, "POWER", PAL_UI);

    // Selection Indicator
    if (cursor == 0) {
        g_hud.render_text(36, 116, "> SELECT <", PAL_UI);
    } else {
        g_hud.render_text(152, 116, "> SELECT <", PAL_UI);
    }

    // Bottom Help Text
    g_hud.render_text(48, 142, "PRESS A OR START", PAL_UI);
}
