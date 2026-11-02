#include "display.hpp"

Display g_display;

void Display::init() {
    next_sprite_id = 0;
    shake_frames = 0;
    shake_intensity = 0;
    clear_sprites();
    copy_oam();
}

void Display::clear_sprites() {
    next_sprite_id = 0;
    for (int i = 0; i < 128; ++i) {
        oam_buffer[i].attr0 = ATTR0_HIDE;
        oam_buffer[i].attr1 = 0;
        oam_buffer[i].attr2 = 0;
        oam_buffer[i].fill = 0;
    }
}

void Display::set_sprite(int id, int x, int y, u16 shape, u16 size, int tile_idx, int pal_bank, bool hflip, bool vflip) {
    if (id < 0 || id >= 128) return;

    u16 a0 = (y & ATTR0_Y_MASK) | shape | ATTR0_4BPP | ATTR0_REGULAR;
    u16 a1 = (x & ATTR1_X_MASK) | size;
    if (hflip) a1 |= ATTR1_HFLIP;
    if (vflip) a1 |= ATTR1_VFLIP;

    u16 a2 = (tile_idx & ATTR2_TILE_MASK) | ATTR2_PALETTE(pal_bank) | ATTR2_PRIO(0);

    oam_buffer[id].attr0 = a0;
    oam_buffer[id].attr1 = a1;
    oam_buffer[id].attr2 = a2;
}

int Display::alloc_sprite(int x, int y, u16 shape, u16 size, int tile_idx, int pal_bank, bool hflip, bool vflip) {
    if (next_sprite_id >= 128) return -1;
    int id = next_sprite_id++;
    set_sprite(id, x, y, shape, size, tile_idx, pal_bank, hflip, vflip);
    return id;
}

void Display::hide_remaining_sprites() {
    for (int i = next_sprite_id; i < 128; ++i) {
        oam_buffer[i].attr0 = ATTR0_HIDE;
    }
    next_sprite_id = 0;
}

void Display::start_screen_shake(int frames, int intensity) {
    shake_frames = frames;
    shake_intensity = intensity;
}

void Display::update_screen_shake() {
    if (shake_frames > 0) {
        shake_frames--;
        int ox = (shake_frames & 1) ? shake_intensity : -shake_intensity;
        int oy = (shake_frames & 2) ? -shake_intensity : shake_intensity;
        REG_BG0HOFS = (u16)ox;
        REG_BG0VOFS = (u16)oy;
    } else {
        REG_BG0HOFS = 0;
        REG_BG0VOFS = 0;
    }
}

void Display::wait_vblank() {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

void Display::copy_oam() {
    volatile u32* src = (volatile u32*)oam_buffer;
    volatile u32* dst = (volatile u32*)0x07000000;
    for (int i = 0; i < 256; ++i) {
        dst[i] = src[i];
    }
}
