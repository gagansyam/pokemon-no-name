#pragma once

#include <SDL2/SDL.h>
#include "types.hpp"
#include "moves.hpp"

enum ProjectileType {
    PROJ_NONE,
    PROJ_THUNDERBOLT,
    PROJ_ELECTRO_BALL,
    PROJ_FLAMETHROWER,
    PROJ_FIRE_SPIN,
    PROJ_MELEE_SWEEP,
    PROJ_CLAW_SCRATCH,
    PROJ_WING_ATTACK,
    PROJ_SCORCHING_SANDS,
    PROJ_DRAGON_CLAW,
    PROJ_VOLT_TACKLE,
    PROJ_DRAGON_RAGE,
    PROJ_EARTHQUAKE,
    PROJ_SEISMIC_GRAB,
    PROJ_AURA_SPHERE,
    PROJ_FORCE_PALM,
    PROJ_CLOSE_COMBAT,
    PROJ_BONE_RUSH,
    PROJ_EXTREME_SPEED,
    PROJ_FLASH_CANNON,
    PROJ_DRAGON_PULSE,
    PROJ_THUNDER,
    PROJ_WATER_SHURIKEN,
    PROJ_NIGHT_SLASH,
    PROJ_HYDRO_PUMP,
    PROJ_AERIAL_ACE
};

struct Projectile {
    bool active = false;
    int owner_id = 0; // 0 = Player, 1 = Dummy
    ProjectileType type = PROJ_NONE;
    MoveId move_id = MOVE_NONE;
    ElementType element_type = TYPE_NORMAL;
    MoveCategory category = CATEGORY_PHYSICAL;
    int base_power = 40;
    PokemonStats owner_stats;
    ElementType owner_type1 = TYPE_NORMAL;
    ElementType owner_type2 = TYPE_NONE;

    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float radius = 12.0f;
    int damage = 20; // Pre-calculated or fallback base
    float lifetime = 0;
    float max_lifetime = 0;
    bool homing = false;
    bool piercing = false;
    float target_x = 0, target_y = 0;
    int hit_cooldown = 0;
    float delay_timer = 0.0f;
    float max_delay = 0.0f;
    bool has_struck = false;
    bool can_damage = true;
    float angle = 0.0f;
    float base_radius = 12.0f;
    float extra_timer = 0.0f;
    float z_min = 0.0f;
    float z_max = 60.0f;
    bool is_ground_level = false;
};

struct FloatingText {
    bool active = false;
    float x = 0, y = 0;
    float vy = -2.5f;
    int damage = 0;
    bool is_crit = false;
    float effectiveness = 1.0f; // 2.0 = SUPER EFFECTIVE, 0.5 = RESISTED, 0.0 = IMMUNE
    char label[16] = {0};
    Uint8 custom_r = 255;
    Uint8 custom_g = 255;
    Uint8 custom_b = 255;
    int alpha = 255;
    int lifetime = 40;
};

enum ParticleStyle {
    PSTYLE_SPARK,       // Stretched directional velocity streak, additive blend, high speed, bounce
    PSTYLE_SMOKE_DUST,  // Expanding soft circular puff, deceleration, alpha fade, floor drift
    PSTYLE_EMBER,       // Buoyant floating embers, upward draft, sinusoidal turbulence, additive blend
    PSTYLE_ELECTRIC,    // High-frequency jittering lightning sparks, rapid snap turns, additive glow
    PSTYLE_DEBRIS,      // Tumbling rock/bone polygonal shards, heavy gravity, ground bounce
    PSTYLE_AURA_MOTE,   // Swirling glowing energy orbs, ease-out alpha, orbital/drifting motion
    PSTYLE_RING_PULSE   // Micro-shockwave ring expanding rapidly
};

struct HitParticle {
    bool active = false;
    ParticleStyle style = PSTYLE_SPARK;
    float x = 0.0f, y = 0.0f, z = 0.0f;       // 3D coordinates (z = height off ground)
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;    // 3D velocities
    float gravity = 0.0f;                     // Downward acceleration (px/s^2)
    float drag = 0.92f;                       // Frame-independent drag coefficient
    float bounce = 0.0f;                      // Restitution on ground impact (z <= 0)
    float ground_friction = 0.6f;             // Horizontal slowdown when sliding on ground

    float size_start = 4.0f;
    float size_end = 1.0f;

    Uint8 r_start = 255, g_start = 255, b_start = 255;
    Uint8 r_end = 255, g_end = 255, b_end = 255;
    Uint8 a_start = 255, a_end = 0;

    // Compatibility fields for legacy code/tests
    Uint8 r = 255, g = 255, b = 255;
    float size = 4.0f;
    int lifetime = 20;
    int max_lifetime = 20;

    float age = 0.0f;                         // Elapsed time in seconds
    float max_life_sec = 0.35f;               // Total lifespan in seconds

    float rotation = 0.0f;                    // Current rotation angle (for debris/shards)
    float v_rot = 0.0f;                       // Angular velocity (rad/s)
    float turbulence_amp = 0.0f;              // Jitter / drift amplitude
    float turbulence_freq = 0.0f;             // Jitter / drift frequency
    float seed = 0.0f;                        // Random phase seed

    bool additive = true;                     // SDL_BLENDMODE_ADD vs SDL_BLENDMODE_BLEND
};

struct ShockwaveRing {
    bool active = false;
    float x = 0, y = 0;
    float current_radius = 0.0f;
    float max_radius = 90.0f;
    float lifetime = 0.28f;
    float max_lifetime = 0.28f;
    int thickness = 4;
    Uint8 r = 255, g = 255, b = 255;
};

#define MAX_PROJ 32
#define MAX_TEXTS 32
#define MAX_PARTICLES 1024
#define MAX_SHOCKWAVES 16

class Fighter;

class ProjectileManager {
public:
    Projectile pool[MAX_PROJ];
    FloatingText texts[MAX_TEXTS];
    HitParticle particles[MAX_PARTICLES];
    ShockwaveRing shockwaves[MAX_SHOCKWAVES];
    int particle_cursor = 0;
    float screen_shake = 0.0f;
    float hitstop_timer = 0.0f;
    float impact_flash_timer = 0.0f;
    float impact_flash_x = 0.0f;
    float impact_flash_y = 0.0f;

    void init();
    void update(float dt, float p1_x, float p1_y, float p2_x, float p2_y);
    void update_effects(float dt);
    void render(SDL_Renderer* ren, float cam_x, float cam_y);

    void spawn(int owner_id, ProjectileType type, float x, float y, float vx, float vy, int damage, float lifetime, float target_x = 0.0f, float target_y = 0.0f,
               ElementType element_type = TYPE_NORMAL, MoveCategory category = CATEGORY_PHYSICAL, int base_power = 0,
               const PokemonStats* stats = nullptr, ElementType t1 = TYPE_NORMAL, ElementType t2 = TYPE_NONE,
               MoveId move_id = MOVE_NONE);
    void spawn_damage_number(float x, float y, int damage, bool is_crit, float effectiveness = 1.0f);
    void spawn_custom_text(float x, float y, const char* label, Uint8 r, Uint8 g, Uint8 b, int lifetime = 45);
    void spawn_particles(float x, float y, int count, Uint8 r, Uint8 g, Uint8 b, float speed);
    void spawn_sparks(float x, float y, float z, int count, Uint8 r, Uint8 g, Uint8 b, float speed, float dir_angle = 0.0f, float spread_angle = 6.28318f);
    void spawn_dust(float x, float y, int count, Uint8 r, Uint8 g, Uint8 b, float speed, float radius = 8.0f);
    void spawn_embers(float x, float y, float z, int count, float speed);
    void spawn_electric_sparks(float x, float y, float z, int count, float speed);
    void spawn_debris(float x, float y, float z, int count, float speed, Uint8 r = 160, Uint8 g = 140, Uint8 b = 120);
    void spawn_aura_motes(float x, float y, float z, int count, float speed, Uint8 r, Uint8 g, Uint8 b);
    void spawn_shockwave(float x, float y, float max_radius, float lifetime, Uint8 r = 255, Uint8 g = 255, Uint8 b = 255, int thickness = 4);
    void trigger_hitstop(float duration, float x, float y);
    bool check_melee_clash(Fighter* p1, Fighter* p2);
};

extern ProjectileManager g_projectiles;

