#include "ai.hpp"
#include "projectile.hpp"
#include "arena.hpp"
#include "types.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

BattleAI g_ai_p1;
BattleAI g_ai_p2;
BattleAI& g_ai = g_ai_p2;

void BattleAI::init(AIMode ai_mode) {
    mode = ai_mode;
    state = AI_STATE_SPACING;
    stance = AI_STANCE_NEUTRAL;
    stance_timer = 0.0f;
    stance_decision_timer = 1.0f + (float)(rand() % 15) * 0.1f;
    decision_timer = 0.0f;
    attack_cooldown_timer = (mode == AI_MODE_CASUAL) ? 0.9f : 0.5f;
    reaction_timer = 0.0f;
    strafe_timer = 2.0f;
    strafe_direction = 1.0f;
    evade_lock_timer = 0.0f;
    ai_dodge_cooldown_timer = 0.0f;
    threat_detected = false;
    threat_dodge_x = 0.0f;
    threat_dodge_y = 0.0f;
    human_reaction_timer = 0.0f;
    casual_cadence_timer = 0.0f;
    ai_target_clone_idx = -1;
    ai_retarget_timer = 0.0f;
    hit_confirm_active = false;
    combo_stage = 0;
    combo_timer = 0.0f;
    combo_last_move = MOVE_NONE;
}

void BattleAI::update_tactical_stance(Fighter* cpu, Fighter* player, float dt, float dist) {
    (void)dist;
    if (!cpu || !player || cpu->hp <= 0 || player->hp <= 0) return;

    if (stance_timer > 0.0f) stance_timer -= dt;
    if (stance_decision_timer > 0.0f) stance_decision_timer -= dt;

    // 1. Critical Stamina Depletion Safeguard:
    // If stamina drops below 20 SP, immediately abort aggressive stances and disengage to recharge!
    if (cpu->stamina < 20.0f && stance != AI_STANCE_RECHARGE) {
        stance = AI_STANCE_RECHARGE;
        stance_timer = 1.8f + (float)(rand() % 8) * 0.1f; // 1.8s - 2.5s retreat
        return;
    }

    // 2. Recharge Stance Handling:
    if (stance == AI_STANCE_RECHARGE) {
        // Return to neutral when stamina is comfortably restored (>= 75 SP) or timer expires
        if (cpu->stamina >= 75.0f || stance_timer <= 0.0f) {
            stance = AI_STANCE_NEUTRAL;
            stance_timer = 0.0f;
            stance_decision_timer = 0.4f + (float)(rand() % 6) * 0.1f; // Quick evaluation after recharging!
        }
        return;
    }

    // 3. Rushdown Stance Handling:
    if (stance == AI_STANCE_RUSHDOWN) {
        // Exit rushdown if stamina drops below 20 SP or time runs out
        if (stance_timer <= 0.0f || cpu->stamina < 20.0f) {
            stance = AI_STANCE_NEUTRAL;
            stance_timer = 0.0f;
            stance_decision_timer = 0.8f + (float)(rand() % 8) * 0.1f;
        }
        return;
    }

    // 4. Neutral Stance -> Periodic Aggression / Rushdown Evaluation:
    if (stance == AI_STANCE_NEUTRAL && stance_decision_timer <= 0.0f) {
        stance_decision_timer = 0.6f + (float)(rand() % 8) * 0.1f; // Check frequently (0.6s - 1.4s)

        // Can we enter rushdown?
        // Pre-requisites:
        // A) Healthy stamina (>= 55 SP, or high stamina wait)
        // B) CPU is not in hitstun, flying, or attacked
        // C) Has close-range move or gap-closer ready
        if (cpu->stamina >= 55.0f && cpu->hitstun_timer <= 0.0f && !cpu->is_flying) {
            bool has_close_move = false;
            for (int i = 0; i < 4; ++i) {
                MoveId mid = cpu->abilities[i].move_id;
                if (mid == MOVE_NONE) continue;
                const MoveDef& def = MoveRegistry::get(mid);
                if (def.max_range <= 240.0f || mid == MOVE_EARTHQUAKE || mid == MOVE_SEISMIC_TOSS ||
                    mid == MOVE_WING_ATTACK || mid == MOVE_EXTREME_SPEED || mid == MOVE_BONE_RUSH) {
                    if (cpu->abilities[i].cooldown_timer <= 0.5f) {
                        has_close_move = true;
                        break;
                    }
                }
            }

            if (has_close_move) {
                // If stamina is high (>= 75 SP), high probability to rushdown!
                int roll = rand() % 100;
                int threshold = (cpu->stamina >= 75.0f) ? 80 : ((mode == AI_MODE_BOSS) ? 65 : 45);
                if (roll < threshold) {
                    stance = AI_STANCE_RUSHDOWN;
                    stance_timer = 3.5f + (float)(rand() % 15) * 0.1f; // 3.5s - 5.0s rushdown burst
                }
            }
        }
    }
}


bool BattleAI::evaluate_whiff_punish(Fighter* cpu, Fighter* player, float dist) {
    if (!cpu || !player || cpu->hp <= 0 || player->hp <= 0) return false;
    if (cpu->state == STATE_ATTACK || cpu->dash_timer > 0.0f || cpu->hitstun_timer > 0.0f) return false;
    if (cpu->stamina < 25.0f) return false;

    // Is the player vulnerable? (In attack active/recovery or shield break stun)
    bool is_vulnerable = (player->state == STATE_ATTACK && player->attack_timer > 0.0f) ||
                         (player->shield_break_timer > 0.0f);
    if (!is_vulnerable) return false;

    // Find the best gap-closing punish move in cpu loadout
    int best_slot = -1;
    float best_reach = 0.0f;
    for (int i = 0; i < 4; ++i) {
        MoveId mid = cpu->abilities[i].move_id;
        if (mid == MOVE_NONE) continue;
        const MoveDef& def = MoveRegistry::get(mid);
        if (cpu->abilities[i].cooldown_timer > 0.0f || cpu->stamina < (float)def.stamina_cost) continue;

        // Gap-closers & heavy punishes
        bool is_punish_move = (mid == MOVE_QUICK_ATTACK || mid == MOVE_EXTREME_SPEED ||
                               mid == MOVE_WING_ATTACK || mid == MOVE_BONE_RUSH ||
                               mid == MOVE_VOLT_TACKLE || mid == MOVE_FORCE_PALM ||
                               mid == MOVE_AERIAL_ACE || mid == MOVE_NIGHT_SLASH ||
                               (dist <= 100.0f && mid == MOVE_SCRATCH));
        if (is_punish_move && dist <= def.max_range && def.max_range > best_reach) {
            best_reach = def.max_range;
            best_slot = i;
        }
    }

    if (best_slot >= 0) {
        cpu->use_ability(best_slot, player, player->x, player->y);
        attack_cooldown_timer = (mode == AI_MODE_CASUAL) ? 0.35f : 0.15f;
        return true;
    }

    return false;
}

bool BattleAI::execute_hit_confirm_combo(Fighter* cpu, Fighter* player, float dist, float target_x, float target_y) {
    if (!cpu || !player || cpu->hp <= 0 || player->hp <= 0) return false;
    if (cpu->state == STATE_ATTACK || cpu->dash_timer > 0.0f || cpu->hitstun_timer > 0.0f) return false;
    if (cpu->stamina < 15.0f) return false;

    // Player must be in confirmed hitstun
    if (player->state != STATE_HITSTUN || player->hitstun_timer <= 0.08f) {
        hit_confirm_active = false;
        combo_stage = 0;
        return false;
    }

    hit_confirm_active = true;

    // Combo branches per species:
    if (cpu->type == PKMN_PIKACHU) {
        // Pikachu: Quick Attack / Volt Tackle hitstun -> Iron Tail slam or Electro Ball point-blank
        for (int i = 0; i < 4; ++i) {
            MoveId mid = cpu->abilities[i].move_id;
            if (mid == MOVE_IRON_TAIL && cpu->abilities[i].cooldown_timer <= 0.0f && dist <= 160.0f && cpu->stamina >= 25.0f) {
                cpu->use_ability(i, player, target_x, target_y);
                attack_cooldown_timer = 0.18f;
                combo_stage++;
                combo_last_move = MOVE_IRON_TAIL;
                return true;
            }
        }
        for (int i = 0; i < 4; ++i) {
            MoveId mid = cpu->abilities[i].move_id;
            if (mid == MOVE_THUNDER && cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= 40.0f) {
                cpu->use_ability(i, player, target_x, target_y);
                attack_cooldown_timer = 0.24f;
                combo_stage++;
                combo_last_move = MOVE_THUNDER;
                return true;
            }
        }
        for (int i = 0; i < 4; ++i) {
            MoveId mid = cpu->abilities[i].move_id;
            if (mid == MOVE_ELECTRO_BALL && cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= 20.0f) {
                cpu->use_ability(i, player, target_x, target_y);
                attack_cooldown_timer = 0.20f;
                combo_stage++;
                combo_last_move = MOVE_ELECTRO_BALL;
                return true;
            }
        }
    } else if (cpu->type == PKMN_LUCARIO) {
        // Lucario: Bone Rush -> Force Palm -> Close Combat sequence
        if (combo_stage == 0) {
            // Stage 1: Point-blank Force Palm thrust
            for (int i = 0; i < 4; ++i) {
                if (cpu->abilities[i].move_id == MOVE_FORCE_PALM && cpu->abilities[i].cooldown_timer <= 0.0f &&
                    dist <= 120.0f && cpu->stamina >= 18.0f) {
                    cpu->use_ability(i, player, target_x, target_y);
                    attack_cooldown_timer = 0.16f;
                    combo_stage = 1;
                    combo_last_move = MOVE_FORCE_PALM;
                    return true;
                }
            }
        }
        // Stage 2: Heavy Close Combat flurry or Aura Sphere finisher
        for (int i = 0; i < 4; ++i) {
            MoveId mid = cpu->abilities[i].move_id;
            if ((mid == MOVE_CLOSE_COMBAT && dist <= 110.0f) || (mid == MOVE_AURA_SPHERE && dist <= 280.0f)) {
                if (cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= (float)cpu->abilities[i].stamina_cost) {
                    cpu->use_ability(i, player, target_x, target_y);
                    attack_cooldown_timer = 0.22f;
                    combo_stage = 2;
                    combo_last_move = mid;
                    return true;
                }
            }
        }
    } else if (cpu->type == PKMN_CHARIZARD) {
        // Charizard: Wing Attack dive -> Scratch or Flamethrower point blank
        for (int i = 0; i < 4; ++i) {
            MoveId mid = cpu->abilities[i].move_id;
            if (mid == MOVE_SCRATCH && dist <= 110.0f && cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= 15.0f) {
                cpu->use_ability(i, player, target_x, target_y);
                attack_cooldown_timer = 0.18f;
                combo_stage++;
                combo_last_move = MOVE_SCRATCH;
                return true;
            }
            if (mid == MOVE_FLAMETHROWER && dist <= 240.0f && cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= 25.0f) {
                cpu->use_ability(i, player, target_x, target_y);
                attack_cooldown_timer = 0.22f;
                combo_stage++;
                combo_last_move = MOVE_FLAMETHROWER;
                return true;
            }
        }
    } else if (cpu->type == PKMN_GRENINJA) {
        // Greninja: Water Shuriken stun -> Aerial Ace gap-close -> Night Slash execute
        if (combo_stage == 0) {
            // Stage 1: Aerial Ace gap-closer
            for (int i = 0; i < 4; ++i) {
                if (cpu->abilities[i].move_id == MOVE_AERIAL_ACE && cpu->abilities[i].cooldown_timer <= 0.0f &&
                    dist <= 380.0f && cpu->stamina >= 26.0f) {
                    cpu->use_ability(i, player, target_x, target_y);
                    attack_cooldown_timer = 0.16f;
                    combo_stage = 1;
                    combo_last_move = MOVE_AERIAL_ACE;
                    return true;
                }
            }
        }
        // Stage 2: Night Slash point-blank critical strike or Hydro Pump finisher
        for (int i = 0; i < 4; ++i) {
            MoveId mid = cpu->abilities[i].move_id;
            if ((mid == MOVE_NIGHT_SLASH && dist <= 220.0f) || (mid == MOVE_HYDRO_PUMP && dist <= 420.0f)) {
                if (cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= (float)cpu->abilities[i].stamina_cost) {
                    cpu->use_ability(i, player, target_x, target_y);
                    attack_cooldown_timer = 0.20f;
                    combo_stage = 2;
                    combo_last_move = mid;
                    return true;
                }
            }
        }
    }

    return false;
}

void BattleAI::evaluate_threats(Fighter* cpu, Fighter* player, float dt) {
    (void)player;

    if (threat_detected) {
        reaction_timer -= dt;
        if (reaction_timer <= 0.0f) {
            // Reaction delay expired - execute dodge action
            if (mode == AI_MODE_CASUAL) {
                // Casual mode: 55% chance to miss dodge or misjudge timing
                if (rand() % 100 < 55) {
                    threat_detected = false;
                    return;
                }
                // When dodging, casual AI uses standard footwork, no burst dash
                cpu->input_dx = threat_dodge_x;
                cpu->input_dy = threat_dodge_y;
            } else {
                bool can_dash = (cpu->stamina >= DASH_STAMINA_COST &&
                                 cpu->dash_timer <= 0.0f && cpu->dash_cooldown_timer <= 0.0f &&
                                 cpu->hitstun_timer <= 0.0f && ai_dodge_cooldown_timer <= 0.0f);

                cpu->input_dx = threat_dodge_x;
                cpu->input_dy = threat_dodge_y;
                if (can_dash) {
                    cpu->try_dash(threat_dodge_x, threat_dodge_y);
                    evade_lock_timer = 0.32f;
                    ai_dodge_cooldown_timer = 1.25f; // AI cannot burst dash again for 1.25s
                }
            }
            threat_detected = false;
        }
        return;
    }

    // Scan for incoming threats from the player
    float closest_threat_time = 999.0f;
    float best_dodge_x = 0.0f;
    float best_dodge_y = 0.0f;
    bool found_threat = false;

    // Check if CPU has Protect equipped and ready for precision parry
    int protect_slot = -1;
    for (int s = 0; s < 4; ++s) {
        if (cpu->abilities[s].move_id == MOVE_PROTECT && cpu->abilities[s].cooldown_timer <= 0.0f && cpu->stamina >= 25.0f) {
            protect_slot = s;
            break;
        }
    }

    for (int i = 0; i < MAX_PROJ; ++i) {
        const Projectile& p = g_projectiles.pool[i];
        if (!p.active || p.owner_id == cpu->id || p.damage == 0) continue;

        // Relative position and velocity
        float rx = cpu->x - p.x;
        float ry = cpu->y - p.y;
        float dist = sqrtf(rx * rx + ry * ry);
        if (dist > 550.0f) continue; // Out of immediate reaction range

        float p_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);

        // Ground telegraph attack countdown: Thunder celestial pillar or Earthquake epicenter
        if ((p.type == PROJ_THUNDER || p.type == PROJ_EARTHQUAKE) && p.delay_timer > 0.0f) {
            if (dist < (cpu->radius + p.radius + 20.0f)) {
                if (protect_slot >= 0 && mode == AI_MODE_BOSS && p.delay_timer <= 0.09f) {
                    cpu->use_ability(protect_slot, player, cpu->x, cpu->y);
                    threat_detected = false;
                    return;
                }
                found_threat = true;
                best_dodge_x = (dist > 0.001f) ? (rx / dist) : 1.0f;
                best_dodge_y = (dist > 0.001f) ? (ry / dist) : 0.0f;
                threat_detected = true;
                reaction_timer = (mode == AI_MODE_CASUAL) ? 0.12f : 0.0f;
                threat_dodge_x = best_dodge_x;
                threat_dodge_y = best_dodge_y;
                return;
            }
            continue;
        }

        // Stationary / residual hazards are continuously handled via apply_hazard_avoidance
        if (p_spd < 10.0f) continue;

        // Dot product between projectile heading and relative vector to CPU
        float dot = (rx * p.vx + ry * p.vy) / (dist * p_spd);
        if (dot > 0.60f) { // Projectile is heading toward CPU
            float time_to_impact = dist / p_spd;
            if (time_to_impact < closest_threat_time && time_to_impact < 0.65f) {
                // If Boss AI has Protect ready and impact is within 0.09s, trigger Perfect Parry!
                if (protect_slot >= 0 && mode == AI_MODE_BOSS && time_to_impact <= 0.09f) {
                    cpu->use_ability(protect_slot, player, cpu->x, cpu->y);
                    threat_detected = false;
                    return;
                }
                closest_threat_time = time_to_impact;
                found_threat = true;

                // Perpendicular vector to projectile trajectory
                float perp_x = -p.vy / p_spd;
                float perp_y = p.vx / p_spd;

                // Pick direction away from arena borders
                float test_x = cpu->x + perp_x * 100.0f;
                float test_y = cpu->y + perp_y * 100.0f;
                if (test_x < ARENA_MIN_X + 150.0f || test_x > ARENA_MAX_X - 150.0f ||
                    test_y < ARENA_MIN_Y + 150.0f || test_y > ARENA_MAX_Y - 150.0f) {
                    perp_x = -perp_x;
                    perp_y = -perp_y;
                }

                best_dodge_x = perp_x;
                best_dodge_y = perp_y;
            }
        }
    }

    if (found_threat) {
        threat_detected = true;
        // Human reaction latency: Casual mode (380-460ms) vs Boss mode (170-230ms)
        reaction_timer = (mode == AI_MODE_CASUAL) ? (0.38f + ((float)(rand() % 80) / 1000.0f))
                                                  : (0.17f + ((float)(rand() % 60) / 1000.0f));
        threat_dodge_x = best_dodge_x;
        threat_dodge_y = best_dodge_y;
    }
}

void BattleAI::apply_hazard_avoidance(Fighter* cpu, float& out_dx, float& out_dy) {
    if (!cpu) return;

    for (int i = 0; i < MAX_PROJ; ++i) {
        const Projectile& p = g_projectiles.pool[i];
        if (!p.active || p.owner_id == cpu->id) continue;

        float haz_x = 0.0f;
        float haz_y = 0.0f;
        float haz_radius = 0.0f;
        bool is_hazard = false;

        if (p.type == PROJ_FIRE_SPIN) {
            if (p.has_struck) {
                // Active spinning fire cyclone residue on ground
                haz_x = p.x;
                haz_y = p.y;
                haz_radius = p.radius; // 52px
                is_hazard = true;
            } else if (p.delay_timer > 0.0f) {
                // Incoming ember seed landing coordinate
                haz_x = p.target_x;
                haz_y = p.target_y;
                haz_radius = 52.0f;
                is_hazard = true;
            }
        } else if (p.type == PROJ_EARTHQUAKE && p.delay_timer > 0.0f) {
            haz_x = p.x;
            haz_y = p.y;
            haz_radius = p.radius;
            is_hazard = true;
        } else if (p.type == PROJ_THUNDER && p.delay_timer > 0.0f) {
            haz_x = p.x;
            haz_y = p.y;
            haz_radius = p.radius;
            is_hazard = true;
        }

        if (!is_hazard) continue;

        float rx = cpu->x - haz_x;
        float ry = cpu->y - haz_y;
        float dist = sqrtf(rx * rx + ry * ry);

        // Safety clearance: hazard radius + fighter radius + 55px buffer
        float clearance = haz_radius + cpu->radius + 55.0f;

        if (dist < clearance) {
            float nx = (dist > 0.001f) ? (rx / dist) : 1.0f;
            float ny = (dist > 0.001f) ? (ry / dist) : 0.0f;

            // 1. Critical proximity emergency escape:
            // If the fighter is inside or immediately touching the fire vortex:
            if (dist < (haz_radius + cpu->radius + 4.0f)) {
                out_dx = nx;
                out_dy = ny;

                // Immediate burst dash escape if available
                if (cpu->dash_timer <= 0.0f && cpu->dash_cooldown_timer <= 0.0f &&
                    cpu->stamina >= DASH_STAMINA_COST && ai_dodge_cooldown_timer <= 0.0f) {
                    cpu->try_dash(nx, ny);
                    evade_lock_timer = 0.20f;
                    ai_dodge_cooldown_timer = 0.5f;
                }
                return;
            }

            // 2. Proximity repulsion:
            float t = 1.0f - (dist / clearance);
            float repulse_power = t * t * 4.5f;

            // Cancel any movement component heading towards the hazard
            float approach = out_dx * (-nx) + out_dy * (-ny);
            if (approach > 0.0f) {
                out_dx -= (-nx) * approach;
                out_dy -= (-ny) * approach;
            }

            // Tangential deflection: steer around circular hazard
            float perp_x = -ny * strafe_direction;
            float perp_y = nx * strafe_direction;

            out_dx += nx * repulse_power + perp_x * 2.0f;
            out_dy += ny * repulse_power + perp_y * 2.0f;
        }
    }

    float len = sqrtf(out_dx * out_dx + out_dy * out_dy);
    if (len > 1.0f) {
        out_dx /= len;
        out_dy /= len;
    }
}

void BattleAI::apply_arena_steering(Fighter* cpu, float& out_dx, float& out_dy) {
    const float margin = 260.0f;
    float steer_x = 0.0f;
    float steer_y = 0.0f;

    if (cpu->x < ARENA_MIN_X + margin) {
        float f = 1.0f - (cpu->x - ARENA_MIN_X) / margin;
        steer_x += std::max(0.0f, f) * 2.2f;
    } else if (cpu->x > ARENA_MAX_X - margin) {
        float f = 1.0f - (ARENA_MAX_X - cpu->x) / margin;
        steer_x -= std::max(0.0f, f) * 2.2f;
    }

    if (cpu->y < ARENA_MIN_Y + margin) {
        float f = 1.0f - (cpu->y - ARENA_MIN_Y) / margin;
        steer_y += std::max(0.0f, f) * 2.2f;
    } else if (cpu->y > ARENA_MAX_Y - margin) {
        float f = 1.0f - (ARENA_MAX_Y - cpu->y) / margin;
        steer_y -= std::max(0.0f, f) * 2.2f;
    }

    out_dx += steer_x;
    out_dy += steer_y;

    float len = sqrtf(out_dx * out_dx + out_dy * out_dy);
    if (len > 1.0f) {
        out_dx /= len;
        out_dy /= len;
    }
}

void BattleAI::handle_movement_and_spacing(Fighter* cpu, Fighter* player, float dt, float dist, float dx, float dy) {
    (void)player;
    if (dist < 1.0f) {
        dx = cosf(cpu->aim_angle);
        dy = sinf(cpu->aim_angle);
        dist = 1.0f;
    }

    float dir_x = 0.0f;
    float dir_y = 0.0f;

    float ideal_min = 200.0f;
    float ideal_max = 380.0f;
    float approach_thresh = 480.0f;

    if (stance == AI_STANCE_RUSHDOWN) {
        // Point-blank infighting band: 35px - 120px
        ideal_min = 35.0f;
        ideal_max = 120.0f;
        approach_thresh = 140.0f;

        // Gap closer: If target is > 150px and we have healthy stamina (>= 55 SP)
        // and burst dash is ready, verify path does NOT cross hazard before dashing!
        if (dist > 150.0f && cpu->stamina >= 55.0f && cpu->dash_timer <= 0.0f &&
            cpu->dash_cooldown_timer <= 0.0f && ai_dodge_cooldown_timer <= 0.0f && cpu->state != STATE_ATTACK) {
            bool path_crosses_hazard = false;
            for (int i = 0; i < MAX_PROJ; ++i) {
                const Projectile& p = g_projectiles.pool[i];
                if (!p.active || p.owner_id == cpu->id) continue;
                if ((p.type == PROJ_FIRE_SPIN && p.has_struck) ||
                    (p.type == PROJ_FIRE_SPIN && p.delay_timer > 0.0f)) {
                    float hz_x = p.has_struck ? p.x : p.target_x;
                    float hz_y = p.has_struck ? p.y : p.target_y;
                    float seg_dx = dx;
                    float seg_dy = dy;
                    float seg_len_sq = seg_dx * seg_dx + seg_dy * seg_dy;
                    if (seg_len_sq > 1.0f) {
                        float u = ((hz_x - cpu->x) * seg_dx + (hz_y - cpu->y) * seg_dy) / seg_len_sq;
                        if (u > 0.0f && u < 1.0f) {
                            float close_x = cpu->x + u * seg_dx;
                            float close_y = cpu->y + u * seg_dy;
                            float d2 = (hz_x - close_x) * (hz_x - close_x) + (hz_y - close_y) * (hz_y - close_y);
                            if (d2 < (p.radius + 35.0f) * (p.radius + 35.0f)) {
                                path_crosses_hazard = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (!path_crosses_hazard) {
                cpu->try_dash(dx / dist, dy / dist);
                ai_dodge_cooldown_timer = 0.8f;
            }
        }
    } else if (stance == AI_STANCE_RECHARGE) {
        // Disengage / retreat band: 420px - 580px
        ideal_min = 420.0f;
        ideal_max = 580.0f;
        approach_thresh = 620.0f;
    } else {
        // AI_STANCE_NEUTRAL
        if (cpu->type == PKMN_LUCARIO) {
            ideal_min = 60.0f;
            ideal_max = 160.0f;
            approach_thresh = 210.0f;
        } else if (cpu->type == PKMN_GRENINJA) {
            ideal_min = 120.0f;
            ideal_max = 260.0f;
            approach_thresh = 320.0f;
        } else {
            ideal_min = 200.0f;
            ideal_max = 380.0f;
            approach_thresh = 480.0f;
        }
    }

    strafe_timer -= dt;
    if (strafe_timer <= 0.0f) {
        strafe_direction = -strafe_direction;
        strafe_timer = 1.6f + (float)(rand() % 14) * 0.1f;
    }

    if (dist > approach_thresh) {
        // Far out: Close in toward player with slight lateral weave
        state = AI_STATE_APPROACH;
        float ux = dx / dist;
        float uy = dy / dist;
        float weave_factor = (stance == AI_STANCE_RUSHDOWN) ? 0.12f : 0.35f;
        float tx = -uy * strafe_direction * weave_factor;
        float ty = ux * strafe_direction * weave_factor;
        dir_x = ux + tx;
        dir_y = uy + ty;
    } else if (dist < ideal_min) {
        // Too close: Back up while facing player
        state = AI_STATE_SPACING;
        dir_x = -dx / dist;
        dir_y = -dy / dist;
    } else {
        // Sweet spot: Tangential circle-strafe
        state = AI_STATE_STRAFE;
        float tx = (-dy / dist) * strafe_direction;
        float ty = (dx / dist) * strafe_direction;

        // Minor radial correction to stay inside the band
        float radial = 0.0f;
        float outer_bound = (ideal_min + ideal_max) * 0.55f;
        float inner_bound = (ideal_min + ideal_max) * 0.45f;
        if (dist > outer_bound) radial = 0.30f;
        else if (dist < inner_bound) radial = -0.30f;

        dir_x = tx + (dx / dist) * radial;
        dir_y = ty + (dy / dist) * radial;
    }

    apply_hazard_avoidance(cpu, dir_x, dir_y);
    apply_arena_steering(cpu, dir_x, dir_y);

    cpu->input_dx = dir_x;
    cpu->input_dy = dir_y;
}

void BattleAI::select_attack(Fighter* cpu, Fighter* player, float dist, float target_x, float target_y) {
    if (cpu->state == STATE_ATTACK || cpu->dash_timer > 0.0f || cpu->hitstun_timer > 0.0f) return;

    // 0. Hit-Confirm True Combo Engine:
    // If the opponent was struck into hitstun, bypass neutral cooldown and link into follow-up
    if (player->state == STATE_HITSTUN && player->hitstun_timer > 0.08f) {
        if (execute_hit_confirm_combo(cpu, player, dist, target_x, target_y)) {
            return;
        }
    } else {
        hit_confirm_active = false;
        combo_stage = 0;
    }

    // 0. Whiff Punish Engine:
    // If opponent whiffs a high-commitment move and is locked in active/recovery end-lag, punish!
    if (evaluate_whiff_punish(cpu, player, dist)) {
        return;
    }

    if (attack_cooldown_timer > 0.0f) return;
    if (mode == AI_MODE_CASUAL && casual_cadence_timer > 0.0f) return;

    bool player_airborne = (player->z > 25.0f);
    // 0. Primed Stance Follow-up (e.g. Iron Tail Stage 2 Release)
    if (cpu->primed_move == MOVE_IRON_TAIL || cpu->iron_tail_primed) {
        for (int i = 0; i < 4; ++i) {
            if (cpu->abilities[i].move_id == MOVE_IRON_TAIL) {
                const MoveDef& def = MoveRegistry::get(MOVE_IRON_TAIL);
                if (dist <= def.max_range && cpu->stamina >= 25.0f) {
                    cpu->use_ability(i, player, target_x, target_y);
                    attack_cooldown_timer = (mode == AI_MODE_CASUAL) ? 0.65f : 0.35f;
                    if (mode == AI_MODE_CASUAL) casual_cadence_timer = 0.85f;
                    return;
                }
            }
        }
    }

    // 0. Agility Speed Surge Buff
    for (int i = 0; i < 4; ++i) {
        if (cpu->abilities[i].move_id == MOVE_AGILITY) {
            if (cpu->agility_timer <= 0.0f && cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= 30.0f) {
                cpu->use_ability(i, player, target_x, target_y);
                attack_cooldown_timer = 0.20f;
                return;
            }
        }
    }

    // 0. Anti-Air Tactics against Airborne Target (player->z > 25px)
    if (player_airborne) {
        for (int i = 0; i < 4; ++i) {
            MoveId mid = cpu->abilities[i].move_id;
            if (mid == MOVE_THUNDER || mid == MOVE_THUNDERBOLT || mid == MOVE_DRAGON_PULSE || mid == MOVE_AURA_SPHERE || mid == MOVE_WING_ATTACK) {
                const MoveDef& def = MoveRegistry::get(mid);
                if (cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= (float)def.stamina_cost) {
                    cpu->use_ability(i, player, target_x, target_y);
                    attack_cooldown_timer = (mode == AI_MODE_CASUAL) ? 0.65f : 0.30f;
                    if (mode == AI_MODE_CASUAL) casual_cadence_timer = 0.85f;
                    return;
                }
            }
        }
    }

    // 1. Point-Blank Ground Nuke Priority in Rushdown Stance:
    // If Charizard/opponent is inside Earthquake radius (<= 180px) during rushdown,
    // fire Earthquake immediately without waiting or falling back to ranged pokes!
    if (stance == AI_STANCE_RUSHDOWN && !player_airborne) {
        for (int i = 0; i < 4; ++i) {
            MoveId mid = cpu->abilities[i].move_id;
            if (mid == MOVE_EARTHQUAKE && dist <= 165.0f) {
                const MoveDef& def = MoveRegistry::get(MOVE_EARTHQUAKE);
                if (cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= (float)def.stamina_cost) {
                    cpu->use_ability(i, player, target_x, target_y);
                    attack_cooldown_timer = (mode == AI_MODE_CASUAL) ? 0.85f : 0.40f;
                    if (mode == AI_MODE_CASUAL) casual_cadence_timer = 0.85f;
                    return;
                }
            } else if (mid == MOVE_SEISMIC_TOSS && dist <= 150.0f) {
                const MoveDef& def = MoveRegistry::get(MOVE_SEISMIC_TOSS);
                if (cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= (float)def.stamina_cost) {
                    cpu->use_ability(i, player, target_x, target_y);
                    attack_cooldown_timer = (mode == AI_MODE_CASUAL) ? 0.75f : 0.35f;
                    if (mode == AI_MODE_CASUAL) casual_cadence_timer = 0.85f;
                    return;
                }
            }
        }
    }

    // 2. Weighted Candidate Move Selection:
    // Eliminates hardcoded array slot starvation (Slot 0 always blocking Slot 2).
    // Collects all valid abilities in range and chooses probabilistically based on
    // type effectiveness, range suitability, and active tactical stance.
    struct MoveCandidate {
        int slot;
        MoveId mid;
        int weight;
        float cooldown_after;
    };
    MoveCandidate candidates[4];
    int candidate_count = 0;

    float reserve = (mode == AI_MODE_CASUAL) ? 5.0f : 10.0f;
    for (int i = 0; i < 4; ++i) {
        MoveId mid = cpu->abilities[i].move_id;
        if (mid == MOVE_NONE || mid == MOVE_PROTECT || mid == MOVE_AGILITY) continue;
        if (player_airborne && (mid == MOVE_EARTHQUAKE || mid == MOVE_SCORCHING_SANDS)) continue;

        const MoveDef& def = MoveRegistry::get(mid);
        if (dist >= def.min_range && dist <= def.max_range) {
            // In Rushdown stance, withhold ranged pokes (dist > 160px) to preserve stamina for close-range nukes!
            if (stance == AI_STANCE_RUSHDOWN && dist > 160.0f && def.min_range > 0.0f) {
                continue;
            }

            if (cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= (float)(def.stamina_cost + reserve)) {
                if (player->invuln_timer > 0.05f) continue;

                float mult = TypeChart::get_multiplier(def.element_type, player->type1, player->type2);
                int base_weight = (int)(mult * 40.0f); // 1.0x -> 40, 2.0x -> 80

                // Stance-based adaptation:
                if (stance == AI_STANCE_RUSHDOWN) {
                    if (mid == MOVE_EARTHQUAKE || mid == MOVE_SEISMIC_TOSS || mid == MOVE_CLOSE_COMBAT ||
                        mid == MOVE_FORCE_PALM || mid == MOVE_SCRATCH || mid == MOVE_QUICK_ATTACK || def.max_range <= 220.0f) {
                        base_weight *= 6; // Massive priority for in-fighting nukes and strikes
                    } else {
                        base_weight = std::max(2, base_weight / 4); // Deprioritize ranged pokes during rushdown
                    }
                } else if (stance == AI_STANCE_NEUTRAL) {
                    if (dist <= 160.0f && (mid == MOVE_EARTHQUAKE || mid == MOVE_SEISMIC_TOSS)) {
                        base_weight *= 3; // In neutral, if opponent is in melee range, punish!
                    } else if (def.max_range >= 240.0f) {
                        base_weight = (int)((float)base_weight * 1.5f); // Favor ranged pokes in neutral at distance
                    }
                } else if (stance == AI_STANCE_RECHARGE) {
                    // In recharge stance, deprioritize attacking unless point blank
                    if (dist > 120.0f) base_weight = std::max(5, base_weight / 2);
                }

                if (base_weight < 5) base_weight = 5;

                float cd_after = (mode == AI_MODE_CASUAL) ? 0.65f : 0.28f;
                if (mid == MOVE_EARTHQUAKE) cd_after = (mode == AI_MODE_CASUAL) ? 0.85f : 0.40f;
                else if (mid == MOVE_SEISMIC_TOSS) cd_after = (mode == AI_MODE_CASUAL) ? 0.75f : 0.35f;
                else if (mid == MOVE_THUNDER) cd_after = (mode == AI_MODE_CASUAL) ? 0.90f : 0.45f;
                else if (mid == MOVE_SCRATCH || mid == MOVE_QUICK_ATTACK || mid == MOVE_NIGHT_SLASH) cd_after = (mode == AI_MODE_CASUAL) ? 0.45f : 0.22f;
                else if (mid == MOVE_CLOSE_COMBAT || mid == MOVE_HYDRO_PUMP) cd_after = (mode == AI_MODE_CASUAL) ? 0.75f : 0.35f;
                else if (mid == MOVE_WATER_SHURIKEN) cd_after = (mode == AI_MODE_CASUAL) ? 0.50f : 0.25f;
                else if (mid == MOVE_AERIAL_ACE) cd_after = (mode == AI_MODE_CASUAL) ? 0.55f : 0.28f;

                candidates[candidate_count++] = { i, mid, base_weight, cd_after };
            }
        }
    }

    if (candidate_count > 0) {
        int total_weight = 0;
        for (int c = 0; c < candidate_count; ++c) total_weight += candidates[c].weight;
        int roll = rand() % total_weight;
        int running = 0;
        for (int c = 0; c < candidate_count; ++c) {
            running += candidates[c].weight;
            if (roll < running) {
                cpu->use_ability(candidates[c].slot, player, target_x, target_y);
                attack_cooldown_timer = candidates[c].cooldown_after;
                if (mode == AI_MODE_CASUAL) casual_cadence_timer = 0.85f;
                return;
            }
        }
    }
}

void BattleAI::update(Fighter* cpu, Fighter* player, float dt) {
    if (!cpu || !player || cpu->hp <= 0 || player->hp <= 0) {
        if (cpu) {
            cpu->input_dx = 0.0f;
            cpu->input_dy = 0.0f;
        }
        return;
    }

    if (attack_cooldown_timer > 0.0f) attack_cooldown_timer -= dt;
    if (decision_timer > 0.0f) decision_timer -= dt;
    if (evade_lock_timer > 0.0f) evade_lock_timer -= dt;
    if (ai_dodge_cooldown_timer > 0.0f) ai_dodge_cooldown_timer -= dt;
    if (casual_cadence_timer > 0.0f) casual_cadence_timer -= dt;

    // Double Team Decoy Target Evaluation:
    bool has_clones = false;
    int active_clone_count = 0;
    if (player->double_team_timer > 0.0f) {
        for (int c = 0; c < Fighter::MAX_CLONES; ++c) {
            if (player->clones[c].active) {
                has_clones = true;
                active_clone_count++;
            }
        }
    }

    if (!has_clones) {
        ai_target_clone_idx = -1;
        ai_retarget_timer = 0.0f;
    } else {
        ai_retarget_timer -= dt;
        bool target_dead = (ai_target_clone_idx >= 0 && !player->clones[ai_target_clone_idx].active);
        if (ai_retarget_timer <= 0.0f || target_dead) {
            ai_retarget_timer = (mode == AI_MODE_CASUAL) ? (1.5f + (float)(rand() % 4) * 0.1f)
                                                         : (0.85f + (float)(rand() % 6) * 0.1f);

            // Probabilistic target selection among real player + active clones
            int total_options = 1 + active_clone_count;
            int pick = rand() % total_options;
            if (pick == 0) {
                ai_target_clone_idx = -1; // Picked real player
            } else {
                int cur_active = 0;
                for (int c = 0; c < Fighter::MAX_CLONES; ++c) {
                    if (player->clones[c].active) {
                        cur_active++;
                        if (cur_active == pick) {
                            ai_target_clone_idx = c;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Determine target coordinate (real body vs decoy illusion)
    float target_x = player->x;
    float target_y = player->y;
    if (ai_target_clone_idx >= 0 && player->clones[ai_target_clone_idx].active) {
        target_x = player->clones[ai_target_clone_idx].x;
        target_y = player->clones[ai_target_clone_idx].y;
    }

    float dx = target_x - cpu->x;
    float dy = target_y - cpu->y;
    float dist = sqrtf(dx * dx + dy * dy);

    // 0. Dynamic Tactical Stance State Machine (Neutral, Rushdown, Recharge)
    update_tactical_stance(cpu, player, dt, dist);

    // Safety invariant: flight state cannot persist outside active attack duration
    if (cpu->attack_timer <= 0.0f && cpu->is_flying) {
        cpu->is_flying = false;
    }

    // Direct line-of-sight aim tracking toward target (unless locked in flight rush)
    if (!cpu->is_flying && dist > 0.001f) {
        cpu->aim_angle = atan2f(dy, dx);
    }

    // Hitstun, Dash, or Flight rush locks inputs
    if (cpu->hitstun_timer > 0.0f || cpu->dash_timer > 0.0f || cpu->is_flying) {
        cpu->input_dx = 0.0f;
        cpu->input_dy = 0.0f;
        return;
    }

    // Smart AoE Counterplay (Boss AI only):
    // In Boss mode, Charizard prioritizes Earthquake if >= 2 targets are clustered within 200px.
    // In Casual mode, AI is deceived by illusions and attacks them individually.
    if (mode == AI_MODE_BOSS && has_clones && cpu->state != STATE_ATTACK && cpu->dash_timer <= 0.0f && cpu->hitstun_timer <= 0.0f && attack_cooldown_timer <= 0.0f) {
        int targets_near_cpu = 0;
        float real_dist = sqrtf((player->x - cpu->x) * (player->x - cpu->x) + (player->y - cpu->y) * (player->y - cpu->y));
        if (real_dist <= 200.0f) targets_near_cpu++;
        for (int c = 0; c < Fighter::MAX_CLONES; ++c) {
            if (player->clones[c].active) {
                float cdist = sqrtf((player->clones[c].x - cpu->x) * (player->clones[c].x - cpu->x) +
                                    (player->clones[c].y - cpu->y) * (player->clones[c].y - cpu->y));
                if (cdist <= 200.0f) targets_near_cpu++;
            }
        }

        if (targets_near_cpu >= 2) {
            for (int i = 0; i < 4; ++i) {
                MoveId mid = cpu->abilities[i].move_id;
                if (mid == MOVE_EARTHQUAKE && cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= 40.0f) {
                    cpu->use_ability(i, player, cpu->x, cpu->y);
                    attack_cooldown_timer = 0.60f;
                    return;
                }
            }
        }
    }

    // 1. Reactive Dodging & Sensory Analysis
    evaluate_threats(cpu, player, dt);
    if (evade_lock_timer > 0.0f) {
        return;
    }

    // 2. Stamina Recovery / Tactical Evasive Weave in Recharge Stance
    bool should_retreat = (stance == AI_STANCE_RECHARGE || cpu->stamina < 25.0f);
    if (should_retreat && dist > 450.0f && cpu->stamina > player->stamina) {
        should_retreat = false; // Break standoff deadlock to pressure lower-stamina opponent
    }

    if (should_retreat) {
        state = AI_STATE_RETREAT;
        strafe_timer -= dt;
        if (strafe_timer <= 0.0f) {
            strafe_direction = -strafe_direction;
            strafe_timer = 0.8f + (float)(rand() % 10) * 0.1f;
        }

        // Tangential circle weave + backing away (angular evasion to make skillshots miss)
        float ux = (dist > 0.001f) ? dx / dist : 0.0f;
        float uy = (dist > 0.001f) ? dy / dist : 0.0f;
        float tx = -uy * strafe_direction;
        float ty = ux * strafe_direction;
        float retreat_x = -ux * 0.5f + tx * 0.85f;
        float retreat_y = -uy * 0.5f + ty * 0.85f;

        apply_hazard_avoidance(cpu, retreat_x, retreat_y);
        apply_arena_steering(cpu, retreat_x, retreat_y);
        cpu->input_dx = retreat_x;
        cpu->input_dy = retreat_y;

        // Defensive swat: if player rushes recklessly into melee range during retreat, swat with melee!
        if (dist <= 85.0f && cpu->abilities[0].cooldown_timer <= 0.0f && cpu->stamina >= 15.0f) {
            cpu->use_ability(0, player, target_x, target_y);
            attack_cooldown_timer = 0.25f;
        } else if (dist >= 120.0f && dist <= 320.0f && cpu->stamina >= 30.0f && attack_cooldown_timer <= 0.0f) {
            // Active Kite Deterrent: Fire ranged projectile to punish mindless chaser!
            for (int i = 0; i < 4; ++i) {
                MoveId mid = cpu->abilities[i].move_id;
                if (mid == MOVE_FLAMETHROWER || mid == MOVE_FIRE_SPIN || mid == MOVE_ELECTRO_BALL || mid == MOVE_THUNDERBOLT ||
                    mid == MOVE_AURA_SPHERE || mid == MOVE_FLASH_CANNON || mid == MOVE_DRAGON_PULSE ||
                    mid == MOVE_WATER_SHURIKEN || mid == MOVE_HYDRO_PUMP) {
                    if (cpu->abilities[i].cooldown_timer <= 0.0f && cpu->stamina >= (float)cpu->abilities[i].stamina_cost) {
                        cpu->use_ability(i, player, target_x, target_y);
                        attack_cooldown_timer = 0.35f;
                        break;
                    }
                }
            }
        }
        return;
    }

    // 3. Spacing & Tactical Movement
    handle_movement_and_spacing(cpu, player, dt, dist, dx, dy);

    // 4. Attack Selection & Execution
    select_attack(cpu, player, dist, target_x, target_y);
}

