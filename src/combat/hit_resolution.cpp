#include "fighter.hpp"
#include "projectile.hpp"
#include "arena.hpp"
#include "combat_log.hpp"
#include <cmath>

bool Fighter::check_hits(Fighter* opponent) {
    if (!opponent) return false;
    bool any_hit = false;

    // Projectile collisions against opponent
    for (int i = 0; i < MAX_PROJ; ++i) {
        Projectile& p = g_projectiles.pool[i];
        if (!p.active || p.owner_id != id) continue;
        if (!p.can_damage) continue;
        if (p.hit_cooldown > 0) continue;
        if (p.delay_timer > 0.0f) continue; // Telegraph warning deals NO damage

        // 0. Double Team Decoy Collision Check:
        if (opponent->double_team_timer > 0.0f) {
            for (int c = 0; c < MAX_CLONES; ++c) {
                if (!opponent->clones[c].active) continue;
                float c_hit_x = p.x;
                float c_hit_y = p.y;
                float cdist = 0.0f;
                if (p.type == PROJ_THUNDERBOLT) {
                    float seg_dx = p.x - p.target_x;
                    float seg_dy = p.y - p.target_y;
                    float seg_len_sq = seg_dx * seg_dx + seg_dy * seg_dy;
                    if (seg_len_sq > 1.0f) {
                        float u = ((opponent->clones[c].x - p.target_x) * seg_dx + (opponent->clones[c].y - p.target_y) * seg_dy) / seg_len_sq;
                        if (u < 0.0f) u = 0.0f;
                        if (u > 1.0f) u = 1.0f;
                        c_hit_x = p.target_x + u * seg_dx;
                        c_hit_y = p.target_y + u * seg_dy;
                    }
                    float cdx = opponent->clones[c].x - c_hit_x;
                    float cdy = opponent->clones[c].y - c_hit_y;
                    cdist = sqrtf(cdx * cdx + cdy * cdy);
                } else {
                    float cdx = opponent->clones[c].x - p.x;
                    float cdy = opponent->clones[c].y - p.y;
                    cdist = sqrtf(cdx * cdx + cdy * cdy);
                }

                if (cdist < (opponent->radius + p.radius)) {
                    if (p.is_ground_level && opponent->z >= 20.0f) {
                        continue;
                    }

                    // Decoy Illusion popped!
                    opponent->clones[c].active = false;
                    CombatLogger::record_clone_pop(opponent->id);
                    any_hit = true;
                    g_projectiles.trigger_hitstop(0.040f, opponent->clones[c].x, opponent->clones[c].y);
                    g_projectiles.spawn_particles(opponent->clones[c].x, opponent->clones[c].y, 18, 255, 255, 255, 180.0f);
                    g_projectiles.spawn_particles(opponent->clones[c].x, opponent->clones[c].y, 12, 56, 189, 248, 140.0f);
                    g_projectiles.spawn_shockwave(opponent->clones[c].x, opponent->clones[c].y, 50.0f, 0.18f, 255, 255, 255, 3);
                    g_projectiles.spawn_custom_text(opponent->clones[c].x, opponent->clones[c].y - 20.0f, "POOF!", 200, 220, 255, 30);

                    if (!p.piercing) {
                        p.active = false;
                        break; // Non-piercing projectile absorbed by decoy!
                    }
                }
            }
            if (!p.active) {
                return true; // Decoy absorbed non-piercing projectile, protecting opponent!
            }
        }

        float hit_pt_x = p.x;
        float hit_pt_y = p.y;
        float dist = 0.0f;
        float dx = 0.0f;
        float dy = 0.0f;

        if (p.type == PROJ_THUNDERBOLT) {
            float seg_dx = p.x - p.target_x;
            float seg_dy = p.y - p.target_y;
            float seg_len_sq = seg_dx * seg_dx + seg_dy * seg_dy;
            if (seg_len_sq > 1.0f) {
                float u = ((opponent->x - p.target_x) * seg_dx + (opponent->y - p.target_y) * seg_dy) / seg_len_sq;
                if (u < 0.0f) u = 0.0f;
                if (u > 1.0f) u = 1.0f;
                hit_pt_x = p.target_x + u * seg_dx;
                hit_pt_y = p.target_y + u * seg_dy;
            }
            dx = opponent->x - hit_pt_x;
            dy = opponent->y - hit_pt_y;
            dist = sqrtf(dx * dx + dy * dy);
        } else {
            dx = opponent->x - p.x;
            dy = opponent->y - p.y;
            dist = sqrtf(dx * dx + dy * dy);
        }

        if (dist < (opponent->radius + p.radius)) {
            // 3D Z-Axis Elevation & Low-Profile Evasion Check:
            if (p.is_ground_level && opponent->z >= 20.0f) {
                // Low-profile ground attack sweeps harmlessly underneath airborne target!
                // Spawn evasive wind particles at target's shadow
                g_projectiles.spawn_particles(opponent->x, opponent->y, 2, 220, 240, 255, 60.0f);
                continue; // Cleanly evaded in 2.5D mid-air!
            }

            if (p.type == PROJ_SEISMIC_GRAB) {
                // Command grab connects on targets with z < 28px
                if (opponent->is_grabbed || opponent->z >= 28.0f) {
                    continue;
                }
                // Command Grab connected on grounded opponent!
                p.active = false;
                active_move = MOVE_SEISMIC_TOSS;
                state = STATE_ATTACK;
                move_phase = 1; // Stratosphere Globe Loop & Earth Slam
                attack_duration = 0.95f;
                attack_timer = 0.95f;
                seismic_toss_center_x = (x + opponent->x) * 0.5f;
                seismic_toss_center_y = (y + opponent->y) * 0.5f;
                flight_angle = atan2f(y - seismic_toss_center_y, x - seismic_toss_center_x);
                vx = 0.0f;
                vy = 0.0f;

                opponent->is_grabbed = true;
                opponent->grabbed_by_id = id;
                opponent->state = STATE_HITSTUN;
                opponent->vx = 0.0f;
                opponent->vy = 0.0f;
                opponent->x = x;
                opponent->y = y;
                opponent->z = z;

                g_projectiles.spawn_custom_text(x, y - 30.0f, "GOTCHA!", 239, 68, 68, 40);
                g_projectiles.trigger_hitstop(0.060f, x, y);
                g_projectiles.spawn_shockwave(x, y, 70.0f, 0.20f, 239, 68, 68, 4);
                return true;
            }

            // Check Protect / Precision Parry / Guard Block:
            if (opponent->is_protecting()) {
                if (opponent->protect_parry_window > 0.0f) {
                    // === 1. PERFECT PARRY (First 0.12s / ~7 frames) ===
                    any_hit = true;
                    CombatLogger::record_parry(opponent->id);
                    // 80ms global freeze
                    g_projectiles.trigger_hitstop(0.080f, opponent->x, opponent->y);

                    // Attacker recoil knockback
                    float recoil_dx = x - opponent->x;
                    float recoil_dy = y - opponent->y;
                    float rlen = sqrtf(recoil_dx * recoil_dx + recoil_dy * recoil_dy);
                    if (rlen > 0.001f) {
                        vx = (recoil_dx / rlen) * 440.0f;
                        vy = (recoil_dy / rlen) * 440.0f;
                    } else {
                        vx = -cosf(aim_angle) * 440.0f;
                        vy = -sinf(aim_angle) * 440.0f;
                    }
                    if (state == STATE_ATTACK) {
                        // Stagger the attacker!
                        state = STATE_HITSTUN;
                        hitstun_timer = 0.32f;
                    }

                    // Perfect Parry Visual & Auditory FX
                    g_projectiles.spawn_custom_text(opponent->x, opponent->y - 32.0f, "PARRY!", 16, 185, 129, 45);
                    g_projectiles.spawn_shockwave(opponent->x, opponent->y, 90.0f, 0.25f, 16, 185, 129, 6);
                    g_projectiles.spawn_shockwave(opponent->x, opponent->y, 55.0f, 0.18f, 255, 255, 255, 4);
                    g_projectiles.spawn_particles(opponent->x, opponent->y, 24, 16, 185, 129, 220.0f);
                    g_projectiles.spawn_particles(opponent->x, opponent->y, 16, 255, 255, 255, 180.0f);
                    g_projectiles.screen_shake = 0.22f;

                    // Opponent frame advantage: cancel remaining protect timer smoothly
                    opponent->protect_timer = 0.0f;
                    opponent->protect_parry_window = 0.0f;
                    opponent->state = STATE_IDLE;
                    opponent->attack_timer = 0.0f;

                    if (!p.piercing) {
                        p.active = false;
                    } else {
                        p.hit_cooldown = 20;
                    }
                    return true;
                } else {
                    // === 2. GUARD BLOCK (0.12s - 0.40s) ===
                    any_hit = true;
                    int base_pwr = (p.base_power > 0) ? p.base_power : p.damage;
                    int drain_sp = (base_pwr > 0) ? base_pwr : 15;
                    opponent->stamina -= (float)drain_sp;

                    if (opponent->stamina <= 0.0f) {
                        // --- SHIELD BREAK (Guard Broken / Dazed) ---
                        opponent->stamina = 0.0f;
                        opponent->protect_timer = 0.0f;
                        opponent->protect_parry_window = 0.0f;
                        opponent->shield_break_timer = 0.90f;
                        opponent->state = STATE_HITSTUN;
                        opponent->hitstun_timer = 0.90f;
                        opponent->vx = -cosf(opponent->aim_angle) * 180.0f;
                        opponent->vy = -sinf(opponent->aim_angle) * 180.0f;

                        g_projectiles.trigger_hitstop(0.085f, opponent->x, opponent->y);
                        g_projectiles.spawn_custom_text(opponent->x, opponent->y - 35.0f, "SHIELD BREAK!", 239, 68, 68, 48);
                        g_projectiles.spawn_shockwave(opponent->x, opponent->y, 95.0f, 0.30f, 239, 68, 68, 6);
                        g_projectiles.spawn_particles(opponent->x, opponent->y, 28, 16, 185, 129, 260.0f); // Shattered emerald shards
                        g_projectiles.spawn_particles(opponent->x, opponent->y, 18, 255, 255, 255, 200.0f);
                        g_projectiles.screen_shake = 0.30f;
                    } else {
                        // --- NORMAL GUARD ABSORPTION ---
                        g_projectiles.trigger_hitstop(0.035f, opponent->x, opponent->y);
                        // Minor pushback on defender
                        float push_dx = opponent->x - p.x;
                        float push_dy = opponent->y - p.y;
                        float plen = sqrtf(push_dx * push_dx + push_dy * push_dy);
                        if (plen > 0.001f) {
                            opponent->vx += (push_dx / plen) * 120.0f;
                            opponent->vy += (push_dy / plen) * 120.0f;
                        }

                        g_projectiles.spawn_shockwave(opponent->x, opponent->y, 50.0f, 0.16f, 16, 185, 129, 3);
                        g_projectiles.spawn_particles(opponent->x, opponent->y, 10, 16, 185, 129, 140.0f);
                        g_projectiles.spawn_custom_text(opponent->x, opponent->y - 20.0f, "BLOCKED", 200, 240, 220, 24);
                    }

                    if (!p.piercing) {
                        p.active = false;
                    } else {
                        p.hit_cooldown = 15;
                    }
                    return true;
                }
            }

            float kx = p.vx * 0.4f;
            float ky = p.vy * 0.4f;
            if (kx == 0 && ky == 0) {
                kx = (dx != 0 || dy != 0) ? (dx / dist) * 220.0f : 0;
                ky = (dx != 0 || dy != 0) ? (dy / dist) * 220.0f : 0;
            }

            int base_pwr = (p.base_power > 0) ? p.base_power : p.damage;
            if (p.type == PROJ_MELEE_SWEEP && p.damage >= 40) {
                // Proximity sweet spot: distance from attacker's body to opponent
                float attacker_dist = sqrtf((opponent->x - x) * (opponent->x - x) + (opponent->y - y) * (opponent->y - y));
                float push_dir_x = (opponent->x - x);
                float push_dir_y = (opponent->y - y);
                float push_len = sqrtf(push_dir_x * push_dir_x + push_dir_y * push_dir_y);
                float push_nx = (push_len > 0.001f) ? (push_dir_x / push_len) : cosf(aim_angle);
                float push_ny = (push_len > 0.001f) ? (push_dir_y / push_len) : sinf(aim_angle);

                if (attacker_dist <= 62.0f) {
                    // 1. Point-Blank Sweet Spot (<= 62px): Maximum kinetic anime slam!
                    base_pwr = 38;
                    kx = push_nx * 480.0f;
                    ky = push_ny * 480.0f;
                    g_projectiles.trigger_hitstop(0.065f, opponent->x, opponent->y); // 4-frame heavy freeze (65ms)
                    g_projectiles.spawn_particles(opponent->x, opponent->y, 32, 255, 255, 255, 280.0f);
                    g_projectiles.spawn_shockwave(opponent->x, opponent->y, 95.0f, 0.28f, 220, 240, 255, 6);
                    g_projectiles.screen_shake = 0.22f;
                    g_arena.add_decal(DECAL_IMPACT_CRATER, opponent->x, opponent->y, 36.0f, 0.0f, 30, 25, 20, 220);
                } else if (attacker_dist <= 115.0f) {
                    // 2. Solid Mid-Range Strike (62px - 115px)
                    base_pwr = 28;
                    kx = push_nx * 320.0f;
                    ky = push_ny * 320.0f;
                    g_projectiles.trigger_hitstop(0.050f, opponent->x, opponent->y); // 3-frame freeze (50ms)
                    g_projectiles.spawn_particles(opponent->x, opponent->y, 22, 210, 235, 255, 220.0f);
                    g_projectiles.spawn_shockwave(opponent->x, opponent->y, 80.0f, 0.24f, 200, 230, 255, 5);
                    g_projectiles.screen_shake = 0.15f;
                } else {
                    // 3. Outer Tip Glancing Blow (> 115px)
                    base_pwr = 18;
                    kx = push_nx * 180.0f;
                    ky = push_ny * 180.0f;
                    g_projectiles.trigger_hitstop(0.035f, opponent->x, opponent->y); // 2-frame freeze (35ms)
                    g_projectiles.spawn_particles(opponent->x, opponent->y, 14, 180, 210, 245, 160.0f);
                    g_projectiles.spawn_shockwave(opponent->x, opponent->y, 60.0f, 0.20f, 180, 210, 245, 4);
                    g_projectiles.screen_shake = 0.08f;
                }
            } else if (p.type == PROJ_MELEE_SWEEP) {
                // Quick Attack: 2-frame hitstop (35ms) + supersonic air shockwave
                g_projectiles.trigger_hitstop(0.035f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 18, 230, 245, 255, 210.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 65.0f, 0.20f, 255, 255, 255, 4);
                g_projectiles.screen_shake = 0.12f;
            } else if (p.type == PROJ_ELECTRO_BALL) {
                // 3-frame hitstop (50ms) + high-voltage radial detonation & scorch decal
                g_projectiles.trigger_hitstop(0.050f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 26, 250, 204, 21, 240.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 16, 56, 189, 248, 190.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 80.0f, 0.24f, 250, 204, 21, 5);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 50.0f, 0.18f, 56, 189, 248, 3);
                g_projectiles.screen_shake = 0.18f;
                g_arena.add_decal(DECAL_SCORCH_MARK, opponent->x, opponent->y, 34.0f, 0.0f, 40, 30, 10, 210);
            } else if (p.type == PROJ_THUNDERBOLT) {
                // High-voltage continuous lightning surge strike: 3-frame freeze (50ms) + intense electric shockwave
                g_projectiles.trigger_hitstop(0.050f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 26, 250, 204, 21, 250.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 16, 56, 189, 248, 200.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 10, 255, 255, 255, 220.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 75.0f, 0.22f, 56, 189, 248, 5);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 45.0f, 0.16f, 250, 204, 21, 3);
                g_projectiles.screen_shake = 0.20f;
                g_arena.add_decal(DECAL_SCORCH_MARK, opponent->x, opponent->y, 34.0f, 0.0f, 40, 30, 10, 220);
                p.hit_cooldown = 11; // Multi-hit tick cooldown (~0.18s)
            } else if (p.type == PROJ_THUNDER) {
                // Cataclysmic celestial sky pillar strike: 5-frame freeze (80ms) + 0.36f screen shake + scorch crater
                g_projectiles.trigger_hitstop(0.080f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 32, 250, 204, 21, 290.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 20, 56, 189, 248, 240.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 16, 255, 255, 255, 260.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 110.0f, 0.28f, 56, 189, 248, 6);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 70.0f, 0.20f, 250, 204, 21, 4);
                g_projectiles.screen_shake = 0.36f;
                g_arena.add_decal(DECAL_SCORCH_MARK, opponent->x, opponent->y, 48.0f, 0.0f, 30, 25, 20, 240);
                p.can_damage = false; // Single impact hit per thunderbolt column
            } else if (p.type == PROJ_WING_ATTACK) {
                // Heavy physical flight collision: 3-frame hitstop (50ms) + high-contrast impact flash & sonic shockwave
                g_projectiles.trigger_hitstop(0.055f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 24, 255, 230, 160, 240.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 80.0f, 0.25f, 255, 255, 255, 5);
            } else if (p.type == PROJ_CLAW_SCRATCH) {
                g_projectiles.trigger_hitstop(0.035f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 16, 239, 68, 68, 180.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 8, 255, 255, 255, 150.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 50.0f, 0.18f, 239, 68, 68, 3);
                g_projectiles.screen_shake = 0.10f;
            } else if (p.type == PROJ_DRAGON_CLAW) {
                // 4-frame hitstop (65ms) on Dragon Claw impact!
                g_projectiles.trigger_hitstop(0.065f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 22, 16, 185, 129, 220.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 75.0f, 0.22f, 52, 211, 153, 5);
                g_projectiles.screen_shake = 0.16f;
            } else if (p.type == PROJ_VOLT_TACKLE) {
                // Cataclysmic high-voltage physical impact: 80ms hitstop + dual electric shockwave
                g_projectiles.trigger_hitstop(0.080f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 36, 250, 204, 21, 280.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 20, 255, 255, 255, 240.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 120.0f, 0.28f, 250, 204, 21, 6);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 75.0f, 0.20f, 56, 189, 248, 4);
                g_projectiles.screen_shake = 0.28f;
                g_arena.add_decal(DECAL_SCORCH_MARK, opponent->x, opponent->y, 45.0f, 0.0f, 40, 30, 10, 220);
            } else if (p.type == PROJ_DRAGON_RAGE) {
                // Azure dragon plasma blast detonation
                g_projectiles.trigger_hitstop(0.055f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 28, 99, 102, 241, 230.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 16, 56, 189, 248, 190.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 85.0f, 0.24f, 56, 189, 248, 5);
                g_projectiles.screen_shake = 0.20f;
            } else if (p.type == PROJ_EARTHQUAKE) {
                // Heavy seismic ground rupture
                g_projectiles.trigger_hitstop(0.060f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 32, 180, 130, 80, 240.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 18, 220, 180, 120, 190.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 135.0f, 0.30f, 180, 130, 80, 7);
                g_projectiles.screen_shake = 0.30f;
            } else if (p.type == PROJ_SCORCHING_SANDS) {
                g_projectiles.trigger_hitstop(0.045f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 20, 245, 158, 11, 190.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 12, 180, 83, 9, 140.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 70.0f, 0.22f, 217, 119, 6, 4);
                g_projectiles.screen_shake = 0.14f;
                g_arena.add_decal(DECAL_SAND_FURROW, opponent->x, opponent->y, 28.0f, p.angle, 120, 53, 15, 210);
            } else if (p.type == PROJ_FLAMETHROWER) {
                // Searing continuous flame stream impact: 2-frame micro-hitstop (33ms) + camera shake + ember burst
                g_projectiles.trigger_hitstop(0.033f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 14, 249, 115, 22, 160.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 8, 255, 200, 50, 190.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 45.0f, 0.14f, 249, 115, 22, 3);
                g_projectiles.screen_shake = 0.12f;
                if (rand() % 2 == 0) {
                    g_arena.add_decal(DECAL_SCORCH_MARK, opponent->x, opponent->y, 28.0f, 0.0f, 40, 25, 10, 210);
                }
                // Thermal pushback along stream direction
                float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
                if (p_spd > 0.001f) {
                    kx = (p.vx / p_spd) * 160.0f;
                    ky = (p.vy / p_spd) * 160.0f;
                }
                p.hit_cooldown = 10; // Multi-hit tick cooldown
            } else if (p.type == PROJ_AURA_SPHERE) {
                g_projectiles.trigger_hitstop(0.055f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 24, 56, 189, 248, 220.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 75.0f, 0.22f, 56, 189, 248, 5);
                g_projectiles.screen_shake = 0.18f;
                g_arena.add_decal(DECAL_IMPACT_CRATER, opponent->x, opponent->y, 35.0f, 0.0f, 25, 20, 15, 210);
            } else if (p.type == PROJ_FORCE_PALM) {
                g_projectiles.trigger_hitstop(0.060f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 22, 255, 255, 255, 240.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 70.0f, 0.20f, 56, 189, 248, 5);
                g_projectiles.screen_shake = 0.18f;
                g_arena.add_decal(DECAL_IMPACT_CRATER, opponent->x, opponent->y, 28.0f, 0.0f, 30, 25, 20, 200);
            } else if (p.type == PROJ_CLOSE_COMBAT) {
                g_projectiles.trigger_hitstop(0.068f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 32, 239, 68, 68, 260.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 18, 250, 204, 21, 220.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 85.0f, 0.25f, 239, 68, 68, 6);
                g_projectiles.screen_shake = 0.25f;
                g_arena.add_decal(DECAL_IMPACT_CRATER, opponent->x, opponent->y, 38.0f, 0.0f, 28, 18, 12, 220);
            } else if (p.type == PROJ_BONE_RUSH) {
                g_projectiles.trigger_hitstop(0.055f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 20, 56, 189, 248, 200.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 65.0f, 0.20f, 56, 189, 248, 4);
                g_projectiles.screen_shake = 0.16f;
                g_arena.add_decal(DECAL_IMPACT_CRATER, opponent->x, opponent->y, 26.0f, 0.0f, 35, 30, 20, 190);
            } else if (p.type == PROJ_EXTREME_SPEED) {
                g_projectiles.trigger_hitstop(0.060f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 26, 255, 255, 255, 250.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 80.0f, 0.22f, 255, 255, 255, 5);
                g_projectiles.screen_shake = 0.20f;
                g_arena.add_decal(DECAL_SAND_FURROW, opponent->x, opponent->y, 34.0f, p.vx != 0.0f || p.vy != 0.0f ? atan2f(p.vy, p.vx) : 0.0f, 120, 53, 15, 220);
            } else if (p.type == PROJ_FLASH_CANNON) {
                g_projectiles.trigger_hitstop(0.058f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 28, 241, 245, 249, 240.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 80.0f, 0.22f, 203, 213, 225, 5);
                g_projectiles.screen_shake = 0.20f;
                g_arena.add_decal(DECAL_IMPACT_CRATER, opponent->x, opponent->y, 40.0f, 0.0f, 30, 30, 35, 210);
            } else if (p.type == PROJ_DRAGON_PULSE) {
                g_projectiles.trigger_hitstop(0.055f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 24, 52, 211, 153, 220.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 75.0f, 0.22f, 99, 102, 241, 5);
                g_projectiles.screen_shake = 0.18f;
                g_arena.add_decal(DECAL_SCORCH_MARK, opponent->x, opponent->y, 36.0f, 0.0f, 20, 35, 30, 200);
            } else if (p.type == PROJ_WATER_SHURIKEN) {
                g_projectiles.trigger_hitstop(0.022f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 18, 56, 189, 248, 190.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 10, 224, 242, 254, 160.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 52.0f, 0.16f, 56, 189, 248, 3);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 28.0f, 0.12f, 224, 242, 254, 2);
                g_projectiles.screen_shake = 0.14f;
                g_arena.add_decal(DECAL_SCORCH_MARK, opponent->x, opponent->y, 22.0f, 0.0f, 30, 80, 140, 150);
            } else if (p.type == PROJ_NIGHT_SLASH) {
                g_projectiles.trigger_hitstop(0.065f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 26, 126, 34, 206, 230.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 16, 217, 70, 239, 200.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 78.0f, 0.22f, 168, 85, 247, 5);
                g_projectiles.screen_shake = 0.26f;
                g_arena.add_decal(DECAL_SCORCH_MARK, opponent->x, opponent->y, 36.0f, 0.0f, 25, 15, 35, 220);
                if (rand() % 4 == 0) { // 25% critical strike rate
                    base_pwr = (int)((float)base_pwr * 1.5f);
                    g_projectiles.spawn_custom_text(opponent->x, opponent->y - 25.0f, "CRITICAL!", 244, 63, 94, 38);
                    g_projectiles.spawn_shockwave(opponent->x, opponent->y, 90.0f, 0.25f, 217, 70, 239, 6);
                }
            } else if (p.type == PROJ_HYDRO_PUMP) {
                g_projectiles.trigger_hitstop(0.065f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 36, 56, 189, 248, 250.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 20, 224, 242, 254, 210.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 100.0f, 0.28f, 56, 189, 248, 6);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 60.0f, 0.18f, 224, 242, 254, 4);
                g_projectiles.screen_shake = 0.32f;
                g_arena.add_decal(DECAL_SAND_FURROW, opponent->x, opponent->y, 44.0f, p.vx != 0.0f || p.vy != 0.0f ? atan2f(p.vy, p.vx) : 0.0f, 30, 90, 160, 210);
                float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
                if (p_spd > 0.001f) {
                    kx = (p.vx / p_spd) * 320.0f;
                    ky = (p.vy / p_spd) * 320.0f;
                }
                p.hit_cooldown = 12;
            } else if (p.type == PROJ_AERIAL_ACE) {
                g_projectiles.trigger_hitstop(0.055f, opponent->x, opponent->y);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 26, 255, 255, 255, 230.0f);
                g_projectiles.spawn_particles(opponent->x, opponent->y, 14, 203, 213, 225, 190.0f);
                g_projectiles.spawn_shockwave(opponent->x, opponent->y, 85.0f, 0.22f, 255, 255, 255, 5);
                g_projectiles.screen_shake = 0.22f;
                g_arena.add_decal(DECAL_SAND_FURROW, opponent->x, opponent->y, 36.0f, p.vx != 0.0f || p.vy != 0.0f ? atan2f(p.vy, p.vx) : 0.0f, 180, 195, 210, 200);
            }

            // Activate Hit-Cancel window on attacker when landing a melee strike
            if (p.type == PROJ_CLAW_SCRATCH || p.type == PROJ_DRAGON_CLAW ||
                p.type == PROJ_MELEE_SWEEP || p.type == PROJ_VOLT_TACKLE ||
                p.type == PROJ_FORCE_PALM || p.type == PROJ_CLOSE_COMBAT ||
                p.type == PROJ_BONE_RUSH || p.type == PROJ_EXTREME_SPEED ||
                p.type == PROJ_NIGHT_SLASH || p.type == PROJ_AERIAL_ACE) {
                hit_cancel_timer = 0.18f; // 0.18s (~11 frames) combo cancel window
            }

            // Real-Time Type Advantage, Stat Stages & Status scaling
            float type_mult = 1.0f;
            int atk_stg = (p.category == CATEGORY_PHYSICAL) ? stage_atk : stage_spa;
            int def_stg = (p.category == CATEGORY_PHYSICAL) ? opponent->stage_def : opponent->stage_spd;
            bool burned = (status == STATUS_BURN);
            bool blaze = blaze_active;
            int final_damage = TypeChart::calculate_damage(
                base_pwr, p.category, p.element_type,
                p.owner_stats, p.owner_type1, p.owner_type2,
                opponent->base_stats, opponent->type1, opponent->type2,
                atk_stg, def_stg, burned, blaze,
                &type_mult
            );

            opponent->take_damage(final_damage, kx, ky, type_mult);
            CombatLogger::record_move_hit(id, p.move_id, final_damage, opponent->hp <= 0);

            if (p.type == PROJ_THUNDER || p.type == PROJ_THUNDERBOLT || p.type == PROJ_EARTHQUAKE) {
                p.can_damage = false; // Closed single-hit impact window
            }

            if (!p.piercing) {
                p.active = false;
            } else {
                p.hit_cooldown = 15;
            }
            return true;
        }
    }
    return any_hit;
}



bool ProjectileManager::check_melee_clash(Fighter* p1, Fighter* p2) {
    if (!p1 || !p2) return false;

    for (int i = 0; i < MAX_PROJ; ++i) {
        Projectile& proj1 = pool[i];
        if (!proj1.active || !proj1.can_damage || proj1.delay_timer > 0.0f) continue;
        bool is_melee_1 = (proj1.type == PROJ_MELEE_SWEEP || proj1.type == PROJ_CLAW_SCRATCH ||
                           proj1.type == PROJ_WING_ATTACK || proj1.type == PROJ_DRAGON_CLAW || proj1.type == PROJ_VOLT_TACKLE ||
                           proj1.type == PROJ_FORCE_PALM || proj1.type == PROJ_CLOSE_COMBAT ||
                           proj1.type == PROJ_BONE_RUSH || proj1.type == PROJ_EXTREME_SPEED ||
                           proj1.type == PROJ_NIGHT_SLASH || proj1.type == PROJ_AERIAL_ACE);
        if (!is_melee_1) continue;

        for (int j = 0; j < MAX_PROJ; ++j) {
            if (i == j) continue;
            Projectile& proj2 = pool[j];
            if (!proj2.active || !proj2.can_damage || proj2.delay_timer > 0.0f) continue;
            if (proj1.owner_id == proj2.owner_id) continue;
            bool is_melee_2 = (proj2.type == PROJ_MELEE_SWEEP || proj2.type == PROJ_CLAW_SCRATCH ||
                               proj2.type == PROJ_WING_ATTACK || proj2.type == PROJ_DRAGON_CLAW || proj2.type == PROJ_VOLT_TACKLE ||
                               proj2.type == PROJ_FORCE_PALM || proj2.type == PROJ_CLOSE_COMBAT ||
                               proj2.type == PROJ_BONE_RUSH || proj2.type == PROJ_EXTREME_SPEED ||
                               proj2.type == PROJ_NIGHT_SLASH || proj2.type == PROJ_AERIAL_ACE);
            if (!is_melee_2) continue;

            float dx = proj1.x - proj2.x;
            float dy = proj1.y - proj2.y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < (proj1.radius + proj2.radius + 18.0f)) {
                // ANIME MELEE CLASH!
                proj1.active = false;
                proj1.can_damage = false;
                proj2.active = false;
                proj2.can_damage = false;

                float cx = (proj1.x + proj2.x) * 0.5f;
                float cy = (proj1.y + proj2.y) * 0.5f;

                trigger_hitstop(0.080f, cx, cy);
                spawn_shockwave(cx, cy, 115.0f, 0.28f, 250, 204, 21, 6);
                spawn_shockwave(cx, cy, 80.0f, 0.22f, 239, 68, 68, 4);
                spawn_sparks(cx, cy, 12.0f, 36, 255, 255, 255, 340.0f);
                spawn_sparks(cx, cy, 10.0f, 22, 250, 204, 21, 260.0f);
                spawn_electric_sparks(cx, cy, 12.0f, 14, 280.0f);
                screen_shake = 0.26f;

                spawn_custom_text(cx, cy - 25.0f, "CLASH!", 255, 230, 80, 50);

                float rdx = p1->x - cx;
                float rdy = p1->y - cy;
                float rlen = sqrtf(rdx * rdx + rdy * rdy);
                if (rlen < 1.0f) { rdx = -1.0f; rdy = 0.0f; rlen = 1.0f; }

                p1->vx = (rdx / rlen) * 320.0f;
                p1->vy = (rdy / rlen) * 320.0f;
                p1->state = STATE_IDLE;
                p1->attack_timer = 0.0f;
                p1->active_move = MOVE_NONE;
                p1->is_flying = false;
                p1->iron_tail_striking = false;

                p2->vx = -(rdx / rlen) * 320.0f;
                p2->vy = -(rdy / rlen) * 320.0f;
                p2->state = STATE_IDLE;
                p2->attack_timer = 0.0f;
                p2->active_move = MOVE_NONE;
                p2->is_flying = false;
                p2->iron_tail_striking = false;

                return true;
            }
        }
    }
    return false;
}

