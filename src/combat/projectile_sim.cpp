#include "projectile.hpp"
#include "fighter.hpp"
#include "arena.hpp"
#include <cmath>

ProjectileManager g_projectiles;

void ProjectileManager::init() {
    for (int i = 0; i < MAX_PROJ; ++i) pool[i] = Projectile{};
    for (int i = 0; i < MAX_TEXTS; ++i) texts[i] = FloatingText{};
    for (int i = 0; i < MAX_PARTICLES; ++i) particles[i] = HitParticle{};
    for (int i = 0; i < MAX_SHOCKWAVES; ++i) shockwaves[i] = ShockwaveRing{};
    particle_cursor = 0;
    screen_shake = 0.0f;
    hitstop_timer = 0.0f;
    impact_flash_timer = 0.0f;
    impact_flash_x = 0.0f;
    impact_flash_y = 0.0f;
}

void ProjectileManager::spawn(int owner_id, ProjectileType type, float x, float y, float vx, float vy, int damage, float lifetime, float target_x, float target_y,
                              ElementType element_type, MoveCategory category, int base_power,
                              const PokemonStats* stats, ElementType t1, ElementType t2,
                              MoveId move_id) {
    for (int i = 0; i < MAX_PROJ; ++i) {
        if (!pool[i].active) {
            Projectile& p = pool[i];
            p.active = true;
            p.owner_id = owner_id;
            p.type = type;
            p.move_id = move_id;
            if (p.move_id == MOVE_NONE) {
                switch (type) {
                    case PROJ_THUNDERBOLT:     p.move_id = MOVE_THUNDERBOLT; break;
                    case PROJ_ELECTRO_BALL:    p.move_id = MOVE_ELECTRO_BALL; break;
                    case PROJ_FLAMETHROWER:    p.move_id = MOVE_FLAMETHROWER; break;
                    case PROJ_FIRE_SPIN:       p.move_id = MOVE_FIRE_SPIN; break;
                    case PROJ_MELEE_SWEEP:     p.move_id = MOVE_QUICK_ATTACK; break;
                    case PROJ_CLAW_SCRATCH:    p.move_id = MOVE_SCRATCH; break;
                    case PROJ_WING_ATTACK:     p.move_id = MOVE_WING_ATTACK; break;
                    case PROJ_SCORCHING_SANDS: p.move_id = MOVE_SCORCHING_SANDS; break;
                    case PROJ_DRAGON_CLAW:     p.move_id = MOVE_DRAGON_CLAW; break;
                    case PROJ_VOLT_TACKLE:     p.move_id = MOVE_VOLT_TACKLE; break;
                    case PROJ_DRAGON_RAGE:     p.move_id = MOVE_DRAGON_RAGE; break;
                    case PROJ_EARTHQUAKE:      p.move_id = MOVE_EARTHQUAKE; break;
                    case PROJ_SEISMIC_GRAB:    p.move_id = MOVE_SEISMIC_TOSS; break;
                    case PROJ_AURA_SPHERE:     p.move_id = MOVE_AURA_SPHERE; break;
                    case PROJ_FORCE_PALM:      p.move_id = MOVE_FORCE_PALM; break;
                    case PROJ_CLOSE_COMBAT:    p.move_id = MOVE_CLOSE_COMBAT; break;
                    case PROJ_BONE_RUSH:       p.move_id = MOVE_BONE_RUSH; break;
                    case PROJ_EXTREME_SPEED:   p.move_id = MOVE_EXTREME_SPEED; break;
                    case PROJ_FLASH_CANNON:    p.move_id = MOVE_FLASH_CANNON; break;
                    case PROJ_DRAGON_PULSE:    p.move_id = MOVE_DRAGON_PULSE; break;
                    case PROJ_THUNDER:         p.move_id = MOVE_THUNDER; break;
                    case PROJ_WATER_SHURIKEN:  p.move_id = MOVE_WATER_SHURIKEN; break;
                    case PROJ_NIGHT_SLASH:     p.move_id = MOVE_NIGHT_SLASH; break;
                    case PROJ_HYDRO_PUMP:      p.move_id = MOVE_HYDRO_PUMP; break;
                    case PROJ_AERIAL_ACE:      p.move_id = MOVE_AERIAL_ACE; break;
                    default:                   p.move_id = MOVE_NONE; break;
                }
            }
            p.element_type = element_type;
            p.category = category;
            p.base_power = (base_power > 0) ? base_power : damage;
            if (stats != nullptr) {
                p.owner_stats = *stats;
                p.owner_type1 = t1;
                p.owner_type2 = t2;
            }
            p.x = x;
            p.y = y;
            p.vx = vx;
            p.vy = vy;
            p.damage = damage;
            p.lifetime = lifetime;
            p.max_lifetime = lifetime;
            p.hit_cooldown = 0;
            p.target_x = target_x;
            p.target_y = target_y;

            p.has_struck = false;
            p.can_damage = true;
            p.angle = atan2f(vy, vx);
            p.base_radius = 12.0f;
            p.delay_timer = 0.0f;
            p.max_delay = 0.0f;
            p.extra_timer = 0.0f;
            p.z_min = 0.0f;
            p.z_max = 60.0f;
            p.is_ground_level = false;

            if (type == PROJ_THUNDERBOLT) {
                p.radius = 26.0f; // Continuous high-voltage electric beam hitbox thickness
                p.homing = false;
                p.piercing = true; // Piercing beam with hit_cooldown multi-hit ticking
                p.delay_timer = 0.0f;
                p.max_delay = 0.0f;
                p.has_struck = false;
                p.can_damage = true;
                p.z_min = 0.0f;
                p.z_max = 55.0f; // Covers ground and mid-air
                p.is_ground_level = false;
            } else if (type == PROJ_THUNDER) {
                p.radius = 56.0f; // Runic ground warning seal and sky-nuke column
                p.homing = false;
                p.piercing = false;
                p.delay_timer = 0.32f; // 320ms runic telegraph
                p.max_delay = 0.32f;
                p.has_struck = false;
                p.can_damage = false; // Zero damage during telegraph warning
                p.z_min = 0.0f;
                p.z_max = 320.0f; // Massive vertical sky column crashes onto grounded or airborne targets
                p.is_ground_level = false;
            } else if (type == PROJ_ELECTRO_BALL) {
                p.radius = 18.0f;
                p.homing = true;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 60.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_FLAMETHROWER) {
                p.base_radius = 12.0f;
                p.radius = 12.0f;
                p.homing = false;
                p.piercing = true;
                p.z_min = 0.0f;
                p.z_max = 50.0f; // Hits grounded and mid-air targets
                p.is_ground_level = false;
            } else if (type == PROJ_FIRE_SPIN) {
                float spd = sqrtf(vx * vx + vy * vy);
                if ((target_x != 0.0f || target_y != 0.0f) && spd > 1.0f) {
                    float dx = target_x - x;
                    float dy = target_y - y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    p.delay_timer = (dist > 1.0f) ? (dist / spd) : 0.0f;
                    p.max_delay = p.delay_timer;
                    p.has_struck = false;
                    p.radius = 14.0f; // Small ember seed in flight
                } else {
                    p.delay_timer = 0.0f;
                    p.max_delay = 0.0f;
                    p.has_struck = true;
                    p.radius = 52.0f;
                }
                p.homing = false;
                p.piercing = true;
                p.z_min = 0.0f;
                p.z_max = 18.0f; // Low-profile ground flame vortex
                p.is_ground_level = true;
            } else if (type == PROJ_CLAW_SCRATCH) {
                p.radius = 36.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 18.0f; // Low ground claw swipe
                p.is_ground_level = true;
            } else if (type == PROJ_WING_ATTACK) {
                p.radius = 50.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 70.0f; // Aerodynamic flight wings hit ground & mid-air
                p.is_ground_level = false;
            } else if (type == PROJ_MELEE_SWEEP) {
                p.radius = (damage >= 40) ? 46.0f : 28.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 60.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_SCORCHING_SANDS) {
                p.radius = 34.0f;
                p.homing = false;
                p.piercing = true;
                p.delay_timer = 0.0f; // Instant wave launch scooped from feet
                p.max_delay = 0.0f;
                p.has_struck = true;
                p.can_damage = true;
                p.z_min = 0.0f;
                p.z_max = 22.0f; // Low ground wave - jumpable over 22px
                p.is_ground_level = true;
            } else if (type == PROJ_SEISMIC_GRAB) {
                p.radius = 42.0f;
                p.homing = false;
                p.piercing = false;
                p.delay_timer = 0.0f;
                p.max_delay = 0.0f;
                p.has_struck = true;
                p.can_damage = true;
                p.z_min = 0.0f;
                p.z_max = 28.0f; // Intercepts grounded and low-air targets
                p.is_ground_level = false;
            } else if (type == PROJ_DRAGON_CLAW) {
                p.radius = 40.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 48.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_VOLT_TACKLE) {
                p.radius = 48.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 55.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_DRAGON_RAGE) {
                p.radius = 32.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 55.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_EARTHQUAKE) {
                p.radius = 60.0f;
                p.homing = false;
                p.piercing = true;
                p.delay_timer = 0.30f; // 300ms seismic ground fracture telegraph
                p.max_delay = 0.30f;
                p.has_struck = false;
                p.can_damage = false;  // Harmless during telegraph
                p.z_min = 0.0f;
                p.z_max = 22.0f; // Pure ground-level tectonic shockwave (jumpable over 22px!)
                p.is_ground_level = true;
            } else if (type == PROJ_AURA_SPHERE) {
                p.radius = 18.0f;
                p.homing = true;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 60.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_FORCE_PALM) {
                p.radius = 32.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 45.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_CLOSE_COMBAT) {
                p.radius = 36.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 50.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_BONE_RUSH) {
                p.radius = 34.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 20.0f;
                p.is_ground_level = true;
            } else if (type == PROJ_EXTREME_SPEED) {
                p.radius = 36.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 55.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_FLASH_CANNON) {
                p.radius = 20.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 60.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_DRAGON_PULSE) {
                p.radius = 32.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 55.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_WATER_SHURIKEN) {
                p.radius = 14.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 55.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_NIGHT_SLASH) {
                p.radius = 26.0f;
                p.homing = false;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 48.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_HYDRO_PUMP) {
                p.base_radius = 18.0f;
                p.radius = 18.0f;
                p.homing = false;
                p.piercing = true;
                p.z_min = 0.0f;
                p.z_max = 55.0f;
                p.is_ground_level = false;
            } else if (type == PROJ_AERIAL_ACE) {
                p.radius = 26.0f;
                p.homing = true;
                p.piercing = false;
                p.z_min = 0.0f;
                p.z_max = 60.0f;
                p.is_ground_level = false;
            }
            return;
        }
    }
}

void ProjectileManager::update(float dt, float p1_x, float p1_y, float p2_x, float p2_y) {
    update_effects(dt);

    // 1. Update Projectiles
    for (int i = 0; i < MAX_PROJ; ++i) {
        if (!pool[i].active) continue;
        Projectile& p = pool[i];

        // Telegraph delay processing
        if (p.delay_timer > 0.0f) {
            p.delay_timer -= dt;

            if (p.type == PROJ_FIRE_SPIN) {
                // Moving ember seed in flight toward target
                p.x += p.vx * dt;
                p.y += p.vy * dt;
                if (rand() % 2 == 0) {
                    spawn_particles(p.x, p.y, 1, 255, 160, 40, 70.0f);
                }
                if (p.delay_timer <= 0.0f && !p.has_struck) {
                    p.has_struck = true;
                    p.delay_timer = 0.0f;
                    p.x = p.target_x;
                    p.y = p.target_y;
                    p.vx = 0.0f;
                    p.vy = 0.0f;
                    p.radius = 52.0f; // Expand into full cyclone vortex
                    // Landed explosive fire vortex burst
                    spawn_embers(p.x, p.y, 8.0f, 24, 210.0f);
                    spawn_dust(p.x, p.y, 14, 180, 83, 9, 140.0f, 22.0f);
                }
                continue; // Do not decrement vortex lifetime until it lands!
            } else if (p.type == PROJ_EARTHQUAKE) {
                // Severe seismic rumbling telegraph with expanding tectonic tremors
                if (rand() % 2 == 0) {
                    float a = (float)(rand() % 360) * 0.01745f;
                    float r = (float)(rand() % (int)p.radius);
                    spawn_dust(p.x + cosf(a) * r, p.y + sinf(a) * r, 1, 180, 140, 90, 45.0f, 6.0f);
                }
                if (p.delay_timer <= 0.0f && !p.has_struck) {
                    p.has_struck = true;
                    p.can_damage = true;
                    p.delay_timer = 0.0f;
                    p.extra_timer = 0.0f; // Reset extra_timer to 0 at strike!
                    screen_shake = 0.38f;
                    spawn_shockwave(p.x, p.y, 160.0f, 0.32f, 180, 83, 9, 7);
                    spawn_shockwave(p.x, p.y, 110.0f, 0.24f, 245, 158, 11, 5);
                    spawn_debris(p.x, p.y, 8.0f, 36, 260.0f, 130, 80, 40);
                    spawn_dust(p.x, p.y, 20, 160, 140, 110, 160.0f, 32.0f);
                    spawn_sparks(p.x, p.y, 8.0f, 20, 245, 158, 11, 220.0f);
                    g_arena.add_decal(DECAL_TECTONIC_CRACK, p.x, p.y, 140.0f, (float)(rand() % 360) * 0.01745f);
                    g_arena.add_decal(DECAL_IMPACT_CRATER, p.x, p.y, 50.0f);
                }
                continue;
            } else if (p.type == PROJ_THUNDER) {
                // Thunder celestial ground warning telegraph
                if (rand() % 2 == 0) {
                    float a = (float)(rand() % 360) * 0.01745f;
                    float r = (float)(rand() % (int)p.radius);
                    spawn_electric_sparks(p.x + cosf(a) * r, p.y + sinf(a) * r, 5.0f, 2, 80.0f);
                }
                if (p.delay_timer <= 0.0f && !p.has_struck) {
                    p.has_struck = true;
                    p.can_damage = true; // Damage enabled at strike impact
                    p.delay_timer = 0.0f;
                    p.extra_timer = 0.0f; // Reset extra_timer to 0 at strike!
                    screen_shake = 0.36f; // Severe camera shake
                    spawn_shockwave(p.x, p.y, 140.0f, 0.32f, 56, 189, 248, 7); // Cyan electric shockwave
                    spawn_shockwave(p.x, p.y, 90.0f, 0.22f, 250, 204, 21, 5);  // Solar yellow blast ring
                    // Heavy explosive lightning impact particles
                    spawn_electric_sparks(p.x, p.y, 18.0f, 32, 340.0f);
                    spawn_sparks(p.x, p.y, 12.0f, 24, 255, 255, 255, 300.0f);
                    spawn_dust(p.x, p.y, 14, 180, 200, 220, 140.0f, 22.0f);
                    g_arena.add_decal(DECAL_SCORCH_MARK, p.x, p.y, 48.0f, 0.0f, 30, 25, 20, 240);
                }
                continue; // During telegraph, do not move or expire lifetime
            }
        }

        p.lifetime -= dt;
        if (p.lifetime <= 0) {
            p.active = false;
            continue;
        }
        if (p.hit_cooldown > 0) p.hit_cooldown--;

        p.extra_timer += dt;

        // Instant strike windows:
        // Thunder closes damage window after initial strike impact
        if (p.type == PROJ_THUNDER && p.has_struck) {
            if (p.extra_timer > 0.08f) {
                p.can_damage = false;
            }
        }

        // Thunderbolt beam advance & anchor clamp
        if (p.type == PROJ_THUNDERBOLT) {
            float bdx = p.x - p.target_x;
            float bdy = p.y - p.target_y;
            float blen = sqrtf(bdx * bdx + bdy * bdy);
            const float MAX_BEAM_LEN = 440.0f;

            if (blen >= MAX_BEAM_LEN) {
                if (blen > 0.001f) {
                    p.x = p.target_x + (bdx / blen) * MAX_BEAM_LEN;
                    p.y = p.target_y + (bdy / blen) * MAX_BEAM_LEN;
                }
                p.vx = 0.0f;
                p.vy = 0.0f;
            }

            // Continuous crackling sparks along beam length and spearhead
            if (rand() % 2 == 0) {
                float seg_t = (float)(rand() % 100) / 100.0f;
                float spk_x = p.target_x + bdx * seg_t;
                float spk_y = p.target_y + bdy * seg_t;
                spawn_electric_sparks(spk_x, spk_y, 8.0f, 2, 110.0f);
            }
        }

        // Dynamic expansion and particles for Flamethrower
        if (p.type == PROJ_FLAMETHROWER) {
            float life_prog = (p.max_lifetime > 0.001f) ? (1.0f - p.lifetime / p.max_lifetime) : 1.0f;
            p.radius = p.base_radius + life_prog * 24.0f; // Expands from 12px at mouth to 36px at tip!
            if (rand() % 2 == 0) {
                spawn_embers(p.x + (float)(rand() % 14 - 7), p.y + (float)(rand() % 14 - 7), 6.0f, 1, 75.0f);
            }
            if (life_prog > 0.4f && rand() % 6 == 0) {
                g_arena.add_decal(DECAL_SCORCH_MARK, p.x, p.y, 22.0f, 0.0f, 30, 20, 10, 180);
            }
        } else if (p.type == PROJ_FIRE_SPIN) {
            if (rand() % 2 == 0) {
                float spin_a = p.extra_timer * 7.5f + (float)(rand() % 360) * 0.01745f;
                float r_dist = 12.0f + (float)(rand() % (int)p.radius);
                spawn_embers(p.x + cosf(spin_a) * r_dist, p.y + sinf(spin_a) * r_dist, 5.0f, 1, 75.0f);
            }
        } else if (p.type == PROJ_WING_ATTACK) {
            if (rand() % 2 == 0) {
                spawn_dust(p.x - p.vx * dt * 0.4f, p.y - p.vy * dt * 0.4f, 1, 230, 240, 255, 60.0f, 6.0f);
            }
        } else if (p.type == PROJ_SCORCHING_SANDS) {
            if (rand() % 2 == 0) {
                spawn_debris(p.x + (float)(rand() % 24 - 12), p.y + (float)(rand() % 24 - 12), 4.0f, 1, 90.0f, 210, 140, 60);
                spawn_dust(p.x, p.y, 1, 180, 140, 90, 50.0f, 8.0f);
            }
        } else if (p.type == PROJ_SEISMIC_GRAB) {
            if (rand() % 2 == 0) {
                spawn_sparks(p.x, p.y, 6.0f, 1, 239, 68, 68, 90.0f);
                spawn_sparks(p.x, p.y, 6.0f, 1, 250, 204, 21, 70.0f);
            }
        } else if (p.type == PROJ_EARTHQUAKE && p.has_struck) {
            // Dynamic expanding seismic shockwave ring: expands from 70px to 250px over 0.30s!
            float prog = std::min(1.0f, p.extra_timer / 0.30f);
            p.radius = 70.0f + prog * 180.0f;
            if (p.extra_timer > 0.30f) {
                p.can_damage = false; // Ground shockwave dissipates after 0.30s
            }
            if (rand() % 2 == 0) {
                float a = (float)(rand() % 360) * 0.01745f;
                float dist = (float)(rand() % (int)p.radius);
                spawn_debris(p.x + cosf(a) * dist, p.y + sinf(a) * dist, 4.0f, 1, 85.0f, 146, 64, 14);
            }
        } else if (p.type == PROJ_DRAGON_CLAW) {
            if (rand() % 2 == 0) {
                spawn_sparks(p.x, p.y, 6.0f, 1, 16, 185, 129, 65.0f);
            }
        } else if (p.type == PROJ_DRAGON_RAGE) {
            if (rand() % 2 == 0) {
                spawn_sparks(p.x, p.y, 8.0f, 1, 99, 102, 241, 75.0f);
                spawn_sparks(p.x, p.y, 8.0f, 1, 56, 189, 248, 90.0f);
            }
        } else if (p.type == PROJ_VOLT_TACKLE) {
            spawn_electric_sparks(p.x, p.y, 8.0f, 2, 120.0f);
            if (rand() % 2 == 0) {
                spawn_sparks(p.x, p.y, 6.0f, 1, 250, 204, 21, 130.0f);
            }
        } else if (p.type == PROJ_AURA_SPHERE) {
            if (rand() % 2 == 0) {
                spawn_aura_motes(p.x, p.y, 8.0f, 1, 70.0f, 56, 189, 248);
            }
        } else if (p.type == PROJ_FORCE_PALM) {
            spawn_sparks(p.x, p.y, 8.0f, 1, 255, 255, 255, 100.0f);
        } else if (p.type == PROJ_CLOSE_COMBAT) {
            if (rand() % 2 == 0) {
                spawn_sparks(p.x, p.y, 8.0f, 1, 239, 68, 68, 90.0f);
                spawn_sparks(p.x, p.y, 8.0f, 1, 250, 204, 21, 90.0f);
            }
        } else if (p.type == PROJ_BONE_RUSH) {
            spawn_sparks(p.x, p.y, 4.0f, 1, 56, 189, 248, 85.0f);
            if (rand() % 2 == 0) {
                spawn_dust(p.x, p.y, 1, 160, 120, 80, 55.0f, 6.0f);
            }
        } else if (p.type == PROJ_EXTREME_SPEED) {
            spawn_sparks(p.x, p.y, 6.0f, 2, 255, 255, 255, 140.0f);
            spawn_dust(p.x, p.y, 1, 240, 245, 255, 60.0f, 6.0f);
        } else if (p.type == PROJ_FLASH_CANNON) {
            if (rand() % 2 == 0) {
                spawn_sparks(p.x, p.y, 8.0f, 1, 241, 245, 249, 120.0f);
            }
        } else if (p.type == PROJ_DRAGON_PULSE) {
            spawn_aura_motes(p.x, p.y, 12.0f, 1, 95.0f, 16, 185, 129);
            if (rand() % 2 == 0) {
                spawn_sparks(p.x, p.y, 8.0f, 1, 99, 102, 241, 85.0f);
                spawn_sparks(p.x, p.y, 8.0f, 1, 250, 204, 21, 70.0f);
            }
        } else if (p.type == PROJ_ELECTRO_BALL) {
            spawn_electric_sparks(p.x, p.y, 6.0f, 2, 110.0f);
            if (rand() % 2 == 0) {
                spawn_aura_motes(p.x, p.y, 6.0f, 1, 60.0f, 250, 204, 21);
            }
        } else if (p.type == PROJ_MELEE_SWEEP) {
            if (p.move_id == MOVE_QUICK_ATTACK && rand() % 2 == 0) {
                spawn_sparks(p.x, p.y, 4.0f, 1, 255, 255, 255, 80.0f);
            }
        } else if (p.type == PROJ_WATER_SHURIKEN) {
            p.angle += 45.0f * dt;
            if (rand() % 2 == 0) {
                spawn_particles(p.x, p.y, 1, 56, 189, 248, 90.0f);
                spawn_particles(p.x, p.y, 1, 224, 242, 254, 70.0f);
            }
            if (rand() % 4 == 0) {
                spawn_shockwave(p.x, p.y, 16.0f, 0.12f, 56, 189, 248, 1);
                spawn_aura_motes(p.x, p.y, 5.0f, 1, 65.0f, 186, 230, 253);
            }
        } else if (p.type == PROJ_NIGHT_SLASH) {
            spawn_sparks(p.x, p.y, 8.0f, 2, 217, 70, 239, 110.0f);
            spawn_dust(p.x, p.y, 1, 15, 23, 42, 50.0f, 5.0f);
            if (rand() % 3 == 0) {
                spawn_aura_motes(p.x, p.y, 6.0f, 1, 70.0f, 168, 85, 247);
            }
        } else if (p.type == PROJ_HYDRO_PUMP) {
            float life_prog = (p.max_lifetime > 0.001f) ? (1.0f - p.lifetime / p.max_lifetime) : 1.0f;
            p.radius = p.base_radius + life_prog * 16.0f;
            spawn_aura_motes(p.x + (float)(rand() % 20 - 10), p.y + (float)(rand() % 20 - 10), 10.0f, 2, 120.0f, 56, 189, 248);
            spawn_particles(p.x, p.y, 2, 224, 242, 254, 110.0f);
            if (rand() % 4 == 0) {
                spawn_shockwave(p.x, p.y, p.radius * 0.9f, 0.12f, 56, 189, 248, 2);
            }
        } else if (p.type == PROJ_AERIAL_ACE) {
            spawn_sparks(p.x, p.y, 8.0f, 2, 255, 255, 255, 140.0f);
            spawn_dust(p.x, p.y, 1, 240, 245, 255, 75.0f, 6.0f);
            if (rand() % 4 == 0) {
                spawn_shockwave(p.x, p.y, 24.0f, 0.10f, 255, 255, 255, 2);
            }
        }

        if (p.homing) {
            float tx = (p.owner_id == 0) ? p2_x : p1_x;
            float ty = (p.owner_id == 0) ? p2_y : p1_y;
            float dx = tx - p.x;
            float dy = ty - p.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > 10.0f) {
                float steer_spd = 1350.0f * dt;
                p.vx += (dx / dist) * steer_spd;
                p.vy += (dy / dist) * steer_spd;
                float cur_spd = sqrtf(p.vx * p.vx + p.vy * p.vy);
                float max_spd = 850.0f;
                if (cur_spd > max_spd) {
                    p.vx = (p.vx / cur_spd) * max_spd;
                    p.vy = (p.vy / cur_spd) * max_spd;
                }
            }
            // Emit trailing electric spark
            if (rand() % 2 == 0) {
                spawn_particles(p.x, p.y, 1, 100, 240, 255, 60.0f);
            }
        }

        p.x += p.vx * dt;
        p.y += p.vy * dt;

        // Arena boundary collision
        if (p.x < ARENA_MIN_X || p.x > ARENA_MAX_X || p.y < ARENA_MIN_Y || p.y > ARENA_MAX_Y) {
            if (p.type == PROJ_THUNDERBOLT) {
                p.x = std::max((float)ARENA_MIN_X, std::min((float)ARENA_MAX_X, p.x));
                p.y = std::max((float)ARENA_MIN_Y, std::min((float)ARENA_MAX_Y, p.y));
                p.vx = 0.0f;
                p.vy = 0.0f;
            } else {
                p.active = false;
                spawn_particles(p.x, p.y, 8, 255, 230, 80, 100.0f);
            }
        }
    }
}

