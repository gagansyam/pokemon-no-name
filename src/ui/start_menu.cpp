#include "ui.hpp"
#include "font.hpp"
#include "types.hpp"
#include "moves.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

struct MenuDustParticle {
    float x;
    float y;
    float speed_x;
    float speed_y;
    int size;
    float alpha_phase;
};
static MenuDustParticle s_menu_dust[48];
static bool s_dust_inited = false;

// Authentic GBA Pokémon FireRed battle grass & dirt encounter pedestal
static void draw_gba_battle_platform(SDL_Renderer* ren, int cx, int cy) {
    // 1. Soft elliptical drop shadow below platform
    int sy = cy + 34;
    int s_rx = 68, s_ry = 18;
    SDL_SetRenderDrawColor(ren, 14, 8, 12, 160);
    for (int dy = -s_ry; dy <= s_ry; ++dy) {
        float f = (float)(dy * dy) / (float)(s_ry * s_ry);
        if (f > 1.0f) continue;
        int span = (int)((float)s_rx * sqrtf(1.0f - f));
        SDL_RenderDrawLine(ren, cx - span, sy + dy, cx + span, sy + dy);
    }

    // 2. Lush Green Grass Rim (Classic FireRed stadium turf)
    int py = cy + 28;
    int g_rx = 62, g_ry = 16;
    SDL_SetRenderDrawColor(ren, 20, 83, 45, 255); // Dark grass contour
    for (int dy = -g_ry; dy <= g_ry; ++dy) {
        float f = (float)(dy * dy) / (float)(g_ry * g_ry);
        if (f > 1.0f) continue;
        int span = (int)((float)g_rx * sqrtf(1.0f - f));
        SDL_RenderDrawLine(ren, cx - span, py + dy, cx + span, py + dy);
    }
    SDL_SetRenderDrawColor(ren, 34, 197, 94, 255); // Vivid GBA turf green
    for (int dy = -g_ry + 2; dy <= g_ry - 2; ++dy) {
        float f = (float)(dy * dy) / (float)(g_ry * g_ry);
        if (f > 1.0f) continue;
        int span = (int)((float)(g_rx - 2) * sqrtf(1.0f - f));
        SDL_RenderDrawLine(ren, cx - span, py + dy, cx + span, py + dy);
    }

    // 3. Sandy battle dirt center circle
    int d_rx = 48, d_ry = 11;
    SDL_SetRenderDrawColor(ren, 180, 142, 98, 255); // Sand border
    for (int dy = -d_ry; dy <= d_ry; ++dy) {
        float f = (float)(dy * dy) / (float)(d_ry * d_ry);
        if (f > 1.0f) continue;
        int span = (int)((float)d_rx * sqrtf(1.0f - f));
        SDL_RenderDrawLine(ren, cx - span, py + dy, cx + span, py + dy);
    }
    SDL_SetRenderDrawColor(ren, 224, 196, 154, 255); // Warm sunny sand fill
    for (int dy = -d_ry + 2; dy <= d_ry - 2; ++dy) {
        float f = (float)(dy * dy) / (float)(d_ry * d_ry);
        if (f > 1.0f) continue;
        int span = (int)((float)(d_rx - 3) * sqrtf(1.0f - f));
        SDL_RenderDrawLine(ren, cx - span, py + dy, cx + span, py + dy);
    }

    // 4. Subtle grass blades / rim highlights
    SDL_SetRenderDrawColor(ren, 134, 239, 172, 220); // Mint highlight
    SDL_RenderDrawLine(ren, cx - 40, py - g_ry + 1, cx + 40, py - g_ry + 1);
}

// Helper to draw a FireRed summary screen styled stat gauge bar
static void draw_stat_bar(SDL_Renderer* ren, int x, int y, int w, int h,
                          const char* label, int val, int max_val,
                          Uint8 r, Uint8 g, Uint8 b) {
    Font::draw_text(ren, x, y, label, 1, 254, 240, 138); // Warm ivory gold

    char val_str[16];
    snprintf(val_str, sizeof(val_str), "%d", val);
    int vw = Font::get_text_width(val_str, 1);
    Font::draw_text(ren, x + 50 - vw, y, val_str, 1, 255, 255, 255);

    int bar_x = x + 56;
    int bar_w = w - 56;
    SDL_Rect bg = { bar_x, y + 2, bar_w, h - 4 };
    SDL_SetRenderDrawColor(ren, 22, 14, 18, 255);
    SDL_RenderFillRect(ren, &bg);
    SDL_SetRenderDrawColor(ren, 68, 34, 42, 220);
    SDL_RenderDrawRect(ren, &bg);

    float ratio = (max_val > 0) ? ((float)val / (float)max_val) : 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio < 0.0f) ratio = 0.0f;

    int fill_w = (int)((float)(bar_w - 2) * ratio);
    if (fill_w > 0) {
        SDL_Rect fill = { bar_x + 1, y + 3, fill_w, h - 6 };
        SDL_SetRenderDrawColor(ren, r, g, b, 245);
        SDL_RenderFillRect(ren, &fill);

        // 1px top highlight for 3D cartridge bevel
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 130);
        SDL_RenderDrawLine(ren, bar_x + 1, y + 3, bar_x + fill_w, y + 3);
    }
}

static const char* get_move_description(MoveId id) {
    switch (id) {
        case MOVE_QUICK_ATTACK:    return "Priority forward burst strike. Fast recovery.";
        case MOVE_THUNDERBOLT:     return "Heavy sky strike with ground blast. Evade with dash.";
        case MOVE_ELECTRO_BALL:    return "High-velocity guided sphere with aggressive homing.";
        case MOVE_IRON_TAIL:       return "Hardened tail stance into anime 360 jump-spin sweet-spot.";
        case MOVE_VOLT_TACKLE:     return "Supersonic 700px/s rush with electric blast. Heavy hitstop.";
        case MOVE_AGILITY:         return "+40% speed buff and instant burst dash reset.";
        case MOVE_DOUBLE_TEAM:     return "Deploys 4 decoy afterimages across arena to absorb hits.";
        case MOVE_PROTECT:         return "0.12s perfect parry barrier. Negates DMG, recoils striker.";
        case MOVE_SCRATCH:         return "Swift close-range claw slash for defensive punishes.";
        case MOVE_FLAMETHROWER:    return "Billowing multi-hit molten flame torrent with snout flash.";
        case MOVE_FIRE_SPIN:       return "High-arc ember seed exploding into 3.2s fiery vortex trap.";
        case MOVE_WING_ATTACK:     return "PMD aerial flight rush with hyper-armor against light hits.";
        case MOVE_SCORCHING_SANDS: return "Ground furrow scooping forward earth wave (ground hit).";
        case MOVE_DRAGON_CLAW:     return "Forward lunge slash with melee clash collision priority.";
        case MOVE_DRAGON_RAGE:     return "High-velocity draconic plasma orb with recoil kickback.";
        case MOVE_EARTHQUAKE:      return "Seismic arena stomp shockwave. Cleanly evaded by jumping!";
        case MOVE_SEISMIC_TOSS:    return "Unblockable command grab lifting victim into orbit slam!";
        case MOVE_AURA_SPHERE:     return "Guided sapphire aura orb with subtle magnetic homing.";
        case MOVE_FORCE_PALM:      return "Martial palm strike releasing compressed kinetic shockwave.";
        case MOVE_CLOSE_COMBAT:    return "Devastating close-range martial flurry rush and slam.";
        case MOVE_BONE_RUSH:       return "Long-range auric bone javelin rush & spinning earth strike.";
        case MOVE_EXTREME_SPEED:   return "Supersonic 1050px/s priority warp dash with afterimages.";
        case MOVE_FLASH_CANNON:    return "Concentrated silver metallic beam with blinding flare.";
        case MOVE_DRAGON_PULSE:    return "Roaring spectral dragon head beam with serpentine plasma wake.";
        default:                   return "Standard combat ability.";
    }
}

static const char* get_pokemon_name(PokemonType t) {
    switch (t) {
        case PKMN_PIKACHU:   return "PIKACHU #025";
        case PKMN_CHARIZARD: return "CHARIZARD #006";
        case PKMN_LUCARIO:   return "LUCARIO #0448";
        case PKMN_GRENINJA:  return "GRENINJA #0658";
        default:             return "POKEMON";
    }
}

static void draw_pokemon_type_pills(SDL_Renderer* ren, int x, int y, PokemonType t) {
    if (t == PKMN_PIKACHU) {
        Font::draw_text(ren, x, y, "[ELECTRIC]", 1, 250, 204, 21);
    } else if (t == PKMN_CHARIZARD) {
        Font::draw_text(ren, x, y, "[FIRE] [FLYING]", 1, 239, 68, 68);
    } else if (t == PKMN_LUCARIO) {
        Font::draw_text(ren, x, y, "[FIGHTING] [STEEL]", 1, 56, 189, 248);
    } else if (t == PKMN_GRENINJA) {
        Font::draw_text(ren, x, y, "[WATER] [DARK]", 1, 56, 189, 248);
    }
}

void UI::render_start_menu(
    SDL_Renderer* ren,
    const Fighter& p1_preview, const Fighter& p2_preview,
    PokemonType p1_type, PokemonType p2_type,
    const MoveId p1_moves[4], const MoveId p2_moves[4],
    int p1_selected_slot, int p2_selected_slot, int active_panel,
    MatchType match_type, AIMode p1_ai_mode, AIMode p2_ai_mode,
    AimMode aim_mode, bool dynamic_cam,
    float mouse_x, float mouse_y,
    int screen_w, int screen_h,
    float anim_timer,
    bool show_log_modal,
    bool show_settings_modal,
    int drafter_target,
    int drafter_slot
) {
    (void)active_panel;
    (void)p1_ai_mode;
    if (!s_dust_inited) {
        for (int i = 0; i < 48; ++i) {
            s_menu_dust[i].x = (float)(rand() % screen_w);
            s_menu_dust[i].y = (float)(rand() % screen_h);
            s_menu_dust[i].speed_x = -12.0f + (float)(rand() % 24);
            s_menu_dust[i].speed_y = -20.0f - (float)(rand() % 28); // Rising warm embers
            s_menu_dust[i].size = 1 + (rand() % 3);
            s_menu_dust[i].alpha_phase = (float)(rand() % 628) / 100.0f;
        }
        s_dust_inited = true;
    }

    // =========================================================================
    // 1. ARENA BACKDROP: POKÉMON FIRERED CRIMSON MAROON WITH GBA SCANLINES
    // =========================================================================
    SDL_SetRenderDrawColor(ren, 42, 16, 22, 255); // Rich FireRed deep burgundy
    SDL_Rect full_scr = { 0, 0, screen_w, screen_h };
    SDL_RenderFillRect(ren, &full_scr);

    // Subtle GBA alternating scanline striping
    SDL_SetRenderDrawColor(ren, 34, 12, 18, 180);
    for (int y = 0; y < screen_h; y += 4) {
        SDL_RenderDrawLine(ren, 0, y, screen_w, y);
    }

    // Ambient arena battle rings with warm ruby glow
    SDL_SetRenderDrawColor(ren, 72, 24, 32, 110);
    SDL_Rect outer_ring = { screen_w / 2 - 240, screen_h / 2 - 240, 480, 480 };
    SDL_RenderDrawRect(ren, &outer_ring);
    SDL_Rect inner_ring = { screen_w / 2 - 120, screen_h / 2 - 120, 240, 240 };
    SDL_RenderDrawRect(ren, &inner_ring);

    // Ambient floating fire embers & golden sparks (48 particles)
    for (int i = 0; i < 48; ++i) {
        float px = fmodf(s_menu_dust[i].x + s_menu_dust[i].speed_x * anim_timer + (float)screen_w * 4.0f, (float)screen_w);
        float py = fmodf(s_menu_dust[i].y + s_menu_dust[i].speed_y * anim_timer + (float)screen_h * 4.0f, (float)screen_h);
        float alpha_pulse = sinf(anim_timer * 2.8f + s_menu_dust[i].alpha_phase) * 0.5f + 0.5f;
        Uint8 alpha = (Uint8)(40 + alpha_pulse * 140);
        if (i % 3 == 0) {
            SDL_SetRenderDrawColor(ren, 249, 115, 22, alpha); // Ember orange
        } else if (i % 3 == 1) {
            SDL_SetRenderDrawColor(ren, 250, 204, 21, alpha); // Golden spark
        } else {
            SDL_SetRenderDrawColor(ren, 239, 68, 68, alpha);  // Crimson ember
        }
        SDL_Rect pr = { (int)px, (int)py, s_menu_dust[i].size, s_menu_dust[i].size };
        SDL_RenderFillRect(ren, &pr);
    }

    // =========================================================================
    // 2. TOP HEADER BAR: CLASSIC FIRERED RED TITLE RIBBON
    // =========================================================================
    SDL_SetRenderDrawColor(ren, 185, 28, 28, 255); // FireRed ruby banner
    SDL_Rect top_bar = { 0, 0, screen_w, 64 };
    SDL_RenderFillRect(ren, &top_bar);

    // Beveled highlight line at top & shadow at bottom
    SDL_SetRenderDrawColor(ren, 239, 68, 68, 255);
    SDL_RenderDrawLine(ren, 0, 1, screen_w, 1);
    SDL_SetRenderDrawColor(ren, 120, 16, 16, 255);
    SDL_RenderDrawLine(ren, 0, 61, screen_w, 61);

    // Golden accent border
    SDL_SetRenderDrawColor(ren, 250, 204, 21, 255);
    SDL_RenderDrawLine(ren, 0, 62, screen_w, 62);
    SDL_RenderDrawLine(ren, 0, 63, screen_w, 63);

    const char* title = "POKEMON REAL-TIME BATTLE";
    int tw = Font::get_text_width(title, 2);
    // Drop shadow
    Font::draw_text(ren, (screen_w - tw) / 2 + 1, 15, title, 2, 70, 10, 10);
    Font::draw_text(ren, (screen_w - tw) / 2, 14, title, 2, 255, 255, 255);

    const char* subtitle = (match_type == MATCH_TYPE_SPECTATOR)
        ? "AUTONOMOUS AI SHOWDOWN - SELECT COMBATANTS"
        : "CHOOSE YOUR FIGHTER & CONFIGURE YOUR OPPONENT";
    int sw = Font::get_text_width(subtitle, 1);
    Font::draw_text(ren, (screen_w - sw) / 2, 42, subtitle, 1, 254, 243, 199);

    // Top-Right Header Action: [⚙ SETTINGS (S)]
    SDL_Rect top_set_btn = { screen_w - 240, 16, 130, 32 };
    bool set_hover = (mouse_x >= top_set_btn.x && mouse_x <= top_set_btn.x + top_set_btn.w &&
                      mouse_y >= top_set_btn.y && mouse_y <= top_set_btn.y + top_set_btn.h);
    SDL_SetRenderDrawColor(ren, set_hover ? 140 : 100, set_hover ? 24 : 16, set_hover ? 24 : 16, 255);
    SDL_RenderFillRect(ren, &top_set_btn);
    SDL_SetRenderDrawColor(ren, 250, 204, 21, set_hover ? 255 : 200);
    SDL_RenderDrawRect(ren, &top_set_btn);
    Font::draw_text(ren, top_set_btn.x + 10, top_set_btn.y + 8, "[S] SETTINGS", 1, 254, 240, 138);

    // Top-Right Header Action: [📊 LOG (L)]
    if (CombatLogger::has_completed_log()) {
        SDL_Rect top_log_btn = { screen_w - 100, 16, 85, 32 };
        bool log_hover = (mouse_x >= top_log_btn.x && mouse_x <= top_log_btn.x + top_log_btn.w &&
                          mouse_y >= top_log_btn.y && mouse_y <= top_log_btn.y + top_log_btn.h);
        SDL_SetRenderDrawColor(ren, log_hover ? 140 : 100, log_hover ? 24 : 16, log_hover ? 24 : 16, 255);
        SDL_RenderFillRect(ren, &top_log_btn);
        SDL_SetRenderDrawColor(ren, 250, 204, 21, log_hover ? 255 : 200);
        SDL_RenderDrawRect(ren, &top_log_btn);
        Font::draw_text(ren, top_log_btn.x + 10, top_log_btn.y + 8, "[L] LOG", 1, 254, 240, 138);
    }

    // =========================================================================
    // 3. LEFT SHOWCASE: PLAYER 1 CARD (GBA SUMMARY WINDOW)
    // =========================================================================
    int p1_x = 45, p1_y = 74, p1_w = 460, p1_h = 582;
    SDL_SetRenderDrawColor(ren, 26, 16, 22, 250); // Dark GBA summary window
    SDL_Rect p1_card = { p1_x, p1_y, p1_w, p1_h };
    SDL_RenderFillRect(ren, &p1_card);

    // Beveled FireRed Frame (Multi-tone ruby bevel)
    SDL_SetRenderDrawColor(ren, 185, 28, 28, 255);
    SDL_RenderDrawRect(ren, &p1_card);
    SDL_Rect p1_inner_edge = { p1_card.x + 2, p1_card.y + 2, p1_card.w - 4, p1_card.h - 4 };
    SDL_SetRenderDrawColor(ren, 239, 68, 68, 160);
    SDL_RenderDrawRect(ren, &p1_inner_edge);

    // P1 Title Banner
    const char* p1_tag = (match_type == MATCH_TYPE_SPECTATOR) ? "PLAYER 1 (CPU SHOWDOWN)" : "PLAYER 1 (YOU)";
    Font::draw_text(ren, p1_x + 16, p1_y + 12, p1_tag, 1, 250, 204, 21);

    // P1 Carousel Switcher (x: 57..493, y: 100..130)
    SDL_Rect p1_switch_btn = { p1_x + 12, p1_y + 26, p1_w - 24, 30 };
    bool p1_switch_hover = (mouse_x >= p1_switch_btn.x && mouse_x <= p1_switch_btn.x + p1_switch_btn.w &&
                            mouse_y >= p1_switch_btn.y && mouse_y <= p1_switch_btn.y + p1_switch_btn.h);
    SDL_SetRenderDrawColor(ren, p1_switch_hover ? 54 : 36, p1_switch_hover ? 24 : 16, p1_switch_hover ? 28 : 20, 255);
    SDL_RenderFillRect(ren, &p1_switch_btn);
    SDL_SetRenderDrawColor(ren, p1_switch_hover ? 250 : 217, p1_switch_hover ? 204 : 119, p1_switch_hover ? 21 : 6, 255);
    SDL_RenderDrawRect(ren, &p1_switch_btn);

    const char* p1_name = get_pokemon_name(p1_type);
    Font::draw_text(ren, p1_switch_btn.x + 12, p1_switch_btn.y + 8, "<", 1, 250, 204, 21);
    Font::draw_text(ren, p1_switch_btn.x + 36, p1_switch_btn.y + 8, p1_name, 1, 255, 255, 255);
    Font::draw_text(ren, p1_switch_btn.x + p1_switch_btn.w - 20, p1_switch_btn.y + 8, ">", 1, 250, 204, 21);

    // Type Pills
    draw_pokemon_type_pills(ren, p1_switch_btn.x + 220, p1_switch_btn.y + 8, p1_type);

    // P1 Large Pedestal & Animated Model Showcase (Center: cx = 275, cy = 234)
    int p1_cx = p1_x + p1_w / 2;
    int p1_cy = p1_y + 160;

    // Authentic FireRed Battle Platform (Lush grass turf & warm sand dirt)
    draw_gba_battle_platform(ren, p1_cx, p1_cy);

    // Animated Sprite Preview at 3.5x Scale
    int p1_anim_frame = ((int)(anim_timer * 6.0f)) % 4;
    int p1_dir = (p1_type == PKMN_CHARIZARD ? 6 : (p1_type == PKMN_LUCARIO ? 2 : (p1_type == PKMN_GRENINJA ? 2 : 2)));
    float p1_scale = (p1_type == PKMN_LUCARIO ? 3.1f : (p1_type == PKMN_GRENINJA ? 3.1f : 3.4f));
    p1_preview.render_preview(ren, p1_cx, p1_cy, p1_dir, p1_anim_frame, p1_scale);

    // P1 Stat Gauges Section (y: 279 .. 384)
    SDL_Rect p1_stat_box = { p1_x + 14, p1_y + 205, p1_w - 28, 105 };
    SDL_SetRenderDrawColor(ren, 20, 12, 16, 220);
    SDL_RenderFillRect(ren, &p1_stat_box);
    SDL_SetRenderDrawColor(ren, 80, 36, 46, 200);
    SDL_RenderDrawRect(ren, &p1_stat_box);

    int max_ref_hp = 800;
    int max_ref_stat = 125;
    draw_stat_bar(ren, p1_stat_box.x + 10, p1_stat_box.y + 10, p1_stat_box.w - 20, 16,
                  "HP", p1_preview.base_stats.hp, max_ref_hp, 34, 197, 94);
    draw_stat_bar(ren, p1_stat_box.x + 10, p1_stat_box.y + 32, p1_stat_box.w - 20, 16,
                  "ATK", p1_preview.base_stats.attack, max_ref_stat, 239, 68, 68);
    draw_stat_bar(ren, p1_stat_box.x + 10, p1_stat_box.y + 54, p1_stat_box.w - 20, 16,
                  "DEF", p1_preview.base_stats.defense, max_ref_stat, 59, 130, 246);
    draw_stat_bar(ren, p1_stat_box.x + 10, p1_stat_box.y + 76, p1_stat_box.w - 20, 16,
                  "SPD", p1_preview.base_stats.speed, max_ref_stat, 234, 179, 8);

    // P1 Equipped Abilities Strip (y: 416 .. 560)
    Font::draw_text(ren, p1_x + 16, p1_y + 322, "EQUIPPED ABILITIES [CLICK SLOT TO DRAFT]", 1, 254, 240, 138);
    for (int i = 0; i < 4; ++i) {
        int sy = p1_y + 342 + i * 36;
        SDL_Rect sr = { p1_x + 14, sy, p1_w - 28, 32 };
        bool is_sel = (p1_selected_slot == i);
        bool is_hov = (mouse_x >= sr.x && mouse_x <= sr.x + sr.w &&
                       mouse_y >= sr.y && mouse_y <= sr.y + sr.h);

        SDL_SetRenderDrawColor(ren, is_sel ? 64 : (is_hov ? 48 : 32),
                                    is_sel ? 26 : (is_hov ? 20 : 16),
                                    is_sel ? 32 : (is_hov ? 24 : 20), 255);
        SDL_RenderFillRect(ren, &sr);
        SDL_SetRenderDrawColor(ren, is_sel ? 250 : (is_hov ? 217 : 90),
                                    is_sel ? 204 : (is_hov ? 119 : 45),
                                    is_sel ? 21  : (is_hov ? 6   : 55), 255);
        SDL_RenderDrawRect(ren, &sr);

        // Cycle Left Arrow Button [<] (x: sr.x + 4, y: sy + 4, w: 20, h: 24)
        SDL_Rect btn_l = { sr.x + 4, sy + 4, 20, 24 };
        bool hov_l = (mouse_x >= btn_l.x && mouse_x <= btn_l.x + btn_l.w &&
                      mouse_y >= btn_l.y && mouse_y <= btn_l.y + btn_l.h);
        SDL_SetRenderDrawColor(ren, hov_l ? 120 : 60, hov_l ? 26 : 18, hov_l ? 30 : 20, 255);
        SDL_RenderFillRect(ren, &btn_l);
        Font::draw_text(ren, btn_l.x + 6, btn_l.y + 6, "<", 1, hov_l ? 255 : 220, hov_l ? 255 : 200, hov_l ? 255 : 80);

        // Slot Number Badge
        char slot_lbl[16];
        snprintf(slot_lbl, sizeof(slot_lbl), "S%d", i + 1);
        Font::draw_text(ren, sr.x + 28, sy + 8, slot_lbl, 1, is_sel ? 250 : 200, is_sel ? 204 : 170, is_sel ? 21 : 120);

        // Move Name
        const MoveDef& mdef = MoveRegistry::get(p1_moves[i]);
        Font::draw_text(ren, sr.x + 56, sy + 8, mdef.name, 1, 255, 255, 255);

        // Element Type Pill
        Uint8 mr = 148, mg = 163, mb = 184;
        TypeChart::get_type_color(mdef.element_type, mr, mg, mb);
        Font::draw_text(ren, sr.x + 230, sy + 8, TypeChart::get_type_name(mdef.element_type), 1, mr, mg, mb);

        // Stats: Cost and Power
        char cost_str[24];
        snprintf(cost_str, sizeof(cost_str), "%dSP | P%d", mdef.stamina_cost, mdef.base_power);
        Font::draw_text(ren, sr.x + 312, sy + 8, cost_str, 1, 254, 243, 199);

        // Cycle Right Arrow Button [>] (x: sr.x + sr.w - 24, y: sy + 4, w: 20, h: 24)
        SDL_Rect btn_r = { sr.x + sr.w - 24, sy + 4, 20, 24 };
        bool hov_r = (mouse_x >= btn_r.x && mouse_x <= btn_r.x + btn_r.w &&
                      mouse_y >= btn_r.y && mouse_y <= btn_r.y + btn_r.h);
        SDL_SetRenderDrawColor(ren, hov_r ? 120 : 60, hov_r ? 26 : 18, hov_r ? 30 : 20, 255);
        SDL_RenderFillRect(ren, &btn_r);
        Font::draw_text(ren, btn_r.x + 6, btn_r.y + 6, ">", 1, hov_r ? 255 : 220, hov_r ? 255 : 200, hov_r ? 255 : 80);
    }

    // P1 Full Movepool Drafter Button (y: 568 .. 606)
    SDL_Rect p1_draft_btn = { p1_x + 14, p1_y + 494, p1_w - 28, 38 };
    bool p1_draft_hov = (mouse_x >= p1_draft_btn.x && mouse_x <= p1_draft_btn.x + p1_draft_btn.w &&
                         mouse_y >= p1_draft_btn.y && mouse_y <= p1_draft_btn.y + p1_draft_btn.h);
    SDL_SetRenderDrawColor(ren, p1_draft_hov ? 140 : 90, p1_draft_hov ? 28 : 18, p1_draft_hov ? 32 : 22, 255);
    SDL_RenderFillRect(ren, &p1_draft_btn);
    SDL_SetRenderDrawColor(ren, 250, 204, 21, p1_draft_hov ? 255 : 180);
    SDL_RenderDrawRect(ren, &p1_draft_btn);

    char p1_btn_txt[64];
    snprintf(p1_btn_txt, sizeof(p1_btn_txt), "[M] DRAFT FULL MOVESET (%d MOVES AVAILABLE)", p1_preview.movepool_count);
    Font::draw_text(ren, p1_draft_btn.x + 18, p1_draft_btn.y + 12, p1_btn_txt, 1, 254, 240, 138);

    Font::draw_text(ren, p1_x + 16, p1_y + 544, "Use [<] [>] on slots or click above to open full drafter.", 1, 203, 185, 175);

    // =========================================================================
    // 4. RIGHT SHOWCASE: CPU OPPONENT CARD
    // =========================================================================
    // =========================================================================
    // 4. RIGHT SHOWCASE: CPU OPPONENT CARD (GBA SUMMARY WINDOW)
    // =========================================================================
    int p2_x = 775, p2_y = 74, p2_w = 460, p2_h = 582;
    SDL_SetRenderDrawColor(ren, 26, 16, 22, 250);
    SDL_Rect p2_card = { p2_x, p2_y, p2_w, p2_h };
    SDL_RenderFillRect(ren, &p2_card);

    // Beveled Cobalt/Rival Frame
    SDL_SetRenderDrawColor(ren, 37, 99, 235, 255);
    SDL_RenderDrawRect(ren, &p2_card);
    SDL_Rect p2_inner_edge = { p2_card.x + 2, p2_card.y + 2, p2_card.w - 4, p2_card.h - 4 };
    SDL_SetRenderDrawColor(ren, 96, 165, 250, 160);
    SDL_RenderDrawRect(ren, &p2_inner_edge);

    // P2 Title Banner
    const char* p2_tag = (match_type == MATCH_TYPE_SPECTATOR) ? "PLAYER 2 (CPU SHOWDOWN)" : "CPU OPPONENT";
    Font::draw_text(ren, p2_x + 16, p2_y + 12, p2_tag, 1, 96, 165, 250);

    // P2 Carousel Switcher (x: 787..1223, y: 100..130)
    SDL_Rect p2_switch_btn = { p2_x + 12, p2_y + 26, p2_w - 24, 30 };
    bool p2_switch_hover = (mouse_x >= p2_switch_btn.x && mouse_x <= p2_switch_btn.x + p2_switch_btn.w &&
                            mouse_y >= p2_switch_btn.y && mouse_y <= p2_switch_btn.y + p2_switch_btn.h);
    SDL_SetRenderDrawColor(ren, p2_switch_hover ? 54 : 36, p2_switch_hover ? 24 : 16, p2_switch_hover ? 28 : 20, 255);
    SDL_RenderFillRect(ren, &p2_switch_btn);
    SDL_SetRenderDrawColor(ren, p2_switch_hover ? 96 : 37, p2_switch_hover ? 165 : 99, p2_switch_hover ? 250 : 235, 255);
    SDL_RenderDrawRect(ren, &p2_switch_btn);

    const char* p2_name = get_pokemon_name(p2_type);
    Font::draw_text(ren, p2_switch_btn.x + 12, p2_switch_btn.y + 8, "<", 1, 250, 204, 21);
    Font::draw_text(ren, p2_switch_btn.x + 36, p2_switch_btn.y + 8, p2_name, 1, 255, 255, 255);
    Font::draw_text(ren, p2_switch_btn.x + p2_switch_btn.w - 20, p2_switch_btn.y + 8, ">", 1, 250, 204, 21);

    // Type Pills
    draw_pokemon_type_pills(ren, p2_switch_btn.x + 220, p2_switch_btn.y + 8, p2_type);

    // P2 Large Pedestal & Animated Model Showcase (Center: cx = 1005, cy = 234)
    int p2_cx = p2_x + p2_w / 2;
    int p2_cy = p2_y + 160;

    // Authentic FireRed Battle Platform (Lush grass turf & warm sand dirt)
    draw_gba_battle_platform(ren, p2_cx, p2_cy);

    int p2_anim_frame = ((int)(anim_timer * 6.0f)) % 4;
    int p2_dir = (p2_type == PKMN_CHARIZARD ? 6 : (p2_type == PKMN_LUCARIO ? 2 : (p2_type == PKMN_GRENINJA ? 2 : 2)));
    float p2_scale = (p2_type == PKMN_LUCARIO ? 3.1f : (p2_type == PKMN_GRENINJA ? 3.1f : 3.4f));
    p2_preview.render_preview(ren, p2_cx, p2_cy, p2_dir, p2_anim_frame, p2_scale);

    // P2 Stat Gauges Section (y: 279 .. 384)
    SDL_Rect p2_stat_box = { p2_x + 14, p2_y + 205, p2_w - 28, 105 };
    SDL_SetRenderDrawColor(ren, 20, 12, 16, 220);
    SDL_RenderFillRect(ren, &p2_stat_box);
    SDL_SetRenderDrawColor(ren, 80, 36, 46, 200);
    SDL_RenderDrawRect(ren, &p2_stat_box);

    draw_stat_bar(ren, p2_stat_box.x + 10, p2_stat_box.y + 10, p2_stat_box.w - 20, 16,
                  "HP", p2_preview.base_stats.hp, max_ref_hp, 34, 197, 94);
    draw_stat_bar(ren, p2_stat_box.x + 10, p2_stat_box.y + 32, p2_stat_box.w - 20, 16,
                  "ATK", p2_preview.base_stats.attack, max_ref_stat, 239, 68, 68);
    draw_stat_bar(ren, p2_stat_box.x + 10, p2_stat_box.y + 54, p2_stat_box.w - 20, 16,
                  "DEF", p2_preview.base_stats.defense, max_ref_stat, 59, 130, 246);
    draw_stat_bar(ren, p2_stat_box.x + 10, p2_stat_box.y + 76, p2_stat_box.w - 20, 16,
                  "SPD", p2_preview.base_stats.speed, max_ref_stat, 234, 179, 8);

    // P2 Equipped Abilities Strip (y: 416 .. 560)
    Font::draw_text(ren, p2_x + 16, p2_y + 322, "EQUIPPED ABILITIES [CLICK SLOT TO DRAFT]", 1, 254, 240, 138);
    for (int i = 0; i < 4; ++i) {
        int sy = p2_y + 342 + i * 36;
        SDL_Rect sr = { p2_x + 14, sy, p2_w - 28, 32 };
        bool is_sel = (p2_selected_slot == i);
        bool is_hov = (mouse_x >= sr.x && mouse_x <= sr.x + sr.w &&
                       mouse_y >= sr.y && mouse_y <= sr.y + sr.h);

        SDL_SetRenderDrawColor(ren, is_sel ? 64 : (is_hov ? 48 : 32),
                                    is_sel ? 26 : (is_hov ? 20 : 16),
                                    is_sel ? 32 : (is_hov ? 24 : 20), 255);
        SDL_RenderFillRect(ren, &sr);
        SDL_SetRenderDrawColor(ren, is_sel ? 96 : (is_hov ? 60 : 45),
                                    is_sel ? 165 : (is_hov ? 120 : 65),
                                    is_sel ? 250 : (is_hov ? 180 : 90), 255);
        SDL_RenderDrawRect(ren, &sr);

        // Cycle Left Arrow Button [<] (x: sr.x + 4, y: sy + 4, w: 20, h: 24)
        SDL_Rect btn_l = { sr.x + 4, sy + 4, 20, 24 };
        bool hov_l = (mouse_x >= btn_l.x && mouse_x <= btn_l.x + btn_l.w &&
                      mouse_y >= btn_l.y && mouse_y <= btn_l.y + btn_l.h);
        SDL_SetRenderDrawColor(ren, hov_l ? 120 : 60, hov_l ? 26 : 18, hov_l ? 30 : 20, 255);
        SDL_RenderFillRect(ren, &btn_l);
        Font::draw_text(ren, btn_l.x + 6, btn_l.y + 6, "<", 1, hov_l ? 255 : 220, hov_l ? 255 : 200, hov_l ? 255 : 80);

        // Slot Number Badge
        char slot_lbl[16];
        snprintf(slot_lbl, sizeof(slot_lbl), "S%d", i + 1);
        Font::draw_text(ren, sr.x + 28, sy + 8, slot_lbl, 1, is_sel ? 96 : 148, is_sel ? 165 : 163, is_sel ? 250 : 184);

        // Move Name
        const MoveDef& mdef = MoveRegistry::get(p2_moves[i]);
        Font::draw_text(ren, sr.x + 56, sy + 8, mdef.name, 1, 255, 255, 255);

        // Element Type Pill
        Uint8 mr = 148, mg = 163, mb = 184;
        TypeChart::get_type_color(mdef.element_type, mr, mg, mb);
        Font::draw_text(ren, sr.x + 230, sy + 8, TypeChart::get_type_name(mdef.element_type), 1, mr, mg, mb);

        // Stats: Cost and Power
        char cost_str[24];
        snprintf(cost_str, sizeof(cost_str), "%dSP | P%d", mdef.stamina_cost, mdef.base_power);
        Font::draw_text(ren, sr.x + 312, sy + 8, cost_str, 1, 254, 243, 199);

        // Cycle Right Arrow Button [>] (x: sr.x + sr.w - 24, y: sy + 4, w: 20, h: 24)
        SDL_Rect btn_r = { sr.x + sr.w - 24, sy + 4, 20, 24 };
        bool hov_r = (mouse_x >= btn_r.x && mouse_x <= btn_r.x + btn_r.w &&
                      mouse_y >= btn_r.y && mouse_y <= btn_r.y + btn_r.h);
        SDL_SetRenderDrawColor(ren, hov_r ? 120 : 60, hov_r ? 26 : 18, hov_r ? 30 : 20, 255);
        SDL_RenderFillRect(ren, &btn_r);
        Font::draw_text(ren, btn_r.x + 6, btn_r.y + 6, ">", 1, hov_r ? 255 : 220, hov_r ? 255 : 200, hov_r ? 255 : 80);
    }

    // P2 Full Movepool Drafter Button (y: 568 .. 606)
    SDL_Rect p2_draft_btn = { p2_x + 14, p2_y + 494, p2_w - 28, 38 };
    bool p2_draft_hov = (mouse_x >= p2_draft_btn.x && mouse_x <= p2_draft_btn.x + p2_draft_btn.w &&
                         mouse_y >= p2_draft_btn.y && mouse_y <= p2_draft_btn.y + p2_draft_btn.h);
    SDL_SetRenderDrawColor(ren, p2_draft_hov ? 140 : 90, p2_draft_hov ? 28 : 18, p2_draft_hov ? 32 : 22, 255);
    SDL_RenderFillRect(ren, &p2_draft_btn);
    SDL_SetRenderDrawColor(ren, 250, 204, 21, p2_draft_hov ? 255 : 180);
    SDL_RenderDrawRect(ren, &p2_draft_btn);

    char p2_btn_txt[64];
    snprintf(p2_btn_txt, sizeof(p2_btn_txt), "[N] DRAFT FULL MOVESET (%d MOVES AVAILABLE)", p2_preview.movepool_count);
    Font::draw_text(ren, p2_draft_btn.x + 18, p2_draft_btn.y + 12, p2_btn_txt, 1, 254, 240, 138);

    Font::draw_text(ren, p2_x + 16, p2_y + 544, "Use [<] [>] on slots or click above to open full drafter.", 1, 203, 185, 175);

    // =========================================================================
    // 5. CENTER COLUMN: VS CREST & FIRERED START ACTION (x: 524 .. 756)
    // =========================================================================
    int c_x = 524;
    int c_w = 232;

    // Stylized GBA VS Crest with Ruby-Gold slash dividers (y: 110)
    int vs_tw = Font::get_text_width("VS", 2);
    Font::draw_text(ren, c_x + (c_w - vs_tw) / 2 + 1, 117, "VS", 2, 70, 10, 10);
    Font::draw_text(ren, c_x + (c_w - vs_tw) / 2, 116, "VS", 2, 250, 204, 21);
    SDL_SetRenderDrawColor(ren, 239, 68, 68, 200);
    SDL_RenderDrawLine(ren, c_x + 20, 126, c_x + (c_w - vs_tw) / 2 - 12, 126);
    SDL_SetRenderDrawColor(ren, 250, 204, 21, 200);
    SDL_RenderDrawLine(ren, c_x + (c_w + vs_tw) / 2 + 12, 126, c_x + c_w - 20, 126);

    // Current Mode Summary Box (Styled like FireRed Battle Rule Placard)
    SDL_Rect mode_box = { c_x + 10, 155, c_w - 20, 120 };
    SDL_SetRenderDrawColor(ren, 26, 16, 22, 240);
    SDL_RenderFillRect(ren, &mode_box);
    SDL_SetRenderDrawColor(ren, 90, 40, 50, 220);
    SDL_RenderDrawRect(ren, &mode_box);

    const char* m_str = (match_type == MATCH_TYPE_SPECTATOR) ? "[AI VS AI SHOWDOWN]" : "[PLAYER VS CPU]";
    int m_tw = Font::get_text_width(m_str, 1);
    Font::draw_text(ren, mode_box.x + (mode_box.w - m_tw) / 2, mode_box.y + 12, m_str, 1,
                    (match_type == MATCH_TYPE_SPECTATOR) ? 250 : 254,
                    (match_type == MATCH_TYPE_SPECTATOR) ? 204 : 240,
                    (match_type == MATCH_TYPE_SPECTATOR) ? 21  : 138);

    const char* ai_str = (p2_ai_mode == AI_MODE_BOSS) ? "OPPONENT: BOSS AI" :
                         ((p2_ai_mode == AI_MODE_CASUAL) ? "OPPONENT: CASUAL" : "OPPONENT: DUMMY");
    int ai_tw = Font::get_text_width(ai_str, 1);
    Font::draw_text(ren, mode_box.x + (mode_box.w - ai_tw) / 2, mode_box.y + 36, ai_str, 1, 255, 255, 255);

    const char* cam_str = dynamic_cam ? "CAM: DYNAMIC" : "CAM: FIXED 1.0X";
    const char* aim_str = (aim_mode == AIM_MODE_LOCK_ON) ? "AIM: LOCK-ON" : "AIM: FREE MOUSE";
    char cfg_buf[48];
    snprintf(cfg_buf, sizeof(cfg_buf), "%s | %s", aim_str, cam_str);
    int cfg_tw = Font::get_text_width(cfg_buf, 1);
    Font::draw_text(ren, mode_box.x + (mode_box.w - cfg_tw) / 2, mode_box.y + 60, cfg_buf, 1, 203, 185, 175);

    const char* set_hint = "CLICK [SETTINGS] TO CHANGE";
    int sh_tw = Font::get_text_width(set_hint, 1);
    Font::draw_text(ren, mode_box.x + (mode_box.w - sh_tw) / 2, mode_box.y + 92, set_hint, 1, 250, 204, 21);

    // =========================================================================
    // BUTTON 5: START BATTLE - ICONIC GBA "FIGHT!" BUTTON (y = 302 .. 358)
    // =========================================================================
    SDL_Rect start_btn = { c_x, 302, c_w, 56 };
    bool start_hover = (mouse_x >= start_btn.x && mouse_x <= start_btn.x + start_btn.w &&
                        mouse_y >= start_btn.y && mouse_y <= start_btn.y + start_btn.h);
    float pulse = sinf(anim_timer * 5.0f) * 0.5f + 0.5f;

    Uint8 s_glow_r = (Uint8)(start_hover ? 255 : (225 + pulse * 30));
    Uint8 s_glow_g = (Uint8)(start_hover ? 235 : (180 + pulse * 45));
    // Classic FireRed "FIGHT" red-orange gradient fill
    SDL_SetRenderDrawColor(ren, start_hover ? 225 : 190, start_hover ? 45 : 30, start_hover ? 40 : 25, 255);
    SDL_RenderFillRect(ren, &start_btn);

    // Golden double-beveled border
    SDL_SetRenderDrawColor(ren, s_glow_r, s_glow_g, 21, 255);
    SDL_RenderDrawRect(ren, &start_btn);
    SDL_Rect inner_sb = { start_btn.x + 2, start_btn.y + 2, start_btn.w - 4, start_btn.h - 4 };
    SDL_SetRenderDrawColor(ren, 254, 240, 138, 200);
    SDL_RenderDrawRect(ren, &inner_sb);

    const char* sb_txt = (match_type == MATCH_TYPE_SPECTATOR) ? "SPECTATE MATCH" : "START BATTLE";
    int sb_tw = Font::get_text_width(sb_txt, 2);
    // Drop shadow
    Font::draw_text(ren, start_btn.x + (start_btn.w - sb_tw) / 2 + 1, start_btn.y + 13, sb_txt, 2, 70, 10, 10);
    Font::draw_text(ren, start_btn.x + (start_btn.w - sb_tw) / 2, start_btn.y + 12, sb_txt, 2, 255, 255, 255);

    const char* sb_sub = "[ENTER / SPACE] OR CLICK";
    int sb_sw = Font::get_text_width(sb_sub, 1);
    Font::draw_text(ren, start_btn.x + (start_btn.w - sb_sw) / 2, start_btn.y + 36, sb_sub, 1, 254, 240, 138);

    // Quick [⚙ MATCH SETTINGS] Card Button (y: 374 .. 418)
    SDL_Rect mid_set_btn = { c_x, 374, c_w, 44 };
    bool mid_set_hover = (mouse_x >= mid_set_btn.x && mouse_x <= mid_set_btn.x + mid_set_btn.w &&
                          mouse_y >= mid_set_btn.y && mouse_y <= mid_set_btn.y + mid_set_btn.h);
    SDL_SetRenderDrawColor(ren, mid_set_hover ? 64 : 40, mid_set_hover ? 26 : 18, mid_set_hover ? 32 : 22, 255);
    SDL_RenderFillRect(ren, &mid_set_btn);
    SDL_SetRenderDrawColor(ren, 250, 204, 21, mid_set_hover ? 255 : 180);
    SDL_RenderDrawRect(ren, &mid_set_btn);
    Font::draw_text(ren, mid_set_btn.x + 22, mid_set_btn.y + 8, "[⚙] MATCH SETTINGS", 1, 254, 240, 138);
    Font::draw_text(ren, mid_set_btn.x + 18, mid_set_btn.y + 24, "Configure AI, Aim & Camera", 1, 203, 185, 175);

    // Quick [📊 COMBAT LOG] Card Button (y: 430 .. 474)
    if (CombatLogger::has_completed_log()) {
        SDL_Rect mid_log_btn = { c_x, 430, c_w, 44 };
        bool mid_log_hover = (mouse_x >= mid_log_btn.x && mouse_x <= mid_log_btn.x + mid_log_btn.w &&
                              mouse_y >= mid_log_btn.y && mouse_y <= mid_log_btn.y + mid_log_btn.h);
        SDL_SetRenderDrawColor(ren, mid_log_hover ? 64 : 40, mid_log_hover ? 26 : 18, mid_log_hover ? 32 : 22, 255);
        SDL_RenderFillRect(ren, &mid_log_btn);
        SDL_SetRenderDrawColor(ren, 250, 204, 21, mid_log_hover ? 255 : 180);
        SDL_RenderDrawRect(ren, &mid_log_btn);
        Font::draw_text(ren, mid_log_btn.x + 28, mid_log_btn.y + 8, "[📊] MATCH REPORT", 1, 250, 204, 21);
        Font::draw_text(ren, mid_log_btn.x + 24, mid_log_btn.y + 24, "Review Damage & Accuracy", 1, 203, 185, 175);
    }

    // Controls Mini-Cheat (y: 486 .. 656)
    SDL_Rect mini_ctrl = { c_x, 486, c_w, 170 };
    SDL_SetRenderDrawColor(ren, 20, 12, 16, 220);
    SDL_RenderFillRect(ren, &mini_ctrl);
    SDL_SetRenderDrawColor(ren, 80, 36, 46, 180);
    SDL_RenderDrawRect(ren, &mini_ctrl);

    Font::draw_text(ren, mini_ctrl.x + 16, mini_ctrl.y + 10, "BATTLE KEYBINDINGS", 1, 250, 204, 21);
    const char* mini_keys[] = {
        "WASD  - Strafe Movement",
        "LMB   - Slot 1 Melee Jab",
        "RMB   - Slot 2 Skillshot",
        "SHIFT - Slot 3 Utility / AoE",
        "F     - Slot 4 Finisher",
        "SPACE - Burst Dash (30SP)",
        "[S]   - Open Full Settings"
    };
    for (int k = 0; k < 7; ++k) {
        Font::draw_text(ren, mini_ctrl.x + 12, mini_ctrl.y + 30 + k * 19, mini_keys[k], 1, 203, 185, 175);
    }

    // =========================================================================
    // 6. BOTTOM NAVIGATION BAR
    // =========================================================================
    SDL_SetRenderDrawColor(ren, 26, 12, 16, 255);
    SDL_Rect bot_bar = { 0, screen_h - 52, screen_w, 52 };
    SDL_RenderFillRect(ren, &bot_bar);
    SDL_SetRenderDrawColor(ren, 180, 140, 60, 255);
    SDL_RenderDrawLine(ren, 0, screen_h - 52, screen_w, screen_h - 52);

    const char* nav_txt = CombatLogger::has_completed_log()
        ? "[ENTER] START  |  [C/V] CHANGE PKMN  |  [M/N] DRAFT MOVES  |  [S] SETTINGS  |  [L] MATCH LOG  |  [ESC] QUIT"
        : "[ENTER] START  |  [C/V] CHANGE PKMN  |  [M/N] DRAFT MOVES  |  [S] SETTINGS  |  [1-4] EDIT SLOTS  |  [ESC] QUIT";
    int ntw = Font::get_text_width(nav_txt, 1);
    Font::draw_text(ren, (screen_w - ntw) / 2, screen_h - 32, nav_txt, 1, 250, 204, 21);

    // =========================================================================
    // 7. DEDICATED SETTINGS WINDOW MODAL (OVERLAY WINDOW - FIRERED PC STYLE)
    // =========================================================================
    if (show_settings_modal) {
        // Dimmed backdrop
        SDL_SetRenderDrawColor(ren, 16, 8, 10, 235);
        SDL_Rect dim = { 0, 0, screen_w, screen_h };
        SDL_RenderFillRect(ren, &dim);

        // Centered Window Card (x: 480 .. 800)
        int sw_x = 480, sw_y = 60, sw_w = 320, sw_h = 580;
        SDL_SetRenderDrawColor(ren, 28, 16, 22, 255);
        SDL_Rect sw_card = { sw_x, sw_y, sw_w, sw_h };
        SDL_RenderFillRect(ren, &sw_card);
        SDL_SetRenderDrawColor(ren, 185, 28, 28, 255);
        SDL_RenderDrawRect(ren, &sw_card);
        SDL_Rect inner_sw = { sw_card.x + 2, sw_card.y + 2, sw_card.w - 4, sw_card.h - 4 };
        SDL_SetRenderDrawColor(ren, 250, 204, 21, 220);
        SDL_RenderDrawRect(ren, &inner_sw);

        // Header Title & Close Button
        Font::draw_text(ren, sw_x + 24, sw_y + 18, "MATCH RULES & SETTINGS", 1, 250, 204, 21);
        SDL_Rect close_btn = { sw_x + sw_w - 48, sw_y + 12, 34, 24 };
        bool close_hov = (mouse_x >= close_btn.x && mouse_x <= close_btn.x + close_btn.w &&
                          mouse_y >= close_btn.y && mouse_y <= close_btn.y + close_btn.h);
        SDL_SetRenderDrawColor(ren, close_hov ? 190 : 120, 25, 30, 255);
        SDL_RenderFillRect(ren, &close_btn);
        SDL_SetRenderDrawColor(ren, 250, 204, 21, 255);
        SDL_RenderDrawRect(ren, &close_btn);
        Font::draw_text(ren, close_btn.x + 10, close_btn.y + 6, "X", 1, 255, 255, 255);

        // --- BUTTON 1: MATCH MODE (y: 116 .. 156) ---
        SDL_Rect match_btn = { c_x + 14, 116, c_w - 28, 40 };
        bool match_hover = (mouse_x >= match_btn.x && mouse_x <= match_btn.x + match_btn.w &&
                            mouse_y >= match_btn.y && mouse_y <= match_btn.y + match_btn.h);
        if (match_type == MATCH_TYPE_SPECTATOR) {
            SDL_SetRenderDrawColor(ren, match_hover ? 70 : 45, match_hover ? 24 : 16, match_hover ? 60 : 38, 255);
            SDL_RenderFillRect(ren, &match_btn);
            SDL_SetRenderDrawColor(ren, 250, 204, 21, match_hover ? 255 : 200);
            SDL_RenderDrawRect(ren, &match_btn);
            Font::draw_text(ren, match_btn.x + 16, match_btn.y + 6, "[MATCH: AI VS AI]", 1, 250, 204, 21);
            Font::draw_text(ren, match_btn.x + 12, match_btn.y + 22, "[T] Spectate Autonomous Showdown", 1, 203, 185, 175);
        } else {
            SDL_SetRenderDrawColor(ren, match_hover ? 64 : 40, match_hover ? 22 : 14, match_hover ? 28 : 18, 255);
            SDL_RenderFillRect(ren, &match_btn);
            SDL_SetRenderDrawColor(ren, 239, 68, 68, match_hover ? 255 : 200);
            SDL_RenderDrawRect(ren, &match_btn);
            Font::draw_text(ren, match_btn.x + 18, match_btn.y + 6, "[MATCH: PLAYER VS CPU]", 1, 254, 240, 138);
            Font::draw_text(ren, match_btn.x + 16, match_btn.y + 22, "[T] Player 1 vs CPU Opponent", 1, 203, 185, 175);
        }

        // --- BUTTON 2: AI DIFFICULTY (y: 162 .. 202) ---
        SDL_Rect ai_btn = { c_x + 14, 162, c_w - 28, 40 };
        bool ai_hover = (mouse_x >= ai_btn.x && mouse_x <= ai_btn.x + ai_btn.w &&
                         mouse_y >= ai_btn.y && mouse_y <= ai_btn.y + ai_btn.h);
        if (match_type == MATCH_TYPE_SPECTATOR) {
            SDL_SetRenderDrawColor(ren, ai_hover ? 70 : 45, ai_hover ? 24 : 16, ai_hover ? 60 : 38, 255);
            SDL_RenderFillRect(ren, &ai_btn);
            SDL_SetRenderDrawColor(ren, 250, 204, 21, 255);
            SDL_RenderDrawRect(ren, &ai_btn);
            Font::draw_text(ren, ai_btn.x + 22, ai_btn.y + 6, "[AI: SPECTATOR CLASH]", 1, 250, 204, 21);
            Font::draw_text(ren, ai_btn.x + 18, ai_btn.y + 22, "[Y] Cycle AI Matchups", 1, 203, 185, 175);
        } else {
            if (p2_ai_mode == AI_MODE_CASUAL) {
                SDL_SetRenderDrawColor(ren, ai_hover ? 64 : 40, ai_hover ? 22 : 14, ai_hover ? 28 : 18, 255);
                SDL_RenderFillRect(ren, &ai_btn);
                SDL_SetRenderDrawColor(ren, 96, 165, 250, ai_hover ? 255 : 200);
                SDL_RenderDrawRect(ren, &ai_btn);
                Font::draw_text(ren, ai_btn.x + 18, ai_btn.y + 6, "[AI: CASUAL TRAINER]", 1, 96, 165, 250);
                Font::draw_text(ren, ai_btn.x + 14, ai_btn.y + 22, "[Y] Relaxed reaction pacing", 1, 203, 185, 175);
            } else if (p2_ai_mode == AI_MODE_BOSS) {
                SDL_SetRenderDrawColor(ren, ai_hover ? 28 : 18, ai_hover ? 64 : 40, ai_hover ? 36 : 24, 255);
                SDL_RenderFillRect(ren, &ai_btn);
                SDL_SetRenderDrawColor(ren, 34, 197, 94, ai_hover ? 255 : 200);
                SDL_RenderDrawRect(ren, &ai_btn);
                Font::draw_text(ren, ai_btn.x + 28, ai_btn.y + 6, "[AI: ELITE BOSS AI]", 1, 34, 197, 94);
                Font::draw_text(ren, ai_btn.x + 18, ai_btn.y + 22, "[Y] Dodges, weaves & kites", 1, 203, 185, 175);
            } else {
                SDL_SetRenderDrawColor(ren, ai_hover ? 54 : 36, ai_hover ? 36 : 24, ai_hover ? 42 : 28, 255);
                SDL_RenderFillRect(ren, &ai_btn);
                SDL_SetRenderDrawColor(ren, 180, 160, 160, ai_hover ? 255 : 200);
                SDL_RenderDrawRect(ren, &ai_btn);
                Font::draw_text(ren, ai_btn.x + 16, ai_btn.y + 6, "[AI: TRAINING DUMMY]", 1, 180, 160, 160);
                Font::draw_text(ren, ai_btn.x + 16, ai_btn.y + 22, "[Y] Stationary practice foe", 1, 180, 160, 160);
            }
        }

        // --- BUTTON 3: AIMING MODE (y: 208 .. 248) ---
        SDL_Rect aim_btn = { c_x + 14, 208, c_w - 28, 40 };
        bool aim_hover = (mouse_x >= aim_btn.x && mouse_x <= aim_btn.x + aim_btn.w &&
                          mouse_y >= aim_btn.y && mouse_y <= aim_btn.y + aim_btn.h);
        if (aim_mode == AIM_MODE_LOCK_ON) {
            SDL_SetRenderDrawColor(ren, aim_hover ? 64 : 40, aim_hover ? 22 : 14, aim_hover ? 28 : 18, 255);
            SDL_RenderFillRect(ren, &aim_btn);
            SDL_SetRenderDrawColor(ren, 250, 204, 21, aim_hover ? 255 : 200);
            SDL_RenderDrawRect(ren, &aim_btn);
            Font::draw_text(ren, aim_btn.x + 22, aim_btn.y + 6, "[AIM: TARGET LOCK-ON]", 1, 250, 204, 21);
            Font::draw_text(ren, aim_btn.x + 18, aim_btn.y + 22, "[Z] Auto-track foe | LOCKED", 1, 203, 185, 175);
        } else {
            SDL_SetRenderDrawColor(ren, aim_hover ? 48 : 32, aim_hover ? 24 : 16, aim_hover ? 28 : 18, 255);
            SDL_RenderFillRect(ren, &aim_btn);
            SDL_SetRenderDrawColor(ren, 180, 160, 160, aim_hover ? 240 : 180);
            SDL_RenderDrawRect(ren, &aim_btn);
            Font::draw_text(ren, aim_btn.x + 20, aim_btn.y + 6, "[AIM: FREE MOUSE AIM]", 1, 203, 185, 175);
            Font::draw_text(ren, aim_btn.x + 16, aim_btn.y + 22, "[Z] 360-deg Cursor | LOCKED", 1, 160, 140, 140);
        }

        // --- BUTTON 4: CAMERA MODE (y: 254 .. 294) ---
        SDL_Rect cam_btn = { c_x + 14, 254, c_w - 28, 40 };
        bool cam_hover = (mouse_x >= cam_btn.x && mouse_x <= cam_btn.x + cam_btn.w &&
                          mouse_y >= cam_btn.y && mouse_y <= cam_btn.y + cam_btn.h);
        if (dynamic_cam) {
            SDL_SetRenderDrawColor(ren, cam_hover ? 64 : 40, cam_hover ? 22 : 14, cam_hover ? 28 : 18, 255);
            SDL_RenderFillRect(ren, &cam_btn);
            SDL_SetRenderDrawColor(ren, 250, 204, 21, cam_hover ? 255 : 200);
            SDL_RenderDrawRect(ren, &cam_btn);
            Font::draw_text(ren, cam_btn.x + 18, cam_btn.y + 6, "[CAM: DYNAMIC ADAPTIVE]", 1, 250, 204, 21);
            Font::draw_text(ren, cam_btn.x + 14, cam_btn.y + 22, "[G] Zoom 0.8x-1.25x | LOCKED", 1, 203, 185, 175);
        } else {
            SDL_SetRenderDrawColor(ren, cam_hover ? 48 : 32, cam_hover ? 24 : 16, cam_hover ? 28 : 18, 255);
            SDL_RenderFillRect(ren, &cam_btn);
            SDL_SetRenderDrawColor(ren, 180, 160, 160, cam_hover ? 240 : 180);
            SDL_RenderDrawRect(ren, &cam_btn);
            Font::draw_text(ren, cam_btn.x + 18, cam_btn.y + 6, "[CAM: FIXED ARENA 1.0X]", 1, 203, 185, 175);
            Font::draw_text(ren, cam_btn.x + 20, cam_btn.y + 22, "[G] Classic Static | LOCKED", 1, 160, 140, 140);
        }

        // Full Controls Guide inside Settings Window (y: 310 .. 505)
        SDL_Rect sw_ctrl = { sw_x + 20, 310, sw_w - 40, 195 };
        SDL_SetRenderDrawColor(ren, 20, 12, 16, 220);
        SDL_RenderFillRect(ren, &sw_ctrl);
        SDL_SetRenderDrawColor(ren, 80, 36, 46, 180);
        SDL_RenderDrawRect(ren, &sw_ctrl);

        Font::draw_text(ren, sw_ctrl.x + 12, sw_ctrl.y + 8, "SYSTEM CONTROLS & RULES", 1, 250, 204, 21);
        const char* sw_rules[] = {
            "W/A/S/D - 8-Directional Movement",
            "LMB / 1 - Primary Melee Strike",
            "RMB / 2 - Special Elemental Move",
            "SHIFT/3 - Utility / AoE Trap",
            "F Key/4 - High-Impact Finisher",
            "SPACE   - Dash (30 Stamina Cost)",
            "R Key   - Rematch Immediately",
            "ESC     - Return to Menu / Exit"
        };
        for (int k = 0; k < 8; ++k) {
            Font::draw_text(ren, sw_ctrl.x + 12, sw_ctrl.y + 28 + k * 20, sw_rules[k], 1, 220, 200, 190);
        }

        // Close / Apply Button (y: 520 .. 558)
        SDL_Rect apply_btn = { sw_x + 20, 520, sw_w - 40, 38 };
        bool apply_hov = (mouse_x >= apply_btn.x && mouse_x <= apply_btn.x + apply_btn.w &&
                          mouse_y >= apply_btn.y && mouse_y <= apply_btn.y + apply_btn.h);
        SDL_SetRenderDrawColor(ren, apply_hov ? 34 : 20, apply_hov ? 90 : 60, apply_hov ? 45 : 30, 255);
        SDL_RenderFillRect(ren, &apply_btn);
        SDL_SetRenderDrawColor(ren, 34, 197, 94, apply_hov ? 255 : 200);
        SDL_RenderDrawRect(ren, &apply_btn);
        const char* ap_txt = "APPLY & CLOSE [ESC]";
        int ap_tw = Font::get_text_width(ap_txt, 1);
        Font::draw_text(ren, apply_btn.x + (apply_btn.w - ap_tw) / 2, apply_btn.y + 12, ap_txt, 1, 34, 197, 94);
    }

    // =========================================================================
    // 8. DEDICATED MOVEPOOL DRAFTER MODAL (drafter_target: 1 = P1, 2 = P2)
    // =========================================================================
    if (drafter_target == 1 || drafter_target == 2) {
        // Dimmed frosted backdrop
        SDL_SetRenderDrawColor(ren, 16, 8, 10, 240);
        SDL_Rect dim = { 0, 0, screen_w, screen_h };
        SDL_RenderFillRect(ren, &dim);

        // Centered Window Card (x: 140, y: 36, w: 1000, h: 648)
        int mw_x = 140, mw_y = 36, mw_w = 1000, mw_h = 648;
        SDL_SetRenderDrawColor(ren, 26, 16, 22, 255);
        SDL_Rect mw_card = { mw_x, mw_y, mw_w, mw_h };
        SDL_RenderFillRect(ren, &mw_card);

        bool is_p1 = (drafter_target == 1);
        SDL_SetRenderDrawColor(ren, is_p1 ? 185 : 37, is_p1 ? 28 : 99, is_p1 ? 28 : 235, 255);
        SDL_RenderDrawRect(ren, &mw_card);
        SDL_Rect inner_mw = { mw_card.x + 2, mw_card.y + 2, mw_card.w - 4, mw_card.h - 4 };
        SDL_SetRenderDrawColor(ren, 250, 204, 21, 220);
        SDL_RenderDrawRect(ren, &inner_mw);

        const Fighter& target_preview = is_p1 ? p1_preview : p2_preview;
        PokemonType pk_type = is_p1 ? p1_type : p2_type;
        const MoveId* active_moves = is_p1 ? p1_moves : p2_moves;

        // Header Title Banner & Subtitle
        char title_buf[80];
        snprintf(title_buf, sizeof(title_buf), "%s MOVESET DRAFTER - %s",
                 is_p1 ? "PLAYER 1" : "OPPONENT",
                 get_pokemon_name(pk_type));
        Font::draw_text(ren, mw_x + 24, mw_y + 14, title_buf, 1, 250, 204, 21);

        char sub_buf[128];
        snprintf(sub_buf, sizeof(sub_buf), "Select a slot tab below, then click any of the %d learnable moves to equip. Duplicates swap automatically.", target_preview.movepool_count);
        Font::draw_text(ren, mw_x + 24, mw_y + 32, sub_buf, 1, 203, 185, 175);

        // Close Button [X] (x: mw_x + mw_w - 46, y: mw_y + 12, w: 34, h: 24)
        SDL_Rect close_btn = { mw_x + mw_w - 46, mw_y + 12, 34, 24 };
        bool close_hov = (mouse_x >= close_btn.x && mouse_x <= close_btn.x + close_btn.w &&
                          mouse_y >= close_btn.y && mouse_y <= close_btn.y + close_btn.h);
        SDL_SetRenderDrawColor(ren, close_hov ? 190 : 120, 25, 30, 255);
        SDL_RenderFillRect(ren, &close_btn);
        SDL_SetRenderDrawColor(ren, 250, 204, 21, 255);
        SDL_RenderDrawRect(ren, &close_btn);
        Font::draw_text(ren, close_btn.x + 12, close_btn.y + 6, "X", 1, 255, 255, 255);

        // --- TOP SECTION: 4 ACTIVE EQUIPPED SLOTS TABS (y: mw_y + 52 .. mw_y + 104) ---
        int tab_margin = 20;
        int tab_spacing = 10;
        int tab_w = (mw_w - tab_margin * 2 - tab_spacing * 3) / 4;
        int tab_y = mw_y + 52;
        int tab_h = 52;

        for (int i = 0; i < 4; ++i) {
            int tx = mw_x + tab_margin + i * (tab_w + tab_spacing);
            SDL_Rect tr = { tx, tab_y, tab_w, tab_h };
            bool is_curr = (drafter_slot == i);
            bool tab_hov = (mouse_x >= tr.x && mouse_x <= tr.x + tr.w &&
                            mouse_y >= tr.y && mouse_y <= tr.y + tr.h);

            if (is_curr) {
                SDL_SetRenderDrawColor(ren, is_p1 ? 70 : 45, is_p1 ? 26 : 22, is_p1 ? 32 : 55, 255);
            } else {
                SDL_SetRenderDrawColor(ren, tab_hov ? 48 : 32, tab_hov ? 20 : 14, tab_hov ? 24 : 18, 255);
            }
            SDL_RenderFillRect(ren, &tr);

            if (is_curr) {
                SDL_SetRenderDrawColor(ren, 250, 204, 21, 255); // Golden highlight for target slot
            } else {
                SDL_SetRenderDrawColor(ren, is_p1 ? 185 : 37, is_p1 ? 28 : 99, is_p1 ? 28 : 235, tab_hov ? 220 : 120);
            }
            SDL_RenderDrawRect(ren, &tr);

            char s_hdr[32];
            snprintf(s_hdr, sizeof(s_hdr), "SLOT %d %s", i + 1, is_curr ? "[TARGET]" : "");
            Font::draw_text(ren, tr.x + 8, tr.y + 6, s_hdr, 1, is_curr ? 250 : 200, is_curr ? 204 : 170, is_curr ? 21 : 120);

            const MoveDef& cur_m = MoveRegistry::get(active_moves[i]);
            Font::draw_text(ren, tr.x + 8, tr.y + 22, cur_m.name, 1, 255, 255, 255);

            Uint8 tr_r = 148, tr_g = 163, tr_b = 184;
            TypeChart::get_type_color(cur_m.element_type, tr_r, tr_g, tr_b);
            char inf[32];
            snprintf(inf, sizeof(inf), "%s | %dSP", TypeChart::get_type_name(cur_m.element_type), cur_m.stamina_cost);
            Font::draw_text(ren, tr.x + 8, tr.y + 36, inf, 1, tr_r, tr_g, tr_b);
        }

        // Horizontal Separator
        SDL_SetRenderDrawColor(ren, 80, 36, 46, 255);
        SDL_RenderDrawLine(ren, mw_x + 20, mw_y + 114, mw_x + mw_w - 20, mw_y + 114);

        // --- CENTER SECTION: COMPLETE LEARNABLE MOVEPOOL GRID ---
        int grid_y = mw_y + 122;
        int grid_col_w = (mw_w - 40 - 16) / 2;
        int card_h = 84;
        int card_gap = 8;

        for (int k = 0; k < target_preview.movepool_count; ++k) {
            MoveId mid = target_preview.movepool[k];
            const MoveDef& m = MoveRegistry::get(mid);

            int col = k % 2;
            int row = k / 2;
            int cx = mw_x + 20 + col * (grid_col_w + 16);
            int cy = grid_y + row * (card_h + card_gap);
            SDL_Rect cr = { cx, cy, grid_col_w, card_h };

            bool card_hov = (mouse_x >= cr.x && mouse_x <= cr.x + cr.w &&
                             mouse_y >= cr.y && mouse_y <= cr.y + cr.h);

            // Determine equipped status
            int equipped_slot = -1;
            for (int s = 0; s < 4; ++s) {
                if (active_moves[s] == mid) {
                    equipped_slot = s;
                    break;
                }
            }

            bool in_current_slot = (equipped_slot == drafter_slot);
            bool in_other_slot = (equipped_slot >= 0 && !in_current_slot);

            if (in_current_slot) {
                SDL_SetRenderDrawColor(ren, 24, 48, 32, 255);
                SDL_RenderFillRect(ren, &cr);
                SDL_SetRenderDrawColor(ren, 34, 197, 94, 255);
                SDL_RenderDrawRect(ren, &cr);
            } else if (in_other_slot) {
                SDL_SetRenderDrawColor(ren, card_hov ? 48 : 34, card_hov ? 36 : 24, card_hov ? 20 : 14, 255);
                SDL_RenderFillRect(ren, &cr);
                SDL_SetRenderDrawColor(ren, 250, 204, 21, card_hov ? 240 : 140);
                SDL_RenderDrawRect(ren, &cr);
            } else {
                SDL_SetRenderDrawColor(ren, card_hov ? 48 : 32, card_hov ? 24 : 16, card_hov ? 28 : 20, 255);
                SDL_RenderFillRect(ren, &cr);
                SDL_SetRenderDrawColor(ren, 185, 28, 28, card_hov ? 220 : 90);
                SDL_RenderDrawRect(ren, &cr);
            }

            // Line 1: Move Name + Type Badge + Category + Status Badge
            Font::draw_text(ren, cr.x + 10, cr.y + 8, m.name, 1, 255, 255, 255);

            Uint8 er = 148, eg = 163, eb = 184;
            TypeChart::get_type_color(m.element_type, er, eg, eb);
            SDL_Rect type_pill = { cr.x + 150, cr.y + 6, 75, 18 };
            SDL_SetRenderDrawColor(ren, er / 4, eg / 4, eb / 4, 200);
            SDL_RenderFillRect(ren, &type_pill);
            SDL_SetRenderDrawColor(ren, er, eg, eb, 220);
            SDL_RenderDrawRect(ren, &type_pill);
            Font::draw_text(ren, type_pill.x + 6, type_pill.y + 4, TypeChart::get_type_name(m.element_type), 1, er, eg, eb);

            const char* cat_str = (m.category == CATEGORY_PHYSICAL) ? "PHYS" : ((m.category == CATEGORY_SPECIAL) ? "SPEC" : "STAT");
            Font::draw_text(ren, cr.x + 235, cr.y + 8, cat_str, 1, 203, 185, 175);

            // Status Badge on right
            if (in_current_slot) {
                Font::draw_text(ren, cr.x + cr.w - 145, cr.y + 8, "[EQUIPPED: TARGET]", 1, 34, 197, 94);
            } else if (in_other_slot) {
                char swp_buf[32];
                snprintf(swp_buf, sizeof(swp_buf), "[SLOT %d: SWAP]", equipped_slot + 1);
                Font::draw_text(ren, cr.x + cr.w - 120, cr.y + 8, swp_buf, 1, 250, 204, 21);
            } else {
                Font::draw_text(ren, cr.x + cr.w - 110, cr.y + 8, "[CLICK TO EQUIP]", 1, card_hov ? 250 : 160, card_hov ? 204 : 140, card_hov ? 21 : 140);
            }

            // Line 2: Stat Block (Power, SP Cost, Cooldown, Range)
            char stat_line[64];
            if (m.base_power > 0) {
                snprintf(stat_line, sizeof(stat_line), "Pwr: %d  |  Cost: %d SP  |  CD: %.1fs  |  MaxR: %dpx",
                         m.base_power, m.stamina_cost, m.cooldown, (int)m.max_range);
            } else {
                snprintf(stat_line, sizeof(stat_line), "Buff/Utility  |  Cost: %d SP  |  CD: %.1fs",
                         m.stamina_cost, m.cooldown);
            }
            Font::draw_text(ren, cr.x + 10, cr.y + 30, stat_line, 1, 254, 243, 199);

            // Line 3: Anime Tactical Description
            Font::draw_text(ren, cr.x + 10, cr.y + 48, get_move_description(mid), 1, 203, 185, 175);
        }

        // --- FOOTER BAR (y: mw_y + 588 .. mw_y + 636) ---
        SDL_Rect reset_btn = { mw_x + 20, mw_y + 592, 220, 38 };
        bool reset_hov = (mouse_x >= reset_btn.x && mouse_x <= reset_btn.x + reset_btn.w &&
                          mouse_y >= reset_btn.y && mouse_y <= reset_btn.y + reset_btn.h);
        SDL_SetRenderDrawColor(ren, reset_hov ? 70 : 45, reset_hov ? 24 : 16, reset_hov ? 28 : 20, 255);
        SDL_RenderFillRect(ren, &reset_btn);
        SDL_SetRenderDrawColor(ren, 239, 68, 68, reset_hov ? 255 : 180);
        SDL_RenderDrawRect(ren, &reset_btn);
        Font::draw_text(ren, reset_btn.x + 18, reset_btn.y + 12, "[RESET DEFAULT MOVES]", 1, 239, 68, 68);

        SDL_Rect confirm_btn = { mw_x + mw_w - 240, mw_y + 592, 220, 38 };
        bool conf_hov = (mouse_x >= confirm_btn.x && mouse_x <= confirm_btn.x + confirm_btn.w &&
                         mouse_y >= confirm_btn.y && mouse_y <= confirm_btn.y + confirm_btn.h);
        SDL_SetRenderDrawColor(ren, conf_hov ? 34 : 20, conf_hov ? 90 : 60, conf_hov ? 45 : 30, 255);
        SDL_RenderFillRect(ren, &confirm_btn);
        SDL_SetRenderDrawColor(ren, 34, 197, 94, conf_hov ? 255 : 180);
        SDL_RenderDrawRect(ren, &confirm_btn);
        Font::draw_text(ren, confirm_btn.x + 28, confirm_btn.y + 12, "[CONFIRM & CLOSE]", 1, 34, 197, 94);

        const char* d_hint = "[1-4] Switch Slot  |  Click Move to Equip/Swap  |  [ESC/ENTER] Done";
        int dht_w = Font::get_text_width(d_hint, 1);
        Font::draw_text(ren, mw_x + (mw_w - dht_w) / 2, mw_y + 604, d_hint, 1, 254, 243, 199);
    }

    // =========================================================================
    // 9. LAST MATCH REVIEW MODAL OVERLAY
    // =========================================================================
    if (show_log_modal && CombatLogger::has_completed_log()) {
        render_post_match_analytics(ren, CombatLogger::get_last_completed_log(), screen_w, screen_h, true);
    }
}
