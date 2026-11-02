#include "ui.hpp"
#include "font.hpp"
#include "combat_log.hpp"
#include <cmath>
#include <cstdio>

void UI::render_post_match_analytics(
    SDL_Renderer* ren,
    const MatchLog& log,
    int screen_w, int screen_h,
    bool in_menu
) {
    if (!log.valid) return;

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    // Dimmed full-screen backdrop
    SDL_SetRenderDrawColor(ren, 5, 8, 16, 225);
    SDL_Rect full_dim = { 0, 0, screen_w, screen_h };
    SDL_RenderFillRect(ren, &full_dim);

    // Main Card: 920px wide, 520px high
    int card_w = 920;
    int card_h = 520;
    int card_x = (screen_w - card_w) / 2;
    int card_y = (screen_h - card_h) / 2;

    // Card Background
    SDL_SetRenderDrawColor(ren, 13, 19, 33, 252);
    SDL_Rect card_rect = { card_x, card_y, card_w, card_h };
    SDL_RenderFillRect(ren, &card_rect);

    // Border: Winner tint
    Uint8 border_r = 56, border_g = 189, border_b = 248; // Cyan default
    if (in_menu) {
        border_r = 56; border_g = 189; border_b = 248;
    } else if (log.winner_id == 0) {
        border_r = 250; border_g = 204; border_b = 21; // Gold
    } else if (log.winner_id == 1) {
        border_r = (log.match_type == MATCH_TYPE_SPECTATOR) ? 250 : 239;
        border_g = (log.match_type == MATCH_TYPE_SPECTATOR) ? 204 : 68;
        border_b = (log.match_type == MATCH_TYPE_SPECTATOR) ? 21 : 68;
    }

    SDL_SetRenderDrawColor(ren, border_r, border_g, border_b, 255);
    SDL_RenderDrawRect(ren, &card_rect);
    SDL_Rect inner_border = { card_x + 2, card_y + 2, card_w - 4, card_h - 4 };
    SDL_RenderDrawRect(ren, &inner_border);

    // Top Header Banner
    SDL_SetRenderDrawColor(ren, 19, 27, 46, 255);
    SDL_Rect hdr_rect = { card_x + 3, card_y + 3, card_w - 6, 72 };
    SDL_RenderFillRect(ren, &hdr_rect);
    SDL_SetRenderDrawColor(ren, 30, 41, 59, 255);
    SDL_RenderDrawLine(ren, card_x + 3, card_y + 75, card_x + card_w - 4, card_y + 75);

    // Title
    char title_buf[64];
    if (in_menu) {
        snprintf(title_buf, sizeof(title_buf), "PREVIOUS MATCH COMBAT ANALYTICS");
    } else if (log.winner_id == 0) {
        if (log.match_type == MATCH_TYPE_SPECTATOR) {
            snprintf(title_buf, sizeof(title_buf), "P1 %s WINS!", TypeChart::get_species_name(log.p1.pokemon_type));
        } else {
            snprintf(title_buf, sizeof(title_buf), "VICTORY!");
        }
    } else if (log.winner_id == 1) {
        if (log.match_type == MATCH_TYPE_SPECTATOR) {
            snprintf(title_buf, sizeof(title_buf), "P2 %s WINS!", TypeChart::get_species_name(log.p2.pokemon_type));
        } else {
            snprintf(title_buf, sizeof(title_buf), "DEFEAT!");
        }
    } else {
        snprintf(title_buf, sizeof(title_buf), "MATCH TIED");
    }

    int tw = Font::get_text_width(title_buf, 2);
    Font::draw_text(ren, card_x + (card_w - tw) / 2, card_y + 14, title_buf, 2, border_r, border_g, border_b);

    // Subtitle: Match Duration & Finisher Move
    int dur = (int)log.match_duration;
    int mins = dur / 60;
    int secs = dur % 60;
    char sub_buf[128];
    if (log.finisher_move != MOVE_NONE && log.finisher_name[0] != '\0') {
        snprintf(sub_buf, sizeof(sub_buf), "TIME %02d:%02d  |  FINISHER: %s", mins, secs, log.finisher_name);
    } else {
        snprintf(sub_buf, sizeof(sub_buf), "TIME %02d:%02d  |  TOTAL DAMAGE DEALT: %d", mins, secs, log.p1.total_damage_dealt + log.p2.total_damage_dealt);
    }
    int sw = Font::get_text_width(sub_buf, 1);
    Font::draw_text(ren, card_x + (card_w - sw) / 2, card_y + 48, sub_buf, 1, 203, 213, 225);

    // Center divider
    SDL_SetRenderDrawColor(ren, 30, 41, 59, 255);
    SDL_RenderDrawLine(ren, card_x + card_w / 2, card_y + 75, card_x + card_w / 2, card_y + card_h - 42);

    // Two Fighter Columns
    int col_w = (card_w - 44) / 2; // ~438px
    int col1_x = card_x + 14;
    int col2_x = card_x + card_w / 2 + 8;
    int content_y = card_y + 86;

    auto render_side = [&](const FighterCombatLog& f, int cx, bool is_winner) {
        const char* p_name = TypeChart::get_species_name(f.pokemon_type);
        char badge[64];
        if (log.match_type == MATCH_TYPE_SPECTATOR) {
            snprintf(badge, sizeof(badge), "%s (CPU)", p_name);
        } else if (f.is_player) {
            snprintf(badge, sizeof(badge), "%s (PLAYER)", p_name);
        } else {
            const char* ai_lvl = (f.ai_mode == AI_MODE_CASUAL) ? "CASUAL" : ((f.ai_mode == AI_MODE_DUMMY) ? "DUMMY" : "BOSS");
            snprintf(badge, sizeof(badge), "%s (CPU %s)", p_name, ai_lvl);
        }

        // Fighter Header Bar
        SDL_SetRenderDrawColor(ren, 19, 27, 46, 255);
        SDL_Rect fhdr = { cx, content_y, col_w, 26 };
        SDL_RenderFillRect(ren, &fhdr);
        if (is_winner) {
            SDL_SetRenderDrawColor(ren, 250, 204, 21, 200);
            SDL_RenderDrawRect(ren, &fhdr);
        }

        Font::draw_text(ren, cx + 8, content_y + 6, badge, 1, is_winner ? 250 : 226, is_winner ? 204 : 232, is_winner ? 21 : 240);
        if (is_winner) {
            Font::draw_text(ren, cx + col_w - 74, content_y + 6, "[WINNER]", 1, 250, 204, 21);
        }

        // Summary Metric Pills
        int max_hit = 0;
        for (int i = 0; i < f.move_count; ++i) {
            if (f.moves[i].max_single_hit > max_hit) max_hit = f.moves[i].max_single_hit;
        }

        char s1[64], s2[64];
        snprintf(s1, sizeof(s1), "TOTAL DMG: %d    MAX HIT: %d", f.total_damage_dealt, max_hit);
        snprintf(s2, sizeof(s2), "ACCURACY: %.0f%%    PARRIES: %d", f.get_accuracy(), f.parries_performed);
        Font::draw_text(ren, cx + 8, content_y + 34, s1, 1, 56, 189, 248);
        Font::draw_text(ren, cx + 8, content_y + 49, s2, 1, 148, 163, 184);

        // Clones or Wall Splat info if any
        int extra_y = content_y + 64;
        if (f.clones_spawned > 0 || f.wall_splat_damage > 0) {
            char ex[64] = {0};
            if (f.clones_spawned > 0 && f.wall_splat_damage > 0) {
                snprintf(ex, sizeof(ex), "CLONES: %d (%d POPPED) | WALL: %d DMG", f.clones_spawned, f.clones_popped, f.wall_splat_damage);
            } else if (f.clones_spawned > 0) {
                snprintf(ex, sizeof(ex), "DOUBLE TEAM: %d CLONES (%d POPPED)", f.clones_spawned, f.clones_popped);
            } else {
                snprintf(ex, sizeof(ex), "WALL SPLAT IMPACT: %d DMG", f.wall_splat_damage);
            }
            Font::draw_text(ren, cx + 8, extra_y, ex, 1, 251, 191, 36);
            extra_y += 14;
        }

        // Table Header
        int table_y = extra_y + 6;
        SDL_SetRenderDrawColor(ren, 15, 23, 42, 255);
        SDL_Rect tbl_hdr = { cx, table_y, col_w, 20 };
        SDL_RenderFillRect(ren, &tbl_hdr);

        // Columns: MOVE (cx+6), CAST (cx+180), HIT (cx+230), ACC% (cx+276), DMG (cx+336)
        Font::draw_text(ren, cx + 6, table_y + 4, "MOVE", 1, 148, 163, 184);
        Font::draw_text(ren, cx + 180, table_y + 4, "CAST", 1, 148, 163, 184);
        Font::draw_text(ren, cx + 230, table_y + 4, "HIT", 1, 148, 163, 184);
        Font::draw_text(ren, cx + 276, table_y + 4, "ACC%", 1, 148, 163, 184);
        Font::draw_text(ren, cx + 336, table_y + 4, "DMG", 1, 148, 163, 184);

        int row_y = table_y + 24;
        for (int i = 0; i < f.move_count && i < 6; ++i) {
            const MoveLogEntry& m = f.moves[i];
            if (m.move_id == MOVE_NONE) continue;
            const MoveDef& def = MoveRegistry::get(m.move_id);

            // Row background alternating
            if (i % 2 == 1) {
                SDL_SetRenderDrawColor(ren, 15, 23, 42, 140);
                SDL_Rect r_bg = { cx, row_y - 2, col_w, 26 };
                SDL_RenderFillRect(ren, &r_bg);
            }

            // Move Name (truncated or formatted)
            char m_name[20];
            snprintf(m_name, sizeof(m_name), "%s", def.name);
            Uint8 mr = 241, mg = 245, mb = 249;
            if (m.is_finisher) {
                mr = 250; mg = 204; mb = 21; // Gold for K.O. finisher
            }
            Font::draw_text(ren, cx + 6, row_y, m_name, 1, mr, mg, mb);
            if (m.is_finisher) {
                Font::draw_text(ren, cx + 140, row_y, "*KO*", 1, 250, 204, 21);
            }

            // Casts
            char c_buf[8];
            snprintf(c_buf, sizeof(c_buf), "%d", m.uses);
            Font::draw_text(ren, cx + 188, row_y, c_buf, 1, 203, 213, 225);

            // Hits
            char h_buf[8];
            snprintf(h_buf, sizeof(h_buf), "%d", m.hits);
            Font::draw_text(ren, cx + 236, row_y, h_buf, 1, 203, 213, 225);

            // Accuracy %
            int acc = (m.uses > 0) ? (m.successful_casts * 100 / m.uses) : 0;
            if (acc > 100) acc = 100;
            char a_buf[8];
            snprintf(a_buf, sizeof(a_buf), "%d%%", acc);
            Font::draw_text(ren, cx + 280, row_y, a_buf, 1, acc >= 75 ? 52 : (acc >= 40 ? 250 : 239),
                                                          acc >= 75 ? 211 : (acc >= 40 ? 204 : 68),
                                                          acc >= 75 ? 153 : (acc >= 40 ? 21 : 68));

            // Damage
            char d_buf[16];
            snprintf(d_buf, sizeof(d_buf), "%d", m.total_damage);
            Font::draw_text(ren, cx + 336, row_y, d_buf, 1, 250, 204, 21);

            // Mini Damage Share Meter Bar below row
            if (f.total_damage_dealt > 0 && m.total_damage > 0) {
                int max_bar_w = 390;
                int bar_w = (m.total_damage * max_bar_w) / f.total_damage_dealt;
                if (bar_w < 3) bar_w = 3;
                if (bar_w > max_bar_w) bar_w = max_bar_w;

                SDL_SetRenderDrawColor(ren, 56, 189, 248, 160);
                if (m.is_finisher) SDL_SetRenderDrawColor(ren, 250, 204, 21, 220);
                SDL_Rect bar_r = { cx + 6, row_y + 13, bar_w, 3 };
                SDL_RenderFillRect(ren, &bar_r);
            }

            row_y += 28;
        }
    };

    render_side(log.p1, col1_x, log.winner_id == 0);
    render_side(log.p2, col2_x, log.winner_id == 1);

    // Bottom Navigation Hint Footer
    SDL_SetRenderDrawColor(ren, 19, 27, 46, 255);
    SDL_Rect ftr_rect = { card_x + 3, card_y + card_h - 40, card_w - 6, 37 };
    SDL_RenderFillRect(ren, &ftr_rect);
    SDL_SetRenderDrawColor(ren, 30, 41, 59, 255);
    SDL_RenderDrawLine(ren, card_x + 3, card_y + card_h - 40, card_x + card_w - 4, card_y + card_h - 40);

    const char* ftr_txt = in_menu
        ? "[ESC] OR [L] CLOSE REVIEW  |  DETAILED LOG SAVED TO battle_log.txt"
        : "[R] REMATCH  |  [ESC] START MENU  |  DETAILED LOG SAVED TO battle_log.txt";
    int fw = Font::get_text_width(ftr_txt, 1);
    Font::draw_text(ren, card_x + (card_w - fw) / 2, card_y + card_h - 26, ftr_txt, 1, 250, 204, 21);
}
