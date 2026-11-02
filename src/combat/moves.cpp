#include "moves.hpp"
#include "fighter.hpp"
#include "projectile.hpp"
#include "arena.hpp"
#include "combat_log.hpp"
#include <cmath>
#include <cstdlib>

static MoveDef s_move_defs[MOVE_COUNT];
static bool s_registry_initialized = false;

// 1. Quick Attack: High-speed physical forward burst strike
static void exec_quick_attack(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.24f;
    user->attack_timer = 0.24f;
    user->active_move = MOVE_QUICK_ATTACK;

    user->vx = cos_a * (user->move_speed * 2.8f);
    user->vy = sin_a * (user->move_speed * 2.8f);

    // Beat 1/2: Sonic speed ring & particle motes
    g_projectiles.spawn_shockwave(user->x, user->y, 50.0f, 0.16f, 255, 255, 255, 3);
    g_projectiles.spawn_particles(user->x, user->y, 14, 230, 245, 255, 180.0f);

    g_projectiles.spawn(user->id, PROJ_MELEE_SWEEP, user->x + cos_a * 25.0f, user->y + sin_a * 25.0f, 0, 0, 18, 0.15f,
                        0.0f, 0.0f, TYPE_NORMAL, CATEGORY_PHYSICAL, 16, &user->base_stats, user->type1, user->type2, MOVE_QUICK_ATTACK);
}

// 2. Thunderbolt: High-voltage continuous electric torrent erupting directly from Pikachu's cheeks/body
static void exec_thunderbolt(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.36f;
    user->attack_timer = 0.36f;
    user->active_move = MOVE_THUNDERBOLT;
    user->move_phase = 1;
    user->move_phase_timer = 0.0f;

    // Recoil pushback on caster
    user->vx = -cos_a * 55.0f;
    user->vy = -sin_a * 55.0f;

    float spawn_x = user->x + cos_a * 24.0f;
    float spawn_y = user->y + sin_a * 24.0f;

    // Anime Beat 1: High-voltage cheek spark detonation & dual shockwave
    g_projectiles.spawn_electric_sparks(spawn_x, spawn_y, 14.0f, 20, 220.0f);
    g_projectiles.spawn_shockwave(spawn_x, spawn_y, 50.0f, 0.18f, 250, 204, 21, 4);
    g_projectiles.spawn_shockwave(spawn_x, spawn_y, 35.0f, 0.14f, 56, 189, 248, 3);

    // Continuous anchored lightning beam:
    // Origin is stored in target_x, target_y (locked to Pikachu's cheeks).
    // Front starts at spawn_x, spawn_y and advances at 1600 px/s up to 440px range.
    // Lifetime is 0.36s matching the full duration of Pikachu's attack stance!
    g_projectiles.spawn(user->id, PROJ_THUNDERBOLT, spawn_x, spawn_y,
                        cos_a * 1600.0f, sin_a * 1600.0f, 14, 0.36f,
                        spawn_x, spawn_y, TYPE_ELECTRIC, CATEGORY_SPECIAL, 14,
                        &user->base_stats, user->type1, user->type2, MOVE_THUNDERBOLT);
}

// 2b. Thunder: Apocalyptic sky-called celestial lightning pillar crashing down on target coordinates
static void exec_thunder(Fighter* user, Fighter* target, float target_x, float target_y) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    float strike_x = 0.0f, strike_y = 0.0f;
    if (target_x != 0.0f || target_y != 0.0f) {
        strike_x = target_x;
        strike_y = target_y;
    } else if (target != nullptr) {
        strike_x = target->x;
        strike_y = target->y;
    } else {
        strike_x = user->x + cos_a * 280.0f;
        strike_y = user->y + sin_a * 280.0f;
    }

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.38f;
    user->attack_timer = 0.38f;
    user->active_move = MOVE_THUNDER;
    user->vx = 0.0f;
    user->vy = 0.0f;

    // Anticipation: Caster crackles with electric charge into the heavens
    g_projectiles.spawn_electric_sparks(user->x, user->y, 16.0f, 14, 190.0f);
    g_projectiles.spawn_shockwave(user->x, user->y, 50.0f, 0.18f, 56, 189, 248, 3);

    // Spawn targeted celestial sky-pillar projectile with 0.32s runic telegraph
    g_projectiles.spawn(user->id, PROJ_THUNDER, strike_x, strike_y,
                        0.0f, 0.0f, 36, 0.55f, strike_x, strike_y,
                        TYPE_ELECTRIC, CATEGORY_SPECIAL, 36, &user->base_stats, user->type1, user->type2, MOVE_THUNDER);
}

// 3. Electro Ball: High-velocity guided sphere (750 px/s)
static void exec_electro_ball(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.25f;
    user->attack_timer = 0.25f;
    user->active_move = MOVE_ELECTRO_BALL;
    user->vx = 0.0f;
    user->vy = 0.0f;

    // Beat 1: Inward motes & golden aura burst
    g_projectiles.spawn_aura_motes(user->x + cos_a * 25.0f, user->y + sin_a * 25.0f, 12.0f, 10, 90.0f, 250, 204, 21);
    g_projectiles.spawn_electric_sparks(user->x + cos_a * 25.0f, user->y + sin_a * 25.0f, 10.0f, 8, 140.0f);
    g_projectiles.spawn_shockwave(user->x + cos_a * 25.0f, user->y + sin_a * 25.0f, 40.0f, 0.18f, 56, 189, 248, 3);

    g_projectiles.spawn(user->id, PROJ_ELECTRO_BALL, user->x + cos_a * 30.0f, user->y + sin_a * 30.0f,
                        cos_a * 750.0f, sin_a * 750.0f, 32, 2.0f,
                        0.0f, 0.0f, TYPE_ELECTRIC, CATEGORY_SPECIAL, 20, &user->base_stats, user->type1, user->type2, MOVE_ELECTRO_BALL);
}

// 4. Iron Tail: 2-stage stance -> anime jump-spin leap with sweet-spot damage
static void exec_iron_tail(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    if (user->primed_move != MOVE_IRON_TAIL && !user->iron_tail_primed) {
        // Stage 1: Prime hardened steel stance (15 SP consumed in execute caller)
        user->primed_move = MOVE_IRON_TAIL;
        user->primed_timer = 6.0f;
        user->iron_tail_primed = true;
        user->iron_tail_timer = 6.0f;
        g_projectiles.spawn_particles(user->x - cos_a * 15.0f, user->y - sin_a * 15.0f, 16, 215, 230, 255, 110.0f);
    } else {
        // Stage 2: Unleash Anime 360-degree Jump-Spin Leap! (25 SP consumed in execute caller)
        user->primed_move = MOVE_NONE;
        user->primed_timer = 0.0f;
        user->iron_tail_primed = false;
        user->iron_tail_timer = 0.0f;

        user->active_move = MOVE_IRON_TAIL;
        user->move_phase = 1;
        user->iron_tail_striking = true;

        user->state = STATE_ATTACK;
        user->anim_timer = 0.0f;
        user->anim_frame = 0;
        user->attack_duration = 0.38f;
        user->attack_timer = 0.38f;
        user->z = 0.0f;
        user->iron_tail_jump_z = 0.0f;

        // Directional vault forward (520 px/s over 0.38s ~ 195px range)
        user->vx = cos_a * 520.0f;
        user->vy = sin_a * 520.0f;

        float strike_x = user->x + cos_a * 38.0f;
        float strike_y = user->y + sin_a * 38.0f;

        // Stamp permanent metallic impact crater on the arena floor!
        g_arena.add_decal(DECAL_IMPACT_CRATER, strike_x, strike_y, 32.0f, user->aim_angle, 35, 25, 18, 230);

        g_projectiles.spawn(user->id, PROJ_MELEE_SWEEP, strike_x, strike_y,
                            cos_a * 260.0f, sin_a * 260.0f, 45, 0.32f,
                            0.0f, 0.0f, TYPE_STEEL, CATEGORY_PHYSICAL, 28, &user->base_stats, user->type1, user->type2, MOVE_IRON_TAIL);
        g_projectiles.spawn_particles(strike_x, strike_y, 18, 230, 240, 255, 190.0f);
    }
}

// 5. Scratch: Swift dragon claw slash with razor kinetic sweep
static void exec_scratch(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.28f;
    user->attack_timer = 0.28f;
    user->active_move = MOVE_SCRATCH;

    user->vx = cos_a * 300.0f;
    user->vy = sin_a * 300.0f;

    float spawn_x = user->x + cos_a * 34.0f;
    float spawn_y = user->y + sin_a * 34.0f;

    // Anime Beat 1: Razor wind slash ripple & scarlet smear motes
    g_projectiles.spawn_shockwave(spawn_x, spawn_y, 40.0f, 0.14f, 239, 68, 68, 3);
    g_projectiles.spawn_particles(spawn_x, spawn_y, 16, 239, 68, 68, 170.0f);
    g_projectiles.spawn_particles(spawn_x, spawn_y, 8, 255, 255, 255, 140.0f);

    g_projectiles.spawn(user->id, PROJ_CLAW_SCRATCH, spawn_x, spawn_y,
                        cos_a * 280.0f, sin_a * 280.0f, 20, 0.22f,
                        0.0f, 0.0f, TYPE_NORMAL, CATEGORY_PHYSICAL, 14, &user->base_stats, user->type1, user->type2, MOVE_SCRATCH);
}

// 6. Flamethrower: Continuous roaring molten torrent streaming from Charizard's maw
static void exec_flamethrower(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.38f;
    user->attack_timer = 0.38f;
    user->active_move = MOVE_FLAMETHROWER;
    user->move_phase = 1;         // Continuous stream emission
    user->move_phase_timer = 0.0f;

    // Breath recoil pushback
    user->vx = -cos_a * 50.0f;
    user->vy = -sin_a * 50.0f;

    float spawn_x = user->x + cos_a * 28.0f;
    float spawn_y = user->y + sin_a * 28.0f;

    // Muzzle thermal flash & smoke burst
    g_projectiles.spawn_shockwave(spawn_x, spawn_y, 45.0f, 0.16f, 249, 115, 22, 3);
    g_projectiles.spawn_embers(spawn_x, spawn_y, 8.0f, 16, 180.0f);
    g_projectiles.spawn_dust(spawn_x, spawn_y, 8, 120, 50, 20, 90.0f, 16.0f);

    // Initial first expanding flame vortex
    g_projectiles.spawn(user->id, PROJ_FLAMETHROWER,
                        spawn_x, spawn_y,
                        cos_a * 620.0f, sin_a * 620.0f,
                        6, 0.34f, 0.0f, 0.0f,
                        TYPE_FIRE, CATEGORY_SPECIAL, 6, &user->base_stats, user->type1, user->type2, MOVE_FLAMETHROWER);
}

// 7. Fire Spin: Ember seed spit -> ground warning seal -> spinning vortex
static void exec_fire_spin(Fighter* user, Fighter* target, float target_x, float target_y) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    float dest_x = 0.0f, dest_y = 0.0f;
    if (target_x != 0.0f || target_y != 0.0f) {
        dest_x = target_x;
        dest_y = target_y;
    } else if (target != nullptr) {
        dest_x = target->x;
        dest_y = target->y;
    } else {
        dest_x = user->x + cos_a * 340.0f;
        dest_y = user->y + sin_a * 340.0f;
    }

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.30f;
    user->attack_timer = 0.30f;
    user->active_move = MOVE_FIRE_SPIN;
    user->vx = 0.0f;
    user->vy = 0.0f;

    g_projectiles.spawn(user->id, PROJ_FIRE_SPIN,
                        user->x + cos_a * 24.0f, user->y + sin_a * 24.0f,
                        cos_a * 640.0f, sin_a * 640.0f,
                        12, 3.2f, dest_x, dest_y,
                        TYPE_FIRE, CATEGORY_SPECIAL, 10, &user->base_stats, user->type1, user->type2);
    g_projectiles.spawn_particles(user->x + cos_a * 26.0f, user->y + sin_a * 26.0f, 12, 249, 115, 22, 160.0f);
}

// 8. Wing Attack: Takeoff windup -> sonic boom compression -> supersonic flight rush (28 SP rebalanced)
static void exec_wing_attack(Fighter* user, Fighter* target, float target_x, float target_y) {
    if (target_x != 0.0f || target_y != 0.0f) {
        user->flight_angle = atan2f(target_y - user->y, target_x - user->x);
    } else if (target != nullptr) {
        user->flight_angle = atan2f(target->y - user->y, target->x - user->x);
    } else {
        user->flight_angle = user->aim_angle;
    }

    user->pmd_dir = angle_to_pmd_dir(user->flight_angle);
    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->active_move = MOVE_WING_ATTACK;
    user->move_phase = 0; // Phase 0: Takeoff windup telegraph (0.20s)
    user->move_phase_timer = 0.20f;
    user->wing_attack_windup = 0.20f;
    user->wing_attack_rush = 0.48f;
    user->attack_duration = 0.68f;
    user->attack_timer = 0.68f;
    user->is_flying = true;
    user->vx = 0.0f;
    user->vy = 0.0f;

    g_projectiles.spawn_particles(user->x, user->y + 10.0f, 16, 220, 240, 255, 140.0f);
}

// 9. Scorching Sands: Scoop boiling sand from beneath feet and hurl surging ground wave forward
static void exec_scorching_sands(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.32f;
    user->attack_timer = 0.32f;
    user->active_move = MOVE_SCORCHING_SANDS;
    user->vx = -cos_a * 45.0f; // Earth-shoveling recoil kickback
    user->vy = -sin_a * 45.0f;

    // 1. Excavate sand trench decal directly beneath caster's feet
    g_arena.add_decal(DECAL_SAND_FURROW, user->x, user->y, 32.0f, user->aim_angle, 120, 53, 15, 235);

    // 2. Burst of boiling sand clods kicked up from under feet
    g_projectiles.spawn_particles(user->x, user->y + 8.0f, 20, 180, 83, 9, 140.0f);
    g_projectiles.spawn_particles(user->x, user->y + 8.0f, 12, 245, 158, 11, 190.0f);

    // 3. Spawns surging ground wave launching forward from beneath feet
    g_projectiles.spawn(user->id, PROJ_SCORCHING_SANDS,
                        user->x + cos_a * 20.0f, user->y + sin_a * 20.0f,
                        cos_a * 620.0f, sin_a * 620.0f,
                        22, 0.55f, 0.0f, 0.0f,
                        TYPE_GROUND, CATEGORY_SPECIAL, 22, &user->base_stats, user->type1, user->type2);
}

// 10. Dragon Claw: Emerald razor strike with forward step & 65ms hitstop combo starter
static void exec_dragon_claw(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.28f;
    user->attack_timer = 0.28f;
    user->active_move = MOVE_DRAGON_CLAW;

    // Fast forward lunge step
    user->vx = cos_a * 380.0f;
    user->vy = sin_a * 380.0f;

    float spawn_x = user->x + cos_a * 36.0f;
    float spawn_y = user->y + sin_a * 36.0f;

    // Anime Beat 1: Draconic emerald shockwave & indigo energy burst
    g_projectiles.spawn_shockwave(spawn_x, spawn_y, 45.0f, 0.16f, 52, 211, 153, 3);
    g_projectiles.spawn_particles(spawn_x, spawn_y, 18, 16, 185, 129, 200.0f);
    g_projectiles.spawn_particles(spawn_x, spawn_y, 10, 79, 70, 229, 160.0f);

    g_projectiles.spawn(user->id, PROJ_DRAGON_CLAW, spawn_x, spawn_y,
                        cos_a * 340.0f, sin_a * 340.0f, 24, 0.24f,
                        0.0f, 0.0f, TYPE_DRAGON, CATEGORY_PHYSICAL, 24, &user->base_stats, user->type1, user->type2, MOVE_DRAGON_CLAW);
}

// 11. Volt Tackle: Pikachu's high-speed electrified rush attack (30 BP, 32 SP)
static void exec_volt_tackle(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.40f;
    user->attack_timer = 0.40f;
    user->active_move = MOVE_VOLT_TACKLE;

    user->vx = cos_a * 700.0f;
    user->vy = sin_a * 700.0f;

    g_projectiles.spawn(user->id, PROJ_VOLT_TACKLE, user->x + cos_a * 32.0f, user->y + sin_a * 32.0f,
                        cos_a * 700.0f, sin_a * 700.0f, 32, 0.38f,
                        0.0f, 0.0f, TYPE_ELECTRIC, CATEGORY_PHYSICAL, 30, &user->base_stats, user->type1, user->type2);
    g_projectiles.spawn_particles(user->x + cos_a * 25.0f, user->y + sin_a * 25.0f, 20, 250, 204, 21, 220.0f);
}

// 12. Dragon Rage: Iconic anime draconic breath plasma orb (25 BP, 26 SP, 680 px/s)
static void exec_dragon_rage(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.30f;
    user->attack_timer = 0.30f;
    user->active_move = MOVE_DRAGON_RAGE;
    user->vx = -cos_a * 60.0f; // Subtle draconic recoil kickback
    user->vy = -sin_a * 60.0f;

    g_projectiles.spawn(user->id, PROJ_DRAGON_RAGE, user->x + cos_a * 35.0f, user->y + sin_a * 35.0f,
                        cos_a * 680.0f, sin_a * 680.0f, 26, 0.75f,
                        0.0f, 0.0f, TYPE_DRAGON, CATEGORY_SPECIAL, 25, &user->base_stats, user->type1, user->type2);
    g_projectiles.spawn_particles(user->x + cos_a * 30.0f, user->y + sin_a * 30.0f, 20, 99, 102, 241, 190.0f);
}

// 13. Earthquake: Heavy tectonic seismic arena stomp (18 BP, 40 SP, R=180 area shockwave)
static void exec_earthquake(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.35f;
    user->attack_timer = 0.35f;
    user->active_move = MOVE_EARTHQUAKE;
    user->vx = 0.0f;
    user->vy = 0.0f;

    // Spawns seismic earthquake centered at user with 0.30s telegraph before expanding shockwave burst
    g_projectiles.spawn(user->id, PROJ_EARTHQUAKE, user->x, user->y, 0.0f, 0.0f, 50, 0.65f,
                        0.0f, 0.0f, TYPE_GROUND, CATEGORY_PHYSICAL, 18, &user->base_stats, user->type1, user->type2);
    g_projectiles.spawn_particles(user->x, user->y, 22, 180, 130, 80, 200.0f);
}

// 14. Seismic Toss: Charizard's iconic anime grab -> stratosphere orbit -> earth slam piledriver (34 BP, 36 SP)
static void exec_seismic_toss(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.38f;
    user->attack_timer = 0.38f;
    user->active_move = MOVE_SEISMIC_TOSS;
    user->move_phase = 0; // Phase 0: Forward Grab Lunge
    user->seismic_toss_timer = 0.0f;

    // High-speed predatory grab lunge step
    user->vx = cos_a * 640.0f;
    user->vy = sin_a * 640.0f;

    g_projectiles.spawn(user->id, PROJ_SEISMIC_GRAB,
                        user->x + cos_a * 38.0f, user->y + sin_a * 38.0f,
                        cos_a * 640.0f, sin_a * 640.0f,
                        42, 0.32f, 0.0f, 0.0f,
                        TYPE_FIGHTING, CATEGORY_PHYSICAL, 34, &user->base_stats, user->type1, user->type2);
    g_projectiles.spawn_particles(user->x + cos_a * 25.0f, user->y + sin_a * 25.0f, 16, 248, 113, 113, 160.0f);
}

// 15. Agility: Pikachu's anime speedster burst (+40% speed, fast dash cooldown, afterimage trail) (20 SP)
static void exec_agility(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.18f;
    user->attack_timer = 0.18f;
    user->active_move = MOVE_AGILITY;
    user->agility_timer = 3.2f; // 3.2 seconds of high-speed agility

    // Anime speed burst forward impulse
    float spd = user->move_speed * 1.8f;
    user->vx = cosf(user->aim_angle) * spd;
    user->vy = sinf(user->aim_angle) * spd;

    // Speed burst ring FX
    g_projectiles.spawn_shockwave(user->x, user->y, 85.0f, 0.22f, 56, 189, 248, 4);
    g_projectiles.spawn_shockwave(user->x, user->y, 55.0f, 0.18f, 250, 204, 21, 3);
    g_projectiles.spawn_particles(user->x, user->y, 22, 56, 189, 248, 220.0f);
    g_projectiles.spawn_particles(user->x, user->y, 14, 255, 255, 255, 180.0f);
    g_projectiles.spawn_custom_text(user->x, user->y - 25.0f, "AGILITY UP!", 56, 189, 248, 45);
}

static void exec_double_team(Fighter* user, Fighter* target, float target_x, float target_y) {
    (void)target; (void)target_x; (void)target_y;
    user->active_move = MOVE_DOUBLE_TEAM;
    user->spawn_double_team();

    // Split explosion FX at user and clones
    g_projectiles.spawn_shockwave(user->x, user->y, 75.0f, 0.22f, 255, 255, 255, 4);
    g_projectiles.spawn_shockwave(user->x, user->y, 45.0f, 0.16f, 56, 189, 248, 3);
    g_projectiles.spawn_particles(user->x, user->y, 20, 255, 255, 255, 200.0f);
    g_projectiles.spawn_particles(user->x, user->y, 14, 56, 189, 248, 160.0f);

    for (int i = 0; i < Fighter::MAX_CLONES; ++i) {
        if (user->clones[i].active) {
            g_projectiles.spawn_shockwave(user->clones[i].x, user->clones[i].y, 50.0f, 0.18f, 255, 255, 255, 3);
            g_projectiles.spawn_particles(user->clones[i].x, user->clones[i].y, 12, 56, 189, 248, 140.0f);
        }
    }
    g_projectiles.spawn_custom_text(user->x, user->y - 25.0f, "DOUBLE TEAM!", 56, 189, 248, 45);
}

static void exec_protect(Fighter* user, Fighter* target, float target_x, float target_y) {
    (void)target; (void)target_x; (void)target_y;
    user->active_move = MOVE_PROTECT;
    user->protect_timer = 0.40f;
    user->protect_parry_window = 0.12f;
    user->shield_break_timer = 0.0f;
    user->vx *= 0.20f;
    user->vy *= 0.20f;

    // Emerald geodesic dome barrier activation FX
    g_projectiles.spawn_shockwave(user->x, user->y, 50.0f, 0.20f, 16, 185, 129, 4);
    g_projectiles.spawn_shockwave(user->x, user->y, 35.0f, 0.15f, 255, 255, 255, 3);
    g_projectiles.spawn_particles(user->x, user->y, 16, 16, 185, 129, 160.0f);
    g_projectiles.spawn_particles(user->x, user->y, 8, 255, 255, 255, 120.0f);
    g_projectiles.spawn_custom_text(user->x, user->y - 25.0f, "PROTECT!", 16, 185, 129, 40);
}

// 18. Aura Sphere: Lucario's signature sapphire aura orb with subtle magnetic homing (24 BP, 25 SP)
static void exec_aura_sphere(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.28f;
    user->attack_timer = 0.28f;
    user->active_move = MOVE_AURA_SPHERE;
    user->vx *= 0.30f;
    user->vy *= 0.30f;

    float spd = 580.0f;
    g_projectiles.spawn(user->id, PROJ_AURA_SPHERE,
                        user->x + cos_a * 32.0f, user->y + sin_a * 32.0f,
                        cos_a * spd, sin_a * spd,
                        24, 1.25f, 0.0f, 0.0f,
                        TYPE_FIGHTING, CATEGORY_SPECIAL, 24, &user->base_stats, user->type1, user->type2,
                        MOVE_AURA_SPHERE);
    g_projectiles.spawn_particles(user->x + cos_a * 24.0f, user->y + sin_a * 24.0f, 18, 56, 189, 248, 160.0f);
    g_projectiles.spawn_shockwave(user->x + cos_a * 24.0f, user->y + sin_a * 24.0f, 45.0f, 0.18f, 56, 189, 248, 3);
}

// 19. Force Palm: Close-range martial thrust with compressed kinetic shockwave (20 BP, 18 SP)
static void exec_force_palm(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.20f;
    user->attack_timer = 0.20f;
    user->active_move = MOVE_FORCE_PALM;

    // Forward palm thrust lunge
    user->vx = cos_a * 340.0f;
    user->vy = sin_a * 340.0f;

    g_projectiles.spawn(user->id, PROJ_FORCE_PALM,
                        user->x + cos_a * 36.0f, user->y + sin_a * 36.0f,
                        cos_a * 420.0f, sin_a * 420.0f,
                        20, 0.16f, 0.0f, 0.0f,
                        TYPE_FIGHTING, CATEGORY_PHYSICAL, 20, &user->base_stats, user->type1, user->type2,
                        MOVE_FORCE_PALM);
    g_projectiles.spawn_shockwave(user->x + cos_a * 30.0f, user->y + sin_a * 30.0f, 55.0f, 0.18f, 56, 189, 248, 4);
    g_projectiles.spawn_particles(user->x + cos_a * 30.0f, user->y + sin_a * 30.0f, 16, 255, 255, 255, 180.0f);
}

// 20. Close Combat: Rapid martial flurry rush ending in heavy knockdown slam (32 BP, 38 SP)
static void exec_close_combat(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.32f;
    user->attack_timer = 0.32f;
    user->active_move = MOVE_CLOSE_COMBAT;

    // Forward rush step
    user->vx = cos_a * 460.0f;
    user->vy = sin_a * 460.0f;

    g_projectiles.spawn(user->id, PROJ_CLOSE_COMBAT,
                        user->x + cos_a * 40.0f, user->y + sin_a * 40.0f,
                        cos_a * 460.0f, sin_a * 460.0f,
                        32, 0.28f, 0.0f, 0.0f,
                        TYPE_FIGHTING, CATEGORY_PHYSICAL, 32, &user->base_stats, user->type1, user->type2,
                        MOVE_CLOSE_COMBAT);
    g_projectiles.spawn_shockwave(user->x + cos_a * 35.0f, user->y + sin_a * 35.0f, 65.0f, 0.20f, 239, 68, 68, 4);
    g_projectiles.spawn_particles(user->x + cos_a * 35.0f, user->y + sin_a * 35.0f, 24, 250, 204, 21, 200.0f);
}

// 21. Bone Rush: Cyan aura bone staff sweep with vault hop (22 BP, 24 SP)
static void exec_bone_rush(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.28f;
    user->attack_timer = 0.28f;
    user->active_move = MOVE_BONE_RUSH;

    // Pole-vault rushing surge forward
    user->vx = cos_a * 580.0f;
    user->vy = sin_a * 580.0f;

    g_projectiles.spawn_aura_motes(user->x, user->y, 14.0f, 12, 110.0f, 56, 189, 248);

    float bone_spd = 740.0f;
    g_projectiles.spawn(user->id, PROJ_BONE_RUSH,
                        user->x + cos_a * 45.0f, user->y + sin_a * 45.0f,
                        cos_a * bone_spd, sin_a * bone_spd,
                        22, 0.55f, 0.0f, 0.0f,
                        TYPE_GROUND, CATEGORY_PHYSICAL, 22, &user->base_stats, user->type1, user->type2,
                        MOVE_BONE_RUSH);
    g_projectiles.spawn_shockwave(user->x + cos_a * 40.0f, user->y + sin_a * 40.0f, 65.0f, 0.20f, 56, 189, 248, 4);
    g_projectiles.spawn_particles(user->x + cos_a * 40.0f, user->y + sin_a * 40.0f, 20, 56, 189, 248, 180.0f);
    g_projectiles.spawn_particles(user->x, user->y, 10, 180, 83, 9, 120.0f);
}

// 22. Extreme Speed: Supersonic priority warp strike (22 BP, 28 SP)
static void exec_extreme_speed(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.28f;
    user->attack_timer = 0.28f;
    user->active_move = MOVE_EXTREME_SPEED;

    // Supersonic burst velocity
    user->vx = cos_a * 1050.0f;
    user->vy = sin_a * 1050.0f;

    g_projectiles.spawn(user->id, PROJ_EXTREME_SPEED,
                        user->x + cos_a * 50.0f, user->y + sin_a * 50.0f,
                        cos_a * 1050.0f, sin_a * 1050.0f,
                        22, 0.28f, 0.0f, 0.0f,
                        TYPE_NORMAL, CATEGORY_PHYSICAL, 22, &user->base_stats, user->type1, user->type2,
                        MOVE_EXTREME_SPEED);
    g_projectiles.spawn_shockwave(user->x, user->y, 80.0f, 0.20f, 255, 255, 255, 5);
    g_projectiles.spawn_particles(user->x, user->y, 25, 255, 255, 255, 240.0f);
}

// 23. Flash Cannon: High-speed concentrated steel energy blast (26 BP, 30 SP)
static void exec_flash_cannon(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.24f;
    user->attack_timer = 0.24f;
    user->active_move = MOVE_FLASH_CANNON;
    user->vx *= 0.30f;
    user->vy *= 0.30f;

    // Beat 1: Inward steel magnetic motes
    g_projectiles.spawn_aura_motes(user->x + cos_a * 30.0f, user->y + sin_a * 30.0f, 14.0f, 10, 100.0f, 203, 213, 225);

    float spd = 820.0f;
    g_projectiles.spawn(user->id, PROJ_FLASH_CANNON,
                        user->x + cos_a * 35.0f, user->y + sin_a * 35.0f,
                        cos_a * spd, sin_a * spd,
                        26, 0.70f, 0.0f, 0.0f,
                        TYPE_STEEL, CATEGORY_SPECIAL, 26, &user->base_stats, user->type1, user->type2,
                        MOVE_FLASH_CANNON);
    g_projectiles.spawn_shockwave(user->x + cos_a * 30.0f, user->y + sin_a * 30.0f, 50.0f, 0.18f, 203, 213, 225, 4);
    g_projectiles.spawn_particles(user->x + cos_a * 30.0f, user->y + sin_a * 30.0f, 20, 241, 245, 249, 190.0f);
}

// 24. Dragon Pulse: Roaring spectral dragon head beam (22 BP, 26 SP)
static void exec_dragon_pulse(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.28f;
    user->attack_timer = 0.28f;
    user->active_move = MOVE_DRAGON_PULSE;
    user->vx *= 0.30f;
    user->vy *= 0.30f;

    float spd = 780.0f;
    g_projectiles.spawn(user->id, PROJ_DRAGON_PULSE,
                        user->x + cos_a * 38.0f, user->y + sin_a * 38.0f,
                        cos_a * spd, sin_a * spd,
                        22, 0.85f, 0.0f, 0.0f,
                        TYPE_DRAGON, CATEGORY_SPECIAL, 22, &user->base_stats, user->type1, user->type2,
                        MOVE_DRAGON_PULSE);
    g_projectiles.spawn_shockwave(user->x + cos_a * 30.0f, user->y + sin_a * 30.0f, 65.0f, 0.22f, 16, 185, 129, 4);
    g_projectiles.spawn_particles(user->x + cos_a * 30.0f, user->y + sin_a * 30.0f, 22, 52, 211, 153, 200.0f);
    g_projectiles.spawn_particles(user->x + cos_a * 30.0f, user->y + sin_a * 30.0f, 12, 99, 102, 241, 160.0f);
}

// 25. Water Shuriken: Rapid ninja aqua-stars (15 BP per hit, 22 SP, multi-hit x3)
static void exec_water_shuriken(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.32f;
    user->attack_timer = 0.32f;
    user->active_move = MOVE_WATER_SHURIKEN;
    user->move_phase = 1;
    user->move_phase_timer = 0.0f;

    user->vx *= 0.25f;
    user->vy *= 0.25f;

    float spd = 720.0f;
    float sx = user->x + cos_a * 24.0f;
    float sy = user->y + sin_a * 24.0f;
    g_projectiles.spawn(user->id, PROJ_WATER_SHURIKEN, sx, sy,
                        cos_a * spd, sin_a * spd,
                        10, 0.75f, 0.0f, 0.0f,
                        TYPE_WATER, CATEGORY_SPECIAL, 10,
                        &user->base_stats, user->type1, user->type2, MOVE_WATER_SHURIKEN);
    g_projectiles.spawn_shockwave(sx, sy, 38.0f, 0.16f, 56, 189, 248, 3);
    g_projectiles.spawn_particles(sx, sy, 8, 56, 189, 248, 140.0f);
}

// 26. Night Slash: Swift shadow-strike with heightened critical rate (26 BP, 20 SP)
static void exec_night_slash(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.22f;
    user->attack_timer = 0.22f;
    user->active_move = MOVE_NIGHT_SLASH;

    // Swift ninja forward dash
    user->vx = cos_a * 480.0f;
    user->vy = sin_a * 480.0f;

    float sx = user->x + cos_a * 28.0f;
    float sy = user->y + sin_a * 28.0f;

    g_projectiles.spawn(user->id, PROJ_NIGHT_SLASH, sx, sy,
                        cos_a * 380.0f, sin_a * 380.0f,
                        26, 0.18f, 0.0f, 0.0f,
                        TYPE_DARK, CATEGORY_PHYSICAL, 26,
                        &user->base_stats, user->type1, user->type2, MOVE_NIGHT_SLASH);

    g_projectiles.spawn_shockwave(sx, sy, 55.0f, 0.18f, 168, 85, 247, 4);
    g_projectiles.spawn_sparks(sx, sy, 8.0f, 14, 168, 85, 247, 180.0f);
    g_projectiles.spawn_particles(sx, sy, 10, 30, 27, 75, 120.0f);
}

// 27. Hydro Pump: High-pressure hydraulic piercing cannon (32 BP, 38 SP)
static void exec_hydro_pump(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.42f;
    user->attack_timer = 0.42f;
    user->active_move = MOVE_HYDRO_PUMP;
    user->move_phase = 1;
    user->move_phase_timer = 0.0f;

    // Strong hydraulic recoil pushback on Greninja
    user->vx = -cos_a * 150.0f;
    user->vy = -sin_a * 150.0f;

    float sx = user->x + cos_a * 28.0f;
    float sy = user->y + sin_a * 28.0f;
    float spd = 820.0f;

    g_projectiles.spawn(user->id, PROJ_HYDRO_PUMP, sx, sy,
                        cos_a * spd, sin_a * spd,
                        32, 0.65f, 0.0f, 0.0f,
                        TYPE_WATER, CATEGORY_SPECIAL, 32,
                        &user->base_stats, user->type1, user->type2, MOVE_HYDRO_PUMP);

    g_projectiles.spawn_shockwave(sx, sy, 65.0f, 0.20f, 56, 189, 248, 5);
    g_projectiles.spawn_particles(sx, sy, 20, 56, 189, 248, 210.0f);
    g_projectiles.spawn_particles(sx, sy, 14, 224, 242, 254, 190.0f);
}

// 28. Aerial Ace: Supersonic tracking leap strike that never misses (22 BP, 28 SP)
static void exec_aerial_ace(Fighter* user, Fighter* /*target*/, float /*tx*/, float /*ty*/) {
    float cos_a = cosf(user->aim_angle);
    float sin_a = sinf(user->aim_angle);

    user->state = STATE_ATTACK;
    user->anim_timer = 0.0f;
    user->anim_frame = 0;
    user->attack_duration = 0.30f;
    user->attack_timer = 0.30f;
    user->active_move = MOVE_AERIAL_ACE;
    user->move_phase = 1;
    user->move_phase_timer = 0.0f;

    // Sonic burst leap velocity
    user->vx = cos_a * 760.0f;
    user->vy = sin_a * 760.0f;

    g_projectiles.spawn_shockwave(user->x, user->y, 65.0f, 0.18f, 255, 255, 255, 4);
    g_projectiles.spawn_particles(user->x, user->y, 16, 255, 255, 255, 220.0f);

    float sx = user->x + cos_a * 35.0f;
    float sy = user->y + sin_a * 35.0f;
    g_projectiles.spawn(user->id, PROJ_AERIAL_ACE, sx, sy,
                        cos_a * 760.0f, sin_a * 760.0f,
                        22, 0.28f, 0.0f, 0.0f,
                        TYPE_FLYING, CATEGORY_PHYSICAL, 22,
                        &user->base_stats, user->type1, user->type2, MOVE_AERIAL_ACE);
}

void MoveRegistry::init() {
    if (s_registry_initialized) return;

    //                               ID                    Name               Type           Category           Pwr  Cost   CD    MinR   MaxR   AIReserve Targeted Exec
    s_move_defs[MOVE_NONE]            = { MOVE_NONE,            "",                TYPE_NONE,     CATEGORY_PHYSICAL,   0,   0,   0.0f,   0.0f,   0.0f,  0.0f, false, nullptr };
    s_move_defs[MOVE_QUICK_ATTACK]    = { MOVE_QUICK_ATTACK,    "Quick Attack",    TYPE_NORMAL,   CATEGORY_PHYSICAL,  16,  15,   0.4f,   0.0f, 260.0f, 15.0f, false, exec_quick_attack };
    s_move_defs[MOVE_THUNDERBOLT]     = { MOVE_THUNDERBOLT,     "Thunderbolt",     TYPE_ELECTRIC, CATEGORY_SPECIAL,   24,  25,   1.4f,   0.0f, 480.0f, 15.0f, false, exec_thunderbolt };
    s_move_defs[MOVE_ELECTRO_BALL]    = { MOVE_ELECTRO_BALL,    "Electro Ball",    TYPE_ELECTRIC, CATEGORY_SPECIAL,   20,  35,   1.6f,  80.0f, 650.0f, 25.0f, false, exec_electro_ball };
    s_move_defs[MOVE_IRON_TAIL]       = { MOVE_IRON_TAIL,       "Iron Tail",       TYPE_STEEL,    CATEGORY_PHYSICAL,  28,  15,   2.2f,   0.0f, 240.0f, 20.0f, false, exec_iron_tail };
    s_move_defs[MOVE_SCRATCH]         = { MOVE_SCRATCH,         "Scratch",         TYPE_NORMAL,   CATEGORY_PHYSICAL,  14,  15,   0.5f,   0.0f,  90.0f, 15.0f, false, exec_scratch };
    s_move_defs[MOVE_FLAMETHROWER]    = { MOVE_FLAMETHROWER,    "Flamethrower",    TYPE_FIRE,     CATEGORY_SPECIAL,    6,  30,   1.0f, 100.0f, 480.0f, 20.0f, false, exec_flamethrower };
    s_move_defs[MOVE_FIRE_SPIN]       = { MOVE_FIRE_SPIN,       "Fire Spin",       TYPE_FIRE,     CATEGORY_SPECIAL,   10,  35,   2.5f, 150.0f, 550.0f, 25.0f, true,  exec_fire_spin };
    s_move_defs[MOVE_WING_ATTACK]     = { MOVE_WING_ATTACK,     "Wing Attack",     TYPE_FLYING,   CATEGORY_PHYSICAL,  24,  28,   2.0f, 150.0f, 400.0f, 18.0f, false, exec_wing_attack };
    s_move_defs[MOVE_SCORCHING_SANDS] = { MOVE_SCORCHING_SANDS, "Scorching Sands", TYPE_GROUND,   CATEGORY_SPECIAL,   22,  25,   1.8f, 100.0f, 480.0f, 20.0f, false, exec_scorching_sands };
    s_move_defs[MOVE_DRAGON_CLAW]     = { MOVE_DRAGON_CLAW,     "Dragon Claw",     TYPE_DRAGON,   CATEGORY_PHYSICAL,  24,  18,   0.7f,   0.0f, 140.0f, 15.0f, false, exec_dragon_claw };
    s_move_defs[MOVE_VOLT_TACKLE]     = { MOVE_VOLT_TACKLE,     "Volt Tackle",     TYPE_ELECTRIC, CATEGORY_PHYSICAL,  30,  32,   2.2f, 100.0f, 380.0f, 25.0f, false, exec_volt_tackle };
    s_move_defs[MOVE_DRAGON_RAGE]     = { MOVE_DRAGON_RAGE,     "Dragon Rage",     TYPE_DRAGON,   CATEGORY_SPECIAL,   25,  26,   1.5f,  80.0f, 520.0f, 20.0f, false, exec_dragon_rage };
    s_move_defs[MOVE_EARTHQUAKE]      = { MOVE_EARTHQUAKE,      "Earthquake",      TYPE_GROUND,   CATEGORY_PHYSICAL,  18,  40,   3.2f,   0.0f, 180.0f, 25.0f, false, exec_earthquake };
    s_move_defs[MOVE_SEISMIC_TOSS]    = { MOVE_SEISMIC_TOSS,    "Seismic Toss",    TYPE_FIGHTING, CATEGORY_PHYSICAL,  34,  36,   2.8f,   0.0f, 220.0f, 25.0f, false, exec_seismic_toss };
    s_move_defs[MOVE_AGILITY]         = { MOVE_AGILITY,         "Agility",         TYPE_PSYCHIC,  CATEGORY_STATUS,     0,  20,   4.0f,   0.0f, 999.0f, 15.0f, false, exec_agility };
    s_move_defs[MOVE_DOUBLE_TEAM]     = { MOVE_DOUBLE_TEAM,     "Double Team",     TYPE_NORMAL,   CATEGORY_STATUS,     0,  25,   4.5f,   0.0f, 999.0f, 15.0f, false, exec_double_team };
    s_move_defs[MOVE_PROTECT]         = { MOVE_PROTECT,         "Protect",         TYPE_NORMAL,   CATEGORY_STATUS,     0,  25,   4.0f,   0.0f, 150.0f, 25.0f, false, exec_protect };
    s_move_defs[MOVE_AURA_SPHERE]     = { MOVE_AURA_SPHERE,     "Aura Sphere",     TYPE_FIGHTING, CATEGORY_SPECIAL,   24,  25,   1.6f,  60.0f, 600.0f, 20.0f, false, exec_aura_sphere };
    s_move_defs[MOVE_FORCE_PALM]      = { MOVE_FORCE_PALM,      "Force Palm",      TYPE_FIGHTING, CATEGORY_PHYSICAL,  20,  18,   1.0f,   0.0f, 110.0f, 15.0f, false, exec_force_palm };
    s_move_defs[MOVE_CLOSE_COMBAT]    = { MOVE_CLOSE_COMBAT,    "Close Combat",    TYPE_FIGHTING, CATEGORY_PHYSICAL,  32,  38,   3.4f,   0.0f, 130.0f, 25.0f, false, exec_close_combat };
    s_move_defs[MOVE_BONE_RUSH]       = { MOVE_BONE_RUSH,       "Bone Rush",       TYPE_GROUND,   CATEGORY_PHYSICAL,  22,  24,   1.8f,  60.0f, 480.0f, 20.0f, false, exec_bone_rush };
    s_move_defs[MOVE_EXTREME_SPEED]   = { MOVE_EXTREME_SPEED,   "Extreme Speed",   TYPE_NORMAL,   CATEGORY_PHYSICAL,  22,  28,   2.5f,  80.0f, 520.0f, 20.0f, false, exec_extreme_speed };
    s_move_defs[MOVE_FLASH_CANNON]    = { MOVE_FLASH_CANNON,    "Flash Cannon",    TYPE_STEEL,    CATEGORY_SPECIAL,   26,  30,   2.2f,  80.0f, 550.0f, 20.0f, false, exec_flash_cannon };
    s_move_defs[MOVE_DRAGON_PULSE]    = { MOVE_DRAGON_PULSE,    "Dragon Pulse",    TYPE_DRAGON,   CATEGORY_SPECIAL,   22,  26,   2.0f,  80.0f, 620.0f, 20.0f, false, exec_dragon_pulse };
    s_move_defs[MOVE_THUNDER]         = { MOVE_THUNDER,         "Thunder",         TYPE_ELECTRIC, CATEGORY_SPECIAL,   36,  40,   3.5f,  60.0f, 580.0f, 25.0f, true,  exec_thunder };
    s_move_defs[MOVE_WATER_SHURIKEN]  = { MOVE_WATER_SHURIKEN,  "Water Shuriken",  TYPE_WATER,    CATEGORY_SPECIAL,   10,  22,   1.4f,  80.0f, 560.0f, 20.0f, false, exec_water_shuriken };
    s_move_defs[MOVE_NIGHT_SLASH]     = { MOVE_NIGHT_SLASH,     "Night Slash",     TYPE_DARK,     CATEGORY_PHYSICAL,  26,  20,   1.2f,   0.0f, 220.0f, 15.0f, false, exec_night_slash };
    s_move_defs[MOVE_HYDRO_PUMP]      = { MOVE_HYDRO_PUMP,      "Hydro Pump",      TYPE_WATER,    CATEGORY_SPECIAL,   32,  38,   3.0f, 120.0f, 600.0f, 25.0f, false, exec_hydro_pump };
    s_move_defs[MOVE_AERIAL_ACE]      = { MOVE_AERIAL_ACE,      "Aerial Ace",      TYPE_FLYING,   CATEGORY_PHYSICAL,  22,  26,   1.8f,  80.0f, 440.0f, 20.0f, false, exec_aerial_ace };

    s_registry_initialized = true;
}

const MoveDef& MoveRegistry::get(MoveId id) {
    if (!s_registry_initialized) init();
    if (id < 0 || id >= MOVE_COUNT) return s_move_defs[MOVE_NONE];
    return s_move_defs[id];
}

bool MoveRegistry::execute(MoveId id, Fighter* user, Fighter* target, float target_x, float target_y) {
    if (!user || user->hitstun_timer > 0.0f || user->dash_timer > 0.0f) return false;
    const MoveDef& def = get(id);
    if (!def.execute) return false;

    // Special validation for targeted moves (e.g. Thunder, Scorching Sands range limit)
    if (id == MOVE_THUNDER || id == MOVE_SCORCHING_SANDS) {
        float cos_a = cosf(user->aim_angle);
        float sin_a = sinf(user->aim_angle);
        float tdx = (target_x != 0.0f || target_y != 0.0f) ? (target_x - user->x) : (target ? (target->x - user->x) : cos_a * 260.0f);
        float tdy = (target_x != 0.0f || target_y != 0.0f) ? (target_y - user->y) : (target ? (target->y - user->y) : sin_a * 260.0f);
        float tdist = sqrtf(tdx * tdx + tdy * tdy);
        if (tdist > def.max_range) {
            user->stamina_flash_timer = 0.35f;
            return false; // Out of range! No stamina consumed, no cooldown triggered
        }
    }

    // Special handling for 2-stage Iron Tail
    if (id == MOVE_IRON_TAIL) {
        bool is_primed = (user->primed_move == MOVE_IRON_TAIL || user->iron_tail_primed);
        int cost = is_primed ? 25 : def.stamina_cost;
        if (user->stamina < (float)cost) {
            user->stamina_flash_timer = 0.35f;
            return false;
        }

        // Locate ability slot
        int slot_idx = -1;
        for (int i = 0; i < 4; ++i) {
            if (user->abilities[i].move_id == MOVE_IRON_TAIL) {
                slot_idx = i;
                break;
            }
        }

        if (!is_primed) {
            // Stage 1: Prime stance
            if (slot_idx >= 0 && user->abilities[slot_idx].cooldown_timer > 0.0f) return false;
            user->stamina -= (float)cost;
            def.execute(user, target, target_x, target_y);
            return true;
        } else {
            // Stage 2: Release strike
            user->stamina -= (float)cost;
            if (slot_idx >= 0) user->abilities[slot_idx].cooldown_timer = def.cooldown;
            CombatLogger::record_move_use(user->id, MOVE_IRON_TAIL);
            def.execute(user, target, target_x, target_y);
            return true;
        }
    }

    // Standard Move Execution
    int slot_idx = -1;
    for (int i = 0; i < 4; ++i) {
        if (user->abilities[i].move_id == id) {
            slot_idx = i;
            break;
        }
    }

    if (slot_idx >= 0 && user->abilities[slot_idx].cooldown_timer > 0.0f) return false;
    if (user->stamina < (float)def.stamina_cost) {
        user->stamina_flash_timer = 0.35f;
        return false;
    }

    user->stamina -= (float)def.stamina_cost;
    if (slot_idx >= 0) user->abilities[slot_idx].cooldown_timer = def.cooldown;

    CombatLogger::record_move_use(user->id, id);
    def.execute(user, target, target_x, target_y);
    return true;
}

void MoveRegistry::update_active_move(Fighter* user, float dt, Fighter* opponent) {
    if (!user) return;

    // 1. Primed stance decay (e.g. Iron Tail Stage 1 stance)
    if (user->primed_move != MOVE_NONE) {
        user->primed_timer -= dt;
        user->iron_tail_timer = user->primed_timer;
        if (user->primed_timer <= 0.0f) {
            user->primed_move = MOVE_NONE;
            user->primed_timer = 0.0f;
            user->iron_tail_primed = false;
            user->iron_tail_timer = 0.0f;
            g_projectiles.spawn_particles(user->x, user->y, 8, 180, 205, 230, 70.0f);
        }
    }

    // 2. Active move lifecycle state machine
    if (user->attack_timer > 0.0f) {
        if (user->active_move == MOVE_WING_ATTACK) {
            user->pmd_dir = angle_to_pmd_dir(user->flight_angle);
            if (user->move_phase == 0) {
                // Phase 0: Takeoff windup telegraph (0.20s)
                user->move_phase_timer -= dt;
                user->wing_attack_windup = user->move_phase_timer;
                user->vx = 0.0f;
                user->vy = 0.0f;

                if (rand() % 2 == 0) {
                    float fx = cosf(user->flight_angle);
                    float fy = sinf(user->flight_angle);
                    g_projectiles.spawn_particles(user->x + fx * 30.0f + (rand() % 20 - 10),
                                                   user->y + fy * 30.0f + (rand() % 20 - 10),
                                                   1, 230, 245, 255, 60.0f);
                }

                if (user->move_phase_timer <= 0.0f) {
                    // Transition to Phase 1: High-speed flight rush along locked flight vector!
                    user->move_phase = 1;
                    user->move_phase_timer = 0.48f;
                    user->wing_attack_rush = 0.48f;
                    float fx = cosf(user->flight_angle);
                    float fy = sinf(user->flight_angle);
                    user->vx = fx * 820.0f;
                    user->vy = fy * 820.0f;

                    g_projectiles.spawn(user->id, PROJ_WING_ATTACK, user->x + fx * 44.0f, user->y + fy * 44.0f,
                                        fx * 820.0f, fy * 820.0f, 34, 0.46f,
                                        0.0f, 0.0f, TYPE_FLYING, CATEGORY_PHYSICAL, 24, &user->base_stats, user->type1, user->type2);

                    // Sonic boom shockwaves & screen rumble
                    g_projectiles.spawn_shockwave(user->x, user->y, 115.0f, 0.28f, 255, 255, 255, 7);
                    g_projectiles.spawn_shockwave(user->x, user->y, 75.0f, 0.22f, 186, 230, 253, 4);
                    g_projectiles.screen_shake = 0.22f;
                    g_projectiles.spawn_particles(user->x, user->y, 28, 240, 250, 255, 260.0f);
                }
            } else if (user->move_phase == 1) {
                // Phase 1: Supersonic flight rush
                user->move_phase_timer -= dt;
                user->wing_attack_rush = user->move_phase_timer;
                user->vx = cosf(user->flight_angle) * 820.0f;
                user->vy = sinf(user->flight_angle) * 820.0f;

                if (rand() % 2 == 0) {
                    g_projectiles.spawn_particles(user->x - user->vx * dt * 0.3f, user->y - user->vy * dt * 0.3f, 2, 200, 235, 255, 90.0f);
                }

                user->ghost_timer += dt;
                if (user->ghost_timer >= 0.04f) {
                    user->ghost_timer = 0.0f;
                    for (int i = 0; i < 8; ++i) {
                        if (user->dash_ghosts[i].alpha <= 0.0f) {
                            user->dash_ghosts[i].x = user->x;
                            user->dash_ghosts[i].y = user->y;
                            user->dash_ghosts[i].pmd_dir = user->pmd_dir;
                            user->dash_ghosts[i].anim_frame = user->anim_frame;
                            user->dash_ghosts[i].alpha = 160.0f;
                            break;
                        }
                    }
                }

                if (user->move_phase_timer <= 0.0f) {
                    user->is_flying = false;
                    user->vx = 0.0f;
                    user->vy = 0.0f;
                }
            }
        } else if (user->active_move == MOVE_IRON_TAIL) {
            // Jump-spin aerial parabolic arc and 360-degree rotation!
            if (user->attack_duration > 0.001f) {
                float prog = 1.0f - (user->attack_timer / user->attack_duration);
                if (prog < 0.0f) prog = 0.0f;
                if (prog > 1.0f) prog = 1.0f;

                // Parabolic vertical elevation (apex 32px at prog=0.5)
                user->z = 4.0f * 32.0f * prog * (1.0f - prog);
                user->iron_tail_jump_z = user->z;

                // 360-degree somersault spin
                int base_dir = angle_to_pmd_dir(user->aim_angle);
                int spin_step = (int)(prog * 8.0f) % 8;
                user->pmd_dir = (base_dir + spin_step) % 8;

                if (rand() % 2 == 0) {
                    g_projectiles.spawn_particles(user->x, user->y - user->z, 1, 230, 245, 255, 60.0f);
                }
            }
        } else if (user->active_move == MOVE_SEISMIC_TOSS) {
            if (user->move_phase == 1) {
                // Phase 1: Stratosphere Globe Loop & Earth Slam!
                if (user->attack_duration > 0.001f) {
                    float prog = 1.0f - (user->attack_timer / user->attack_duration);
                    if (prog < 0.0f) prog = 0.0f;
                    if (prog > 1.0f) prog = 1.0f;

                    // Ascent into sky up to 160px and plummet back down
                    user->z = sinf(prog * 3.14159f) * 160.0f;
                    user->is_flying = true;

                    // Spinning loop trajectory around loop center
                    user->flight_angle += 8.0f * dt;
                    user->pmd_dir = angle_to_pmd_dir(user->flight_angle);
                    user->x = user->seismic_toss_center_x + cosf(user->flight_angle) * 55.0f;
                    user->y = user->seismic_toss_center_y + sinf(user->flight_angle) * 55.0f;

                    // Lock grabbed opponent to user position & altitude
                    if (opponent && opponent->is_grabbed && opponent->grabbed_by_id == user->id) {
                        opponent->x = user->x;
                        opponent->y = user->y;
                        opponent->z = user->z;
                        opponent->state = STATE_HITSTUN;
                        opponent->vx = 0.0f;
                        opponent->vy = 0.0f;
                    }

                    // Fiery atmospheric friction sparks during loop
                    if (rand() % 2 == 0) {
                        g_projectiles.spawn_particles(user->x, user->y - user->z, 2, 249, 115, 22, 120.0f);
                    }

                    // On impact slam at end of flight trajectory
                    if (user->attack_timer <= dt || prog >= 0.98f) {
                        if (opponent && opponent->is_grabbed && opponent->grabbed_by_id == user->id) {
                            opponent->is_grabbed = false;
                            opponent->grabbed_by_id = -1;
                            opponent->invuln_timer = 0.0f;
                            opponent->z = 0.0f;

                            // Calculate and deal massive slam damage (100 BP Fighting physical)
                            float mult = 1.0f;
                            int slam_damage = TypeChart::calculate_damage(
                                100, CATEGORY_PHYSICAL, TYPE_FIGHTING,
                                user->base_stats, user->type1, user->type2,
                                opponent->base_stats, opponent->type1, opponent->type2,
                                user->stage_atk, opponent->stage_def,
                                (user->status == STATUS_BURN), user->blaze_active,
                                &mult
                            );
                            float kx = cosf(user->aim_angle) * 320.0f;
                            float ky = sinf(user->aim_angle) * 320.0f;
                            opponent->take_damage(slam_damage, kx, ky, mult);
                            opponent->hitstun_timer = 0.55f;
                            opponent->state = STATE_HITSTUN;

                            // Impact crater & shockwaves
                            g_arena.add_decal(DECAL_IMPACT_CRATER, opponent->x, opponent->y, 44.0f, 0.0f, 30, 25, 20, 240);
                            g_projectiles.spawn_shockwave(opponent->x, opponent->y, 115.0f, 0.35f, 248, 113, 113, 8);
                            g_projectiles.spawn_shockwave(opponent->x, opponent->y, 75.0f, 0.22f, 255, 255, 255, 6);
                            g_projectiles.spawn_particles(opponent->x, opponent->y, 35, 249, 115, 22, 260.0f);
                            g_projectiles.screen_shake = 0.35f;
                            g_projectiles.trigger_hitstop(0.080f, opponent->x, opponent->y);
                            CombatLogger::record_move_hit(user->id, MOVE_SEISMIC_TOSS, slam_damage, opponent->hp <= 0);
                        }
                    }
                }
            }
        } else if (user->active_move == MOVE_THUNDERBOLT) {
            // Keep beam anchor locked to Pikachu's cheeks
            float cos_a = cosf(user->aim_angle);
            float sin_a = sinf(user->aim_angle);
            float ox = user->x + cos_a * 24.0f;
            float oy = user->y + sin_a * 24.0f;

            for (int i = 0; i < MAX_PROJ; ++i) {
                Projectile& p = g_projectiles.pool[i];
                if (p.active && p.owner_id == user->id && p.type == PROJ_THUNDERBOLT) {
                    p.target_x = ox;
                    p.target_y = oy;
                    break;
                }
            }

            // Continuous crackling cheek & body aura motes
            if (rand() % 2 == 0) {
                g_projectiles.spawn_electric_sparks(ox, oy, 12.0f, 2, 120.0f);
            }
        } else if (user->active_move == MOVE_FLAMETHROWER) {
            // Continuous stream emission over attack duration (every 0.036s = ~2 frames)
            user->move_phase_timer += dt;
            if (user->move_phase_timer >= 0.036f) {
                user->move_phase_timer -= 0.036f;
                float cos_a = cosf(user->aim_angle);
                float sin_a = sinf(user->aim_angle);
                float spawn_x = user->x + cos_a * 28.0f;
                float spawn_y = user->y + sin_a * 28.0f;

                // Slight random angular spray (+/- 0.07 rad = ~4 degrees)
                float jitter_angle = user->aim_angle + ((float)(rand() % 100 - 50) / 50.0f) * 0.07f;
                float puff_speed = 580.0f + (float)(rand() % 60 - 30);

                g_projectiles.spawn(user->id, PROJ_FLAMETHROWER,
                                    spawn_x, spawn_y,
                                    cosf(jitter_angle) * puff_speed, sinf(jitter_angle) * puff_speed,
                                    6, 0.34f, 0.0f, 0.0f,
                                    TYPE_FIRE, CATEGORY_SPECIAL, 6, &user->base_stats, user->type1, user->type2, MOVE_FLAMETHROWER);

                // Continuous muzzle flame motes
                if (rand() % 2 == 0) {
                    g_projectiles.spawn_embers(spawn_x, spawn_y, 6.0f, 2, 80.0f);
                }
            }
        } else if (user->active_move == MOVE_WATER_SHURIKEN) {
            user->move_phase_timer += dt;
            float cos_a = cosf(user->aim_angle);
            float sin_a = sinf(user->aim_angle);
            float spd = 720.0f;
            float sx = user->x + cos_a * 24.0f;
            float sy = user->y + sin_a * 24.0f;

            if (user->move_phase == 1 && user->move_phase_timer >= 0.09f) {
                user->move_phase = 2; // Fire shuriken 2
                float ang = user->aim_angle - 0.08f;
                g_projectiles.spawn(user->id, PROJ_WATER_SHURIKEN, sx, sy,
                                    cosf(ang) * spd, sinf(ang) * spd,
                                    10, 0.75f, 0.0f, 0.0f,
                                    TYPE_WATER, CATEGORY_SPECIAL, 10,
                                    &user->base_stats, user->type1, user->type2, MOVE_WATER_SHURIKEN);
                g_projectiles.spawn_shockwave(sx, sy, 32.0f, 0.14f, 56, 189, 248, 2);
                g_projectiles.spawn_particles(sx, sy, 6, 56, 189, 248, 130.0f);
            } else if (user->move_phase == 2 && user->move_phase_timer >= 0.18f) {
                user->move_phase = 3; // Fire shuriken 3
                float ang = user->aim_angle + 0.08f;
                g_projectiles.spawn(user->id, PROJ_WATER_SHURIKEN, sx, sy,
                                    cosf(ang) * spd, sinf(ang) * spd,
                                    10, 0.75f, 0.0f, 0.0f,
                                    TYPE_WATER, CATEGORY_SPECIAL, 10,
                                    &user->base_stats, user->type1, user->type2, MOVE_WATER_SHURIKEN);
                g_projectiles.spawn_shockwave(sx, sy, 42.0f, 0.16f, 56, 189, 248, 3);
                g_projectiles.spawn_particles(sx, sy, 8, 56, 189, 248, 140.0f);
            }
        } else if (user->active_move == MOVE_NIGHT_SLASH) {
            user->move_phase_timer += dt;
            float cos_a = cosf(user->aim_angle);
            float sin_a = sinf(user->aim_angle);
            float cur_spd = 480.0f - user->move_phase_timer * 800.0f;
            if (cur_spd < 200.0f) cur_spd = 200.0f;
            user->vx = cos_a * cur_spd;
            user->vy = sin_a * cur_spd;
            if (rand() % 2 == 0) {
                g_projectiles.spawn_dust(user->x, user->y + user->radius * 0.4f, 1, 30, 27, 75, 45.0f, 4.0f);
            }
        } else if (user->active_move == MOVE_HYDRO_PUMP) {
            user->move_phase_timer += dt;
            float cos_a = cosf(user->aim_angle);
            float sin_a = sinf(user->aim_angle);
            user->vx = -cos_a * 140.0f;
            user->vy = -sin_a * 140.0f;
            float sx = user->x + cos_a * 28.0f;
            float sy = user->y + sin_a * 28.0f;
            if (rand() % 2 == 0) {
                g_projectiles.spawn_aura_motes(sx, sy, 8.0f, 2, 90.0f, 56, 189, 248);
                g_projectiles.spawn_particles(sx, sy, 2, 224, 242, 254, 110.0f);
            }
        } else if (user->active_move == MOVE_AERIAL_ACE) {
            user->move_phase_timer += dt;
            if (user->attack_duration > 0.001f) {
                float prog = user->move_phase_timer / user->attack_duration;
                if (prog < 0.0f) prog = 0.0f;
                if (prog > 1.0f) prog = 1.0f;
                user->z = sinf(prog * 3.14159f) * 35.0f; // Aerodynamic sonic leap above ground hazards
                if (rand() % 2 == 0) {
                    g_projectiles.spawn_dust(user->x, user->y - user->z, 1, 240, 245, 255, 60.0f, 5.0f);
                }
            }
        }
    }

    if (user->attack_timer <= 0.0f) {
        if (opponent && opponent->is_grabbed && opponent->grabbed_by_id == user->id) {
            opponent->is_grabbed = false;
            opponent->grabbed_by_id = -1;
            opponent->z = 0.0f;
            if (opponent->state == STATE_HITSTUN && opponent->hitstun_timer <= 0.0f) {
                opponent->state = STATE_IDLE;
            }
        }
        user->state = STATE_IDLE;
        user->vx = 0.0f;
        user->vy = 0.0f;
        user->attack_duration = 0.0f;
        user->active_move = MOVE_NONE;
        user->move_phase = 0;
        user->move_phase_timer = 0.0f;
        user->z = 0.0f;
        user->iron_tail_jump_z = 0.0f;
        user->iron_tail_striking = false;
        user->is_flying = false;
        user->wing_attack_windup = 0.0f;
        user->wing_attack_rush = 0.0f;
    }
}
