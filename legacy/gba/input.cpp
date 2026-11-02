#include "input.hpp"

Input g_input;

static const u16 DIR_KEYS[4] = { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT };

void Input::init() {
    current = 0;
    previous = 0;
    for (int i = 0; i < 4; ++i) {
        tap_timer[i] = 0;
        double_tapped[i] = false;
    }
}

void Input::update() {
    previous = current;
    // REG_KEYINPUT is active low, invert and mask 10 keys
    current = (~REG_KEYINPUT) & KEY_MASK;

    for (int i = 0; i < 4; ++i) {
        double_tapped[i] = false;
        u16 k = DIR_KEYS[i];

        if (is_just_pressed(k)) {
            if (tap_timer[i] > 0) {
                double_tapped[i] = true;
                tap_timer[i] = 0;
            } else {
                tap_timer[i] = 15; // 15 frames = 0.25 seconds window
            }
        }

        if (tap_timer[i] > 0 && !is_down(k)) {
            tap_timer[i]--;
        }
    }
}
