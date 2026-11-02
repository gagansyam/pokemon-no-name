#include "projectile.hpp"
#include "arena.hpp"
#include "font.hpp"
#include <cmath>

static void fill_circle(SDL_Renderer* ren, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; ++dy) {
        int dx = (int)sqrtf((float)(radius * radius - dy * dy));
        SDL_RenderDrawLine(ren, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static void draw_circle_outline(SDL_Renderer* ren, int cx, int cy, int radius, int thickness) {
    if (radius <= 0) return;
    for (int t = 0; t < thickness; ++t) {
        int r = radius - t;
        if (r <= 0) break;
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

// Simple fast numeral renderer for damage text without external font files
static void draw_digit(SDL_Renderer* ren, int x, int y, int d, int scale, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_SetRenderDrawColor(ren, r, g, b, a);
    // 3x5 font bit patterns
    static const unsigned short DIGITS[10] = {
        0x7B6F, // 0
        0x2492, // 1
        0x73E7, // 2
        0x73CF, // 3
        0x5BC9, // 4
        0x79CF, // 5
        0x79EF, // 6
        0x7249, // 7
        0x7BEF, // 8
        0x7BCF  // 9
    };
    unsigned short pat = DIGITS[d % 10];
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            int bit = 14 - (row * 3 + col);
            if ((pat >> bit) & 1) {
                SDL_Rect px = { x + col * scale, y + row * scale, scale, scale };
                SDL_RenderFillRect(ren, &px);
            }
        }
    }
}

static void draw_number(SDL_Renderer* ren, int x, int y, int num, int scale, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    // Draw minus sign
    SDL_SetRenderDrawColor(ren, r, g, b, a);
    SDL_Rect minus = { x, y + 2 * scale, 3 * scale, scale };
    SDL_RenderFillRect(ren, &minus);
    x += 4 * scale;

    char buf[16];
    int len = 0;
    int temp = num;
    if (temp == 0) buf[len++] = '0';
    while (temp > 0) {
        buf[len++] = '0' + (temp % 10);
        temp /= 10;
    }
    for (int i = len - 1; i >= 0; --i) {
        draw_digit(ren, x, y, buf[i] - '0', scale, r, g, b, a);
        x += 4 * scale;
    }
}

void ProjectileManager::render(SDL_Renderer* ren, float cam_x, float cam_y) {
    // 0. Render Shockwave Rings (Sonic booms, compression ripples, blast rings)
    for (int i = 0; i < MAX_SHOCKWAVES; ++i) {
        if (!shockwaves[i].active) continue;
        const ShockwaveRing& sw = shockwaves[i];
        float prog = (sw.max_lifetime > 0.001f) ? (1.0f - (sw.lifetime / sw.max_lifetime)) : 1.0f;
        float alpha_fac = (1.0f - prog * prog);
        if (alpha_fac < 0.0f) alpha_fac = 0.0f;
        if (alpha_fac > 1.0f) alpha_fac = 1.0f;
        Uint8 alpha = (Uint8)(alpha_fac * 255.0f);

        int sx = (int)(sw.x - cam_x);
        int sy = (int)(sw.y - cam_y);
        int rad = (int)sw.current_radius;
        if (rad > 0 && alpha > 0) {
            SDL_SetRenderDrawColor(ren, sw.r, sw.g, sw.b, alpha);
            draw_circle_outline(ren, sx, sy, rad, sw.thickness);
        }
    }

    // 1. Render Particles (3D elevation, velocity streaks, thermal cooling, additive blend)
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles[i].active) continue;
        const HitParticle& p = particles[i];
        float prog = (p.max_life_sec > 0.001f) ? (p.age / p.max_life_sec) : 1.0f;
        if (prog < 0.0f) prog = 0.0f;
        if (prog > 1.0f) prog = 1.0f;

        float alpha_fac = 1.0f - prog;
        Uint8 alpha = (Uint8)(alpha_fac * 255.0f);
        if (alpha == 0) continue;

        int sx = (int)(p.x - cam_x);
        int sy = (int)(p.y - p.z - cam_y);
        int floor_y = (int)(p.y - cam_y);

        SDL_SetRenderDrawBlendMode(ren, p.additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);

        switch (p.style) {
        case PSTYLE_SPARK: {
            float speed_sq = p.vx * p.vx + p.vy * p.vy;
            float speed = sqrtf(speed_sq);
            float streak_len = speed * 0.032f;
            if (streak_len < 3.0f) streak_len = 3.0f;
            if (streak_len > 24.0f) streak_len = 24.0f;

            float nx = (speed > 1.0f) ? (p.vx / speed) : 1.0f;
            float ny = (speed > 1.0f) ? (p.vy / speed) : 0.0f;

            int tail_x = sx - (int)(nx * streak_len);
            int tail_y = sy - (int)(ny * streak_len);

            Uint8 cr = (Uint8)(p.r_start + (int)((p.r_end - (int)p.r_start) * prog));
            Uint8 cg = (Uint8)(p.g_start + (int)((p.g_end - (int)p.g_start) * prog));
            Uint8 cb = (Uint8)(p.b_start + (int)((p.b_end - (int)p.b_start) * prog));

            SDL_SetRenderDrawColor(ren, cr, cg, cb, alpha);
            SDL_RenderDrawLine(ren, tail_x, tail_y, sx, sy);
            SDL_RenderDrawLine(ren, tail_x - (int)(ny * 0.8f), tail_y + (int)(nx * 0.8f),
                                    sx - (int)(ny * 0.8f), sy + (int)(nx * 0.8f));

            if (prog < 0.6f) {
                SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)((1.0f - prog / 0.6f) * 255.0f));
                SDL_RenderDrawPoint(ren, sx, sy);
            }
            break;
        }

        case PSTYLE_SMOKE_DUST: {
            float cur_size = p.size_start + (p.size_end - p.size_start) * prog;
            int r_dust = (int)(cur_size * 0.5f);
            if (r_dust < 1) r_dust = 1;

            Uint8 cr = (Uint8)(p.r_start + (int)((p.r_end - (int)p.r_start) * prog));
            Uint8 cg = (Uint8)(p.g_start + (int)((p.g_end - (int)p.g_start) * prog));
            Uint8 cb = (Uint8)(p.b_start + (int)((p.b_end - (int)p.b_start) * prog));

            Uint8 a_outer = (Uint8)(alpha * 0.40f);
            SDL_SetRenderDrawColor(ren, cr, cg, cb, a_outer);
            fill_circle(ren, sx, sy, r_dust);

            if (r_dust > 2) {
                Uint8 a_inner = (Uint8)(alpha * 0.70f);
                SDL_SetRenderDrawColor(ren, cr, cg, cb, a_inner);
                fill_circle(ren, sx, sy, r_dust / 2);
            }
            break;
        }

        case PSTYLE_EMBER: {
            Uint8 cr = (Uint8)(p.r_start + (int)((p.r_end - (int)p.r_start) * prog));
            Uint8 cg = (Uint8)(p.g_start + (int)((p.g_end - (int)p.g_start) * prog));
            Uint8 cb = (Uint8)(p.b_start + (int)((p.b_end - (int)p.b_start) * prog));

            float cur_size = p.size_start * (1.0f - prog * 0.7f);
            int r_emb = (int)(cur_size * 0.5f);
            if (r_emb < 1) r_emb = 1;

            SDL_SetRenderDrawColor(ren, cr, cg, cb, (Uint8)(alpha * 0.55f));
            fill_circle(ren, sx, sy, r_emb + 1);

            SDL_SetRenderDrawColor(ren, 255, 255, 220, alpha);
            fill_circle(ren, sx, sy, r_emb);
            break;
        }

        case PSTYLE_ELECTRIC: {
            float speed_sq = p.vx * p.vx + p.vy * p.vy;
            float speed = sqrtf(speed_sq);
            float streak_len = speed * 0.024f;
            if (streak_len < 4.0f) streak_len = 4.0f;
            if (streak_len > 18.0f) streak_len = 18.0f;

            float nx = (speed > 1.0f) ? (p.vx / speed) : 1.0f;
            float ny = (speed > 1.0f) ? (p.vy / speed) : 0.0f;

            int tail_x = sx - (int)(nx * streak_len);
            int tail_y = sy - (int)(ny * streak_len);
            int mid_x = (sx + tail_x) / 2 + (int)(-ny * 3.0f);
            int mid_y = (sy + tail_y) / 2 + (int)(nx * 3.0f);

            SDL_SetRenderDrawColor(ren, 56, 189, 248, (Uint8)(alpha * 0.75f));
            SDL_RenderDrawLine(ren, tail_x, tail_y, mid_x, mid_y);
            SDL_RenderDrawLine(ren, mid_x, mid_y, sx, sy);

            SDL_SetRenderDrawColor(ren, 255, 255, 255, alpha);
            SDL_RenderDrawPoint(ren, mid_x, mid_y);
            SDL_RenderDrawPoint(ren, sx, sy);
            break;
        }

        case PSTYLE_DEBRIS: {
            if (p.z > 2.0f) {
                float sh_scale = 1.0f - (p.z / 200.0f);
                if (sh_scale < 0.2f) sh_scale = 0.2f;
                int sh_rad = (int)(p.size_start * 0.6f * sh_scale);
                if (sh_rad >= 1) {
                    SDL_SetRenderDrawColor(ren, 20, 20, 30, (Uint8)(70 * sh_scale * (alpha / 255.0f)));
                    fill_circle(ren, sx, floor_y, sh_rad);
                }
            }

            float rad = p.size_start * 0.6f;
            float c_rot = cosf(p.rotation);
            float s_rot = sinf(p.rotation);

            int x1 = sx + (int)(-rad * c_rot - (-rad * 0.6f) * s_rot);
            int y1 = sy + (int)(-rad * s_rot + (-rad * 0.6f) * c_rot);
            int x2 = sx + (int)(rad * c_rot - (-rad * 0.4f) * s_rot);
            int y2 = sy + (int)(rad * s_rot + (-rad * 0.4f) * c_rot);
            int x3 = sx + (int)(rad * 0.8f * c_rot - (rad * 0.7f) * s_rot);
            int y3 = sy + (int)(rad * 0.8f * s_rot + (rad * 0.7f) * c_rot);
            int x4 = sx + (int)(-rad * 0.6f * c_rot - (rad * 0.8f) * s_rot);
            int y4 = sy + (int)(-rad * 0.6f * s_rot + (rad * 0.8f) * c_rot);

            Uint8 cr = p.r_start;
            Uint8 cg = p.g_start;
            Uint8 cb = p.b_start;
            SDL_SetRenderDrawColor(ren, cr, cg, cb, alpha);
            SDL_RenderDrawLine(ren, x1, y1, x2, y2);
            SDL_RenderDrawLine(ren, x2, y2, x3, y3);
            SDL_RenderDrawLine(ren, x3, y3, x4, y4);
            SDL_RenderDrawLine(ren, x4, y4, x1, y1);
            SDL_RenderDrawLine(ren, x1, y1, x3, y3);

            SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(alpha * 0.8f));
            SDL_RenderDrawPoint(ren, x2, y2);
            break;
        }

        case PSTYLE_AURA_MOTE: {
            float cur_size = p.size_start + (p.size_end - p.size_start) * prog;
            int rad = (int)(cur_size * 0.5f);
            if (rad < 1) rad = 1;

            Uint8 cr = p.r_end;
            Uint8 cg = p.g_end;
            Uint8 cb = p.b_end;

            SDL_SetRenderDrawColor(ren, cr, cg, cb, (Uint8)(alpha * 0.50f));
            fill_circle(ren, sx, sy, rad + 1);

            SDL_SetRenderDrawColor(ren, 255, 255, 255, alpha);
            fill_circle(ren, sx, sy, rad);
            break;
        }

        default: {
            SDL_SetRenderDrawColor(ren, p.r, p.g, p.b, alpha);
            SDL_Rect pr = { sx - (int)p.size / 2, sy - (int)p.size / 2, (int)p.size, (int)p.size };
            SDL_RenderFillRect(ren, &pr);
            break;
        }
        }
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    // 2. Render Projectiles
    for (int i = 0; i < MAX_PROJ; ++i) {
        if (!pool[i].active) continue;
        const Projectile& p = pool[i];
        int sx = (int)(p.x - cam_x);
        int sy = (int)(p.y - cam_y);
        int r = (int)p.radius;

        if (p.type == PROJ_THUNDERBOLT) {
            // --- FULL-LENGTH ANCHORED CONTINUOUS FRACTAL LIGHTNING SURGE ---
            // Origin at Pikachu's cheeks/body (p.target_x, p.target_y) -> Front apex (p.x, p.y)
            int ox = (int)(p.target_x - cam_x);
            int oy = (int)(p.target_y - cam_y);
            int ex = sx;
            int ey = sy;

            float bdx = (float)(ex - ox);
            float bdy = (float)(ey - oy);
            float beam_len = sqrtf(bdx * bdx + bdy * bdy);
            if (beam_len < 8.0f) beam_len = 8.0f;

            float dir_x = bdx / beam_len;
            float dir_y = bdy / beam_len;
            float perp_x = -dir_y;
            float perp_y = dir_x;

            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);

            // Layer 0: Pikachu Cheek Muzzle Flash & Emitter Aura at origin
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 160);
            fill_circle(ren, ox, oy, 16);
            SDL_SetRenderDrawColor(ren, 250, 204, 21, 220);
            fill_circle(ren, ox, oy, 10);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, ox, oy, 5);

            // Layer 1: Volumetric Electric Cyan Corona Halo along entire beam length
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 100);
            for (int w = -8; w <= 8; ++w) {
                SDL_RenderDrawLine(ren, ox + (int)(perp_x * w), oy + (int)(perp_y * w),
                                        ex + (int)(perp_x * w), ey + (int)(perp_y * w));
            }

            // Layer 2: 3-Strand Braided Solar Yellow Fractal Lightning Mantle
            SDL_SetRenderDrawColor(ren, 250, 204, 21, 230);
            const int num_segs = std::max(6, std::min(24, (int)(beam_len / 18.0f)));
            for (int strand = 0; strand < 3; ++strand) {
                int prev_x = ox;
                int prev_y = oy;
                float phase = p.extra_timer * 65.0f + (float)strand * 2.094f;
                for (int s = 1; s <= num_segs; ++s) {
                    float t = (float)s / (float)num_segs;
                    float base_x = (float)ox + bdx * t;
                    float base_y = (float)oy + bdy * t;
                    float wave = sinf(phase + (float)s * 1.7f);
                    float jitter = wave * (12.0f + (float)strand * 3.0f);
                    int cur_x = (int)(base_x + perp_x * jitter);
                    int cur_y = (int)(base_y + perp_y * jitter);

                    for (int w = -1; w <= 1; ++w) {
                        SDL_RenderDrawLine(ren, prev_x + (int)(perp_x * w), prev_y + (int)(perp_y * w),
                                                cur_x + (int)(perp_x * w), cur_y + (int)(perp_y * w));
                    }
                    prev_x = cur_x;
                    prev_y = cur_y;
                }
            }

            // Layer 3: High-Frequency Lateral Branching Ground Arcs
            SDL_SetRenderDrawColor(ren, 125, 211, 252, 230); // Pale electric cyan
            int arc_steps = std::max(2, (int)(beam_len / 45.0f));
            for (int a = 0; a < arc_steps; ++a) {
                float at = (float)(a + 1) / (float)(arc_steps + 1);
                int ax = ox + (int)(bdx * at);
                int ay = oy + (int)(bdy * at);
                float arc_side = ((a % 2 == 0) ? 1.0f : -1.0f);
                float arc_len = 16.0f + sinf(p.extra_timer * 75.0f + (float)a * 2.1f) * 8.0f;
                int mid_x = ax + (int)(perp_x * arc_side * (arc_len * 0.6f) + dir_x * (arc_len * 0.2f));
                int mid_y = ay + (int)(perp_y * arc_side * (arc_len * 0.6f) + dir_y * (arc_len * 0.2f));
                int tip_x = ax + (int)(perp_x * arc_side * arc_len + dir_x * (arc_len * 0.5f));
                int tip_y = ay + (int)(perp_y * arc_side * arc_len + dir_y * (arc_len * 0.5f));

                SDL_RenderDrawLine(ren, ax, ay, mid_x, mid_y);
                SDL_RenderDrawLine(ren, mid_x, mid_y, tip_x, tip_y);
            }

            // Layer 4: Incandescent White-Hot Lightning Core Spine
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            {
                int prev_x = ox;
                int prev_y = oy;
                float core_phase = p.extra_timer * 90.0f;
                int core_segs = std::max(4, std::min(16, (int)(beam_len / 22.0f)));
                for (int s = 1; s <= core_segs; ++s) {
                    float t = (float)s / (float)core_segs;
                    float base_x = (float)ox + bdx * t;
                    float base_y = (float)oy + bdy * t;
                    float jitter = ((s % 2 == 0) ? 6.0f : -6.0f) * sinf(core_phase + (float)s * 2.3f);
                    int cur_x = (int)(base_x + perp_x * jitter);
                    int cur_y = (int)(base_y + perp_y * jitter);

                    SDL_RenderDrawLine(ren, prev_x - 1, prev_y, cur_x - 1, cur_y);
                    SDL_RenderDrawLine(ren, prev_x, prev_y, cur_x, cur_y);
                    SDL_RenderDrawLine(ren, prev_x + 1, prev_y, cur_x + 1, cur_y);
                    prev_x = cur_x;
                    prev_y = cur_y;
                }
            }

            // Layer 5: Blinding High-Voltage Spearhead Diamond Burst at apex
            fill_circle(ren, ex, ey, 14);
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 255);
            fill_circle(ren, ex, ey, 8);
            SDL_SetRenderDrawColor(ren, 250, 204, 21, 255);
            fill_circle(ren, ex, ey, 5);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, ex, ey, 2);

            // 4-point specular star flare at spearhead
            int flare_len = 16;
            SDL_RenderDrawLine(ren, ex - flare_len, ey, ex + flare_len, ey);
            SDL_RenderDrawLine(ren, ex, ey - flare_len, ex, ey + flare_len);

            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_THUNDER) {
            if (p.delay_timer > 0.0f) {
                // --- PHASE 1: APOCALYPTIC RUNIC DANGER SEAL (0.32s countdown) ---
                float prog = 1.0f - (p.delay_timer / 0.32f);
                if (prog < 0.0f) prog = 0.0f;
                if (prog > 1.0f) prog = 1.0f;

                // Layer 1: Dark atmospheric vortex glow
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                Uint8 vortex_a = (Uint8)(30 + (int)(prog * 60));
                SDL_SetRenderDrawColor(ren, 15, 23, 42, vortex_a);
                fill_circle(ren, sx, sy, (int)(r * 1.35f));

                // Layer 2: Runic Ground Danger Zone with segmented outer circle
                SDL_SetRenderDrawColor(ren, 250, 204, 21, (Uint8)(100 + prog * 130));
                for (int a = 0; a < 32; ++a) {
                    float rad1 = (float)a * 0.1963f;
                    float rad2 = (float)(a + 1) * 0.1963f;
                    int x1 = sx + (int)(cosf(rad1) * (float)r);
                    int y1 = sy + (int)(sinf(rad1) * (float)r);
                    int x2 = sx + (int)(cosf(rad2) * (float)r);
                    int y2 = sy + (int)(sinf(rad2) * (float)r);
                    if (a % 2 == 0) {
                        SDL_RenderDrawLine(ren, x1, y1, x2, y2);
                        SDL_RenderDrawLine(ren, x1 - 1, y1, x2 - 1, y2);
                    }
                }

                // Layer 3: Runic 8-point rotating star seal
                float star_rot = prog * 2.5f;
                SDL_SetRenderDrawColor(ren, 56, 189, 248, (Uint8)(160 + prog * 95));
                for (int pt = 0; pt < 8; ++pt) {
                    float a1 = star_rot + (float)pt * 0.7854f;
                    float a2 = star_rot + (float)(pt + 3) * 0.7854f;
                    int px1 = sx + (int)(cosf(a1) * (float)r * 0.85f);
                    int py1 = sy + (int)(sinf(a1) * (float)r * 0.85f);
                    int px2 = sx + (int)(cosf(a2) * (float)r * 0.85f);
                    int py2 = sy + (int)(sinf(a2) * (float)r * 0.85f);
                    SDL_RenderDrawLine(ren, px1, py1, px2, py2);
                }

                // Layer 4: Rapidly contracting countdown energy ring
                float contract_r = (float)r * (1.0f - prog * 0.85f);
                if (contract_r < 6.0f) contract_r = 6.0f;
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
                SDL_SetRenderDrawColor(ren, 56, 189, 248, 220);
                fill_circle(ren, sx, sy, (int)contract_r);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                fill_circle(ren, sx, sy, (int)(contract_r * 0.5f));

                // Layer 5: 4 cardinal red-amber pulse arrows pointing directly at epicenter
                int arm = 18;
                SDL_SetRenderDrawColor(ren, 239, 68, 68, 240);
                SDL_RenderDrawLine(ren, sx - r - arm, sy, sx - r + 4, sy);
                SDL_RenderDrawLine(ren, sx + r - 4, sy, sx + r + arm, sy);
                SDL_RenderDrawLine(ren, sx, sy - r - arm, sx, sy - r + 4);
                SDL_RenderDrawLine(ren, sx, sy + r - 4, sx, sy + r + arm);

                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            } else {
                // --- PHASE 2: APOCALYPTIC SKY-PILLAR STRIKE (Descends from sy - 720) ---
                float fade = (p.lifetime > 0.0f) ? (p.lifetime / 0.55f) : 0.0f;
                if (fade > 1.0f) fade = 1.0f;

                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);

                // Layer 1: Expanding 3-Tier Ground Blast Radii
                SDL_SetRenderDrawColor(ren, 250, 204, 21, (Uint8)(fade * 160));
                fill_circle(ren, sx, sy, (int)(r * 1.4f * (1.0f - fade * 0.2f)));
                SDL_SetRenderDrawColor(ren, 56, 189, 248, (Uint8)(fade * 200));
                fill_circle(ren, sx, sy, (int)(r * 1.0f));
                SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(fade * 255));
                fill_circle(ren, sx, sy, (int)(r * 0.65f));

                // Layer 2: Massive 56px-Wide Sky-to-Ground Lightning Column (sy - 720)
                int start_by = sy - 720;
                int trunk_start_x = sx;
                int cur_x = trunk_start_x;
                int cur_y = start_by;

                const int seg_count = 11;
                for (int seg = 0; seg < seg_count; ++seg) {
                    int next_y = start_by + (sy - start_by) * (seg + 1) / seg_count;
                    int next_x = trunk_start_x + (sx - trunk_start_x) * (seg + 1) / seg_count;
                    if (seg < seg_count - 1) {
                        next_x += (seg % 2 == 0) ? 28 : -26;
                    }

                    // Layer 2b: Broad Electric Cyan Aurora Envelope (28 lines wide)
                    SDL_SetRenderDrawColor(ren, 56, 189, 248, (Uint8)(fade * 180));
                    for (int w = -14; w <= 14; ++w) {
                        SDL_RenderDrawLine(ren, cur_x + w, cur_y, next_x + w, next_y);
                    }

                    // Layer 3: Solar Yellow Heavy Plasma Mantle (14 lines wide)
                    SDL_SetRenderDrawColor(ren, 250, 204, 21, (Uint8)(fade * 240));
                    for (int w = -7; w <= 7; ++w) {
                        SDL_RenderDrawLine(ren, cur_x + w, cur_y, next_x + w, next_y);
                    }

                    // Layer 4: Incandescent White Core Nucleus (6 lines wide)
                    SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(fade * 255));
                    for (int w = -2; w <= 2; ++w) {
                        SDL_RenderDrawLine(ren, cur_x + w, cur_y, next_x + w, next_y);
                    }

                    cur_x = next_x;
                    cur_y = next_y;
                }

                // Layer 5: Branching Sky Lightning Forks
                for (int fork = 0; fork < 3; ++fork) {
                    float fork_x_offset = (fork == 0) ? -42.0f : ((fork == 1) ? 46.0f : -18.0f);
                    int fx_start = sx + (int)fork_x_offset;
                    int fy_start = sy - 520 + fork * 90;
                    int fc_x = fx_start;
                    int fc_y = fy_start;
                    int target_fx = sx + (int)(fork_x_offset * 1.3f);

                    for (int seg = 0; seg < 6; ++seg) {
                        int ny = fy_start + (sy - fy_start) * (seg + 1) / 6;
                        int nx = fx_start + (target_fx - fx_start) * (seg + 1) / 6;
                        if (seg < 5) nx += (seg % 2 == 0) ? -20 : 20;

                        SDL_SetRenderDrawColor(ren, 56, 189, 248, (Uint8)(fade * 220));
                        SDL_RenderDrawLine(ren, fc_x - 1, fc_y, nx - 1, ny);
                        SDL_RenderDrawLine(ren, fc_x + 1, fc_y, nx + 1, ny);
                        SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(fade * 255));
                        SDL_RenderDrawLine(ren, fc_x, fc_y, nx, ny);

                        fc_x = nx;
                        fc_y = ny;
                    }
                }

                // Layer 6: 8-Direction Ground Crawling Lightning Tendrils
                for (int arc = 0; arc < 8; ++arc) {
                    float rad = (float)arc * 0.7854f + 0.15f;
                    float arc_len = r * (0.8f + fade * 0.5f);
                    int gx1 = sx;
                    int gy1 = sy;
                    int gx2 = sx + (int)(cosf(rad) * arc_len * 0.5f + sinf(rad) * 12.0f);
                    int gy2 = sy + (int)(sinf(rad) * arc_len * 0.5f - cosf(rad) * 12.0f);
                    int gx3 = sx + (int)(cosf(rad) * arc_len);
                    int gy3 = sy + (int)(sinf(rad) * arc_len);

                    SDL_SetRenderDrawColor(ren, 56, 189, 248, (Uint8)(fade * 220));
                    SDL_RenderDrawLine(ren, gx1, gy1, gx2, gy2);
                    SDL_RenderDrawLine(ren, gx2, gy2, gx3, gy3);
                    SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(fade * 255));
                    SDL_RenderDrawLine(ren, gx1, gy1, gx2, gy2);
                }

                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            }
        } else if (p.type == PROJ_ELECTRO_BALL) {
            // --- 5-LAYER HIGH-VOLTAGE SPHERE WITH SINUSOIDAL TURBULENCE & SPECULAR FLARES ---
            // Layer 1: Ground drop shadow (scaled by height)
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 15, 23, 42, 90);
            fill_circle(ren, sx, sy + 16, (int)(r * 0.75f));

            // Layer 3: Additive Secondary Cyan Electrical Corona with Sinusoidal Breathing
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            float pulse = sinf(p.extra_timer * 22.0f) * 3.5f;
            int aura_r = (int)((float)r * 1.45f + pulse);
            if (aura_r < r + 2) aura_r = r + 2;
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 140);
            fill_circle(ren, sx, sy, aura_r);

            // Layer 4: Saturated Elemental Body (Solar yellow plasma mantle with rotating arc chevrons)
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 250, 204, 21, 240);
            fill_circle(ren, sx, sy, r);

            // Rotating electrical arc coronas
            for (int a = 0; a < 4; ++a) {
                float arc_a = p.extra_timer * 14.0f + (float)a * 1.5708f;
                int ax = sx + (int)(cosf(arc_a) * (float)r);
                int ay = sy + (int)(sinf(arc_a) * (float)r);
                SDL_SetRenderDrawColor(ren, 254, 240, 138, 220);
                fill_circle(ren, ax, ay, 3);
            }

            // Layer 5: Blinding White-Hot Core with 4-Point Specular Star Flares
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, sx, sy, (int)(r * 0.45f));

            int fl = (int)((float)r * 1.6f);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 220);
            SDL_RenderDrawLine(ren, sx - fl, sy, sx + fl, sy);
            SDL_RenderDrawLine(ren, sx, sy - fl, sx, sy + fl);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_FLAMETHROWER) {
            // --- 5-LAYER CONICAL ROARING MOLTEN FLAME JET ---
            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : cosf(p.angle);
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : sinf(p.angle);
            float perp_x = -dir_y;
            float perp_y = dir_x;

            // Lateral turbulence oscillation based on extra_timer & instance index
            float wave = sinf(p.extra_timer * 32.0f + (float)i * 1.5f) * (r * 0.38f);
            int cx = sx + (int)(perp_x * wave);
            int cy = sy + (int)(perp_y * wave);

            float flame_alpha = (p.lifetime > 0.12f) ? 1.0f : (p.lifetime / 0.12f);

            // Layer 0: Ground Thermal Glow beneath puff
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 153, 27, 27, (Uint8)(75 * flame_alpha));
            fill_circle(ren, cx, cy + 6, (int)(r * 1.25f));

            // Layer 1: Outer Dark Crimson Smoke & Thermal Distortion Halo
            SDL_SetRenderDrawColor(ren, 185, 28, 28, (Uint8)(140 * flame_alpha));
            fill_circle(ren, cx, cy, (int)(r * 1.35f));

            // Layer 2: Molten Swirling Orange Fire Vortex
            SDL_SetRenderDrawColor(ren, 249, 115, 22, (Uint8)(230 * flame_alpha));
            fill_circle(ren, cx, cy, r);

            // Layer 3: Brilliant Solar Gold Flame Mantle (with Additive Bloom)
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 251, 191, 36, (Uint8)(240 * flame_alpha));
            fill_circle(ren, cx, cy, (int)(r * 0.65f));

            // Layer 4: Incandescent White-Hot Central Nucleus
            SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(255 * flame_alpha));
            fill_circle(ren, cx, cy, (int)(r * 0.30f));

            // Layer 5: Forward Licking Flame Tongues and Ember Stream along cone edges
            for (int t = -2; t <= 2; ++t) {
                float tf = (float)t * 0.35f;
                float tongue_len = (float)r * (0.8f + sinf(p.extra_timer * 24.0f + (float)t) * 0.3f);
                int tx1 = cx + (int)((dir_x * 0.5f + perp_x * tf) * (float)r);
                int ty1 = cy + (int)((dir_y * 0.5f + perp_y * tf) * (float)r);
                int tx2 = tx1 + (int)((dir_x + perp_x * (tf * 0.4f)) * tongue_len);
                int ty2 = ty1 + (int)((dir_y + perp_y * (tf * 0.4f)) * tongue_len);

                SDL_SetRenderDrawColor(ren, 254, 215, 170, (Uint8)(220 * flame_alpha));
                SDL_RenderDrawLine(ren, tx1, ty1, tx2, ty2);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(200 * flame_alpha));
                SDL_RenderDrawLine(ren, tx1 + 1, ty1, tx2 + 1, ty2);
            }

            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_FIRE_SPIN) {
            if (p.delay_timer > 0.0f) {
                // --- PHASE 1: SPAT EMBER SEED IN FLIGHT TOWARD TARGET ---
                float flight_prog = (p.max_delay > 0.001f) ? (1.0f - p.delay_timer / p.max_delay) : 1.0f;
                if (flight_prog < 0.0f) flight_prog = 0.0f;
                if (flight_prog > 1.0f) flight_prog = 1.0f;

                // 1. Target landing warning seal on ground at target_x, target_y
                int target_sx = (int)(p.target_x - cam_x);
                int target_sy = (int)(p.target_y - cam_y);
                float land_pulse = 0.5f + 0.5f * sinf(p.delay_timer * 22.0f);
                Uint8 seal_alpha = (Uint8)(90 + (int)(land_pulse * 90));
                SDL_SetRenderDrawColor(ren, 239, 68, 68, seal_alpha);
                fill_circle(ren, target_sx, target_sy, 16);
                SDL_SetRenderDrawColor(ren, 251, 191, 36, 240);
                // Rotating warning crosshairs on ground
                for (int a = 0; a < 4; ++a) {
                    float a_rad = (float)a * 1.5708f + p.delay_timer * 10.0f;
                    int x1 = target_sx + (int)(cosf(a_rad) * 12.0f);
                    int y1 = target_sy + (int)(sinf(a_rad) * 12.0f);
                    int x2 = target_sx + (int)(cosf(a_rad) * 26.0f);
                    int y2 = target_sy + (int)(sinf(a_rad) * 26.0f);
                    SDL_RenderDrawLine(ren, x1, y1, x2, y2);
                }

                // 2. Flying ember seed with parabolic height arc
                float arc_height = sinf(flight_prog * 3.14159f) * 36.0f;
                int seed_x = sx;
                int seed_y = sy - (int)arc_height;

                // Ground shadow beneath the ember seed
                SDL_SetRenderDrawColor(ren, 25, 25, 25, 110);
                fill_circle(ren, sx, sy, 7);

                // Trailing flame comet tail behind the seed
                float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
                float tdir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : 1.0f;
                float tdir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : 0.0f;
                for (int tr = 1; tr <= 3; ++tr) {
                    int tx = seed_x - (int)(tdir_x * (float)tr * 10.0f);
                    int ty = seed_y - (int)(tdir_y * (float)tr * 10.0f);
                    SDL_SetRenderDrawColor(ren, 249, 115, 22, (Uint8)(210 - tr * 50));
                    fill_circle(ren, tx, ty, 8 - tr * 2);
                }

                // Glowing fiery ember seed core
                SDL_SetRenderDrawColor(ren, 239, 68, 68, 220);
                fill_circle(ren, seed_x, seed_y, 11);
                SDL_SetRenderDrawColor(ren, 251, 191, 36, 255);
                fill_circle(ren, seed_x, seed_y, 7);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                fill_circle(ren, seed_x, seed_y, 3);
            } else {
                // --- PHASE 2: LANDED ROTATING FIRE CYCLONE VORTEX (7.5 rad/s) ---
                float rot = p.extra_timer * 7.5f;

            // 1. Ground scorched ash imprint beneath the cyclone
            SDL_SetRenderDrawColor(ren, 30, 20, 15, 110);
            fill_circle(ren, sx, sy, r + 4);
            SDL_SetRenderDrawColor(ren, 194, 65, 12, 140); // Fiery boundary rim
            for (int a = 0; a < 20; ++a) {
                float a1 = (float)a * 0.314f + rot * 0.2f;
                float a2 = (float)(a + 1) * 0.314f + rot * 0.2f;
                if (a % 2 == 0) {
                    SDL_RenderDrawLine(ren,
                        sx + (int)(cosf(a1) * (float)r), sy + (int)(sinf(a1) * (float)r),
                        sx + (int)(cosf(a2) * (float)r), sy + (int)(sinf(a2) * (float)r));
                }
            }

            // 2. Three spiraling vortex flame arms (curving inward from edge to center)
            for (int arm = 0; arm < 3; ++arm) {
                float arm_base = rot + (float)arm * 2.0944f; // 120 deg apart
                int prev_x = sx + (int)(cosf(arm_base) * 8.0f);
                int prev_y = sy + (int)(sinf(arm_base) * 8.0f);

                for (int seg = 1; seg <= 7; ++seg) {
                    float s_frac = (float)seg / 7.0f;
                    float arm_angle = arm_base + s_frac * 1.8f;
                    float arm_dist = 8.0f + s_frac * ((float)r - 8.0f);
                    int cur_x = sx + (int)(cosf(arm_angle) * arm_dist);
                    int cur_y = sy + (int)(sinf(arm_angle) * arm_dist);

                    // Multi-layer colored spiral lines (outer crimson, inner orange, core yellow)
                    SDL_SetRenderDrawColor(ren, 239, 68, 68, 220);
                    for (int w = -2; w <= 2; ++w) {
                        SDL_RenderDrawLine(ren, prev_x + w, prev_y, cur_x + w, cur_y);
                    }
                    SDL_SetRenderDrawColor(ren, 251, 191, 36, 255);
                    SDL_RenderDrawLine(ren, prev_x, prev_y, cur_x, cur_y);

                    prev_x = cur_x;
                    prev_y = cur_y;
                }

                // Orbiting fireball cluster at the tip of each spiral arm
                float tip_angle = arm_base + 1.8f;
                int tip_x = sx + (int)(cosf(tip_angle) * (float)r);
                int tip_y = sy + (int)(sinf(tip_angle) * (float)r);
                SDL_SetRenderDrawColor(ren, 249, 115, 22, 230);
                fill_circle(ren, tip_x, tip_y, 7);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                fill_circle(ren, tip_x, tip_y, 3);
            }

            // 3. Central pulsing cyclone eye
            float eye_pulse = 0.5f + 0.5f * sinf(p.extra_timer * 12.0f);
            int eye_r = (int)(9.0f + eye_pulse * 4.0f);
            SDL_SetRenderDrawColor(ren, 254, 240, 138, 240);
            fill_circle(ren, sx, sy, eye_r);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, sx, sy, (int)(eye_r * 0.5f));
            }
        } else if (p.type == PROJ_CLAW_SCRATCH) {
            // --- 5-LAYER VISCERAL RAZOR CLAW SWEEP ---
            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : cosf(p.angle);
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : sinf(p.angle);
            float perp_x = -dir_y;
            float perp_y = dir_x;

            float life_prog = (p.max_lifetime > 0.001f) ? (1.0f - p.lifetime / p.max_lifetime) : 1.0f;
            float slash_len = 44.0f * (0.5f + life_prog * 0.5f);
            float fade = (p.lifetime > 0.08f) ? 1.0f : (p.lifetime / 0.08f);

            // Layer 1: Crimson Kinetic Motion Smear Trail
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 153, 27, 27, (Uint8)(120 * fade));
            for (int claw = -1; claw <= 1; ++claw) {
                float claw_offset = (float)claw * 14.0f;
                int smear_x = sx + (int)(perp_x * claw_offset - dir_x * (slash_len * 0.7f));
                int smear_y = sy + (int)(perp_y * claw_offset - dir_y * (slash_len * 0.7f));
                fill_circle(ren, smear_x, smear_y, 4);
            }

            // 3 Parallel Curved Razor Claw Slashes
            for (int claw = -1; claw <= 1; ++claw) {
                float claw_offset = (float)claw * 14.0f;
                int root_x = sx + (int)(perp_x * claw_offset - dir_x * (slash_len * 0.5f));
                int root_y = sy + (int)(perp_y * claw_offset - dir_y * (slash_len * 0.5f));

                int mid_x = sx + (int)(perp_x * (claw_offset + 6.0f));
                int mid_y = sy + (int)(perp_y * (claw_offset + 6.0f));

                int tip_x = sx + (int)(perp_x * (claw_offset - 4.0f) + dir_x * (slash_len * 0.5f));
                int tip_y = sy + (int)(perp_y * (claw_offset - 4.0f) + dir_y * (slash_len * 0.5f));

                // Layer 2: Tapered Scarlet Slash Blade Body (7 lines wide)
                SDL_SetRenderDrawColor(ren, 220, 38, 38, (Uint8)(240 * fade));
                for (int w = -3; w <= 3; ++w) {
                    SDL_RenderDrawLine(ren, root_x + (int)(perp_x * w), root_y + (int)(perp_y * w),
                                           mid_x + (int)(perp_x * w), mid_y + (int)(perp_y * w));
                    SDL_RenderDrawLine(ren, mid_x + (int)(perp_x * w), mid_y + (int)(perp_y * w),
                                           tip_x + (int)(perp_x * w), tip_y + (int)(perp_y * w));
                }

                // Layer 3: Blazing Molten Orange Blade Spine
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
                SDL_SetRenderDrawColor(ren, 249, 115, 22, (Uint8)(255 * fade));
                for (int w = -1; w <= 1; ++w) {
                    SDL_RenderDrawLine(ren, root_x + (int)(perp_x * w), root_y + (int)(perp_y * w),
                                           mid_x + (int)(perp_x * w), mid_y + (int)(perp_y * w));
                    SDL_RenderDrawLine(ren, mid_x + (int)(perp_x * w), mid_y + (int)(perp_y * w),
                                           tip_x + (int)(perp_x * w), tip_y + (int)(perp_y * w));
                }

                // Layer 4: Razor-Sharp Blinding White Core Cutting Edge & Piercing Tip
                SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(255 * fade));
                SDL_RenderDrawLine(ren, root_x, root_y, mid_x, mid_y);
                SDL_RenderDrawLine(ren, mid_x, mid_y, tip_x, tip_y);
                fill_circle(ren, tip_x, tip_y, 3);
            }

            // Layer 5: Crescent Air-Cutter Shockwave Slice projecting forward
            SDL_SetRenderDrawColor(ren, 254, 226, 226, (Uint8)(180 * fade));
            int arc_front_x = sx + (int)(dir_x * (slash_len * 0.55f));
            int arc_front_y = sy + (int)(dir_y * (slash_len * 0.55f));
            SDL_RenderDrawLine(ren, arc_front_x - (int)(perp_x * 18.0f), arc_front_y - (int)(perp_y * 18.0f),
                                   arc_front_x + (int)(perp_x * 18.0f), arc_front_y + (int)(perp_y * 18.0f));

            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_WING_ATTACK) {
            // Dual sweeping aerodynamic dragon wing blades
            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : cosf(p.angle);
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : sinf(p.angle);
            float perp_x = -dir_y;
            float perp_y = dir_x;

            // 1. Forward air-cutter compression shockwave in front of chest
            int nose_x = sx + (int)(dir_x * 24.0f);
            int nose_y = sy + (int)(dir_y * 24.0f);
            SDL_SetRenderDrawColor(ren, 240, 248, 255, 180);
            for (int a = -6; a <= 6; ++a) {
                float rad = (float)a * 0.15f;
                int ax = nose_x + (int)((dir_x * cosf(rad) + perp_x * sinf(rad)) * 26.0f);
                int ay = nose_y + (int)((dir_y * cosf(rad) + perp_y * sinf(rad)) * 26.0f);
                SDL_RenderDrawLine(ren, nose_x, nose_y, ax, ay);
            }

            // 2. Left and Right sweeping dragon wing blades
            for (int side = -1; side <= 1; side += 2) {
                float s = (float)side;
                int root_x = sx + (int)(perp_x * s * 14.0f);
                int root_y = sy + (int)(perp_y * s * 14.0f);

                int mid_x  = sx + (int)(perp_x * s * 38.0f - dir_x * 10.0f);
                int mid_y  = sy + (int)(perp_y * s * 38.0f - dir_y * 10.0f);

                int tip_x  = sx + (int)(perp_x * s * 58.0f - dir_x * 24.0f);
                int tip_y  = sy + (int)(perp_y * s * 58.0f - dir_y * 24.0f);

                // Cyan/sky-blue razor-wind blade aura (7 lines wide)
                SDL_SetRenderDrawColor(ren, 147, 197, 253, 220);
                for (int w = -3; w <= 3; ++w) {
                    SDL_RenderDrawLine(ren, root_x + (int)(dir_x * w), root_y + (int)(dir_y * w),
                                           mid_x + (int)(dir_x * w), mid_y + (int)(dir_y * w));
                    SDL_RenderDrawLine(ren, mid_x + (int)(dir_x * w), mid_y + (int)(dir_y * w),
                                           tip_x + (int)(dir_x * w), tip_y + (int)(dir_y * w));
                }

                // White razor cutting edge
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                SDL_RenderDrawLine(ren, root_x, root_y, mid_x, mid_y);
                SDL_RenderDrawLine(ren, mid_x, mid_y, tip_x, tip_y);

                // Orange dragon flame plumage at wing tip
                SDL_SetRenderDrawColor(ren, 249, 115, 22, 230);
                fill_circle(ren, tip_x, tip_y, 4);

                // Aerodynamic slipstream trail lines behind wing tips
                SDL_SetRenderDrawColor(ren, 219, 234, 254, 160);
                for (int tr = 1; tr <= 3; ++tr) {
                    int st_x = tip_x - (int)(dir_x * (float)tr * 14.0f);
                    int st_y = tip_y - (int)(dir_y * (float)tr * 14.0f);
                    int en_x = st_x - (int)(dir_x * 12.0f);
                    int en_y = st_y - (int)(dir_y * 12.0f);
                    SDL_RenderDrawLine(ren, st_x, st_y, en_x, en_y);
                }
            }
        } else if (p.type == PROJ_MELEE_SWEEP) {
            if (p.damage >= 40) {
                // Iron Tail: Brilliant metallic steel crescent slash!
                float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
                float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : 1.0f;
                float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : 0.0f;

                float prog = (p.max_lifetime > 0.0f) ? (1.0f - (p.lifetime / p.max_lifetime)) : 0.5f;
                if (prog < 0.0f) prog = 0.0f;
                if (prog > 1.0f) prog = 1.0f;

                float arc_r = (float)r * (1.1f + prog * 0.35f);
                float sweep_span = 1.57f; // ~90 degree arc
                float center_angle = atan2f(dir_y, dir_x);

                // 1. Chrome-metallic outer glow arc (layered width)
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(ren, 186, 230, 253, 190); // Sky-blue / chrome rim
                for (int w = -3; w <= 3; ++w) {
                    float cur_r = arc_r + (float)w * 2.0f;
                    int prev_ax = 0, prev_ay = 0;
                    for (int step = -10; step <= 10; ++step) {
                        float a = center_angle + ((float)step / 10.0f) * (sweep_span * 0.5f);
                        int ax = sx + (int)(cosf(a) * cur_r);
                        int ay = sy + (int)(sinf(a) * cur_r);
                        if (step > -10) {
                            SDL_RenderDrawLine(ren, prev_ax, prev_ay, ax, ay);
                        }
                        prev_ax = ax;
                        prev_ay = ay;
                    }
                }

                // 2. Core brilliant steel blade (pure white razor arc)
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                int prev_ax = 0, prev_ay = 0;
                for (int step = -10; step <= 10; ++step) {
                    float a = center_angle + ((float)step / 10.0f) * (sweep_span * 0.5f);
                    int ax = sx + (int)(cosf(a) * arc_r);
                    int ay = sy + (int)(sinf(a) * arc_r);
                    if (step > -10) {
                        SDL_RenderDrawLine(ren, prev_ax, prev_ay, ax, ay);
                    }
                    prev_ax = ax;
                    prev_ay = ay;
                }

                // 3. Inner metallic silver core
                SDL_SetRenderDrawColor(ren, 224, 242, 254, 230);
                fill_circle(ren, sx, sy, (int)(r * 0.5f));
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                fill_circle(ren, sx, sy, (int)(r * 0.28f));

                // 4. Leading razor tip spark
                int tip_x = sx + (int)(cosf(center_angle + sweep_span * 0.5f) * arc_r);
                int tip_y = sy + (int)(sinf(center_angle + sweep_span * 0.5f) * arc_r);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                fill_circle(ren, tip_x, tip_y, 3);
            } else {
                // Quick Attack: Supersonic anime air-cutter crescent with additive speed aura
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
                float dir_x = cosf(p.angle);
                float dir_y = sinf(p.angle);
                float perp_x = -dir_y;
                float perp_y = dir_x;

                // Layer 3: Additive outer sonic shroud
                SDL_SetRenderDrawColor(ren, 200, 230, 255, 140);
                fill_circle(ren, sx, sy, (int)(r * 1.35f));

                // Layer 4: Saturated silver-white air-compression crescent
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(ren, 240, 245, 255, 230);
                for (int w = -2; w <= 2; ++w) {
                    int ax1 = sx - (int)(perp_x * 16.0f) + (int)(dir_x * (float)w * 2.0f);
                    int ay1 = sy - (int)(perp_y * 16.0f) + (int)(dir_y * (float)w * 2.0f);
                    int ax2 = sx + (int)(dir_x * 14.0f) + (int)(dir_x * (float)w * 2.0f);
                    int ay2 = sy + (int)(dir_y * 14.0f) + (int)(dir_y * (float)w * 2.0f);
                    int ax3 = sx + (int)(perp_x * 16.0f) + (int)(dir_x * (float)w * 2.0f);
                    int ay3 = sy + (int)(perp_y * 16.0f) + (int)(dir_y * (float)w * 2.0f);
                    SDL_RenderDrawLine(ren, ax1, ay1, ax2, ay2);
                    SDL_RenderDrawLine(ren, ax2, ay2, ax3, ay3);
                }

                // Layer 5: Blinding pure white razor edge + specular tip flare
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                int tip_x = sx + (int)(dir_x * 16.0f);
                int tip_y = sy + (int)(dir_y * 16.0f);
                fill_circle(ren, tip_x, tip_y, 4);
                SDL_RenderDrawLine(ren, tip_x - 10, tip_y, tip_x + 10, tip_y);
                SDL_RenderDrawLine(ren, tip_x, tip_y - 10, tip_x, tip_y + 10);
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            }
        } else if (p.type == PROJ_SCORCHING_SANDS) {
            // --- ROLLING SURGING BOILING SAND WAVE ---
            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : 1.0f;
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : 0.0f;

            // 1. Trailing excavated sand furrow wake behind wave crest
            for (int tr = 1; tr <= 3; ++tr) {
                int tx = sx - (int)(dir_x * (float)tr * 12.0f);
                int ty = sy - (int)(dir_y * (float)tr * 12.0f);
                SDL_SetRenderDrawColor(ren, 120, 53, 15, (Uint8)(170 - tr * 40));
                fill_circle(ren, tx, ty, (int)(r * 0.65f - tr * 2));
                SDL_SetRenderDrawColor(ren, 217, 119, 6, (Uint8)(210 - tr * 50));
                fill_circle(ren, tx, ty, (int)(r * 0.35f - tr * 2));
            }

            // 2. Surging forward crescent ground wave
            float center_ang = atan2f(dir_y, dir_x);
            float arc_span = 2.1f; // ~120 degree crescent wave crest
            SDL_SetRenderDrawColor(ren, 245, 158, 11, 230); // Molten boiling sand
            for (int w = -3; w <= 3; ++w) {
                float cur_r = (float)r + (float)w * 2.0f;
                int prev_ax = 0, prev_ay = 0;
                for (int step = -10; step <= 10; ++step) {
                    float a = center_ang + ((float)step / 10.0f) * (arc_span * 0.5f);
                    int ax = sx + (int)(cosf(a) * cur_r);
                    int ay = sy + (int)(sinf(a) * cur_r);
                    if (step > -10) SDL_RenderDrawLine(ren, prev_ax, prev_ay, ax, ay);
                    prev_ax = ax; prev_ay = ay;
                }
            }

            // 3. Bright yellow crest edge
            SDL_SetRenderDrawColor(ren, 253, 224, 71, 255);
            for (int step = -10; step <= 10; ++step) {
                float a = center_ang + ((float)step / 10.0f) * (arc_span * 0.5f);
                int ax = sx + (int)(cosf(a) * (float)r);
                int ay = sy + (int)(sinf(a) * (float)r);
                fill_circle(ren, ax, ay, 2);
            }

            // 4. White-hot churning spray tips
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            int nose_x = sx + (int)(dir_x * (float)r);
            int nose_y = sy + (int)(dir_y * (float)r);
            fill_circle(ren, nose_x, nose_y, 4);
        } else if (p.type == PROJ_SEISMIC_GRAB) {
            // --- KINETIC CRIMSON GRAPPLE TALONS ---
            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : 1.0f;
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : 0.0f;
            float perp_x = -dir_y;
            float perp_y = dir_x;

            // Two curved grappling talons extending forward
            for (int claw = -1; claw <= 1; claw += 2) {
                float c = (float)claw;
                int root_x = sx + (int)(perp_x * c * 10.0f);
                int root_y = sy + (int)(perp_y * c * 10.0f);
                int mid_x  = sx + (int)(perp_x * c * 22.0f + dir_x * 16.0f);
                int mid_y  = sy + (int)(perp_y * c * 22.0f + dir_y * 16.0f);
                int tip_x  = sx + (int)(perp_x * c * 8.0f + dir_x * 32.0f);
                int tip_y  = sy + (int)(perp_y * c * 8.0f + dir_y * 32.0f);

                SDL_SetRenderDrawColor(ren, 239, 68, 68, 230);
                for (int w = -2; w <= 2; ++w) {
                    SDL_RenderDrawLine(ren, root_x + (int)(perp_x * w), root_y + (int)(perp_y * w),
                                           mid_x + (int)(perp_x * w), mid_y + (int)(perp_y * w));
                    SDL_RenderDrawLine(ren, mid_x + (int)(perp_x * w), mid_y + (int)(perp_y * w),
                                           tip_x + (int)(perp_x * w), tip_y + (int)(perp_y * w));
                }
                SDL_SetRenderDrawColor(ren, 250, 204, 21, 255);
                SDL_RenderDrawLine(ren, root_x, root_y, mid_x, mid_y);
                SDL_RenderDrawLine(ren, mid_x, mid_y, tip_x, tip_y);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                fill_circle(ren, tip_x, tip_y, 3);
            }
        } else if (p.type == PROJ_DRAGON_CLAW) {
            // --- 5-LAYER DRACONIC SCYTHE TALONS ---
            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : cosf(p.angle);
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : sinf(p.angle);
            float perp_x = -dir_y;
            float perp_y = dir_x;

            float life_prog = (p.max_lifetime > 0.001f) ? (1.0f - p.lifetime / p.max_lifetime) : 1.0f;
            float slash_len = 50.0f * (0.5f + life_prog * 0.5f);
            float fade = (p.lifetime > 0.08f) ? 1.0f : (p.lifetime / 0.08f);

            // Layer 1: Deep Indigo Spectral Draconic Aura
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 79, 70, 229, (Uint8)(150 * fade));
            for (int claw = -1; claw <= 1; ++claw) {
                float claw_offset = (float)claw * 16.0f;
                int aura_x = sx + (int)(perp_x * claw_offset);
                int aura_y = sy + (int)(perp_y * claw_offset);
                fill_circle(ren, aura_x, aura_y, 8);
            }

            // 3 Parallel Sweeping Curved Draconic Scythe Talons
            for (int claw = -1; claw <= 1; ++claw) {
                float claw_offset = (float)claw * 16.0f;
                int root_x = sx + (int)(perp_x * claw_offset - dir_x * (slash_len * 0.5f));
                int root_y = sy + (int)(perp_y * claw_offset - dir_y * (slash_len * 0.5f));

                int mid_x = sx + (int)(perp_x * (claw_offset + 8.0f));
                int mid_y = sy + (int)(perp_y * (claw_offset + 8.0f));

                int tip_x = sx + (int)(perp_x * (claw_offset - 5.0f) + dir_x * (slash_len * 0.5f));
                int tip_y = sy + (int)(perp_y * (claw_offset - 5.0f) + dir_y * (slash_len * 0.5f));

                // Layer 2: Emerald Dragon Scale Body (7 lines wide)
                SDL_SetRenderDrawColor(ren, 16, 185, 129, (Uint8)(240 * fade));
                for (int w = -3; w <= 3; ++w) {
                    SDL_RenderDrawLine(ren, root_x + (int)(perp_x * w), root_y + (int)(perp_y * w),
                                           mid_x + (int)(perp_x * w), mid_y + (int)(perp_y * w));
                    SDL_RenderDrawLine(ren, mid_x + (int)(perp_x * w), mid_y + (int)(perp_y * w),
                                           tip_x + (int)(perp_x * w), tip_y + (int)(perp_y * w));
                }

                // Layer 3: Brilliant Jade Blade Spine (with Additive Blend)
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
                SDL_SetRenderDrawColor(ren, 52, 211, 153, (Uint8)(255 * fade));
                for (int w = -1; w <= 1; ++w) {
                    SDL_RenderDrawLine(ren, root_x + (int)(perp_x * w), root_y + (int)(perp_y * w),
                                           mid_x + (int)(perp_x * w), mid_y + (int)(perp_y * w));
                    SDL_RenderDrawLine(ren, mid_x + (int)(perp_x * w), mid_y + (int)(perp_y * w),
                                           tip_x + (int)(perp_x * w), tip_y + (int)(perp_y * w));
                }

                // Layer 4: Pale Mint Razor-Sharp Cutting Edge & Piercing Scythe Point
                SDL_SetRenderDrawColor(ren, 209, 250, 229, (Uint8)(255 * fade));
                SDL_RenderDrawLine(ren, root_x, root_y, mid_x, mid_y);
                SDL_RenderDrawLine(ren, mid_x, mid_y, tip_x, tip_y);
                fill_circle(ren, tip_x, tip_y, 3);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(255 * fade));
                fill_circle(ren, tip_x, tip_y, 1);
            }

            // Layer 5: Draconic Energy Spark Arcs projecting forward from scythe tips
            SDL_SetRenderDrawColor(ren, 167, 243, 208, (Uint8)(220 * fade));
            int arc_front_x = sx + (int)(dir_x * (slash_len * 0.52f));
            int arc_front_y = sy + (int)(dir_y * (slash_len * 0.52f));
            SDL_RenderDrawLine(ren, arc_front_x - (int)(perp_x * 20.0f), arc_front_y - (int)(perp_y * 20.0f),
                                   arc_front_x + (int)(perp_x * 20.0f), arc_front_y + (int)(perp_y * 20.0f));

            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_VOLT_TACKLE) {
            // --- SUPERSONIC VOLT TACKLE ANIME RUSH ---
            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : 1.0f;
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : 0.0f;
            float perp_x = -dir_y;
            float perp_y = dir_x;

            // 1. Trailing Ground Scorch & Ionized Spark Wake
            for (int tr = 1; tr <= 4; ++tr) {
                int tx = sx - (int)(dir_x * (float)tr * 16.0f);
                int ty = sy - (int)(dir_y * (float)tr * 16.0f);
                SDL_SetRenderDrawColor(ren, 56, 189, 248, (Uint8)(170 - tr * 35));
                fill_circle(ren, tx, ty, (int)(r * 0.55f - tr * 3));
                SDL_SetRenderDrawColor(ren, 250, 204, 21, (Uint8)(220 - tr * 45));
                fill_circle(ren, tx, ty, (int)(r * 0.32f - tr * 2));
            }

            // 2. Forward Penetrating Plasma Fangs (Shock cones piercing air)
            int nose_x = sx + (int)(dir_x * ((float)r + 14.0f));
            int nose_y = sy + (int)(dir_y * ((float)r + 14.0f));
            for (int f = -1; f <= 1; ++f) {
                float fang_off = (float)f * 10.0f;
                int fx = nose_x + (int)(perp_x * fang_off + dir_x * 8.0f);
                int fy = nose_y + (int)(perp_y * fang_off + dir_y * 8.0f);
                int bx = sx + (int)(dir_x * (float)r * 0.6f + perp_x * (fang_off * 1.8f));
                int by = sy + (int)(dir_y * (float)r * 0.6f + perp_y * (fang_off * 1.8f));
                SDL_SetRenderDrawColor(ren, 250, 204, 21, 230);
                for (int w = -2; w <= 2; ++w) {
                    SDL_RenderDrawLine(ren, bx + (int)(perp_x * w), by + (int)(perp_y * w), fx, fy);
                }
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                SDL_RenderDrawLine(ren, bx, by, fx, fy);
            }

            // 3. Multi-Layer Streamlined Plasma Cocoon
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 160); // Outer azure ionization haze
            fill_circle(ren, sx, sy, r + 10);
            SDL_SetRenderDrawColor(ren, 250, 204, 21, 230); // Roaring electric gold mantle
            fill_circle(ren, sx, sy, r + 3);
            SDL_SetRenderDrawColor(ren, 254, 240, 138, 255); // Radiant solar yellow shell
            fill_circle(ren, sx, sy, (int)(r * 0.65f));
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255); // Blinding white-hot core
            fill_circle(ren, sx, sy, (int)(r * 0.35f));

            // 4. Dynamic Branching Lightning Corona (8 rotating jagged bolts)
            for (int b = 0; b < 8; ++b) {
                float bang = (float)b * 0.7854f + p.extra_timer * 18.0f;
                float bolt_len = (float)r * (1.1f + 0.35f * sinf(p.extra_timer * 25.0f + (float)b));
                int mx1 = sx + (int)(cosf(bang) * (float)r * 0.4f);
                int my1 = sy + (int)(sinf(bang) * (float)r * 0.4f);
                int mx2 = sx + (int)(cosf(bang + 0.15f) * (bolt_len * 0.7f));
                int my2 = sy + (int)(sinf(bang + 0.15f) * (bolt_len * 0.7f));
                int mx3 = sx + (int)(cosf(bang - 0.10f) * bolt_len);
                int my3 = sy + (int)(sinf(bang - 0.10f) * bolt_len);
                SDL_SetRenderDrawColor(ren, 56, 189, 248, 220);
                for (int w = -1; w <= 1; ++w) {
                    SDL_RenderDrawLine(ren, mx1 + w, my1, mx2 + w, my2);
                    SDL_RenderDrawLine(ren, mx2 + w, my2, mx3 + w, my3);
                }
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                SDL_RenderDrawLine(ren, mx1, my1, mx2, my2);
                SDL_RenderDrawLine(ren, mx2, my2, mx3, my3);
            }
        } else if (p.type == PROJ_DRAGON_RAGE) {
            // --- AZURE-INDIGO DRACONIC PLASMA BLAST ---
            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : 1.0f;
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : 0.0f;

            // 1. Trailing Draconic Comet Wake
            for (int tr = 1; tr <= 4; ++tr) {
                int tx = sx - (int)(dir_x * (float)tr * 14.0f);
                int ty = sy - (int)(dir_y * (float)tr * 14.0f);
                SDL_SetRenderDrawColor(ren, 79, 70, 229, (Uint8)(160 - tr * 35)); // Indigo
                fill_circle(ren, tx, ty, (int)(r * 0.65f - tr * 3));
                SDL_SetRenderDrawColor(ren, 56, 189, 248, (Uint8)(210 - tr * 45)); // Electric cyan
                fill_circle(ren, tx, ty, (int)(r * 0.35f - tr * 2));
            }

            // 2. Concentric Draconic Spheres
            SDL_SetRenderDrawColor(ren, 99, 102, 241, 160); // Atmospheric distortion aura
            fill_circle(ren, sx, sy, r + 8);
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 230); // Blazing cyan dragon flame mantle
            fill_circle(ren, sx, sy, r + 2);
            SDL_SetRenderDrawColor(ren, 186, 230, 253, 255); // Sky-blue plasma shell
            fill_circle(ren, sx, sy, (int)(r * 0.62f));
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255); // Incandescent white core
            fill_circle(ren, sx, sy, (int)(r * 0.30f));

            // 3. Three Orbiting Draconic Wisps (Tilted 3D orbit)
            for (int w = 0; w < 3; ++w) {
                float orb_a = p.extra_timer * 11.0f + (float)w * 2.0944f;
                int wx = sx + (int)(cosf(orb_a) * ((float)r + 12.0f));
                int wy = sy + (int)(sinf(orb_a) * ((float)r + 12.0f) * 0.45f);
                SDL_SetRenderDrawColor(ren, 99, 102, 241, 200);
                fill_circle(ren, wx, wy, 5);
                SDL_SetRenderDrawColor(ren, 56, 189, 248, 255);
                fill_circle(ren, wx, wy, 3);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                fill_circle(ren, wx, wy, 1);
            }

            // 4. Crackling Draconic Discharge Filaments
            for (int f = 0; f < 6; ++f) {
                float fang = (float)f * 1.047f + p.extra_timer * 14.0f;
                int fx1 = sx + (int)(cosf(fang) * (float)r * 0.5f);
                int fy1 = sy + (int)(sinf(fang) * (float)r * 0.5f);
                int fx2 = sx + (int)(cosf(fang + 0.25f) * ((float)r + 10.0f));
                int fy2 = sy + (int)(sinf(fang + 0.25f) * ((float)r + 10.0f));
                SDL_SetRenderDrawColor(ren, 165, 180, 252, 230);
                SDL_RenderDrawLine(ren, fx1, fy1, fx2, fy2);
            }
        } else if (p.type == PROJ_EARTHQUAKE) {
            if (p.delay_timer > 0.0f) {
                // --- PHASE 1: GROUND CRACK TELEGRAPH & TECTONIC FISSURES ---
                float prog = 1.0f - (p.delay_timer / 0.20f);
                if (prog < 0.0f) prog = 0.0f;
                if (prog > 1.0f) prog = 1.0f;

                // Pulsing amber ground danger zone
                Uint8 danger_a = (Uint8)(40 + (int)(prog * 80));
                SDL_SetRenderDrawColor(ren, 180, 83, 9, danger_a);
                fill_circle(ren, sx, sy, (int)((float)r * prog));

                // Ground perimeter warning circle with inward chevrons at R=135px
                SDL_SetRenderDrawColor(ren, 245, 158, 11, (Uint8)(120 + prog * 120));
                for (int ch = 0; ch < 12; ++ch) {
                    float ch_ang = (float)ch * 0.5236f;
                    int cx1 = sx + (int)(cosf(ch_ang) * (float)r);
                    int cy1 = sy + (int)(sinf(ch_ang) * (float)r);
                    int cx2 = sx + (int)(cosf(ch_ang) * ((float)r - 12.0f * prog));
                    int cy2 = sy + (int)(sinf(ch_ang) * ((float)r - 12.0f * prog));
                    SDL_RenderDrawLine(ren, cx1, cy1, cx2, cy2);
                }

                // 6 Jagged tectonic fissure fractures spreading outward from center
                SDL_SetRenderDrawColor(ren, 245, 158, 11, 240);
                for (int c = 0; c < 6; ++c) {
                    float c_ang = (float)c * 1.047f + 0.15f;
                    int p1_x = sx + (int)(cosf(c_ang) * 8.0f);
                    int p1_y = sy + (int)(sinf(c_ang) * 8.0f);
                    int p2_x = sx + (int)(cosf(c_ang + 0.22f) * ((float)r * 0.55f * prog));
                    int p2_y = sy + (int)(sinf(c_ang + 0.22f) * ((float)r * 0.55f * prog));
                    int p3_x = sx + (int)(cosf(c_ang - 0.14f) * ((float)r * 0.95f * prog));
                    int p3_y = sy + (int)(sinf(c_ang - 0.14f) * ((float)r * 0.95f * prog));
                    SDL_RenderDrawLine(ren, p1_x, p1_y, p2_x, p2_y);
                    SDL_RenderDrawLine(ren, p2_x, p2_y, p3_x, p3_y);
                }
            } else {
                // --- PHASE 2: GROUND-LEVEL SEISMIC RUPTURE & ERUPTING BOULDERS ---
                float fade = (p.lifetime > 0.0f) ? (p.lifetime / 0.50f) : 0.0f;
                if (fade > 1.0f) fade = 1.0f;

                // 1. Shattered turf and scorched earth crater base
                SDL_SetRenderDrawColor(ren, 78, 42, 10, (Uint8)(fade * 160));
                fill_circle(ren, sx, sy, (int)(r * 0.85f));

                // 2. Six permanent deep tectonic fissure cracks
                SDL_SetRenderDrawColor(ren, 30, 20, 10, (Uint8)(fade * 255));
                for (int c = 0; c < 6; ++c) {
                    float c_ang = (float)c * 1.047f + 0.15f;
                    int p1_x = sx + (int)(cosf(c_ang) * 8.0f);
                    int p1_y = sy + (int)(sinf(c_ang) * 8.0f);
                    int p2_x = sx + (int)(cosf(c_ang + 0.22f) * ((float)r * 0.55f));
                    int p2_y = sy + (int)(sinf(c_ang + 0.22f) * ((float)r * 0.55f));
                    int p3_x = sx + (int)(cosf(c_ang - 0.14f) * ((float)r * 0.95f));
                    int p3_y = sy + (int)(sinf(c_ang - 0.14f) * ((float)r * 0.95f));
                    for (int w = -1; w <= 1; ++w) {
                        SDL_RenderDrawLine(ren, p1_x + w, p1_y, p2_x + w, p2_y);
                        SDL_RenderDrawLine(ren, p2_x + w, p2_y, p3_x + w, p3_y);
                    }
                }

                // 3. Dual Expanding Shockwave Rings
                float wave_r = (float)r * (0.3f + (1.0f - fade) * 0.7f);
                SDL_SetRenderDrawColor(ren, 180, 83, 9, (Uint8)(fade * 240));
                for (int w = -3; w <= 3; ++w) {
                    int cur_ring_r = (int)wave_r + w;
                    if (cur_ring_r > 0) {
                        for (int seg = 0; seg < 32; ++seg) {
                            float a1 = (float)seg * 0.1963f;
                            float a2 = (float)(seg + 1) * 0.1963f;
                            SDL_RenderDrawLine(ren,
                                sx + (int)(cosf(a1) * (float)cur_ring_r), sy + (int)(sinf(a1) * (float)cur_ring_r),
                                sx + (int)(cosf(a2) * (float)cur_ring_r), sy + (int)(sinf(a2) * (float)cur_ring_r));
                        }
                    }
                }

                float wave2_r = wave_r * 0.72f;
                SDL_SetRenderDrawColor(ren, 245, 158, 11, (Uint8)(fade * 255));
                for (int w = -1; w <= 1; ++w) {
                    int cur_ring_r = (int)wave2_r + w;
                    if (cur_ring_r > 0) {
                        for (int seg = 0; seg < 24; ++seg) {
                            float a1 = (float)seg * 0.2618f;
                            float a2 = (float)(seg + 1) * 0.2618f;
                            SDL_RenderDrawLine(ren,
                                sx + (int)(cosf(a1) * (float)cur_ring_r), sy + (int)(sinf(a1) * (float)cur_ring_r),
                                sx + (int)(cosf(a2) * (float)cur_ring_r), sy + (int)(sinf(a2) * (float)cur_ring_r));
                        }
                    }
                }

                // 4. Erupting Rock Shards / Boulders along the shockwave
                for (int rk = 0; rk < 8; ++rk) {
                    float rk_ang = (float)rk * 0.7854f + 0.25f;
                    int rx = sx + (int)(cosf(rk_ang) * wave_r);
                    int ry = sy + (int)(sinf(rk_ang) * wave_r);
                    int rk_size = (int)(6.0f + 4.0f * sinf((float)rk * 1.5f));
                    SDL_SetRenderDrawColor(ren, 120, 53, 15, (Uint8)(fade * 240));
                    fill_circle(ren, rx, ry, rk_size);
                    SDL_SetRenderDrawColor(ren, 217, 119, 6, (Uint8)(fade * 255));
                    fill_circle(ren, rx - 1, ry - 1, rk_size / 2);
                }
            }
        } else if (p.type == PROJ_AURA_SPHERE) {
            // --- 5-LAYER CELESTIAL SAPPHIRE AURA SPHERE ---
            // Layer 1: Ground drop shadow
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 15, 23, 42, 90);
            fill_circle(ren, sx, sy + 18, (int)(p.radius * 0.70f));

            // Layer 3: Additive outer sapphire aura with breathing oscillation
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            float pulse = sinf(p.extra_timer * 16.0f) * 3.5f;
            int aura_r = (int)(p.radius * 1.45f + pulse);
            if (aura_r < (int)p.radius + 2) aura_r = (int)p.radius + 2;
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 140);
            fill_circle(ren, sx, sy, aura_r);

            // Layer 4: Deep celestial blue sphere with revolving curved spiral arms
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 14, 165, 233, 235);
            fill_circle(ren, sx, sy, (int)p.radius);

            // 3 revolving spiraling wisps
            for (int w = 0; w < 3; ++w) {
                float w_ang = p.extra_timer * 12.0f + (float)w * 2.0944f;
                int wx = sx + (int)(cosf(w_ang) * (p.radius + 4.0f));
                int wy = sy + (int)(sinf(w_ang) * (p.radius + 4.0f));
                SDL_SetRenderDrawColor(ren, 186, 230, 253, 210);
                fill_circle(ren, wx, wy, 4);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                fill_circle(ren, wx, wy, 2);
            }

            // Layer 5: Blinding white nucleus with 4-way diagonal specular flares
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, sx, sy, (int)(p.radius * 0.48f));

            int fl = (int)(p.radius * 1.5f);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 220);
            SDL_RenderDrawLine(ren, sx - fl, sy, sx + fl, sy);
            SDL_RenderDrawLine(ren, sx, sy - fl, sx, sy + fl);
            int d_fl = (int)((float)fl * 0.707f);
            SDL_RenderDrawLine(ren, sx - d_fl, sy - d_fl, sx + d_fl, sy + d_fl);
            SDL_RenderDrawLine(ren, sx - d_fl, sy + d_fl, sx + d_fl, sy - d_fl);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_FORCE_PALM) {
            // --- 5-LAYER COMPRESSED KINETIC PALM SHOCKWAVE CONE ---
            float f_cos = cosf(p.angle);
            float f_sin = sinf(p.angle);
            float f_perp_x = -f_sin;
            float f_perp_y = f_cos;

            // Layer 1: Ground air-blast compression furrow shadow
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 15, 23, 42, 80);
            fill_circle(ren, sx, sy + 14, 18);

            // Layer 3: Additive outer cyan kinetic shockwave cone
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            for (int arc = 1; arc <= 4; ++arc) {
                float dist = (float)arc * 9.0f;
                int cx = sx + (int)(f_cos * dist);
                int cy = sy + (int)(f_sin * dist);
                int span = (int)(14.0f + (float)arc * 9.0f);
                SDL_SetRenderDrawColor(ren, 56, 189, 248, (Uint8)(170 - arc * 30));
                for (int w = -1; w <= 1; ++w) {
                    SDL_RenderDrawLine(ren, cx - (int)(f_perp_x * span) + (int)(f_cos * (float)w),
                                           cy - (int)(f_perp_y * span) + (int)(f_sin * (float)w),
                                           cx + (int)(f_perp_x * span) + (int)(f_cos * (float)w),
                                           cy + (int)(f_perp_y * span) + (int)(f_sin * (float)w));
                }
            }

            // Layer 4: Saturated auric gold palm projection imprint
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 250, 204, 21, 230);
            int palm_x = sx + (int)(f_cos * 16.0f);
            int palm_y = sy + (int)(f_sin * 16.0f);
            fill_circle(ren, palm_x, palm_y, 8);
            // 5 radiating martial finger rays
            for (int fin = -2; fin <= 2; ++fin) {
                float f_ang = p.angle + (float)fin * 0.22f;
                int fin_x = palm_x + (int)(cosf(f_ang) * 16.0f);
                int fin_y = palm_y + (int)(sinf(f_ang) * 16.0f);
                SDL_SetRenderDrawColor(ren, 250, 204, 21, 240);
                SDL_RenderDrawLine(ren, palm_x, palm_y, fin_x, fin_y);
            }

            // Layer 5: Blinding white compression core & cross flares
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, palm_x, palm_y, 4);
            SDL_RenderDrawLine(ren, palm_x - 14, palm_y, palm_x + 14, palm_y);
            SDL_RenderDrawLine(ren, palm_x, palm_y - 14, palm_x, palm_y + 14);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_CLOSE_COMBAT) {
            // --- 5-LAYER MARTIAL FLURRY IMPACT BURSTS ---
            // Layer 3: Additive crimson energy blast aura
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 239, 68, 68, 150);
            fill_circle(ren, sx, sy, 26);

            // Layer 4: Multi-angle rapid fist impacts with motion smears
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            for (int k = 0; k < 4; ++k) {
                float flash_ang = (float)k * 1.5708f + p.extra_timer * 18.0f;
                int fx = sx + (int)(cosf(flash_ang) * 16.0f);
                int fy = sy + (int)(sinf(flash_ang) * 16.0f);
                // Trailing fist smear
                int fx_tail = fx - (int)(cosf(flash_ang) * 8.0f);
                int fy_tail = fy - (int)(sinf(flash_ang) * 8.0f);
                SDL_SetRenderDrawColor(ren, 239, 68, 68, 180);
                SDL_RenderDrawLine(ren, fx_tail, fy_tail, fx, fy);

                SDL_SetRenderDrawColor(ren, 250, 204, 21, 250);
                fill_circle(ren, fx, fy, 6);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                fill_circle(ren, fx, fy, 3);
            }

            // Layer 5: Blinding white-hot cross impact bursts
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, sx, sy, 5);
            SDL_RenderDrawLine(ren, sx - 22, sy, sx + 22, sy);
            SDL_RenderDrawLine(ren, sx, sy - 22, sx, sy + 22);
            SDL_RenderDrawLine(ren, sx - 15, sy - 15, sx + 15, sy + 15);
            SDL_RenderDrawLine(ren, sx - 15, sy + 15, sx + 15, sy - 15);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_BONE_RUSH) {
            // --- AURIC BONE JAVELIN: RAPID 3D ROTATING DUAL-ENDED BONE STAFF ---
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

            // 1. High-Speed Angular Rotation
            // Spins rapidly at 26 rad/s around its center
            float rot_ang = p.angle + p.extra_timer * 26.0f;
            float b_cos = cosf(rot_ang);
            float b_sin = sinf(rot_ang);
            float bp_x = -b_sin;
            float bp_y = b_cos;

            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float move_dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : 1.0f;
            float move_dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : 0.0f;

            // 2. Trailing Ground Furrow & Auric Motion Blur Discs
            for (int tr = 1; tr <= 3; ++tr) {
                float tr_dist = (float)tr * 14.0f;
                int tx = sx - (int)(move_dir_x * tr_dist);
                int ty = sy - (int)(move_dir_y * tr_dist);
                // Auric cyan afterimage ring
                SDL_SetRenderDrawColor(ren, 56, 189, 248, (Uint8)(110 - tr * 30));
                draw_circle_outline(ren, tx, ty, (int)(p.radius * 0.75f - tr * 3), 2);
                // Ground dust / ember afterglow
                SDL_SetRenderDrawColor(ren, 217, 119, 6, (Uint8)(85 - tr * 25));
                fill_circle(ren, tx, ty, (int)(4 - tr));
            }

            // 3. Ethereal Radial Spin Blur Disc
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 70);
            fill_circle(ren, sx, sy, (int)p.radius);
            SDL_SetRenderDrawColor(ren, 186, 230, 253, 110);
            draw_circle_outline(ren, sx, sy, (int)p.radius, 2);

            // 4. Central Solid Auric Bone Shaft
            float half_len = 26.0f;
            int x1 = sx - (int)(b_cos * half_len);
            int y1 = sy - (int)(b_sin * half_len);
            int x2 = sx + (int)(b_cos * half_len);
            int y2 = sy + (int)(b_sin * half_len);

            // Thick cyan aura glow sheath (5-line width)
            SDL_SetRenderDrawColor(ren, 14, 165, 233, 180);
            for (int off = -3; off <= 3; ++off) {
                SDL_RenderDrawLine(ren, x1 + (int)(bp_x * off), y1 + (int)(bp_y * off),
                                       x2 + (int)(bp_x * off), y2 + (int)(bp_y * off));
            }
            // Radiant turquoise mantle
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 240);
            for (int off = -1; off <= 1; ++off) {
                SDL_RenderDrawLine(ren, x1 + (int)(bp_x * off), y1 + (int)(bp_y * off),
                                       x2 + (int)(bp_x * off), y2 + (int)(bp_y * off));
            }
            // Core luminous white spine
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            SDL_RenderDrawLine(ren, x1, y1, x2, y2);

            // 5. Dual Sculpted Bulbous Bone Joints (Epiphyses) on Both Ends
            auto draw_bone_head = [&](int cx, int cy, float dir_sign) {
                float head_off = 5.0f;
                int h1_x = cx + (int)(bp_x * head_off + b_cos * dir_sign * 2.0f);
                int h1_y = cy + (int)(bp_y * head_off + b_sin * dir_sign * 2.0f);
                int h2_x = cx - (int)(bp_x * head_off - b_cos * dir_sign * 2.0f);
                int h2_y = cy - (int)(bp_y * head_off - b_sin * dir_sign * 2.0f);

                // Cyan outer joint glow
                SDL_SetRenderDrawColor(ren, 14, 165, 233, 220);
                fill_circle(ren, h1_x, h1_y, 6);
                fill_circle(ren, h2_x, h2_y, 6);

                // Bright auric core
                SDL_SetRenderDrawColor(ren, 56, 189, 248, 255);
                fill_circle(ren, h1_x, h1_y, 4);
                fill_circle(ren, h2_x, h2_y, 4);

                // Brilliant specular tip
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                fill_circle(ren, h1_x, h1_y, 2);
                fill_circle(ren, h2_x, h2_y, 2);
            };

            draw_bone_head(x1, y1, -1.0f);
            draw_bone_head(x2, y2, +1.0f);

            // 6. Central Auric Pivot Gem
            SDL_SetRenderDrawColor(ren, 254, 240, 138, 230);
            fill_circle(ren, sx, sy, 4);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, sx, sy, 2);
        } else if (p.type == PROJ_EXTREME_SPEED) {
            // Supersonic white aerodynamic cone with speed lines
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            float es_cos = cosf(p.angle);
            float es_sin = sinf(p.angle);
            float es_px = -es_sin;
            float es_py = es_cos;

            // Dual outer shockwave cones (Mach cones)
            for (int c = 1; c <= 2; ++c) {
                float c_mult = (float)c * 10.0f;
                int c_tip_x = sx + (int)(es_cos * (28.0f - c_mult * 0.4f));
                int c_tip_y = sy + (int)(es_sin * (28.0f - c_mult * 0.4f));
                int c_b1_x = sx - (int)(es_cos * (14.0f + c_mult) - es_px * (16.0f + c_mult * 0.8f));
                int c_b1_y = sy - (int)(es_sin * (14.0f + c_mult) - es_py * (16.0f + c_mult * 0.8f));
                int c_b2_x = sx - (int)(es_cos * (14.0f + c_mult) + es_px * (16.0f + c_mult * 0.8f));
                int c_b2_y = sy - (int)(es_sin * (14.0f + c_mult) + es_py * (16.0f + c_mult * 0.8f));
                SDL_SetRenderDrawColor(ren, 186, 230, 253, (Uint8)(180 - c * 60));
                SDL_RenderDrawLine(ren, c_tip_x, c_tip_y, c_b1_x, c_b1_y);
                SDL_RenderDrawLine(ren, c_tip_x, c_tip_y, c_b2_x, c_b2_y);
            }

            int tip_x = sx + (int)(es_cos * 28.0f);
            int tip_y = sy + (int)(es_sin * 28.0f);
            int base1_x = sx - (int)(es_cos * 16.0f - es_px * 16.0f);
            int base1_y = sy - (int)(es_sin * 16.0f - es_py * 16.0f);
            int base2_x = sx - (int)(es_cos * 16.0f + es_px * 16.0f);
            int base2_y = sy - (int)(es_sin * 16.0f + es_py * 16.0f);

            SDL_SetRenderDrawColor(ren, 255, 255, 255, 240);
            SDL_RenderDrawLine(ren, tip_x, tip_y, base1_x, base1_y);
            SDL_RenderDrawLine(ren, tip_x, tip_y, base2_x, base2_y);
            SDL_RenderDrawLine(ren, base1_x, base1_y, base2_x, base2_y);

            SDL_SetRenderDrawColor(ren, 186, 230, 253, 190);
            for (int s = -2; s <= 2; ++s) {
                int tail_x = sx - (int)(es_cos * (42.0f + (float)abs(s) * 8.0f) + es_px * (float)s * 10.0f);
                int tail_y = sy - (int)(es_sin * (42.0f + (float)abs(s) * 8.0f) + es_py * (float)s * 10.0f);
                SDL_RenderDrawLine(ren, sx, sy, tail_x, tail_y);
            }
        } else if (p.type == PROJ_FLASH_CANNON) {
            // --- 5-LAYER RELATIVISTIC STEEL CANNON WITH ADDITIVE MAGNETIC AURA ---
            // Layer 1: Ground drop shadow beneath beam
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 15, 23, 42, 85);
            fill_circle(ren, sx, sy + 16, (int)(p.radius * 0.70f));

            // Layer 3: Additive Slate Steel Magnetic Confinement Aura with Sinusoidal Rings
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            float pulse = sinf(p.extra_timer * 24.0f) * 3.0f;
            int aura_r = (int)(p.radius * 1.45f + pulse);
            if (aura_r < (int)p.radius + 2) aura_r = (int)p.radius + 2;
            SDL_SetRenderDrawColor(ren, 148, 163, 184, 150);
            fill_circle(ren, sx, sy, aura_r);

            // Pulsating magnetic rings along the trajectory
            float f_cos = cosf(p.angle);
            float f_sin = sinf(p.angle);
            for (int rg = 1; rg <= 3; ++rg) {
                int rx = sx - (int)(f_cos * (float)rg * 14.0f);
                int ry = sy - (int)(f_sin * (float)rg * 14.0f);
                SDL_SetRenderDrawColor(ren, 203, 213, 225, (Uint8)(160 - rg * 40));
                draw_circle_outline(ren, rx, ry, (int)(p.radius * 0.90f - (float)rg * 2.0f), 2);
            }

            // Layer 4: Saturated Polished Chrome Energy Cylinder with Heavy Border
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 203, 213, 225, 235);
            fill_circle(ren, sx, sy, (int)p.radius);
            SDL_SetRenderDrawColor(ren, 148, 163, 184, 255);
            draw_circle_outline(ren, sx, sy, (int)p.radius, 2);

            // Layer 5: Pure White Relativistic Laser Core + 8-Axis Specular Flare Cross
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, sx, sy, (int)(p.radius * 0.55f));

            int fl = (int)(p.radius * 1.8f);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 240);
            SDL_RenderDrawLine(ren, sx - fl, sy, sx + fl, sy);
            SDL_RenderDrawLine(ren, sx, sy - fl, sx, sy + fl);
            int d_fl = (int)((float)fl * 0.707f);
            SDL_RenderDrawLine(ren, sx - d_fl, sy - d_fl, sx + d_fl, sy + d_fl);
            SDL_RenderDrawLine(ren, sx - d_fl, sy + d_fl, sx + d_fl, sy - d_fl);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_DRAGON_PULSE) {
            // --- SPECTRAL ROARING DRAGON HEAD WITH SERPENTINE BODY ---
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : cosf(p.angle);
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : sinf(p.angle);
            float perp_x = -dir_y;
            float perp_y = dir_x;

            // 1. Serpentine Undulating Draconic Body & Scale Segments (5 segments behind head)
            for (int seg = 5; seg >= 1; --seg) {
                float seg_dist = (float)seg * 16.0f;
                // Sinusoidal serpentine lateral oscillation
                float wave = sinf(p.extra_timer * 18.0f - (float)seg * 0.90f) * (8.0f + (float)seg * 2.0f);
                int bx = sx - (int)(dir_x * seg_dist - perp_x * wave);
                int by = sy - (int)(dir_y * seg_dist - perp_y * wave);

                int seg_r = (int)(p.radius * (1.1f - (float)seg * 0.14f));
                if (seg_r < 5) seg_r = 5;

                // Translucent indigo aura halo
                SDL_SetRenderDrawColor(ren, 99, 102, 241, (Uint8)(100 + seg * 12));
                fill_circle(ren, bx, by, seg_r + 4);

                // Emerald dragon scale mantle
                SDL_SetRenderDrawColor(ren, 16, 185, 129, (Uint8)(170 + seg * 15));
                fill_circle(ren, bx, by, seg_r);

                // Cyan luminous core
                SDL_SetRenderDrawColor(ren, 56, 189, 248, (Uint8)(210 + seg * 8));
                fill_circle(ren, bx, by, (int)(seg_r * 0.55f));

                // Dragon dorsal scale crest (scale sticking outward)
                int dorsal_x = bx + (int)(perp_x * ((float)seg_r + 5.0f));
                int dorsal_y = by + (int)(perp_y * ((float)seg_r + 5.0f));
                SDL_SetRenderDrawColor(ren, 52, 211, 153, 240);
                SDL_RenderDrawLine(ren, bx, by, dorsal_x, dorsal_y);
            }

            // 2. Roaring Draconic Jaws (Gaping Upper & Lower Dragon Mouth)
            int jaw_base_x = sx - (int)(dir_x * 8.0f);
            int jaw_base_y = sy - (int)(dir_y * 8.0f);

            // Upper Snout & Horns:
            int upper_snout_x = sx + (int)(dir_x * 24.0f + perp_x * 9.0f);
            int upper_snout_y = sy + (int)(dir_y * 24.0f + perp_y * 9.0f);

            // Lower Gaping Jaw:
            int lower_snout_x = sx + (int)(dir_x * 20.0f - perp_x * 12.0f);
            int lower_snout_y = sy + (int)(dir_y * 20.0f - perp_y * 12.0f);

            // Dragon Cranium / Crest Horns extending backward:
            int horn_top_x = sx - (int)(dir_x * 18.0f - perp_x * 16.0f);
            int horn_top_y = sy - (int)(dir_y * 18.0f - perp_y * 16.0f);
            int horn_bot_x = sx - (int)(dir_x * 16.0f + perp_x * 16.0f);
            int horn_bot_y = sy - (int)(dir_y * 16.0f + perp_y * 16.0f);

            // Outer Indigo Spectral Hull (thick outlines)
            SDL_SetRenderDrawColor(ren, 79, 70, 229, 220);
            for (int w = -2; w <= 2; ++w) {
                SDL_RenderDrawLine(ren, jaw_base_x + (int)(perp_x * w), jaw_base_y + (int)(perp_y * w),
                                       upper_snout_x + (int)(perp_x * w), upper_snout_y + (int)(perp_y * w));
                SDL_RenderDrawLine(ren, jaw_base_x + (int)(perp_x * w), jaw_base_y + (int)(perp_y * w),
                                       lower_snout_x + (int)(perp_x * w), lower_snout_y + (int)(perp_y * w));
                SDL_RenderDrawLine(ren, jaw_base_x + (int)(perp_x * w), jaw_base_y + (int)(perp_y * w),
                                       horn_top_x + (int)(perp_x * w), horn_top_y + (int)(perp_y * w));
                SDL_RenderDrawLine(ren, jaw_base_x + (int)(perp_x * w), jaw_base_y + (int)(perp_y * w),
                                       horn_bot_x + (int)(perp_x * w), horn_bot_y + (int)(perp_y * w));
            }

            // Emerald Dragon Scales Fill
            SDL_SetRenderDrawColor(ren, 16, 185, 129, 240);
            SDL_RenderDrawLine(ren, jaw_base_x, jaw_base_y, upper_snout_x, upper_snout_y);
            SDL_RenderDrawLine(ren, jaw_base_x, jaw_base_y, lower_snout_x, lower_snout_y);
            SDL_RenderDrawLine(ren, jaw_base_x, jaw_base_y, horn_top_x, horn_top_y);
            SDL_RenderDrawLine(ren, jaw_base_x, jaw_base_y, horn_bot_x, horn_bot_y);

            // 3. Sharp Glowing White Draconic Fangs (4 needle fangs inside mouth)
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            int fang_u1_x = upper_snout_x - (int)(dir_x * 4.0f);
            int fang_u1_y = upper_snout_y - (int)(dir_y * 4.0f);
            SDL_RenderDrawLine(ren, fang_u1_x, fang_u1_y, fang_u1_x - (int)(perp_x * 7.0f), fang_u1_y - (int)(perp_y * 7.0f));

            int fang_u2_x = upper_snout_x - (int)(dir_x * 12.0f);
            int fang_u2_y = upper_snout_y - (int)(dir_y * 12.0f);
            SDL_RenderDrawLine(ren, fang_u2_x, fang_u2_y, fang_u2_x - (int)(perp_x * 6.0f), fang_u2_y - (int)(perp_y * 6.0f));

            int fang_d1_x = lower_snout_x - (int)(dir_x * 4.0f);
            int fang_d1_y = lower_snout_y - (int)(dir_y * 4.0f);
            SDL_RenderDrawLine(ren, fang_d1_x, fang_d1_y, fang_d1_x + (int)(perp_x * 7.0f), fang_d1_y + (int)(perp_y * 7.0f));

            // 4. Blazing Golden/Turquoise Dragon Eyes
            int eye_top_x = sx + (int)(dir_x * 6.0f + perp_x * 7.0f);
            int eye_top_y = sy + (int)(dir_y * 6.0f + perp_y * 7.0f);
            int eye_bot_x = sx + (int)(dir_x * 4.0f - perp_x * 7.0f);
            int eye_bot_y = sy + (int)(dir_y * 4.0f - perp_y * 7.0f);

            SDL_SetRenderDrawColor(ren, 250, 204, 21, 240);
            fill_circle(ren, eye_top_x, eye_top_y, 4);
            fill_circle(ren, eye_bot_x, eye_bot_y, 4);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, eye_top_x, eye_top_y, 1);
            fill_circle(ren, eye_bot_x, eye_bot_y, 1);

            // 5. Blazing Plasma Throat / Dragon Breath Core
            int throat_x = sx + (int)(dir_x * 4.0f);
            int throat_y = sy + (int)(dir_y * 4.0f);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 79, 70, 229, 170);
            fill_circle(ren, throat_x, throat_y, 14);
            SDL_SetRenderDrawColor(ren, 16, 185, 129, 220);
            fill_circle(ren, throat_x, throat_y, 9);
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 255);
            fill_circle(ren, throat_x, throat_y, 5);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, throat_x, throat_y, 2);
            SDL_RenderDrawLine(ren, throat_x - 12, throat_y, throat_x + 12, throat_y);
            SDL_RenderDrawLine(ren, throat_x, throat_y - 12, throat_x, throat_y + 12);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

            // 6. Expanding Draconic Breath Shockwave Cone out of Mouth
            for (int r_wave = 1; r_wave <= 2; ++r_wave) {
                float wave_dist = (float)r_wave * 12.0f;
                int wx = throat_x + (int)(dir_x * wave_dist);
                int wy = throat_y + (int)(dir_y * wave_dist);
                int w_span = (int)(6.0f + (float)r_wave * 7.0f);
                SDL_SetRenderDrawColor(ren, 52, 211, 153, 200 - r_wave * 60);
                SDL_RenderDrawLine(ren, wx - (int)(perp_x * w_span), wy - (int)(perp_y * w_span),
                                       wx + (int)(perp_x * w_span), wy + (int)(perp_y * w_span));
            }
        } else if (p.type == PROJ_WATER_SHURIKEN) {
            // --- 5-LAYER MULTI-TIERED HIGH-PRESSURE HYDROFOIL SHURIKEN ---
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : cosf(p.angle);
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : sinf(p.angle);

            // Layer 1: Trailing Expanding Aqua Ripple Wake Rings & Surface Tension Corona
            for (int r = 1; r <= 3; ++r) {
                float dist_back = (float)r * 13.0f;
                int wkx = sx - (int)(dir_x * dist_back);
                int wky = sy - (int)(dir_y * dist_back);
                int r_rad = (int)(p.radius * (0.6f + (float)r * 0.28f));
                SDL_SetRenderDrawColor(ren, 14, 165, 233, (Uint8)(100 - r * 28));
                draw_circle_outline(ren, wkx, wky, r_rad, 1);
            }
            SDL_SetRenderDrawColor(ren, 14, 165, 233, 90);
            fill_circle(ren, sx, sy, (int)(p.radius * 1.45f));

            // Layer 2: 4 Curved Hydrofoil Sickle Blades & Counter-Rotating Cutter Teeth
            float blade_len = p.radius * 1.75f;
            float wing_w = p.radius * 0.46f;
            for (int b = 0; b < 4; ++b) {
                float blade_ang = p.angle + (float)b * (3.14159265f / 2.0f);
                float cos_b = cosf(blade_ang);
                float sin_b = sinf(blade_ang);
                float perp_bx = -sin_b;
                float perp_by = cos_b;

                // Hydrofoil sickle curve: tip curves forward into spin
                float curve_fwd = 0.28f;
                int tip_x = sx + (int)((cos_b - perp_bx * curve_fwd) * blade_len);
                int tip_y = sy + (int)((sin_b - perp_by * curve_fwd) * blade_len);

                int l_x = sx + (int)(perp_bx * wing_w);
                int l_y = sy + (int)(perp_by * wing_w);
                int r_x = sx - (int)(perp_bx * wing_w);
                int r_y = sy - (int)(perp_by * wing_w);

                // Deep oceanic azure blade body
                SDL_SetRenderDrawColor(ren, 2, 132, 199, 220);
                for (float s = 0.0f; s <= 1.0f; s += 0.06f) {
                    int p1_x = (int)((1.0f - s) * (float)l_x + s * (float)tip_x);
                    int p1_y = (int)((1.0f - s) * (float)l_y + s * (float)tip_y);
                    int p2_x = (int)((1.0f - s) * (float)r_x + s * (float)tip_x);
                    int p2_y = (int)((1.0f - s) * (float)r_y + s * (float)tip_y);
                    SDL_RenderDrawLine(ren, p1_x, p1_y, p2_x, p2_y);
                }

                // Sharp cutting edge (cyan razor line)
                SDL_SetRenderDrawColor(ren, 56, 189, 248, 255);
                SDL_RenderDrawLine(ren, l_x, l_y, tip_x, tip_y);
                SDL_RenderDrawLine(ren, r_x, r_y, tip_x, tip_y);
            }

            // Counter-rotating secondary cutter ring teeth
            float counter_ang = -p.angle * 1.5f;
            for (int b = 0; b < 6; ++b) {
                float tooth_ang = counter_ang + (float)b * 1.047f;
                int tx = sx + (int)(cosf(tooth_ang) * (p.radius * 0.95f));
                int ty = sy + (int)(sinf(tooth_ang) * (p.radius * 0.95f));
                SDL_SetRenderDrawColor(ren, 125, 211, 252, 210);
                SDL_RenderDrawPoint(ren, tx, ty);
                SDL_RenderDrawPoint(ren, tx + 1, ty);
            }

            // Layer 3: Central High-Pressure Vortex Disc
            SDL_SetRenderDrawColor(ren, 3, 105, 161, 235);
            fill_circle(ren, sx, sy, (int)(p.radius * 0.70f));
            SDL_SetRenderDrawColor(ren, 14, 165, 233, 245);
            fill_circle(ren, sx, sy, (int)(p.radius * 0.50f));

            // Layer 4 & 5: Additive Cyan Disc, Crystalline Core and Diamond Star Glints
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 56, 189, 248, 230);
            fill_circle(ren, sx, sy, (int)(p.radius * 0.35f));

            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, sx, sy, (int)(p.radius * 0.18f));

            // 4-point rotating diamond star glints
            float glint_len = p.radius * 0.85f;
            for (int g = 0; g < 4; ++g) {
                float ga = p.angle + (float)g * 1.5708f;
                int gx = sx + (int)(cosf(ga) * glint_len);
                int gy = sy + (int)(sinf(ga) * glint_len);
                SDL_RenderDrawLine(ren, sx, sy, gx, gy);
            }
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_NIGHT_SLASH) {
            // --- 5-LAYER DUAL-CRESCENT X-CROSS VOID SLICE ---
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : cosf(p.angle);
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : sinf(p.angle);
            float perp_x = -dir_y;
            float perp_y = dir_x;

            float arc_span = p.radius * 1.6f;
            float arc_depth = p.radius * 0.90f;

            // Layer 1: Midnight Void Reality Tear (Dark outer miasma fracture)
            SDL_SetRenderDrawColor(ren, 15, 23, 42, 210);
            for (int t = -12; t <= 12; ++t) {
                float prog = (float)t / 12.0f;
                float curve = (1.0f - prog * prog) * arc_depth;
                int px = sx + (int)(dir_x * curve + perp_x * (prog * arc_span));
                int py = sy + (int)(dir_y * curve + perp_y * (prog * arc_span));
                fill_circle(ren, px, py, 7);
            }

            // Layer 2: Dual-Crescent X-Cross Void Slices (+25 deg and -25 deg crossing arcs)
            for (int cross = -1; cross <= 1; cross += 2) {
                float cross_ang = ((float)cross) * 0.42f;
                float c_dir_x = dir_x * cosf(cross_ang) - dir_y * sinf(cross_ang);
                float c_dir_y = dir_x * sinf(cross_ang) + dir_y * cosf(cross_ang);
                float c_perp_x = -c_dir_y;
                float c_perp_y = c_dir_x;

                // Deep violet body fill
                SDL_SetRenderDrawColor(ren, 126, 34, 206, 240);
                for (int t = -14; t <= 14; ++t) {
                    float prog = (float)t / 14.0f;
                    float curve = (1.0f - prog * prog) * arc_depth;
                    int px = sx + (int)(c_dir_x * curve + c_perp_x * (prog * arc_span));
                    int py = sy + (int)(c_dir_y * curve + c_perp_y * (prog * arc_span));
                    fill_circle(ren, px, py, 4);
                }
            }

            // Layer 3: Additive Magenta / Crimson Razor Cutting Edge
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 217, 70, 239, 245);
            for (int cross = -1; cross <= 1; cross += 2) {
                float cross_ang = ((float)cross) * 0.42f;
                float c_dir_x = dir_x * cosf(cross_ang) - dir_y * sinf(cross_ang);
                float c_dir_y = dir_x * sinf(cross_ang) + dir_y * cosf(cross_ang);
                float c_perp_x = -c_dir_y;
                float c_perp_y = c_dir_x;

                for (int t = -16; t <= 16; ++t) {
                    float prog = (float)t / 16.0f;
                    float curve = (1.0f - prog * prog) * (arc_depth + 4.0f);
                    int px = sx + (int)(c_dir_x * curve + c_perp_x * (prog * arc_span));
                    int py = sy + (int)(c_dir_y * curve + c_perp_y * (prog * arc_span));
                    SDL_RenderDrawPoint(ren, px, py);
                    SDL_RenderDrawPoint(ren, px + 1, py);
                    SDL_RenderDrawPoint(ren, px, py + 1);
                }
            }

            // Layer 4: Shadow Smoke Puffs & Ethereal Sparks
            SDL_SetRenderDrawColor(ren, 168, 85, 247, 180);
            for (int s = -2; s <= 2; ++s) {
                int ox = sx + (int)(perp_x * ((float)s * 11.0f) - dir_x * 8.0f);
                int oy = sy + (int)(perp_y * ((float)s * 11.0f) - dir_y * 8.0f);
                fill_circle(ren, ox, oy, 3);
            }

            // Layer 5: Brilliant White Cross Intersection Flash at Apex
            int apex_x = sx + (int)(dir_x * (arc_depth + 3.0f));
            int apex_y = sy + (int)(dir_y * (arc_depth + 3.0f));
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, apex_x, apex_y, 4);
            SDL_RenderDrawLine(ren, apex_x - 14, apex_y, apex_x + 14, apex_y);
            SDL_RenderDrawLine(ren, apex_x, apex_y - 14, apex_x, apex_y + 14);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_HYDRO_PUMP) {
            // --- 5-LAYER BRAIDED DOUBLE-HELIX TSUNAMI CANNON ---
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : cosf(p.angle);
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : sinf(p.angle);
            float perp_x = -dir_y;
            float perp_y = dir_x;

            // Layer 1: Channeled Outer Water Column (Turbulent Expanding Wake)
            for (int seg = 5; seg >= 1; --seg) {
                float seg_dist = (float)seg * 24.0f;
                int bx = sx - (int)(dir_x * seg_dist);
                int by = sy - (int)(dir_y * seg_dist);
                float r_scale = 1.0f - (float)seg * 0.12f;
                int ring_r = (int)(p.radius * r_scale);
                if (ring_r < 6) ring_r = 6;

                // Translucent outer wake shroud
                SDL_SetRenderDrawColor(ren, 14, 165, 233, (Uint8)(80 + seg * 20));
                fill_circle(ren, bx, by, ring_r + 6);

                // Deep water body
                SDL_SetRenderDrawColor(ren, 2, 132, 199, (Uint8)(150 + seg * 18));
                fill_circle(ren, bx, by, ring_r);
            }

            // Layer 2: Double-Helix Braided Water Vortex (Interlocking Spiraling Ribbons)
            float helix_speed = p.lifetime * 28.0f;
            for (int helix = 0; helix < 2; ++helix) {
                float phase_offset = (float)helix * 3.14159265f;
                for (float d = 0.0f; d <= 120.0f; d += 6.0f) {
                    float wave_ang = helix_speed - d * 0.08f + phase_offset;
                    float lateral_disp = sinf(wave_ang) * (p.radius * 0.65f);
                    int hx = sx - (int)(dir_x * d - perp_x * lateral_disp);
                    int hy = sy - (int)(dir_y * d - perp_y * lateral_disp);
                    Uint8 h_alpha = (Uint8)(230.0f * (1.0f - d / 130.0f));
                    SDL_SetRenderDrawColor(ren, 125, 211, 252, h_alpha);
                    fill_circle(ren, hx, hy, 3);
                }
            }

            // Layer 3: Expanding Tsunami Surge Head & Foaming Breaker Spray
            SDL_SetRenderDrawColor(ren, 3, 105, 161, 240);
            fill_circle(ren, sx, sy, (int)p.radius);

            SDL_SetRenderDrawColor(ren, 56, 189, 248, 255);
            fill_circle(ren, sx, sy, (int)(p.radius * 0.70f));

            // Foaming white breaker spray along head rim
            SDL_SetRenderDrawColor(ren, 224, 242, 254, 230);
            for (int a = 0; a < 360; a += 30) {
                float rad = (float)a * 0.0174533f;
                int fx = sx + (int)(cosf(rad) * p.radius);
                int fy = sy + (int)(sinf(rad) * p.radius);
                SDL_RenderDrawPoint(ren, fx, fy);
                SDL_RenderDrawPoint(ren, fx + 1, fy);
            }

            // Layer 4 & 5: Additive Piercing Hydro-Laser Beam & Concentric Shockwave Rings
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 186, 230, 253, 230);
            fill_circle(ren, sx + (int)(dir_x * 8.0f), sy + (int)(dir_y * 8.0f), (int)(p.radius * 0.45f));

            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            fill_circle(ren, sx + (int)(dir_x * 10.0f), sy + (int)(dir_y * 10.0f), (int)(p.radius * 0.25f));

            // Forward hydraulic pressure shockwave rings
            for (int w = 1; w <= 2; ++w) {
                int ring_d = (int)(p.radius * (1.1f + (float)w * 0.25f));
                SDL_SetRenderDrawColor(ren, 125, 211, 252, (Uint8)(200 - w * 60));
                draw_circle_outline(ren, sx + (int)(dir_x * 10.0f), sy + (int)(dir_y * 10.0f), ring_d, 2);
            }
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else if (p.type == PROJ_AERIAL_ACE) {
            // --- 5-LAYER SUPERSONIC SWALLOW-DIVE AIR SHOCKWAVE ---
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

            float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
            float dir_x = (p_spd > 0.001f) ? (p.vx / p_spd) : cosf(p.angle);
            float dir_y = (p_spd > 0.001f) ? (p.vy / p_spd) : sinf(p.angle);
            float perp_x = -dir_y;
            float perp_y = dir_x;

            // Layer 1: Aerodynamic Mach Shock Rings behind head
            for (int r = 1; r <= 4; ++r) {
                float dist_back = (float)r * 18.0f;
                int rx = sx - (int)(dir_x * dist_back);
                int ry = sy - (int)(dir_y * dist_back);
                int ring_span = (int)(p.radius * (0.7f + (float)r * 0.35f));

                SDL_SetRenderDrawColor(ren, 226, 232, 240, (Uint8)(170 - r * 35));
                SDL_RenderDrawLine(ren, rx - (int)(perp_x * ring_span), ry - (int)(perp_y * ring_span),
                                       rx + (int)(perp_x * ring_span), ry + (int)(perp_y * ring_span));
            }

            // Layer 2: Sonic Vapor Hull Cone (Swallow-wing swept back geometry)
            int apex_x = sx + (int)(dir_x * p.radius * 1.35f);
            int apex_y = sy + (int)(dir_y * p.radius * 1.35f);
            int base_l_x = sx - (int)(dir_x * p.radius * 0.65f - perp_x * p.radius * 1.05f);
            int base_l_y = sy - (int)(dir_y * p.radius * 0.65f - perp_y * p.radius * 1.05f);
            int base_r_x = sx - (int)(dir_x * p.radius * 0.65f + perp_x * p.radius * 1.05f);
            int base_r_y = sy - (int)(dir_y * p.radius * 0.65f + perp_y * p.radius * 1.05f);

            SDL_SetRenderDrawColor(ren, 203, 213, 225, 210);
            for (float s = 0.0f; s <= 1.0f; s += 0.05f) {
                int p1_x = (int)((1.0f - s) * (float)base_l_x + s * (float)apex_x);
                int p1_y = (int)((1.0f - s) * (float)base_l_y + s * (float)apex_y);
                int p2_x = (int)((1.0f - s) * (float)base_r_x + s * (float)apex_x);
                int p2_y = (int)((1.0f - s) * (float)base_r_y + s * (float)apex_y);
                SDL_RenderDrawLine(ren, p1_x, p1_y, p2_x, p2_y);
            }

            // Layer 3: Swallow-Wing Cross Razor Blades
            SDL_SetRenderDrawColor(ren, 241, 245, 249, 240);
            SDL_RenderDrawLine(ren, base_l_x, base_l_y, apex_x, apex_y);
            SDL_RenderDrawLine(ren, base_r_x, base_r_y, apex_x, apex_y);

            // Layer 4 & 5: Additive Radiant White Cutting Edges, Sonic Needle and Apex Glint
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            for (int w = -1; w <= 1; ++w) {
                SDL_RenderDrawLine(ren, base_l_x + (int)(perp_x * w), base_l_y + (int)(perp_y * w), apex_x, apex_y);
                SDL_RenderDrawLine(ren, base_r_x + (int)(perp_x * w), base_r_y + (int)(perp_y * w), apex_x, apex_y);
            }
            fill_circle(ren, apex_x, apex_y, 4);
            SDL_RenderDrawLine(ren, apex_x, apex_y, apex_x + (int)(dir_x * 12.0f), apex_y + (int)(dir_y * 12.0f));
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        }
    }

    // 3. Render Floating Damage Numbers & Effectiveness Badges
    for (int i = 0; i < MAX_TEXTS; ++i) {
        if (!texts[i].active) continue;
        const FloatingText& t = texts[i];
        int sx = (int)(t.x - cam_x);
        int sy = (int)(t.y - cam_y);

        if (t.label[0] != '\0') {
            int w = Font::get_text_width(t.label, 2);
            Font::draw_text(ren, sx - w / 2, sy, t.label, 2, t.custom_r, t.custom_g, t.custom_b, (Uint8)t.alpha);
            continue;
        }

        Uint8 r = 255, g = 80, b = 80;
        int scale = 2;

        if (t.effectiveness >= 1.5f) {
            // SUPER EFFECTIVE: brilliant electric gold
            r = 250; g = 204; b = 21;
            scale = 3;
            Font::draw_text(ren, sx - 42, sy - 12, "SUPER EFFECTIVE!", 1, 250, 204, 21);
        } else if (t.effectiveness <= 0.75f && t.effectiveness > 0.001f) {
            // NOT VERY EFFECTIVE / RESISTED: muted slate blue
            r = 148; g = 163; b = 184;
            scale = 2;
            Font::draw_text(ren, sx - 20, sy - 10, "RESISTED", 1, 148, 163, 184);
        } else if (t.is_crit) {
            r = 250; g = 204; b = 21;
            scale = 3;
        }

        draw_number(ren, sx, sy, t.damage, scale, r, g, b, (Uint8)t.alpha);
    }

    // 4. Render High-Contrast Impact Flash & Star Burst (Hitstop visual FX)
    if (impact_flash_timer > 0.0f) {
        int fx = (int)(impact_flash_x - cam_x);
        int fy = (int)(impact_flash_y - cam_y);
        float alpha_ratio = impact_flash_timer / 0.10f;
        if (alpha_ratio > 1.0f) alpha_ratio = 1.0f;

        // Viewport flash overlay (rapid additive burst)
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(alpha_ratio * 75.0f));
        SDL_Rect screen_rect = { 0, 0, 1920, 1080 };
        SDL_RenderFillRect(ren, &screen_rect);

        // Radial High-Contrast Impact Star Burst at (fx, fy)
        float flash_r = (1.0f - alpha_ratio * 0.4f) * 64.0f;
        SDL_SetRenderDrawColor(ren, 255, 255, 255, (Uint8)(alpha_ratio * 245.0f));
        fill_circle(ren, fx, fy, flash_r * 0.45f);

        // 8-point razor star lines
        for (int a = 0; a < 8; ++a) {
            float angle = (float)a * (3.14159f / 4.0f) + (1.0f - alpha_ratio) * 0.6f;
            float cos_ang = cosf(angle);
            float sin_ang = sinf(angle);
            int x1 = fx + (int)(cos_ang * (flash_r * 0.25f));
            int y1 = fy + (int)(sin_ang * (flash_r * 0.25f));
            int x2 = fx + (int)(cos_ang * (flash_r * 1.35f));
            int y2 = fy + (int)(sin_ang * (flash_r * 1.35f));
            SDL_RenderDrawLine(ren, x1, y1, x2, y2);
            SDL_RenderDrawLine(ren, x1 + 1, y1, x2 + 1, y2);
            SDL_RenderDrawLine(ren, x1, y1 + 1, x2, y2 + 1);
        }
    }
}
