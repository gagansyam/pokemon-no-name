#include "fighter.hpp"
#include "arena.hpp"
#include "projectile.hpp"
#include "moves.hpp"
#include "combat_log.hpp"
#include <cmath>
#include <cstdlib>

void Fighter::buffer_ability(int slot, float target_x, float target_y) {
    if (slot < 0 || slot >= 4) return;
    input_buffer.type = BUFFER_MOVE;
    input_buffer.slot = slot;
    input_buffer.target_x = target_x;
    input_buffer.target_y = target_y;
    input_buffer.ttl = 0.12f; // ~7 frames at 60 FPS
}

void Fighter::buffer_dash(float dir_x, float dir_y) {
    input_buffer.type = BUFFER_DASH;
    input_buffer.dir_x = dir_x;
    input_buffer.dir_y = dir_y;
    input_buffer.ttl = 0.12f;
}

void Fighter::process_buffered_action(Fighter* opponent) {
    if (input_buffer.type == BUFFER_NONE || input_buffer.ttl <= 0.0f) return;
    if (!can_act()) return;

    BufferedAction act = input_buffer;
    input_buffer = BufferedAction{}; // Clear buffer before firing

    if (act.type == BUFFER_MOVE) {
        use_ability(act.slot, opponent, act.target_x, act.target_y);
    } else if (act.type == BUFFER_DASH) {
        try_dash(act.dir_x, act.dir_y);
    }
}

void Fighter::try_dash(float dir_x, float dir_y) {
    if (is_grabbed || is_bench) return;
    if (hitstun_timer > 0 || dash_timer > 0 || dash_cooldown_timer > 0) {
        buffer_dash(dir_x, dir_y);
        return;
    }
    if (attack_timer > 0.0f && hit_cancel_timer <= 0.0f) {
        buffer_dash(dir_x, dir_y);
        return;
    }
    if (stamina < DASH_STAMINA_COST) {
        stamina_flash_timer = 0.35f;
        return;
    }

    // Cancel attack recovery if in hit-cancel window
    if (hit_cancel_timer > 0.0f) {
        hit_cancel_timer = 0.0f;
        attack_timer = 0.0f;
        active_move = MOVE_NONE;
        g_projectiles.spawn_particles(x, y, 10, 255, 255, 255, 140.0f);
    }

    if (dir_x == 0 && dir_y == 0) {
        dir_x = cosf(aim_angle);
        dir_y = sinf(aim_angle);
    }
    float len = sqrtf(dir_x * dir_x + dir_y * dir_y);
    if (len == 0) return;

    stamina -= DASH_STAMINA_COST;
    state = STATE_DASH;
    dash_timer = DASH_DURATION;
    invuln_timer = DASH_DURATION;
    ghost_timer = 0.0f;

    float dash_spd = move_speed * 3.2f;
    if (blaze_active) dash_spd *= 1.25f; // Blaze speed boost
    vx = (dir_x / len) * dash_spd;
    vy = (dir_y / len) * dash_spd;

    // High-impact burst dust & ignition sparks
    g_projectiles.spawn_dust(x, y + radius * 0.4f, 14, 220, 220, 205, 120.0f, 14.0f);
    g_projectiles.spawn_sparks(x, y + radius * 0.4f, 2.0f, 6, 255, 255, 255, 150.0f);
}

void Fighter::use_ability(int slot, Fighter* opponent, float target_x, float target_y) {
    if (slot < 0 || slot >= 4 || is_bench) return;
    if (!can_act()) {
        buffer_ability(slot, target_x, target_y);
        return;
    }

    // Hit-cancel combo starter: instant cancel of attack recovery
    if (hit_cancel_timer > 0.0f) {
        hit_cancel_timer = 0.0f;
        attack_timer = 0.0f;
        active_move = MOVE_NONE;
        g_projectiles.spawn_particles(x, y, 12, 255, 255, 255, 150.0f);
        g_projectiles.spawn_shockwave(x, y, 65.0f, 0.20f, 255, 255, 255, 4);
    }

    MoveRegistry::execute(abilities[slot].move_id, this, opponent, target_x, target_y);
}

void Fighter::update(float dt, Fighter* opponent) {
    if (is_bench) return; // Bench fighters do not process physics/movement

    // 0. Smoothly drain lagging HP bar towards actual HP
    if (display_lag_hp > (float)hp) {
        display_lag_hp -= dt * 75.0f;
        if (display_lag_hp < (float)hp) display_lag_hp = (float)hp;
    } else if (display_lag_hp < (float)hp) {
        display_lag_hp = (float)hp;
    }

    // 0b. Status Ailment DoT Clocks
    if (status == STATUS_POISON) {
        status_tick_timer += dt;
        if (status_tick_timer >= 3.0f) {
            status_tick_timer = 0.0f;
            int dot = (int)ceilf((float)max_hp * 0.04f);
            if (dot < 1) dot = 1;
            hp -= dot;
            if (hp < 0) hp = 0;
            g_projectiles.spawn_custom_text(x, y - 26, "PSN -4%", 168, 85, 247, 45);
            g_projectiles.spawn_particles(x, y, 6, 168, 85, 247, 50.0f);
        }
    } else if (status == STATUS_BURN) {
        status_tick_timer += dt;
        if (status_tick_timer >= 3.0f) {
            status_tick_timer = 0.0f;
            int dot = (int)ceilf((float)max_hp * 0.03f);
            if (dot < 1) dot = 1;
            hp -= dot;
            if (hp < 0) hp = 0;
            g_projectiles.spawn_custom_text(x, y - 26, "BRN -3%", 249, 115, 22, 45);
            g_projectiles.spawn_particles(x, y, 6, 249, 115, 22, 60.0f);
        }
    }

    // 0c. Automatic Blaze Mode Activation (Charizard below 30% HP)
    if (type == PKMN_CHARIZARD && !blaze_active && hp > 0 && hp <= (int)((float)max_hp * 0.30f)) {
        trigger_blaze();
    }

    // 0d. Toxic Spikes Hazard Ground Collision
    HazardType hit_hazard = HAZARD_NONE;
    if (g_arena.check_hazard_collision(x, y, z, team_id, hit_hazard)) {
        if (hit_hazard == HAZARD_TOXIC_SPIKES) {
            if (type1 == TYPE_POISON || type2 == TYPE_POISON) {
                // Grounded Poison types absorb the spikes!
                g_arena.clear_hazards(team_id);
                g_projectiles.spawn_custom_text(x, y - 28, "ABSORBED SPIKES!", 168, 85, 247, 45);
            } else if (type1 != TYPE_STEEL && type2 != TYPE_STEEL && type1 != TYPE_FLYING && type2 != TYPE_FLYING) {
                apply_status(STATUS_POISON);
            }
        }
    }

    // 0e. Tag Recall & Entry State Transitions
    if (state == STATE_TAG_RECALL) {
        tag_timer -= dt;
        vx = 0; vy = 0;
        if (tag_timer <= 0.0f) {
            is_bench = true;
            state = STATE_IDLE;
        }
        return;
    } else if (state == STATE_TAG_ENTRY) {
        tag_timer -= dt;
        vx = 0; vy = 0;
        if (tag_timer <= 0.0f) {
            state = STATE_IDLE;
        }
        return;
    }

    // Input buffer decay
    if (input_buffer.ttl > 0.0f) {
        input_buffer.ttl -= dt;
        if (input_buffer.ttl <= 0.0f) {
            input_buffer.type = BUFFER_NONE;
        }
    }

    // Hit-cancel window decay
    if (hit_cancel_timer > 0.0f) {
        hit_cancel_timer -= dt;
        if (hit_cancel_timer < 0.0f) hit_cancel_timer = 0.0f;
    }

    // Check buffered action trigger
    if (can_act() && input_buffer.type != BUFFER_NONE && opponent != nullptr) {
        process_buffered_action(opponent);
    }

    // Cooldown ticks
    for (int i = 0; i < 4; ++i) {
        if (abilities[i].cooldown_timer > 0.0f) {
            abilities[i].cooldown_timer -= dt;
            if (abilities[i].cooldown_timer < 0.0f) abilities[i].cooldown_timer = 0.0f;
        }
    }

    // Stamina regeneration
    float regen_rate = (type == PKMN_CHARIZARD) ? 26.0f : ((type == PKMN_GRENINJA) ? 20.0f : 22.0f);
    if (stamina < max_stamina) {
        stamina += regen_rate * dt;
        if (stamina > max_stamina) stamina = max_stamina;
    }

    // Primed stance timer decay
    if (primed_move != MOVE_NONE) {
        primed_timer -= dt;
        if (primed_timer <= 0.0f) {
            primed_move = MOVE_NONE;
            primed_timer = 0.0f;
            iron_tail_primed = false;
            iron_tail_timer = 0.0f;
        }
    }

    if (stamina_flash_timer > 0.0f) {
        stamina_flash_timer -= dt;
        if (stamina_flash_timer < 0.0f) stamina_flash_timer = 0.0f;
    }

    // Agility speedster burst timer decay
    if (agility_timer > 0.0f) {
        agility_timer -= dt;
        if (agility_timer < 0.0f) agility_timer = 0.0f;
    }

    // Double Team optical clone updates & orbital drift
    if (double_team_timer > 0.0f) {
        double_team_timer -= dt;
        if (double_team_timer <= 0.0f) {
            double_team_timer = 0.0f;
            clear_clones();
        } else {
            clone_orbit_angle += 1.8f * dt;
            for (int i = 0; i < MAX_CLONES; ++i) {
                if (!clones[i].active) continue;
                float cur_ang = clones[i].scatter_angle + clone_orbit_angle;
                float cur_dist = clones[i].scatter_dist;
                clones[i].x = x + cosf(cur_ang) * cur_dist;
                clones[i].y = y + sinf(cur_ang) * cur_dist;
                g_arena.clamp(clones[i].x, clones[i].y, radius);
                clones[i].pmd_dir = pmd_dir;
                clones[i].anim_frame = anim_frame;
                clones[i].alpha = 150.0f + sinf(anim_timer * 12.0f + (float)i) * 35.0f;
            }
        }
    }

    // Update dash afterimage ghosts
    for (int i = 0; i < 8; ++i) {
        if (dash_ghosts[i].alpha > 0.0f) {
            dash_ghosts[i].alpha -= dt * 480.0f;
            if (dash_ghosts[i].alpha < 0.0f) dash_ghosts[i].alpha = 0.0f;
        }
    }

    if (invuln_timer > 0.0f) invuln_timer -= dt;
    if (dash_cooldown_timer > 0.0f) {
        dash_cooldown_timer -= dt;
        if (dash_cooldown_timer < 0.0f) dash_cooldown_timer = 0.0f;
    }

    if (hitstun_timer > 0.0f) {
        hitstun_timer -= dt;
        vx *= 0.90f;
        vy *= 0.90f;
        if (hitstun_timer <= 0.0f) {
            state = STATE_IDLE;
            vx = 0; vy = 0;
        }
    } else if (dash_timer > 0.0f) {
        dash_timer -= dt;
        float dash_prog = 1.0f - (dash_timer / DASH_DURATION);
        if (dash_prog < 0.0f) dash_prog = 0.0f;
        if (dash_prog > 1.0f) dash_prog = 1.0f;
        z = 22.0f * sinf(dash_prog * 3.14159f);

        ghost_timer += dt;
        if (ghost_timer >= 0.045f) {
            ghost_timer = 0.0f;
            for (int i = 0; i < 8; ++i) {
                if (dash_ghosts[i].alpha <= 0.0f) {
                    dash_ghosts[i].x = x;
                    dash_ghosts[i].y = y;
                    dash_ghosts[i].pmd_dir = pmd_dir;
                    dash_ghosts[i].anim_frame = anim_frame;
                    dash_ghosts[i].alpha = 180.0f;
                    dash_ghosts[i].r = 255;
                    dash_ghosts[i].g = 255;
                    dash_ghosts[i].b = 255;
                    break;
                }
            }
        }
        g_projectiles.spawn_dust(x, y + radius * 0.4f, 2, 230, 230, 220, 45.0f, 6.0f);

        if (dash_timer <= 0.0f) {
            dash_timer = 0.0f;
            z = 0.0f;
            state = STATE_IDLE;
            vx = 0; vy = 0;
            float cd = dash_cooldown_max;
            if (blaze_active) cd *= 0.65f;
            if (agility_timer > 0.0f) cd = 0.18f;
            dash_cooldown_timer = cd;
        }
    } else if (attack_timer > 0.0f) {
        MoveRegistry::update_active_move(this, dt, opponent);
        attack_timer -= dt;

        // Anime Motion Blur Ghost Trails during rush moves
        bool is_rush_move = (active_move == MOVE_VOLT_TACKLE || active_move == MOVE_EXTREME_SPEED ||
                             active_move == MOVE_CLOSE_COMBAT || active_move == MOVE_QUICK_ATTACK ||
                             active_move == MOVE_BONE_RUSH || active_move == MOVE_NIGHT_SLASH ||
                             active_move == MOVE_AERIAL_ACE || wing_attack_rush > 0.0f ||
                             agility_timer > 0.0f);
        if (is_rush_move) {
            ghost_timer += dt;
            if (ghost_timer >= 0.034f) {
                ghost_timer = 0.0f;
                for (int i = 0; i < 8; ++i) {
                    if (dash_ghosts[i].alpha <= 0.0f) {
                        dash_ghosts[i].x = x;
                        dash_ghosts[i].y = y;
                        dash_ghosts[i].pmd_dir = pmd_dir;
                        dash_ghosts[i].anim_frame = anim_frame;
                        dash_ghosts[i].alpha = 210.0f;
                        if (active_move == MOVE_NIGHT_SLASH) {
                            dash_ghosts[i].r = 147;
                            dash_ghosts[i].g = 51;
                            dash_ghosts[i].b = 234;
                        } else if (active_move == MOVE_AERIAL_ACE) {
                            dash_ghosts[i].r = 219;
                            dash_ghosts[i].g = 234;
                            dash_ghosts[i].b = 254;
                        } else {
                            dash_ghosts[i].r = 255;
                            dash_ghosts[i].g = 255;
                            dash_ghosts[i].b = 255;
                        }
                        break;
                    }
                }
            }
        }

        if (attack_timer <= 0.0f) {
            attack_timer = 0.0f;
            state = STATE_IDLE;
            active_move = MOVE_NONE;
            move_phase = 0;
            move_phase_timer = 0.0f;
            attack_duration = 0.0f;
            vx = 0.0f;
            vy = 0.0f;
            z = 0.0f;
            is_flying = false;
            iron_tail_jump_z = 0.0f;
            iron_tail_striking = false;
            wing_attack_windup = 0.0f;
            wing_attack_rush = 0.0f;
        }
    } else {
        // Standard Walking Velocity Integration
        if (input_dx != 0.0f || input_dy != 0.0f) {
            state = STATE_MOVE;
            float eff_move_speed = move_speed;
            eff_move_speed *= TypeChart::get_stage_multiplier(stage_spd);
            if (status == STATUS_PARALYSIS) eff_move_speed *= 0.60f;
            if (blaze_active) eff_move_speed *= 1.20f;
            if (agility_timer > 0.0f) eff_move_speed *= 1.40f;

            float len = sqrtf(input_dx * input_dx + input_dy * input_dy);
            float target_vx = (input_dx / len) * eff_move_speed;
            float target_vy = (input_dy / len) * eff_move_speed;

            float cur_spd = sqrtf(vx * vx + vy * vy);
            if (cur_spd < 1.0f) {
                vx = target_vx;
                vy = target_vy;
            } else {
                float dot = (vx * target_vx + vy * target_vy) / (cur_spd * eff_move_speed);
                if (dot >= 0.92f) {
                    vx = target_vx;
                    vy = target_vy;
                } else {
                    float turn_rate = turn_traction * dt;
                    if (turn_rate > 1.0f) turn_rate = 1.0f;
                    vx += (target_vx - vx) * turn_rate;
                    vy += (target_vy - vy) * turn_rate;
                }
            }

            // Agility high-speed running ghost trails & wind particles
            if (agility_timer > 0.0f) {
                ghost_timer += dt;
                if (ghost_timer >= 0.040f) {
                    ghost_timer = 0.0f;
                    for (int i = 0; i < 8; ++i) {
                        if (dash_ghosts[i].alpha <= 0.0f) {
                            dash_ghosts[i].x = x;
                            dash_ghosts[i].y = y;
                            dash_ghosts[i].pmd_dir = pmd_dir;
                            dash_ghosts[i].anim_frame = anim_frame;
                            dash_ghosts[i].alpha = 190.0f;
                            break;
                        }
                    }
                }
                if (rand() % 4 == 0) {
                    g_projectiles.spawn_particles(x, y + radius * 0.3f, 1, 56, 189, 248, 50.0f);
                }
            }
        } else {
            state = STATE_IDLE;
            float stop_rate = 32.0f * dt;
            if (stop_rate > 1.0f) stop_rate = 1.0f;
            vx += (0.0f - vx) * stop_rate;
            vy += (0.0f - vy) * stop_rate;
            if (fabsf(vx) < 5.0f) vx = 0.0f;
            if (fabsf(vy) < 5.0f) vy = 0.0f;
        }
    }

    // Aim angle update
    if (!is_flying && active_move != MOVE_IRON_TAIL && !iron_tail_striking) {
        pmd_dir = angle_to_pmd_dir(aim_angle);
    }

    // Animation frame update
    anim_timer += dt;
    int walk_frames = (walk_sheet.num_frames > 0) ? walk_sheet.num_frames : 4;
    int attack_frames = (attack_sheet.num_frames > 0) ? attack_sheet.num_frames : 6;
    if ((active_move == MOVE_WATER_SHURIKEN || active_move == MOVE_HYDRO_PUMP) && shoot_sheet.num_frames > 0) {
        attack_frames = shoot_sheet.num_frames;
    }
    if (state == STATE_DASH) {
        anim_frame = ((int)(anim_timer * 18.0f)) % walk_frames;
    } else if (state == STATE_MOVE) {
        anim_frame = ((int)(anim_timer * 9.0f)) % walk_frames;
    } else if (state == STATE_ATTACK) {
        if (attack_duration > 0.001f) {
            float prog = 1.0f - (attack_timer / attack_duration);
            if (prog < 0.0f) prog = 0.0f;
            if (prog > 1.0f) prog = 1.0f;
            anim_frame = std::min(attack_frames - 1, (int)(prog * attack_frames));
        } else {
            anim_frame = ((int)(anim_timer * 14.0f)) % attack_frames;
        }
    } else {
        anim_frame = 0;
    }

    // Position integration and arena boundaries
    x += vx * dt;
    y += vy * dt;
    g_arena.clamp(x, y, radius);

    if (can_act() && input_buffer.type != BUFFER_NONE && opponent != nullptr) {
        process_buffered_action(opponent);
    }
}
