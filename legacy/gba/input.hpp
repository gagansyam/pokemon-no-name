#pragma once

#include "gba_types.hpp"
#include "gba_regs.hpp"

class Input {
public:
    u16 current;
    u16 previous;

    // Double tap tracking for directions: UP, DOWN, LEFT, RIGHT
    u8 tap_timer[4]; // 0: Up, 1: Down, 2: Left, 3: Right
    bool double_tapped[4];

    void init();
    void update();

    bool is_down(u16 key) const {
        return (current & key) != 0;
    }

    bool is_just_pressed(u16 key) const {
        return (current & key) && !(previous & key);
    }

    bool is_just_released(u16 key) const {
        return !(current & key) && (previous & key);
    }

    bool has_double_tapped(int dir) const {
        return double_tapped[dir];
    }
};

extern Input g_input;
