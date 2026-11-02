#include "ui.hpp"
#include "font.hpp"
#include "types.hpp"
#include "moves.hpp"
#include "arena.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>

UI g_ui;

static void fill_circle(SDL_Renderer* ren, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; ++dy) {
        int dx = (int)sqrtf((float)(radius * radius - dy * dy));
        SDL_RenderDrawLine(ren, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

// Authentic PMD Sprite Character Portrait Card
static void draw_fighter_portrait(SDL_Renderer* ren, const Fighter& fighter, int x, int y, int size, bool is_p2, float match_timer) {
    // 1. Outer Border & Frame tinted to Primary Element Color
    ElementType t1 = TYPE_NORMAL, t2 = TYPE_NONE;
    TypeChart::get_species_types(fighter.type, t1, t2);
    Uint8 tr = 148, tg = 163, tb = 184;
    TypeChart::get_type_color(fighter.type1, tr, tg, tb);

    // Dark Card Base
    SDL_SetRenderDrawColor(ren, 15, 23, 42, 255);
    SDL_Rect card_bg = { x, y, size, size };
    SDL_RenderFillRect(ren, &card_bg);

    // Subtle inner type tint
    SDL_SetRenderDrawColor(ren, tr / 4, tg / 4, tb / 4, 255);
    SDL_Rect inner_tint = { x + 3, y + 3, size - 6, size - 6 };
    SDL_RenderFillRect(ren, &inner_tint);

    // Set clipping rectangle to keep sprite strictly inside card
    SDL_RenderSetClipRect(ren, &inner_tint);

    int cx = x + size / 2;
    int cy = y + size / 2 + 2;

    if (fighter.walk_sheet.texture != nullptr) {
        // Draw the real high-res PMD sprite!
        // P1 faces towards center/opponent (dir 2); P2 faces towards center/player (dir 6)
        int pmd_dir = is_p2 ? (fighter.type == PKMN_CHARIZARD ? 6 : 6)
                            : (fighter.type == PKMN_CHARIZARD ? 2 : 2);
        int anim_frame = ((int)(match_timer * 4.0f)) % 4;

        float scale = 1.40f;
        if (fighter.type == PKMN_PIKACHU) scale = 1.65f;
        else if (fighter.type == PKMN_CHARIZARD) scale = 1.25f;
        else if (fighter.type == PKMN_LUCARIO) scale = 1.35f;
        else if (fighter.type == PKMN_GRENINJA) scale = 1.35f;

        // Subtle idle ground shadow inside card
        SDL_SetRenderDrawColor(ren, 10, 15, 26, 120);
        int s_rx = (int)(fighter.radius * 0.5f);
        int s_ry = 4;
        for (int dy = -s_ry; dy <= s_ry; ++dy) {
            float row_pct = (float)(dy * dy) / (float)(s_ry * s_ry);
            if (row_pct > 1.0f) continue;
            int span_w = (int)((float)s_rx * sqrtf(1.0f - row_pct));
            SDL_RenderDrawLine(ren, cx - span_w, cy + 18 + dy, cx + span_w, cy + 18 + dy);
        }

        fighter.walk_sheet.draw(ren, anim_frame, pmd_dir, cx, cy, scale, 255);
    } else {
        // Clean fallback: Elegant monogram badge
        SDL_SetRenderDrawColor(ren, tr, tg, tb, 180);
        fill_circle(ren, cx, cy, 18);
        char mono[2] = { TypeChart::get_species_name(fighter.type)[0], '\0' };
        Font::draw_text(ren, cx - 4, cy - 6, mono, 2, 255, 255, 255);
    }

    // Reset clipping
    SDL_RenderSetClipRect(ren, nullptr);

    // 3px Primary Element Border + 1px Inner Highlight
    SDL_SetRenderDrawColor(ren, tr, tg, tb, 255);
    for (int t = 0; t < 3; ++t) {
        SDL_Rect b = { x + t, y + t, size - t * 2, size - t * 2 };
        SDL_RenderDrawRect(ren, &b);
    }
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 120);
    SDL_Rect inner_edge = { x + 3, y + 3, size - 6, size - 6 };
    SDL_RenderDrawRect(ren, &inner_edge);
}

// Full-Width Health Bar with Trailing Damage Lag
static void draw_health_bar(SDL_Renderer* ren, int x, int y, int w, int h, int hp, int max_hp, float lag_hp, bool align_right) {
    (void)align_right;
    // Outer Frame
    SDL_SetRenderDrawColor(ren, 15, 23, 42, 240);
    SDL_Rect bg = { x - 2, y - 2, w + 4, h + 4 };
    SDL_RenderFillRect(ren, &bg);
    SDL_SetRenderDrawColor(ren, 75, 85, 99, 255);
    SDL_RenderDrawRect(ren, &bg);

    // 1. Trailing Damage Lag Bar (White/Amber buffer that slowly drains)
    int lag_fill = (int)((lag_hp * (float)w) / (float)max_hp);
    if (lag_fill < 0) lag_fill = 0;
    if (lag_fill > w) lag_fill = w;
    SDL_SetRenderDrawColor(ren, 254, 240, 138, 230); // Bright white-amber
    SDL_Rect lag_r = { x, y, lag_fill, h };
    SDL_RenderFillRect(ren, &lag_r);

    // 2. Primary HP Gauge
    int fill = (hp * w) / max_hp;
    if (fill < 0) fill = 0;
    if (fill > w) fill = w;

    float pct = (float)hp / (float)max_hp;
    if (pct > 0.50f) {
        SDL_SetRenderDrawColor(ren, 34, 197, 94, 255); // Rich green
    } else if (pct > 0.20f) {
        SDL_SetRenderDrawColor(ren, 234, 179, 8, 255); // Amber yellow
    } else {
        SDL_SetRenderDrawColor(ren, 239, 68, 68, 255); // Danger red
    }
    SDL_Rect hp_r = { x, y, fill, h };
    SDL_RenderFillRect(ren, &hp_r);

    // 3. Bold Numerical Readout inside bar
    char hp_str[32];
    snprintf(hp_str, sizeof(hp_str), "%d / %d", hp, max_hp);
    int txt_w = Font::get_text_width(hp_str, 1);
    Font::draw_text(ren, x + (w - txt_w) / 2, y + 2, hp_str, 1, 255, 255, 255);
}

// Sleek High-Contrast Stamina Gauge
static void draw_stamina_bar(SDL_Renderer* ren, int x, int y, int w, int h, float stamina, float max_stamina, float flash_timer, bool align_right) {
    (void)align_right;
    // Outer Frame
    SDL_SetRenderDrawColor(ren, 15, 23, 42, 240);
    SDL_Rect bg = { x - 2, y - 2, w + 4, h + 4 };
    SDL_RenderFillRect(ren, &bg);
    SDL_SetRenderDrawColor(ren, 51, 65, 85, 255);
    SDL_RenderDrawRect(ren, &bg);

    // Stamina Gauge Fill
    int fill = (int)((stamina * (float)w) / max_stamina);
    if (fill < 0) fill = 0;
    if (fill > w) fill = w;

    if (flash_timer > 0.0f) {
        // Red flashing when attempting action with insufficient stamina
        SDL_SetRenderDrawColor(ren, 239, 68, 68, 255);
    } else {
        // Electric Cyan stamina energy bar
        SDL_SetRenderDrawColor(ren, 56, 189, 248, 255);
    }
    SDL_Rect sp_r = { x, y, fill, h };
    SDL_RenderFillRect(ren, &sp_r);

    // SP Numeric Readout
    char sp_str[32];
    snprintf(sp_str, sizeof(sp_str), "SP %d/%d", (int)stamina, (int)max_stamina);
    int txt_w = Font::get_text_width(sp_str, 1);
    Font::draw_text(ren, x + (w - txt_w) / 2, y - 1, sp_str, 1, 240, 249, 255);
}

// 6-Pokéball Team Status Tracker Row
static void draw_pokeball_row(SDL_Renderer* ren, int x, int y, const Fighter* team, int active_idx, int team_size, const char* trainer_name, Uint8 tr, Uint8 tg, Uint8 tb) {
    // Trainer Name Label
    Font::draw_text(ren, x, y, trainer_name, 1, tr, tg, tb);
    int cur_x = x + Font::get_text_width(trainer_name, 1) + 12;

    for (int i = 0; i < team_size && i < 6; ++i) {
        int cx = cur_x + i * 16;
        int cy = y + 4;

        bool is_active = (i == active_idx);
        bool is_fainted = (team != nullptr && team[i].hp <= 0);

        if (is_active) {
            // Pulsing golden ring around active Pokémon Pokéball
            SDL_SetRenderDrawColor(ren, 250, 204, 21, 255);
            fill_circle(ren, cx, cy, 7);
        }

        if (is_fainted) {
            // Dark grey fainted Pokéball
            SDL_SetRenderDrawColor(ren, 55, 65, 81, 255);
            fill_circle(ren, cx, cy, 5);
            SDL_SetRenderDrawColor(ren, 17, 24, 39, 255);
            SDL_RenderDrawLine(ren, cx - 5, cy, cx + 5, cy);
        } else {
            // Top half red
            SDL_SetRenderDrawColor(ren, 239, 68, 68, 255);
            for (int dy = -5; dy <= 0; ++dy) {
                int dx = (int)sqrtf((float)(25 - dy * dy));
                SDL_RenderDrawLine(ren, cx - dx, cy + dy, cx + dx, cy + dy);
            }
            // Bottom half white
            SDL_SetRenderDrawColor(ren, 245, 245, 250, 255);
            for (int dy = 1; dy <= 5; ++dy) {
                int dx = (int)sqrtf((float)(25 - dy * dy));
                SDL_RenderDrawLine(ren, cx - dx, cy + dy, cx + dx, cy + dy);
            }
            // Black belt & center button
            SDL_SetRenderDrawColor(ren, 17, 24, 39, 255);
            SDL_RenderDrawLine(ren, cx - 5, cy, cx + 5, cy);
            fill_circle(ren, cx, cy, 1);
        }
    }
}

// Dynamic Pill Badges for Types, Status, and Stat Stages
static void draw_pill_badge(SDL_Renderer* ren, int x, int y, const char* text, Uint8 r, Uint8 g, Uint8 b) {
    int w = Font::get_text_width(text, 1) + 10;
    int h = 14;

    SDL_SetRenderDrawColor(ren, r, g, b, 240);
    SDL_Rect pill = { x, y, w, h };
    SDL_RenderFillRect(ren, &pill);

    SDL_SetRenderDrawColor(ren, 255, 255, 255, 180);
    SDL_RenderDrawRect(ren, &pill);

    Font::draw_text(ren, x + 5, y + 3, text, 1, 255, 255, 255);
}

// Sleek White Anime Move Announcement Banner
static void draw_move_banner(SDL_Renderer* ren, int x, int y, MoveId move_id) {
    if (move_id == MOVE_NONE) return;
    const MoveDef& def = MoveRegistry::get(move_id);

    char banner_text[48];
    snprintf(banner_text, sizeof(banner_text), "%s!", def.name);
    int tw = Font::get_text_width(banner_text, 1);
    int bw = tw + 28;
    int bh = 22;

    // Rounded White Card
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_Rect card = { x, y, bw, bh };
    SDL_RenderFillRect(ren, &card);

    // Slate Border
    SDL_SetRenderDrawColor(ren, 15, 23, 42, 255);
    SDL_RenderDrawRect(ren, &card);

    // Italic Bold Text
    Font::draw_text(ren, x + 14, y + 6, banner_text, 1, 15, 23, 42);
}

void UI::render_battle_hud(SDL_Renderer* ren, const Fighter& player, const Fighter& dummy,
                           float mouse_scr_x, float mouse_scr_y, float cam_x, float cam_y,
                           int screen_w, int screen_h, bool locked_on,
                           bool ai_active, int match_state,
                           MatchType match_type,
                           AIMode p1_ai_mode, AIMode p2_ai_mode,
                           bool spectator_paused, float sim_speed,
                           float cam_zoom, bool dynamic_cam_enabled,
                           AimMode aim_mode,
                           const Fighter* p1_team, int p1_active_idx, int p1_team_size,
                           const Fighter* p2_team, int p2_active_idx, int p2_team_size,
                           float match_timer) {
    (void)aim_mode; (void)ai_active; (void)match_state;
    (void)p1_ai_mode; (void)p2_ai_mode; (void)spectator_paused; (void)sim_speed; (void)dynamic_cam_enabled;
    (void)cam_x; (void)cam_y; (void)cam_zoom;

    ElementType t1 = TYPE_NORMAL, t2 = TYPE_NONE;

    // =========================================================================
    // 1. TOP HEADER BROADCAST DISPLAY (Matching Pokemon Battlegrounds Standard)
    // =========================================================================
    int portrait_size = 60;
    int top_y = 12;

    // --- LEFT FIGHTER (ASH / PLAYER 1) ---
    int p1_port_x = 18;
    draw_fighter_portrait(ren, player, p1_port_x, top_y, portrait_size, false, match_timer);

    int p1_info_x = p1_port_x + portrait_size + 12;
    // Species Name
    Font::draw_text(ren, p1_info_x, top_y, TypeChart::get_species_name(player.type), 2, 255, 255, 255);

    // Type Pills
    int badge_x = p1_info_x + Font::get_text_width(TypeChart::get_species_name(player.type), 2) + 12;
    Uint8 tr = 148, tg = 163, tb = 184;
    TypeChart::get_type_color(player.type1, tr, tg, tb);
    draw_pill_badge(ren, badge_x, top_y + 2, TypeChart::get_type_name(player.type1), tr, tg, tb);
    badge_x += Font::get_text_width(TypeChart::get_type_name(player.type1), 1) + 16;

    if (player.type2 != TYPE_NONE) {
        TypeChart::get_type_color(player.type2, tr, tg, tb);
        draw_pill_badge(ren, badge_x, top_y + 2, TypeChart::get_type_name(player.type2), tr, tg, tb);
        badge_x += Font::get_text_width(TypeChart::get_type_name(player.type2), 1) + 16;
    }

    // Status Pill (PSN, BRN, PAR)
    if (player.status == STATUS_POISON) {
        draw_pill_badge(ren, badge_x, top_y + 2, "PSN", 168, 85, 247);
        badge_x += 42;
    } else if (player.status == STATUS_BURN) {
        draw_pill_badge(ren, badge_x, top_y + 2, "BRN", 249, 115, 22);
        badge_x += 42;
    } else if (player.status == STATUS_PARALYSIS) {
        draw_pill_badge(ren, badge_x, top_y + 2, "PAR", 250, 204, 21);
        badge_x += 42;
    }

    // Stat Stage Pills (DEF-1, SPD-3, ATK+2)
    if (player.stage_def != 0) {
        char sbuf[16]; snprintf(sbuf, sizeof(sbuf), "DEF%+d", player.stage_def);
        draw_pill_badge(ren, badge_x, top_y + 2, sbuf, (player.stage_def > 0 ? 234 : 59), (player.stage_def > 0 ? 179 : 130), (player.stage_def > 0 ? 8 : 246));
        badge_x += Font::get_text_width(sbuf, 1) + 16;
    }
    if (player.stage_spd != 0) {
        char sbuf[16]; snprintf(sbuf, sizeof(sbuf), "SPD%+d", player.stage_spd);
        draw_pill_badge(ren, badge_x, top_y + 2, sbuf, (player.stage_spd > 0 ? 234 : 59), (player.stage_spd > 0 ? 179 : 130), (player.stage_spd > 0 ? 8 : 246));
    }

    // Full-Width HP Bar with Trailing Damage Lag
    int bar_w = 340;
    int bar_h = 16;
    draw_health_bar(ren, p1_info_x, top_y + 20, bar_w, bar_h, player.hp, player.max_hp, player.display_lag_hp, false);
    draw_stamina_bar(ren, p1_info_x, top_y + 39, bar_w, 6, player.stamina, player.max_stamina, player.stamina_flash_timer, false);

    // 6-Pokéball Team Status Tracker (Teams mode only)
    if (p1_team != nullptr && p1_team_size > 1) {
        draw_pokeball_row(ren, p1_info_x, top_y + 48, p1_team, p1_active_idx, p1_team_size, "ASH", 239, 68, 68);
    }

    // Active Move Announcement Pill Banner (Left)
    if (player.active_move != MOVE_NONE) {
        int banner_y = (p1_team_size > 1) ? (top_y + 66) : (top_y + 50);
        draw_move_banner(ren, p1_info_x, banner_y, player.active_move);
    }

    // --- TOP CENTER MATCH TIMER ---
    int timer_w = 88;
    int timer_h = 24;
    int timer_x = (screen_w - timer_w) / 2;
    SDL_SetRenderDrawColor(ren, 15, 23, 42, 240);
    SDL_Rect timer_rect = { timer_x, top_y + 4, timer_w, timer_h };
    SDL_RenderFillRect(ren, &timer_rect);
    SDL_SetRenderDrawColor(ren, 100, 116, 139, 200);
    SDL_RenderDrawRect(ren, &timer_rect);

    int minutes = (int)match_timer / 60;
    int seconds = (int)match_timer % 60;
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);
    int tw = Font::get_text_width(time_str, 1);
    Font::draw_text(ren, timer_x + (timer_w - tw) / 2, top_y + 9, time_str, 1, 255, 255, 255);

    // --- RIGHT FIGHTER (PAUL / OPPONENT) ---
    int p2_port_x = screen_w - 18 - portrait_size;
    draw_fighter_portrait(ren, dummy, p2_port_x, top_y, portrait_size, true, match_timer);

    int p2_info_x = p2_port_x - 12 - bar_w;
    // Species Name (Right aligned)
    const char* p2_name = TypeChart::get_species_name(dummy.type);
    Font::draw_text(ren, p2_info_x + bar_w - Font::get_text_width(p2_name, 2), top_y, p2_name, 2, 255, 255, 255);

    // Right Type Badges
    int p2_badge_x = p2_info_x + bar_w - Font::get_text_width(p2_name, 2) - 12;
    TypeChart::get_species_types(dummy.type, t1, t2);
    TypeChart::get_type_color(t1, tr, tg, tb);
    int t1_w = Font::get_text_width(TypeChart::get_type_name(t1), 1) + 10;
    draw_pill_badge(ren, p2_badge_x - t1_w, top_y + 2, TypeChart::get_type_name(t1), tr, tg, tb);
    p2_badge_x -= (t1_w + 6);

    if (t2 != TYPE_NONE) {
        TypeChart::get_type_color(t2, tr, tg, tb);
        int t2_w = Font::get_text_width(TypeChart::get_type_name(t2), 1) + 10;
        draw_pill_badge(ren, p2_badge_x - t2_w, top_y + 2, TypeChart::get_type_name(t2), tr, tg, tb);
        p2_badge_x -= (t2_w + 6);
    }

    if (dummy.status == STATUS_POISON) {
        draw_pill_badge(ren, p2_badge_x - 38, top_y + 2, "PSN", 168, 85, 247);
    } else if (dummy.status == STATUS_BURN) {
        draw_pill_badge(ren, p2_badge_x - 38, top_y + 2, "BRN", 249, 115, 22);
    }

    // Right HP Bar with Trailing Damage Lag
    draw_health_bar(ren, p2_info_x, top_y + 20, bar_w, bar_h, dummy.hp, dummy.max_hp, dummy.display_lag_hp, true);
    draw_stamina_bar(ren, p2_info_x, top_y + 39, bar_w, 6, dummy.stamina, dummy.max_stamina, dummy.stamina_flash_timer, true);

    // 6-Pokéball Team Status Tracker (Teams mode only)
    if (p2_team != nullptr && p2_team_size > 1) {
        draw_pokeball_row(ren, p2_info_x + 90, top_y + 48, p2_team, p2_active_idx, p2_team_size, "PAUL", 168, 85, 247);
    }

    // Active Move Announcement Pill Banner (Right)
    if (dummy.active_move != MOVE_NONE) {
        int banner_y = (p2_team_size > 1) ? (top_y + 66) : (top_y + 50);
        draw_move_banner(ren, p2_port_x - 140, banner_y, dummy.active_move);
    }

    // =========================================================================
    // 2. RETICLE & AIMING TRAJECTORY
    // =========================================================================
    if (match_type != MATCH_TYPE_SPECTATOR) {
        if (locked_on && dummy.hp > 0) {
            float target_scr_x = (dummy.x - cam_x) * cam_zoom;
            float target_scr_y = (dummy.y - dummy.z - cam_y) * cam_zoom;
            int tx = (int)target_scr_x;
            int ty = (int)target_scr_y;
            int box_size = 28;
            int bracket_len = 8;

            SDL_SetRenderDrawColor(ren, 56, 189, 248, 255);
            SDL_RenderDrawLine(ren, tx - box_size, ty - box_size, tx - box_size + bracket_len, ty - box_size);
            SDL_RenderDrawLine(ren, tx - box_size, ty - box_size, tx - box_size, ty - box_size + bracket_len);
            SDL_RenderDrawLine(ren, tx + box_size, ty - box_size, tx + box_size - bracket_len, ty - box_size);
            SDL_RenderDrawLine(ren, tx + box_size, ty - box_size, tx + box_size, ty - box_size + bracket_len);
            SDL_RenderDrawLine(ren, tx - box_size, ty + box_size, tx - box_size + bracket_len, ty + box_size);
            SDL_RenderDrawLine(ren, tx - box_size, ty + box_size, tx - box_size, ty - box_size + bracket_len);
            SDL_RenderDrawLine(ren, tx + box_size, ty + box_size, tx + box_size - bracket_len, ty + box_size);
            SDL_RenderDrawLine(ren, tx + box_size, ty + box_size, tx + box_size, ty - box_size + bracket_len);

            Font::draw_text(ren, tx - 18, ty - box_size - 11, "LOCKED", 1, 56, 189, 248);
        } else {
            int mx = (int)mouse_scr_x;
            int my = (int)mouse_scr_y;
            SDL_SetRenderDrawColor(ren, 250, 204, 21, 240);
            SDL_RenderDrawLine(ren, mx - 8, my, mx - 3, my);
            SDL_RenderDrawLine(ren, mx + 3, my, mx + 8, my);
            SDL_RenderDrawLine(ren, mx, my - 8, mx, my - 3);
            SDL_RenderDrawLine(ren, mx, my + 3, mx, my + 8);
        }
    }

    // =========================================================================
    // 3. BOTTOM ABILITY CARD DECK & BENCH HOTKEYS
    // =========================================================================
    int card_w = 120;
    int card_h = 58;
    int spacing = 10;
    int total_w = 4 * card_w + 3 * spacing;
    int start_x = (screen_w - total_w) / 2;
    int card_y = screen_h - card_h - 14;

    const char* key_labels[4] = { "1", "2", "3", "4" };

    for (int i = 0; i < 4; ++i) {
        const AbilitySlot& ab = player.abilities[i];
        int cx = start_x + i * (card_w + spacing);

        SDL_SetRenderDrawColor(ren, 15, 23, 42, 235);
        SDL_Rect cr = { cx, card_y, card_w, card_h };
        SDL_RenderFillRect(ren, &cr);

        // Key badge
        SDL_SetRenderDrawColor(ren, 38, 42, 54, 255);
        SDL_Rect pill_r = { cx + 6, card_y + 6, 18, 13 };
        SDL_RenderFillRect(ren, &pill_r);
        Font::draw_text(ren, cx + 11, card_y + 8, key_labels[i], 1, 255, 255, 255);

        // Stamina cost
        char sp_buf[16];
        snprintf(sp_buf, sizeof(sp_buf), "%dSP", ab.stamina_cost);
        int sp_w = Font::get_text_width(sp_buf, 1);
        Font::draw_text(ren, cx + card_w - sp_w - 6, card_y + 8, sp_buf, 1, 234, 179, 8);

        // Move Name
        Uint8 name_r = (ab.cooldown_timer > 0.0f) ? 120 : 255;
        Uint8 name_g = (ab.cooldown_timer > 0.0f) ? 120 : 255;
        Uint8 name_b = (ab.cooldown_timer > 0.0f) ? 120 : 255;
        Font::draw_text(ren, cx + 6, card_y + 24, ab.name, 1, name_r, name_g, name_b);

        // Border
        if (ab.cooldown_timer > 0.0f) {
            SDL_SetRenderDrawColor(ren, 75, 85, 99, 200);
        } else if (player.stamina >= (float)ab.stamina_cost) {
            SDL_SetRenderDrawColor(ren, 234, 179, 8, 255);
        } else {
            SDL_SetRenderDrawColor(ren, 239, 68, 68, 200);
        }
        SDL_RenderDrawRect(ren, &cr);

        // Type color bottom edge
        const MoveDef& mdef = MoveRegistry::get(ab.move_id);
        TypeChart::get_type_color(mdef.element_type, tr, tg, tb);
        SDL_SetRenderDrawColor(ren, tr, tg, tb, 255);
        SDL_Rect cbar = { cx + 2, card_y + card_h - 4, card_w - 4, 3 };
        SDL_RenderFillRect(ren, &cbar);
    }
}

void UI::render_cinematic_ko(SDL_Renderer* ren, float focus_x, float focus_y,
                             float cam_x, float cam_y, float cam_zoom,
                             float ko_timer, int screen_w, int screen_h) {
    (void)focus_x; (void)focus_y; (void)cam_x; (void)cam_y; (void)cam_zoom;

    // Cinematic Letterbox black bars (top and bottom)
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 240);
    int bar_h = 70;
    SDL_Rect top_bar = { 0, 0, screen_w, bar_h };
    SDL_Rect bot_bar = { 0, screen_h - bar_h, screen_w, bar_h };
    SDL_RenderFillRect(ren, &top_bar);
    SDL_RenderFillRect(ren, &bot_bar);

    // Vignette impact flash
    float flash_pct = ko_timer / 0.70f;
    if (flash_pct > 1.0f) flash_pct = 1.0f;
    SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(flash_pct * 90.0f));
    SDL_Rect flash_rect = { 0, 0, screen_w, screen_h };
    SDL_RenderFillRect(ren, &flash_rect);

    // Big dramatic K.O.! in center screen
    const char* ko_text = "K. O. !";
    int kw = Font::get_text_width(ko_text, 4);
    Font::draw_text(ren, (screen_w - kw) / 2 + 3, (screen_h - 40) / 2 + 3, ko_text, 4, 15, 23, 42); // Shadow
    Font::draw_text(ren, (screen_w - kw) / 2, (screen_h - 40) / 2, ko_text, 4, 239, 68, 68);
}

void UI::render(SDL_Renderer* ren, const Fighter& player, const Fighter& dummy,
                float mouse_scr_x, float mouse_scr_y, float cam_x, float cam_y,
                int screen_w, int screen_h, bool locked_on,
                bool ai_active, int match_state,
                bool moveset_menu_open, int selected_slot) {
    (void)moveset_menu_open; (void)selected_slot;
    render_battle_hud(ren, player, dummy, mouse_scr_x, mouse_scr_y, cam_x, cam_y, screen_w, screen_h, locked_on, ai_active, match_state);
}
