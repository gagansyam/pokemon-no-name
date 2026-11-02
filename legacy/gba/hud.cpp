#include "hud.hpp"
#include "display.hpp"
#include "sprites.hpp"

HUD g_hud;

void HUD::init() {
    match_intro_timer = 90; // 1.5 seconds intro
}

void HUD::update() {
    if (match_intro_timer > 0) match_intro_timer--;
}

void HUD::render_text(int x, int y, const char* str, int pal_bank) {
    if (!str) return;
    int cur_x = x;
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] != ' ') {
            int t = get_font_tile(str[i]);
            g_display.alloc_sprite(cur_x, y, ATTR0_SQUARE, ATTR1_SIZE_8, t, pal_bank, false, false);
        }
        cur_x += 8;
    }
}

void HUD::render_bar(int x, int y, int current, int max, int width_tiles, int fill_tile, int empty_tile, int pal_bank) {
    if (max <= 0) return;
    int filled_tiles = (current * width_tiles + (max / 2)) / max;
    if (filled_tiles > width_tiles) filled_tiles = width_tiles;
    if (filled_tiles < 0) filled_tiles = 0;

    for (int i = 0; i < width_tiles; ++i) {
        int tile = (i < filled_tiles) ? fill_tile : empty_tile;
        g_display.alloc_sprite(x + i * 8, y, ATTR0_SQUARE, ATTR1_SIZE_8, tile, pal_bank, false, false);
    }
}

void HUD::render(const Fighter& player, const Fighter& cpu) {
    // 1. Player HUD (Top-Left)
    const char* p_name = (player.type == PKMN_PIKACHU) ? "PIKA" : "CHAR";
    render_text(16, 4, p_name, PAL_UI);

    int p_hp_tile = (player.hp <= player.max_hp / 4) ? TILE_BAR_RED : TILE_BAR_GREEN;
    render_bar(52, 4, player.hp, player.max_hp, 6, p_hp_tile, TILE_BAR_EMPTY, PAL_UI);
    // Stamina bar underneath
    render_bar(52, 13, player.stamina, player.max_stamina, 5, TILE_BAR_YELLOW, TILE_BAR_EMPTY, PAL_UI);

    // 2. CPU HUD (Top-Right)
    const char* c_name = (cpu.type == PKMN_PIKACHU) ? "CPU:PIKA" : "CPU:CHAR";
    render_text(136, 4, c_name, PAL_UI);

    int c_hp_tile = (cpu.hp <= cpu.max_hp / 4) ? TILE_BAR_RED : TILE_BAR_GREEN;
    render_bar(176, 13, cpu.hp, cpu.max_hp, 6, c_hp_tile, TILE_BAR_EMPTY, PAL_UI);

    // 3. Move Buttons & Status (Bottom)
    // A Button
    g_display.alloc_sprite(16, 148, ATTR0_SQUARE, ATTR1_SIZE_8, TILE_BTN_A, PAL_UI, false, false);
    render_text(26, 148, (player.type == PKMN_PIKACHU) ? "QA" : "SCR", (player.cooldowns[0] == 0) ? PAL_UI : PAL_CHARIZARD);

    // B Button
    g_display.alloc_sprite(68, 148, ATTR0_SQUARE, ATTR1_SIZE_8, TILE_BTN_B, PAL_UI, false, false);
    render_text(78, 148, (player.type == PKMN_PIKACHU) ? "TBOLT" : "FLAME", (player.cooldowns[1] == 0) ? PAL_UI : PAL_CHARIZARD);

    // L Button
    g_display.alloc_sprite(128, 148, ATTR0_SQUARE, ATTR1_SIZE_8, TILE_BTN_L, PAL_UI, false, false);
    render_text(138, 148, (player.type == PKMN_PIKACHU) ? "EBALL" : "FSPIN", (player.cooldowns[2] == 0) ? PAL_UI : PAL_CHARIZARD);

    // R Button
    g_display.alloc_sprite(188, 148, ATTR0_SQUARE, ATTR1_SIZE_8, TILE_BTN_R, PAL_UI, false, false);
    render_text(198, 148, (player.type == PKMN_PIKACHU) ? "ITAIL" : "WING", (player.cooldowns[3] == 0) ? PAL_UI : PAL_CHARIZARD);

    // 4. Match State Announcements
    if (match_intro_timer > 30) {
        render_text(100, 72, "READY!", PAL_UI);
    } else if (match_intro_timer > 0) {
        render_text(96, 72, "FIGHT!", PAL_UI);
    }

    if (player.state == STATE_FAINTED) {
        render_text(104, 64, "K.O.!", PAL_UI);
        render_text(84, 80, "CPU WINS!", PAL_UI);
        render_text(68, 96, "START: RETRY", PAL_UI);
    } else if (cpu.state == STATE_FAINTED) {
        render_text(104, 64, "K.O.!", PAL_UI);
        render_text(72, 80, "PLAYER WINS!", PAL_UI);
        render_text(68, 96, "START: RETRY", PAL_UI);
    }
}
