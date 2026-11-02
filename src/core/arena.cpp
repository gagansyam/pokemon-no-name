#include "arena.hpp"
#include "font.hpp"
#include <cmath>
#include <cstdlib>

Arena g_arena;

static void fill_circle(SDL_Renderer* ren, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; ++dy) {
        int dx = (int)sqrtf((float)(radius * radius - dy * dy));
        SDL_RenderDrawLine(ren, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static void draw_circle(SDL_Renderer* ren, int cx, int cy, int radius, int thickness = 2) {
    for (int t = 0; t < thickness; ++t) {
        int r = radius - t;
        for (int a = 0; a < 360; a += 1) {
            float rad = (float)a * 0.0174533f;
            int px = cx + (int)(cosf(rad) * (float)r);
            int py = cy + (int)(sinf(rad) * (float)r);
            SDL_RenderDrawPoint(ren, px, py);
        }
    }
}

void Arena::init() {
    clear_decals();
    for (int i = 0; i < MAX_HAZARDS; ++i) {
        hazards[i].active = false;
    }
    hazard_count = 0;
    anim_timer = 0.0f;
}

void Arena::update(float dt) {
    anim_timer += dt;

    // Update decals
    for (int i = 0; i < MAX_GROUND_DECALS; ++i) {
        if (decals[i].active) {
            decals[i].ttl -= dt;
            if (decals[i].ttl <= 0.0f) {
                decals[i].active = false;
            }
        }
    }

    // Update hazards
    for (int i = 0; i < MAX_HAZARDS; ++i) {
        if (hazards[i].active) {
            hazards[i].pulse_timer += dt;
        }
    }
}

void Arena::clear_decals() {
    for (int i = 0; i < MAX_GROUND_DECALS; ++i) {
        decals[i].active = false;
    }
    next_decal_idx = 0;
}

void Arena::add_decal(DecalType type, float x, float y, float radius, float angle,
                      Uint8 r, Uint8 g, Uint8 b, Uint8 alpha, float ttl) {
    int idx = next_decal_idx;
    next_decal_idx = (next_decal_idx + 1) % MAX_GROUND_DECALS;

    GroundDecal& d = decals[idx];
    d.active = true;
    d.type = type;
    d.x = x;
    d.y = y;
    d.radius = radius;
    d.angle = angle;
    d.seed = rand();
    d.r = r;
    d.g = g;
    d.b = b;
    d.alpha = alpha;
    d.ttl = ttl;
    d.max_ttl = ttl;
}

void Arena::add_toxic_spikes(int target_team, float center_x, float center_y) {
    // Spawn a cluster of 6-8 caltrop spikes around the center location
    int count = 6;
    for (int k = 0; k < count && hazard_count < MAX_HAZARDS; ++k) {
        int idx = -1;
        for (int i = 0; i < MAX_HAZARDS; ++i) {
            if (!hazards[i].active) { idx = i; break; }
        }
        if (idx < 0) break;

        float ang = (float)k * (6.28318f / (float)count) + ((float)(rand() % 30) * 0.01f);
        float dist = 20.0f + (float)(rand() % 45);

        ActiveHazard& h = hazards[idx];
        h.active = true;
        h.type = HAZARD_TOXIC_SPIKES;
        h.target_team = target_team;
        h.x = center_x + cosf(ang) * dist;
        h.y = center_y + sinf(ang) * dist;
        h.pulse_timer = (float)(rand() % 100) * 0.01f;
        hazard_count++;
    }
}

void Arena::clear_hazards(int target_team) {
    for (int i = 0; i < MAX_HAZARDS; ++i) {
        if (hazards[i].active && hazards[i].target_team == target_team) {
            hazards[i].active = false;
            hazard_count--;
        }
    }
}

bool Arena::check_hazard_collision(float x, float y, float z, int team_id, HazardType& out_type) {
    if (z > 15.0f) return false; // Airborne clears ground hazards

    for (int i = 0; i < MAX_HAZARDS; ++i) {
        if (hazards[i].active && hazards[i].target_team == team_id) {
            float dx = x - hazards[i].x;
            float dy = y - hazards[i].y;
            if (dx * dx + dy * dy < 28.0f * 28.0f) {
                out_type = hazards[i].type;
                return true;
            }
        }
    }
    return false;
}

void Arena::render(SDL_Renderer* ren, float cam_x, float cam_y, int view_w, int view_h) {
    // =========================================================================
    // 1. STADIUM BACKGROUND & BLEACHERS
    // =========================================================================
    // Outer stadium tier floor (neutral grey concrete)
    SDL_SetRenderDrawColor(ren, 55, 65, 81, 255);
    SDL_RenderClear(ren);

    // =========================================================================
    // 2. GRANDSTAND SPECTATORS (Top & Bottom)
    // =========================================================================
    // Top Grandstand (Bleachers)
    int crowd_top_y = (int)(ARENA_MIN_Y - 95.0f - cam_y);
    int crowd_h = 75;
    if (crowd_top_y + crowd_h > 0 && crowd_top_y < view_h) {
        // Bleacher concrete tiers
        SDL_SetRenderDrawColor(ren, 75, 85, 99, 255);
        SDL_Rect bleacher_rect = { 0, crowd_top_y, view_w, crowd_h };
        SDL_RenderFillRect(ren, &bleacher_rect);

        // Spectator crowd rows
        int bob = ((int)(anim_timer * 3.5f) % 2 == 0) ? 1 : 0;
        for (int row = 0; row < 3; ++row) {
            int ry = crowd_top_y + row * 22 + (row % 2 == 0 ? bob : 1 - bob);
            for (int col = 0; col < view_w; col += 14) {
                int seed = (col * 31 + row * 17) % 6;
                Uint8 cr = 239, cg = 68, cb = 68; // Red
                if (seed == 1) { cr = 59;  cg = 130; cb = 246; } // Blue
                else if (seed == 2) { cr = 245; cg = 158; cb = 11; } // Amber
                else if (seed == 3) { cr = 16;  cg = 185; cb = 129; } // Green
                else if (seed == 4) { cr = 168; cg = 85;  cb = 247; } // Purple
                else if (seed == 5) { cr = 244; cg = 114; cb = 182; } // Pink

                // Head
                SDL_SetRenderDrawColor(ren, 254, 215, 170, 255);
                SDL_Rect head = { col + 2, ry, 6, 6 };
                SDL_RenderFillRect(ren, &head);
                // Body/Shirt
                SDL_SetRenderDrawColor(ren, cr, cg, cb, 255);
                SDL_Rect body = { col, ry + 6, 10, 10 };
                SDL_RenderFillRect(ren, &body);
            }
        }
    }

    // =========================================================================
    // 3. REFEREE BROCK & SINNOH LEAGUE BOOTH (Top Center)
    // =========================================================================
    int booth_cx = (int)(ARENA_WIDTH / 2.0f - cam_x);
    int booth_cy = (int)(ARENA_MIN_Y - 50.0f - cam_y);

    if (booth_cx > -150 && booth_cx < view_w + 150) {
        // Sinnoh League Banner
        SDL_SetRenderDrawColor(ren, 30, 58, 138, 255);
        SDL_Rect banner_bg = { booth_cx - 90, booth_cy - 12, 180, 24 };
        SDL_RenderFillRect(ren, &banner_bg);
        SDL_SetRenderDrawColor(ren, 234, 179, 8, 255); // Gold border
        SDL_RenderDrawRect(ren, &banner_bg);
        Font::draw_text(ren, booth_cx - 68, booth_cy - 5, "SINNOH LEAGUE", 1, 255, 255, 255);

        // Brock Referee Figure
        // Spiky brown hair
        SDL_SetRenderDrawColor(ren, 120, 53, 15, 255);
        SDL_Rect hair = { booth_cx - 6, booth_cy + 12, 12, 8 };
        SDL_RenderFillRect(ren, &hair);
        // Face
        SDL_SetRenderDrawColor(ren, 217, 119, 6, 255);
        SDL_Rect face = { booth_cx - 5, booth_cy + 18, 10, 8 };
        SDL_RenderFillRect(ren, &face);
        // Orange & Green vest
        SDL_SetRenderDrawColor(ren, 234, 88, 12, 255);
        SDL_Rect vest = { booth_cx - 8, booth_cy + 26, 16, 10 };
        SDL_RenderFillRect(ren, &vest);
        // Desk in front of Brock
        SDL_SetRenderDrawColor(ren, 37, 99, 235, 255);
        SDL_Rect desk = { booth_cx - 24, booth_cy + 34, 48, 12 };
        SDL_RenderFillRect(ren, &desk);
        SDL_SetRenderDrawColor(ren, 245, 245, 250, 255);
        SDL_RenderDrawRect(ren, &desk);
    }

    // =========================================================================
    // 4. GREEN PERIMETER TURF (Grass borders around the sand pitch)
    // =========================================================================
    int turf_min_x = (int)(ARENA_MIN_X - 60.0f - cam_x);
    int turf_min_y = (int)(ARENA_MIN_Y - 20.0f - cam_y);
    int turf_max_x = (int)(ARENA_MAX_X + 60.0f - cam_x);
    int turf_max_y = (int)(ARENA_MAX_Y + 20.0f - cam_y);

    SDL_SetRenderDrawColor(ren, 46, 125, 50, 255); // Rich green stadium lawn
    SDL_Rect turf_rect = { turf_min_x, turf_min_y, turf_max_x - turf_min_x, turf_max_y - turf_min_y };
    SDL_RenderFillRect(ren, &turf_rect);

    // Subtle lawn mowing stripes (vertical bands)
    SDL_SetRenderDrawColor(ren, 56, 142, 60, 255);
    for (int x = turf_min_x; x < turf_max_x; x += 64) {
        SDL_Rect stripe = { x, turf_min_y, 32, turf_max_y - turf_min_y };
        SDL_RenderFillRect(ren, &stripe);
    }

    // =========================================================================
    // 5. THE LILY OF THE VALLEY SAND/DIRT BATTLEFIELD PITCH
    // =========================================================================
    int pitch_scr_x = (int)(ARENA_MIN_X - cam_x);
    int pitch_scr_y = (int)(ARENA_MIN_Y - cam_y);
    int pitch_w = (int)(ARENA_MAX_X - ARENA_MIN_X);
    int pitch_h = (int)(ARENA_MAX_Y - ARENA_MIN_Y);

    // Warm sand base (matches the reference video's exact sand hue)
    SDL_SetRenderDrawColor(ren, 208, 178, 134, 255);
    SDL_Rect pitch_rect = { pitch_scr_x, pitch_scr_y, pitch_w, pitch_h };
    SDL_RenderFillRect(ren, &pitch_rect);

    // Sand stippling & fine terrain shading (procedural tiles to avoid flat look)
    SDL_SetRenderDrawColor(ren, 194, 161, 114, 255);
    for (int r = 0; r < pitch_h; r += 28) {
        for (int c = 0; c < pitch_w; c += 28) {
            int hash = (r * 73 + c * 37) % 7;
            if (hash < 2) {
                SDL_Rect speck = { pitch_scr_x + c, pitch_scr_y + r, 18, 12 };
                SDL_RenderFillRect(ren, &speck);
            }
        }
    }

    // Pebble/Earth flecks
    SDL_SetRenderDrawColor(ren, 172, 138, 92, 255);
    for (int r = 8; r < pitch_h; r += 40) {
        for (int c = 12; c < pitch_w; c += 40) {
            if ((r * 13 + c * 29) % 11 == 0) {
                SDL_RenderDrawPoint(ren, pitch_scr_x + c, pitch_scr_y + r);
                SDL_RenderDrawPoint(ren, pitch_scr_x + c + 1, pitch_scr_y + r);
            }
        }
    }

    // =========================================================================
    // 6. REGULATION WHITE PITCH LINES (Authentic Anime League Layout)
    // =========================================================================
    SDL_SetRenderDrawColor(ren, 245, 245, 250, 255); // High-contrast crisp white

    // Outer Boundary Frame (3-pixel thick white line)
    for (int t = 0; t < 3; ++t) {
        SDL_Rect bound = { pitch_scr_x + t, pitch_scr_y + t, pitch_w - t * 2, pitch_h - t * 2 };
        SDL_RenderDrawRect(ren, &bound);
    }

    // Center Vertical Dividing Line
    int center_line_x = (int)(ARENA_WIDTH / 2.0f - cam_x);
    for (int t = -1; t <= 1; ++t) {
        SDL_RenderDrawLine(ren, center_line_x + t, pitch_scr_y, center_line_x + t, pitch_scr_y + pitch_h);
    }

    // Center Circles
    int center_scr_x = (int)(ARENA_WIDTH / 2.0f - cam_x);
    int center_scr_y = (int)(ARENA_HEIGHT / 2.0f - cam_y);

    // Outer Circle (R = 110 px)
    draw_circle(ren, center_scr_x, center_scr_y, 110, 3);
    // Inner Circle (R = 35 px)
    draw_circle(ren, center_scr_x, center_scr_y, 35, 2);

    // Left Penalty / Coaching Area Box
    int box_w = 140;
    int box_h = 280;
    int box_y = center_scr_y - box_h / 2;
    for (int t = 0; t < 2; ++t) {
        SDL_Rect left_box = { pitch_scr_x + t, box_y + t, box_w - t, box_h - t * 2 };
        SDL_RenderDrawRect(ren, &left_box);
    }

    // Right Penalty / Coaching Area Box
    for (int t = 0; t < 2; ++t) {
        SDL_Rect right_box = { pitch_scr_x + pitch_w - box_w + t, box_y + t, box_w - t, box_h - t * 2 };
        SDL_RenderDrawRect(ren, &right_box);
    }

    // =========================================================================
    // 7. SIDELINE TRAINERS (Ash on Left, Paul on Right)
    // =========================================================================
    // Ash Ketchum on Left Sideline
    int ash_scr_x = (int)(ARENA_MIN_X + 45.0f - cam_x);
    int ash_scr_y = (int)(ARENA_HEIGHT / 2.0f - cam_y);

    if (ash_scr_x > -50 && ash_scr_x < view_w + 50) {
        // Cap (Red with white front)
        SDL_SetRenderDrawColor(ren, 220, 38, 38, 255);
        SDL_Rect cap = { ash_scr_x - 7, ash_scr_y - 24, 14, 8 };
        SDL_RenderFillRect(ren, &cap);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_Rect cap_front = { ash_scr_x - 3, ash_scr_y - 22, 6, 4 };
        SDL_RenderFillRect(ren, &cap_front);
        // Face & Hair
        SDL_SetRenderDrawColor(ren, 254, 215, 170, 255);
        SDL_Rect face = { ash_scr_x - 5, ash_scr_y - 16, 10, 8 };
        SDL_RenderFillRect(ren, &face);
        // Blue Sinnoh vest & Yellow trim
        SDL_SetRenderDrawColor(ren, 29, 78, 216, 255);
        SDL_Rect vest = { ash_scr_x - 8, ash_scr_y - 8, 16, 12 };
        SDL_RenderFillRect(ren, &vest);
        SDL_SetRenderDrawColor(ren, 234, 179, 8, 255);
        SDL_Rect trim = { ash_scr_x - 8, ash_scr_y - 2, 16, 3 };
        SDL_RenderFillRect(ren, &trim);
        // Jeans & Shoes
        SDL_SetRenderDrawColor(ren, 30, 58, 138, 255);
        SDL_Rect pants = { ash_scr_x - 6, ash_scr_y + 4, 12, 12 };
        SDL_RenderFillRect(ren, &pants);
        SDL_SetRenderDrawColor(ren, 220, 38, 38, 255);
        SDL_Rect shoes = { ash_scr_x - 7, ash_scr_y + 16, 14, 4 };
        SDL_RenderFillRect(ren, &shoes);
        // Shadow beneath trainer
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 90);
        SDL_Rect shd = { ash_scr_x - 10, ash_scr_y + 20, 20, 4 };
        SDL_RenderFillRect(ren, &shd);
    }

    // Paul on Right Sideline
    int paul_scr_x = (int)(ARENA_MAX_X - 45.0f - cam_x);
    int paul_scr_y = (int)(ARENA_HEIGHT / 2.0f - cam_y);

    if (paul_scr_x > -50 && paul_scr_x < view_w + 50) {
        // Dark purple spiky hair
        SDL_SetRenderDrawColor(ren, 107, 33, 168, 255);
        SDL_Rect hair = { paul_scr_x - 8, paul_scr_y - 24, 16, 10 };
        SDL_RenderFillRect(ren, &hair);
        // Face
        SDL_SetRenderDrawColor(ren, 254, 215, 170, 255);
        SDL_Rect face = { paul_scr_x - 5, paul_scr_y - 14, 10, 7 };
        SDL_RenderFillRect(ren, &face);
        // Dark purple heavy coat (arms crossed stance)
        SDL_SetRenderDrawColor(ren, 88, 28, 135, 255);
        SDL_Rect coat = { paul_scr_x - 9, paul_scr_y - 7, 18, 14 };
        SDL_RenderFillRect(ren, &coat);
        // Grey pants & dark boots
        SDL_SetRenderDrawColor(ren, 75, 85, 99, 255);
        SDL_Rect pants = { paul_scr_x - 6, paul_scr_y + 7, 12, 11 };
        SDL_RenderFillRect(ren, &pants);
        SDL_SetRenderDrawColor(ren, 17, 24, 39, 255);
        SDL_Rect boots = { paul_scr_x - 7, paul_scr_y + 18, 14, 4 };
        SDL_RenderFillRect(ren, &boots);
        // Shadow beneath trainer
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 90);
        SDL_Rect shd = { paul_scr_x - 10, paul_scr_y + 22, 20, 4 };
        SDL_RenderFillRect(ren, &shd);
    }

    // =========================================================================
    // 8. TOXIC SPIKES & HAZARDS LAYER
    // =========================================================================
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < MAX_HAZARDS; ++i) {
        const ActiveHazard& h = hazards[i];
        if (!h.active) continue;

        int hx = (int)(h.x - cam_x);
        int hy = (int)(h.y - cam_y);

        if (hx < -20 || hx > view_w + 20 || hy < -20 || hy > view_h + 20) continue;

        if (h.type == HAZARD_TOXIC_SPIKES) {
            // Purple Toxic Caltrops with subtle pulsing aura
            float pulse = 0.8f + 0.2f * sinf(h.pulse_timer * 4.0f);

            // Purple glow aura
            SDL_SetRenderDrawColor(ren, 168, 85, 247, (Uint8)(60.0f * pulse));
            fill_circle(ren, hx, hy, 10);

            // Dark violet spike spikes (3-pronged caltrop)
            SDL_SetRenderDrawColor(ren, 126, 34, 206, 255);
            SDL_RenderDrawLine(ren, hx, hy - 7, hx, hy + 2);
            SDL_RenderDrawLine(ren, hx - 6, hy + 4, hx + 1, hy);
            SDL_RenderDrawLine(ren, hx + 6, hy + 4, hx - 1, hy);

            // Bright toxic poison tips
            SDL_SetRenderDrawColor(ren, 216, 180, 254, 255);
            SDL_RenderDrawPoint(ren, hx, hy - 7);
            SDL_RenderDrawPoint(ren, hx - 6, hy + 4);
            SDL_RenderDrawPoint(ren, hx + 6, hy + 4);
        }
    }

    // =========================================================================
    // 9. PERSISTENT BATTLE-SCARS & CRATERS LAYER
    // =========================================================================
    for (int i = 0; i < MAX_GROUND_DECALS; ++i) {
        const GroundDecal& d = decals[i];
        if (!d.active) continue;

        int sx = (int)(d.x - cam_x);
        int sy = (int)(d.y - cam_y);

        // Viewport culling
        if (sx + d.radius < -30 || sx - d.radius > view_w + 30 ||
            sy + d.radius < -30 || sy - d.radius > view_h + 30) {
            continue;
        }

        float fade = (d.max_ttl > 0.0f) ? (d.ttl / d.max_ttl) : 1.0f;
        if (fade <= 0.0f) continue;
        Uint8 cur_a = (Uint8)((float)d.alpha * fade);
        if (cur_a < 2) continue;

        if (d.type == DECAL_TECTONIC_CRACK) {
            // Jagged earthquake fault fissure
            for (int c = 0; c < 6; ++c) {
                float c_ang = (float)c * 1.0472f + (float)((d.seed + c * 37) % 100) * 0.008f;
                int p0_x = sx;
                int p0_y = sy;
                int p1_x = sx + (int)(cosf(c_ang) * (d.radius * 0.35f));
                int p1_y = sy + (int)(sinf(c_ang) * (d.radius * 0.35f));
                int p2_x = sx + (int)(cosf(c_ang + 0.25f) * (d.radius * 0.70f));
                int p2_y = sy + (int)(sinf(c_ang + 0.25f) * (d.radius * 0.70f));
                int p3_x = sx + (int)(cosf(c_ang - 0.18f) * d.radius);
                int p3_y = sy + (int)(sinf(c_ang - 0.18f) * d.radius);

                SDL_SetRenderDrawColor(ren, 25, 18, 12, cur_a);
                SDL_RenderDrawLine(ren, p0_x, p0_y, p1_x, p1_y);
                SDL_RenderDrawLine(ren, p1_x, p1_y, p2_x, p2_y);
                SDL_RenderDrawLine(ren, p2_x, p2_y, p3_x, p3_y);
            }
        } else if (d.type == DECAL_IMPACT_CRATER) {
            // Heavy impact crater with fractured perimeter
            SDL_SetRenderDrawColor(ren, 45, 35, 25, (Uint8)(cur_a * 0.75f));
            fill_circle(ren, sx, sy, (int)(d.radius * 0.45f));

            for (int k = 0; k < 8; ++k) {
                float ka = d.angle + (float)k * 0.785f + (float)((d.seed + k * 19) % 50) * 0.006f;
                int kx1 = sx + (int)(cosf(ka) * (d.radius * 0.4f));
                int ky1 = sy + (int)(sinf(ka) * (d.radius * 0.4f));
                int kx2 = sx + (int)(cosf(ka + 0.08f) * (d.radius * 0.75f));
                int ky2 = sy + (int)(sinf(ka + 0.08f) * (d.radius * 0.75f));
                int kx3 = sx + (int)(cosf(ka - 0.05f) * d.radius);
                int ky3 = sy + (int)(sinf(ka - 0.05f) * d.radius);

                SDL_SetRenderDrawColor(ren, 28, 20, 14, cur_a);
                SDL_RenderDrawLine(ren, kx1, ky1, kx2, ky2);
                SDL_RenderDrawLine(ren, kx2, ky2, kx3, ky3);
            }
        } else if (d.type == DECAL_SCORCH_MARK) {
            // Carbonized scorch turf from fire/electric blasts
            SDL_SetRenderDrawColor(ren, 30, 25, 20, (Uint8)(cur_a * 0.65f));
            for (int a = 0; a < 360; a += 18) {
                float rad = (float)a * 0.0174533f;
                float jitter = 0.75f + (float)((d.seed + a) % 30) * 0.01f;
                int px = sx + (int)(cosf(rad) * (d.radius * jitter));
                int py = sy + (int)(sinf(rad) * (d.radius * jitter));
                SDL_RenderDrawLine(ren, sx + (int)(cosf(rad) * 4.0f), sy + (int)(sinf(rad) * 4.0f), px, py);
            }
        } else if (d.type == DECAL_SAND_FURROW) {
            // Trench in the sand
            float dx = cosf(d.angle);
            float dy = sinf(d.angle);
            float px = -dy;
            float py = dx;

            int f1_x = sx - (int)(dx * d.radius * 0.6f);
            int f1_y = sy - (int)(dy * d.radius * 0.6f);
            int f2_x = sx + (int)(dx * d.radius * 1.1f);
            int f2_y = sy + (int)(dy * d.radius * 1.1f);

            SDL_SetRenderDrawColor(ren, 146, 64, 14, (Uint8)(cur_a * 0.85f));
            for (int w = -2; w <= 2; ++w) {
                SDL_RenderDrawLine(ren, f1_x + (int)(px * (float)w), f1_y + (int)(py * (float)w),
                                        f2_x + (int)(px * (float)w), f2_y + (int)(py * (float)w));
            }
        }
    }
}

void Arena::clamp(float& x, float& y, float radius) {
    if (x < ARENA_MIN_X + radius) x = ARENA_MIN_X + radius;
    if (x > ARENA_MAX_X - radius) x = ARENA_MAX_X - radius;
    if (y < ARENA_MIN_Y + radius) y = ARENA_MIN_Y + radius;
    if (y > ARENA_MAX_Y - radius) y = ARENA_MAX_Y - radius;
}
