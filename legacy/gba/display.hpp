#pragma once

#include "gba_types.hpp"
#include "gba_regs.hpp"

class Display {
public:
    OBJ_ATTR oam_buffer[128];
    int next_sprite_id;
    int shake_frames;
    int shake_intensity;

    void init();
    void clear_sprites();
    void set_sprite(int id, int x, int y, u16 shape, u16 size, int tile_idx, int pal_bank, bool hflip, bool vflip);
    int alloc_sprite(int x, int y, u16 shape, u16 size, int tile_idx, int pal_bank, bool hflip, bool vflip);
    void hide_remaining_sprites();
    void start_screen_shake(int frames, int intensity);
    void update_screen_shake();
    void copy_oam();
    static void wait_vblank();
};

extern Display g_display;
