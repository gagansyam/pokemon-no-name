#include "fighter.hpp"
#include "projectile.hpp"
#include "arena.hpp"
#include "font.hpp"
#include <cmath>
#include <cstdlib>

static void fill_circle(SDL_Renderer* ren, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; ++dy) {
        int dx = (int)sqrtf((float)(radius * radius - dy * dy));
        SDL_RenderDrawLine(ren, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static void draw_circle_outline(SDL_Renderer* ren, int cx, int cy, int radius, int thickness) {
    for (int t = 0; t < thickness; ++t) {
        int r = radius - t;
        if (r < 1) continue;
        int x = r;
        int y = 0;
        int err = 0;
        while (x >= y) {
            SDL_RenderDrawPoint(ren, cx + x, cy + y);
            SDL_RenderDrawPoint(ren, cx + y, cy + x);
            SDL_RenderDrawPoint(ren, cx - y, cy + x);
            SDL_RenderDrawPoint(ren, cx - x, cy + y);
            SDL_RenderDrawPoint(ren, cx - x, cy - y);
            SDL_RenderDrawPoint(ren, cx - y, cy - x);
            SDL_RenderDrawPoint(ren, cx + y, cy - x);
            SDL_RenderDrawPoint(ren, cx + x, cy - y);
            if (err <= 0) {
                y += 1;
                err += 2 * y + 1;
            }
            if (err > 0) {
                x -= 1;
                err -= 2 * x + 1;
            }
        }
    }
}

void Fighter::render(SDL_Renderer* ren, float cam_x, float cam_y) {
    if (is_bench) return; // Do not render bench fighters

    int sx = (int)(x - cam_x);
    int sy = (int)(y - cam_y);
    int draw_sy = sy - (int)z;

    int alpha = (invuln_timer > 0.0f && ((int)(invuln_timer * 30.0f) & 1)) ? 100 : 255;
    float scale = (type == PKMN_PIKACHU) ? 2.2f : ((type == PKMN_CHARIZARD) ? 2.6f : ((type == PKMN_GRENINJA) ? 2.3f : 2.4f));

    bool use_iron_tail = (primed_move == MOVE_IRON_TAIL || active_move == MOVE_IRON_TAIL || iron_tail_primed || iron_tail_striking);
    bool use_shoot = ((active_move == MOVE_WATER_SHURIKEN || active_move == MOVE_HYDRO_PUMP) && shoot_sheet.texture != nullptr);
    const SpriteSheet& cur_walk = (use_iron_tail && iron_tail_walk_sheet.texture) ? iron_tail_walk_sheet : walk_sheet;
    const SpriteSheet& cur_attack = (use_iron_tail && iron_tail_attack_sheet.texture) ? iron_tail_attack_sheet
                                  : (use_shoot ? shoot_sheet : attack_sheet);

    // =========================================================================
    // 0a. GROUND DROP SHADOW & AERIAL ALTITUDE TARGET RING
    // =========================================================================
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    if (z > 1.0f) {
        float height_ratio = z / 45.0f;
        int shadow_rx = (int)(radius * 0.95f * (1.0f - height_ratio * 0.20f));
        int shadow_ry = (int)(radius * 0.45f * (1.0f - height_ratio * 0.20f));
        int shadow_alpha = (int)(120.0f * (1.0f - height_ratio * 0.30f));
        if (shadow_rx < 6) shadow_rx = 6;
        if (shadow_ry < 3) shadow_ry = 3;
        if (shadow_alpha < 20) shadow_alpha = 20;

        SDL_SetRenderDrawColor(ren, 15, 23, 42, shadow_alpha);
        for (int dy = -shadow_ry; dy <= shadow_ry; ++dy) {
            float row_pct = (float)(dy * dy) / (float)(shadow_ry * shadow_ry);
            if (row_pct > 1.0f) continue;
            int span_w = (int)((float)shadow_rx * sqrtf(1.0f - row_pct));
            SDL_RenderDrawLine(ren, sx - span_w, sy + dy, sx + span_w, sy + dy);
        }

        // Cyan Aerial Target Ring for high altitude flight (Staraptor, Ninjask, Gliscor)
        if (z >= 25.0f) {
            int ring_r = (int)radius + 8;
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 140);
            for (int a = 0; a < 360; a += 15) {
                float rad = (float)a * 0.0174533f;
                int rx = sx + (int)(cosf(rad) * (float)ring_r);
                int ry = sy + (int)(sinf(rad) * ((float)ring_r * 0.5f));
                SDL_RenderDrawPoint(ren, rx, ry);
            }
            // Crosshair pips
            SDL_RenderDrawLine(ren, sx - ring_r - 4, sy, sx - ring_r + 2, sy);
            SDL_RenderDrawLine(ren, sx + ring_r - 2, sy, sx + ring_r + 4, sy);
        }
    } else {
        // Standard grounded shadow
        SDL_SetRenderDrawColor(ren, 15, 23, 42, 110);
        int shadow_rx = (int)(radius * 0.95f);
        int shadow_ry = (int)(radius * 0.45f);
        for (int dy = -shadow_ry; dy <= shadow_ry; ++dy) {
            float row_pct = (float)(dy * dy) / (float)(shadow_ry * shadow_ry);
            if (row_pct > 1.0f) continue;
            int span_w = (int)((float)shadow_rx * sqrtf(1.0f - row_pct));
            SDL_RenderDrawLine(ren, sx - span_w, sy + dy, sx + span_w, sy + dy);
        }
    }

    // =========================================================================
    // 0b. DASH AFTERIMAGE GHOST TRAILS
    // =========================================================================
    const SpriteSheet& ghost_sheet = (is_flying && fly_sheet.texture) ? fly_sheet : cur_walk;
    if (ghost_sheet.texture) {
        for (int i = 0; i < 8; ++i) {
            if (dash_ghosts[i].alpha > 0.0f) {
                int gx = (int)(dash_ghosts[i].x - cam_x);
                int gy = (int)(dash_ghosts[i].y - cam_y);
                ghost_sheet.draw(ren, dash_ghosts[i].anim_frame, dash_ghosts[i].pmd_dir, gx, gy, scale, (int)dash_ghosts[i].alpha, dash_ghosts[i].r, dash_ghosts[i].g, dash_ghosts[i].b);
            }
        }
    }

    // =========================================================================
    // 0c. DOUBLE TEAM OPTICAL ILLUSION CLONES
    // =========================================================================
    if (double_team_timer > 0.0f && cur_walk.texture) {
        for (int i = 0; i < MAX_CLONES; ++i) {
            if (!clones[i].active) continue;
            int cx = (int)(clones[i].x - cam_x);
            int cy = (int)(clones[i].y - cam_y);

            // Translucent ground shadow underneath clone
            SDL_SetRenderDrawColor(ren, 15, 23, 42, (Uint8)(clones[i].alpha * 0.35f));
            int shadow_rx = (int)(radius * 0.85f);
            int shadow_ry = (int)(radius * 0.40f);
            for (int dy = -shadow_ry; dy <= shadow_ry; ++dy) {
                float row_pct = (float)(dy * dy) / (float)(shadow_ry * shadow_ry);
                if (row_pct > 1.0f) continue;
                int span_w = (int)((float)shadow_rx * sqrtf(1.0f - row_pct));
                SDL_RenderDrawLine(ren, cx - span_w, cy + dy, cx + span_w, cy + dy);
            }

            // Draw shimmering illusion clone sprite
            cur_walk.draw(ren, clones[i].anim_frame, clones[i].pmd_dir, cx, cy, scale, (int)clones[i].alpha);
        }
    }

    // =========================================================================
    // 1. INFERNAPE BLAZE MODE AURA (Radiant Flaming Corona)
    // =========================================================================
    if (blaze_active) {
        float b_time = anim_timer * 8.0f;
        // Inner gold halo
        SDL_SetRenderDrawColor(ren, 250, 204, 21, 90);
        fill_circle(ren, sx, draw_sy, (int)(radius * 1.35f));

        // Outer flickering flame rays
        SDL_SetRenderDrawColor(ren, 239, 68, 68, 140);
        for (int a = 0; a < 12; ++a) {
            float ang = (float)a * 0.523f + sinf(b_time + (float)a) * 0.2f;
            float flen = radius * (1.2f + 0.35f * sinf(b_time * 2.0f + (float)a));
            int fx = sx + (int)(cosf(ang) * flen);
            int fy = draw_sy + (int)(sinf(ang) * flen);
            SDL_RenderDrawLine(ren, sx, draw_sy, fx, fy);
        }

        // Spawn upward drifting ember motes
        if (rand() % 2 == 0) {
            float ox = (float)(rand() % 30 - 15);
            g_projectiles.spawn_particles(x + ox, y - z, 1, 250, 204, 21, 40.0f);
        }
    }

    // =========================================================================
    // 1b. AGILITY SPEED AURA (Cyan Windstream Rings & Speed Particles)
    // =========================================================================
    if (agility_timer > 0.0f) {
        float a_time = anim_timer * 10.0f;
        SDL_SetRenderDrawColor(ren, 56, 189, 248, 130);
        for (int r = 0; r < 2; ++r) {
            float phase_r = radius * (1.1f + 0.3f * fmodf(a_time + (float)r * 1.5f, 3.0f));
            for (int a = 0; a < 360; a += 30) {
                float rad = (float)a * 0.0174533f;
                int ax = sx + (int)(cosf(rad) * phase_r);
                int ay = draw_sy + (int)(sinf(rad) * (phase_r * 0.55f));
                SDL_RenderDrawPoint(ren, ax, ay);
            }
        }
    }

    // =========================================================================
    // 2. TAG RECALL & ENTRY SPECIAL FX BEAMS
    // =========================================================================
    if (state == STATE_TAG_RECALL) {
        // Red Pokéball recall beam pulling fighter upward
        float recall_pct = 1.0f - (tag_timer / 0.35f);
        int beam_w = (int)(radius * 2.0f * (1.0f - recall_pct * 0.5f));
        SDL_SetRenderDrawColor(ren, 239, 68, 68, (Uint8)(200.0f * (1.0f - recall_pct)));
        SDL_Rect beam = { sx - beam_w / 2, draw_sy - 80, beam_w, 100 };
        SDL_RenderFillRect(ren, &beam);
        alpha = (Uint8)(255.0f * (1.0f - recall_pct));
    } else if (state == STATE_TAG_ENTRY) {
        // Brilliant white materialization flash
        float entry_pct = 1.0f - (tag_timer / 0.35f);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(220.0f * (1.0f - entry_pct)));
        fill_circle(ren, sx, draw_sy, (int)(radius * 1.5f * (1.0f - entry_pct)));
    }

    // =========================================================================
    // 3. DRAW FIGHTER SPRITE (PMD 8-Directional Texture)
    // =========================================================================
    if (is_flying && fly_sheet.texture) {
        fly_sheet.draw(ren, anim_frame, pmd_dir, sx, draw_sy, scale, alpha);
    } else if (state == STATE_ATTACK && cur_attack.texture) {
        cur_attack.draw(ren, anim_frame, pmd_dir, sx, draw_sy, scale, alpha);
    } else if (cur_walk.texture) {
        cur_walk.draw(ren, anim_frame, pmd_dir, sx, draw_sy, scale, alpha);
    }

    // =========================================================================
    // 3b. GRENINJA NINJUTSU ACTIVE COMBAT AURAS & FOREARM BLADES
    // =========================================================================
    if (state == STATE_ATTACK && type == PKMN_GRENINJA) {
        float cos_a = cosf(aim_angle);
        float sin_a = sinf(aim_angle);
        float perp_x = -sin_a;
        float perp_y = cos_a;

        if (active_move == MOVE_WATER_SHURIKEN) {
            // Aqua gathering vortex at webbed throwing hand
            int hx = sx + (int)(cos_a * 18.0f);
            int hy = draw_sy + (int)(sin_a * 14.0f);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 190);
            fill_circle(ren, hx, hy, 7);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 230);
            fill_circle(ren, hx, hy, 3);
            for (int m = 0; m < 4; ++m) {
                float ma = anim_timer * 22.0f + (float)m * 1.5708f;
                int mx = hx + (int)(cosf(ma) * 11.0f);
                int my = hy + (int)(sinf(ma) * 11.0f);
                SDL_SetRenderDrawColor(ren, 186, 230, 253, 220);
                SDL_RenderDrawPoint(ren, mx, my);
                SDL_RenderDrawPoint(ren, mx + 1, my);
            }
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (active_move == MOVE_NIGHT_SLASH) {
            // Dual glowing dark energy kunai blades mounted on forearms
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            for (int arm = -1; arm <= 1; arm += 2) {
                float arm_ox = cos_a * 8.0f + perp_x * ((float)arm * 12.0f);
                float arm_oy = sin_a * 8.0f + perp_y * ((float)arm * 12.0f);
                int bx = sx + (int)arm_ox;
                int by = draw_sy + (int)arm_oy;
                float b_ang = aim_angle + (float)arm * 0.28f;
                int tip_x = bx + (int)(cosf(b_ang) * 28.0f);
                int tip_y = by + (int)(sinf(b_ang) * 28.0f);

                // Deep violet blade core
                SDL_SetRenderDrawColor(ren, 126, 34, 206, 220);
                SDL_RenderDrawLine(ren, bx, by, tip_x, tip_y);
                // Magenta razor edge
                SDL_SetRenderDrawColor(ren, 217, 70, 239, 245);
                SDL_RenderDrawLine(ren, bx + (int)(perp_x * (float)arm), by + (int)(perp_y * (float)arm), tip_x, tip_y);
                // Specular tip glint
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                fill_circle(ren, tip_x, tip_y, 2);
            }
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (active_move == MOVE_HYDRO_PUMP) {
            // High-pressure hydraulic muzzle blast rings
            int mx = sx + (int)(cos_a * 22.0f);
            int my = draw_sy + (int)(sin_a * 16.0f);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 180);
            fill_circle(ren, mx, my, 9);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 240);
            fill_circle(ren, mx, my, 4);
            for (int r = 1; r <= 2; ++r) {
                float ro = 12.0f + (float)r * 7.0f + sinf(anim_timer * 25.0f) * 2.5f;
                SDL_SetRenderDrawColor(ren, 186, 230, 253, 170);
                draw_circle_outline(ren, mx, my, (int)ro, 1);
            }
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (active_move == MOVE_AERIAL_ACE) {
            // Aerodynamic sonic slipstream cutting ribbons
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 240, 249, 255, 230);
            for (int s = -1; s <= 1; s += 2) {
                int wx1 = sx - (int)(cos_a * 16.0f - perp_x * ((float)s * 15.0f));
                int wy1 = draw_sy - (int)(sin_a * 16.0f - perp_y * ((float)s * 15.0f));
                int wx2 = sx + (int)(cos_a * 20.0f);
                int wy2 = draw_sy + (int)(sin_a * 20.0f);
                SDL_RenderDrawLine(ren, wx1, wy1, wx2, wy2);
            }
            int apex_x = sx + (int)(cos_a * 22.0f);
            int apex_y = draw_sy + (int)(sin_a * 22.0f);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, apex_x, apex_y, 3);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        }
    }

    // =========================================================================
    // 4. STATUS AILMENT RENDER OVERLAYS
    // =========================================================================
    if (status == STATUS_POISON) {
        // Rising purple poison clouds
        SDL_SetRenderDrawColor(ren, 168, 85, 247, 120);
        int px1 = sx + (int)(sinf(anim_timer * 5.0f) * 12.0f);
        int py1 = draw_sy - 15 - (int)(fmodf(anim_timer * 20.0f, 25.0f));
        fill_circle(ren, px1, py1, 3);

        int px2 = sx - (int)(cosf(anim_timer * 4.0f) * 10.0f);
        int py2 = draw_sy - 10 - (int)(fmodf(anim_timer * 24.0f, 22.0f));
        fill_circle(ren, px2, py2, 2);
    } else if (status == STATUS_BURN) {
        // Flickering burn flames
        SDL_SetRenderDrawColor(ren, 249, 115, 22, 140);
        int bx = sx + (int)(cosf(anim_timer * 6.0f) * 8.0f);
        int by = draw_sy - 12 - (int)(fmodf(anim_timer * 18.0f, 20.0f));
        fill_circle(ren, bx, by, 3);
    }
}

void Fighter::render_preview(SDL_Renderer* ren, int center_x, int center_y, int dir, int frame, float scale) const {
    if (walk_sheet.texture) {
        walk_sheet.draw(ren, frame, dir, center_x, center_y, scale, 255);
    }
}
