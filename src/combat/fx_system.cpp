#include "projectile.hpp"
#include <cmath>
#include <cstdlib>
#include <cstring>

void ProjectileManager::trigger_hitstop(float duration, float x, float y) {
    hitstop_timer = duration;
    impact_flash_timer = duration + 0.045f;
    impact_flash_x = x;
    impact_flash_y = y;
    screen_shake = 0.28f;
}

void ProjectileManager::spawn_damage_number(float x, float y, int damage, bool is_crit, float effectiveness) {
    for (int i = 0; i < MAX_TEXTS; ++i) {
        if (!texts[i].active) {
            texts[i].active = true;
            texts[i].x = x + (float)(rand() % 20 - 10);
            texts[i].y = y - 20.0f;
            texts[i].vy = -3.5f;
            texts[i].damage = damage;
            texts[i].is_crit = is_crit;
            texts[i].effectiveness = effectiveness;
            texts[i].label[0] = '\0';
            texts[i].alpha = 255;
            texts[i].lifetime = 50;
            return;
        }
    }
}

void ProjectileManager::spawn_custom_text(float x, float y, const char* label, Uint8 r, Uint8 g, Uint8 b, int lifetime) {
    if (!label) return;
    for (int i = 0; i < MAX_TEXTS; ++i) {
        if (!texts[i].active) {
            texts[i].active = true;
            texts[i].x = x;
            texts[i].y = y;
            texts[i].vy = -2.6f;
            texts[i].damage = 0;
            texts[i].is_crit = false;
            texts[i].effectiveness = 1.0f;
            strncpy(texts[i].label, label, sizeof(texts[i].label) - 1);
            texts[i].label[sizeof(texts[i].label) - 1] = '\0';
            texts[i].custom_r = r;
            texts[i].custom_g = g;
            texts[i].custom_b = b;
            texts[i].alpha = 255;
            texts[i].lifetime = lifetime;
            return;
        }
    }
}


static inline HitParticle& alloc_particle(ProjectileManager* pm) {
    int idx = pm->particle_cursor;
    pm->particle_cursor = (pm->particle_cursor + 1) % MAX_PARTICLES;
    HitParticle& p = pm->particles[idx];
    p = HitParticle{};
    p.active = true;
    return p;
}

void ProjectileManager::spawn_particles(float x, float y, int count, Uint8 r, Uint8 g, Uint8 b, float speed) {
    spawn_sparks(x, y, 0.0f, count, r, g, b, speed);
}

void ProjectileManager::spawn_sparks(float x, float y, float z, int count, Uint8 r, Uint8 g, Uint8 b, float speed, float dir_angle, float spread_angle) {
    for (int k = 0; k < count; ++k) {
        HitParticle& p = alloc_particle(this);
        p.style = PSTYLE_SPARK;
        p.x = x;
        p.y = y;
        p.z = (z > 0.0f) ? z : 2.0f;
        float angle;
        if (spread_angle >= 6.20f) {
            angle = (float)(rand() % 360) * 0.017453f;
        } else {
            angle = dir_angle + ((float)(rand() % 1000) / 1000.0f - 0.5f) * spread_angle;
        }
        float spd = speed * (0.6f + (float)(rand() % 80) / 100.0f);
        p.vx = cosf(angle) * spd;
        p.vy = sinf(angle) * spd;
        p.vz = 80.0f + (float)(rand() % 220); // Upward ejection arc
        p.gravity = 520.0f;
        p.bounce = 0.45f;
        p.ground_friction = 0.55f;
        p.drag = 0.94f;
        p.size_start = 3.5f + (float)(rand() % 3);
        p.size_end = 1.0f;
        p.r_start = 255; p.g_start = 255; p.b_start = 255; // White-hot flash
        p.r_end = r; p.g_end = g; p.b_end = b;
        p.a_start = 255; p.a_end = 0;
        p.r = r; p.g = g; p.b = b;
        p.size = p.size_start;
        p.max_life_sec = 0.22f + (float)(rand() % 16) / 100.0f;
        p.lifetime = (int)(p.max_life_sec * 60.0f);
        p.max_lifetime = p.lifetime;
        p.additive = true;
    }
}

void ProjectileManager::spawn_dust(float x, float y, int count, Uint8 r, Uint8 g, Uint8 b, float speed, float radius) {
    for (int k = 0; k < count; ++k) {
        HitParticle& p = alloc_particle(this);
        p.style = PSTYLE_SMOKE_DUST;
        float a = (float)(rand() % 360) * 0.017453f;
        float dist = ((float)(rand() % 100) / 100.0f) * radius;
        p.x = x + cosf(a) * dist;
        p.y = y + sinf(a) * dist;
        p.z = 0.0f;
        float spd = speed * (0.3f + (float)(rand() % 70) / 100.0f);
        p.vx = cosf(a) * spd;
        p.vy = sinf(a) * spd;
        p.vz = 15.0f + (float)(rand() % 35);
        p.gravity = -18.0f; // Soft upward buoyant drift
        p.bounce = 0.0f;
        p.ground_friction = 0.85f;
        p.drag = 0.82f; // High air deceleration
        p.size_start = 4.0f + (float)(rand() % 3);
        p.size_end = 12.0f + (float)(rand() % 6); // Expanding smoke puff
        p.r_start = r; p.g_start = g; p.b_start = b;
        p.r_end = (Uint8)(r * 0.75f); p.g_end = (Uint8)(g * 0.75f); p.b_end = (Uint8)(b * 0.75f);
        p.a_start = 180; p.a_end = 0;
        p.r = r; p.g = g; p.b = b;
        p.size = p.size_start;
        p.max_life_sec = 0.32f + (float)(rand() % 20) / 100.0f;
        p.lifetime = (int)(p.max_life_sec * 60.0f);
        p.max_lifetime = p.lifetime;
        p.additive = false;
    }
}

void ProjectileManager::spawn_embers(float x, float y, float z, int count, float speed) {
    for (int k = 0; k < count; ++k) {
        HitParticle& p = alloc_particle(this);
        p.style = PSTYLE_EMBER;
        p.x = x;
        p.y = y;
        p.z = (z > 0.0f) ? z : 4.0f;
        float a = (float)(rand() % 360) * 0.017453f;
        float spd = speed * (0.4f + (float)(rand() % 60) / 100.0f);
        p.vx = cosf(a) * spd;
        p.vy = sinf(a) * spd;
        p.vz = 40.0f + (float)(rand() % 70); // Updraft
        p.gravity = -45.0f; // Floats upward
        p.drag = 0.90f;
        p.turbulence_amp = 35.0f;
        p.turbulence_freq = 14.0f;
        p.seed = (float)(rand() % 100);
        p.size_start = 3.5f + (float)(rand() % 3);
        p.size_end = 1.0f;
        p.r_start = 255; p.g_start = 240; p.b_start = 150; // White-gold core
        p.r_end = 220; p.g_end = 38; p.b_end = 38;         // Crimson cinder
        p.a_start = 240; p.a_end = 0;
        p.r = 249; p.g = 115; p.b = 22;
        p.size = p.size_start;
        p.max_life_sec = 0.36f + (float)(rand() % 24) / 100.0f;
        p.lifetime = (int)(p.max_life_sec * 60.0f);
        p.max_lifetime = p.lifetime;
        p.additive = true;
    }
}

void ProjectileManager::spawn_electric_sparks(float x, float y, float z, int count, float speed) {
    for (int k = 0; k < count; ++k) {
        HitParticle& p = alloc_particle(this);
        p.style = PSTYLE_ELECTRIC;
        p.x = x;
        p.y = y;
        p.z = (z > 0.0f) ? z : 6.0f;
        float a = (float)(rand() % 360) * 0.017453f;
        float spd = speed * (0.7f + (float)(rand() % 80) / 100.0f);
        p.vx = cosf(a) * spd;
        p.vy = sinf(a) * spd;
        p.vz = -40.0f + (float)(rand() % 180);
        p.gravity = 260.0f;
        p.bounce = 0.60f;
        p.ground_friction = 0.60f;
        p.drag = 0.88f;
        p.turbulence_amp = 95.0f;
        p.turbulence_freq = 34.0f; // High frequency electric snap
        p.seed = (float)(rand() % 100);
        p.size_start = 3.5f;
        p.size_end = 1.0f;
        p.r_start = 255; p.g_start = 255; p.b_start = 255;
        p.r_end = 56; p.g_end = 189; p.b_end = 248;
        p.a_start = 255; p.a_end = 0;
        p.r = 250; p.g = 204; p.b = 21;
        p.size = p.size_start;
        p.max_life_sec = 0.16f + (float)(rand() % 14) / 100.0f;
        p.lifetime = (int)(p.max_life_sec * 60.0f);
        p.max_lifetime = p.lifetime;
        p.additive = true;
    }
}

void ProjectileManager::spawn_debris(float x, float y, float z, int count, float speed, Uint8 r, Uint8 g, Uint8 b) {
    for (int k = 0; k < count; ++k) {
        HitParticle& p = alloc_particle(this);
        p.style = PSTYLE_DEBRIS;
        p.x = x;
        p.y = y;
        p.z = (z > 0.0f) ? z : 4.0f;
        float a = (float)(rand() % 360) * 0.017453f;
        float spd = speed * (0.5f + (float)(rand() % 70) / 100.0f);
        p.vx = cosf(a) * spd;
        p.vy = sinf(a) * spd;
        p.vz = 150.0f + (float)(rand() % 280); // Strong upward eruption
        p.gravity = 880.0f; // Heavy downward gravity
        p.bounce = 0.42f; // Bounces off turf floor
        p.ground_friction = 0.50f;
        p.drag = 0.95f;
        p.rotation = (float)(rand() % 360) * 0.017453f;
        p.v_rot = -16.0f + (float)(rand() % 320) / 10.0f;
        p.size_start = 5.0f + (float)(rand() % 5);
        p.size_end = p.size_start;
        p.r_start = r; p.g_start = g; p.b_start = b;
        p.r_end = (Uint8)(r * 0.8f); p.g_end = (Uint8)(g * 0.8f); p.b_end = (Uint8)(b * 0.8f);
        p.a_start = 255; p.a_end = 0;
        p.r = r; p.g = g; p.b = b;
        p.size = p.size_start;
        p.max_life_sec = 0.48f + (float)(rand() % 24) / 100.0f;
        p.lifetime = (int)(p.max_life_sec * 60.0f);
        p.max_lifetime = p.lifetime;
        p.additive = false;
    }
}

void ProjectileManager::spawn_aura_motes(float x, float y, float z, int count, float speed, Uint8 r, Uint8 g, Uint8 b) {
    for (int k = 0; k < count; ++k) {
        HitParticle& p = alloc_particle(this);
        p.style = PSTYLE_AURA_MOTE;
        p.x = x;
        p.y = y;
        p.z = (z > 0.0f) ? z : 6.0f;
        float a = (float)(rand() % 360) * 0.017453f;
        float spd = speed * (0.4f + (float)(rand() % 60) / 100.0f);
        p.vx = cosf(a) * spd;
        p.vy = sinf(a) * spd;
        p.vz = 20.0f + (float)(rand() % 45);
        p.gravity = 40.0f;
        p.drag = 0.88f;
        p.turbulence_amp = 25.0f;
        p.turbulence_freq = 10.0f;
        p.seed = (float)(rand() % 100);
        p.size_start = 4.0f + (float)(rand() % 4);
        p.size_end = 1.5f;
        p.r_start = 255; p.g_start = 255; p.b_start = 255;
        p.r_end = r; p.g_end = g; p.b_end = b;
        p.a_start = 220; p.a_end = 0;
        p.r = r; p.g = g; p.b = b;
        p.size = p.size_start;
        p.max_life_sec = 0.36f + (float)(rand() % 20) / 100.0f;
        p.lifetime = (int)(p.max_life_sec * 60.0f);
        p.max_lifetime = p.lifetime;
        p.additive = true;
    }
}

void ProjectileManager::spawn_shockwave(float x, float y, float max_radius, float lifetime, Uint8 r, Uint8 g, Uint8 b, int thickness) {
    for (int i = 0; i < MAX_SHOCKWAVES; ++i) {
        if (!shockwaves[i].active) {
            ShockwaveRing& sw = shockwaves[i];
            sw.active = true;
            sw.x = x;
            sw.y = y;
            sw.current_radius = 4.0f;
            sw.max_radius = max_radius;
            sw.lifetime = lifetime;
            sw.max_lifetime = lifetime;
            sw.thickness = thickness;
            sw.r = r;
            sw.g = g;
            sw.b = b;
            break;
        }
    }
}

void ProjectileManager::update_effects(float dt) {
    if (screen_shake > 0.0f) {
        screen_shake -= dt;
        if (screen_shake < 0.0f) screen_shake = 0.0f;
    }
    if (impact_flash_timer > 0.0f) {
        impact_flash_timer -= dt;
        if (impact_flash_timer < 0.0f) impact_flash_timer = 0.0f;
    }

    // 2. Update Floating Damage Texts
    for (int i = 0; i < MAX_TEXTS; ++i) {
        if (!texts[i].active) continue;
        FloatingText& t = texts[i];
        t.lifetime--;
        t.y += t.vy;
        t.vy += 0.12f; // Gravity deceleration
        if (t.lifetime < 15) {
            t.alpha = (t.lifetime * 255) / 15;
        }
        if (t.lifetime <= 0) t.active = false;
    }

    // 3. Update Particles with 3D Physics, Gravity, Ground Collision & Turbulence
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles[i].active) continue;
        HitParticle& p = particles[i];
        p.age += dt;
        p.lifetime--;
        if (p.age >= p.max_life_sec || p.lifetime <= 0) {
            p.active = false;
            continue;
        }

        // Frame-rate independent velocity drag
        float damping = powf(p.drag, dt * 60.0f);
        p.vx *= damping;
        p.vy *= damping;

        // Turbulence (sinusoidal jitter/drift)
        float turb_x = 0.0f;
        float turb_y = 0.0f;
        if (p.turbulence_amp > 0.0f) {
            turb_x = sinf(p.age * p.turbulence_freq + p.seed) * p.turbulence_amp;
            turb_y = cosf(p.age * p.turbulence_freq * 1.37f + p.seed) * p.turbulence_amp * 0.7f;
        }

        p.x += (p.vx + turb_x) * dt;
        p.y += (p.vy + turb_y) * dt;

        // 3D Vertical Physics (Gravity & Altitude)
        p.vz -= p.gravity * dt;
        p.z += p.vz * dt;

        // Ground Collision & Restitution (z = 0 is the arena floor)
        if (p.z <= 0.0f) {
            p.z = 0.0f;
            if (fabsf(p.vz) < 30.0f) {
                p.vz = 0.0f;
            } else {
                p.vz = -p.vz * p.bounce;
            }
            float friction = powf(p.ground_friction, dt * 60.0f);
            p.vx *= friction;
            p.vy *= friction;
        }

        // Angular rotation for debris
        p.rotation += p.v_rot * dt;
    }

    // 4. Update Shockwaves
    for (int i = 0; i < MAX_SHOCKWAVES; ++i) {
        if (!shockwaves[i].active) continue;
        ShockwaveRing& sw = shockwaves[i];
        sw.lifetime -= dt;
        if (sw.lifetime <= 0.0f) {
            sw.active = false;
            continue;
        }
        float prog = (sw.max_lifetime > 0.001f) ? (1.0f - (sw.lifetime / sw.max_lifetime)) : 1.0f;
        if (prog < 0.0f) prog = 0.0f;
        if (prog > 1.0f) prog = 1.0f;
        sw.current_radius = sw.max_radius * sqrtf(prog);
    }
}
