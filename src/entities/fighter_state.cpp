#include "fighter.hpp"
#include "arena.hpp"
#include "projectile.hpp"
#include "sprite_manifest.hpp"
#include "combat_log.hpp"
#include <cmath>
#include <cstdlib>

int angle_to_pmd_dir(float rad) {
    float deg = rad * 180.0f / 3.14159265f;
    int sector = (int)floorf((90.0f - deg + 22.5f) / 45.0f);
    return (sector % 8 + 8) % 8;
}

void Fighter::equip_move(int slot, MoveId move_id) {
    if (slot < 0 || slot >= 4) return;
    const MoveDef& def = MoveRegistry::get(move_id);
    abilities[slot].move_id = move_id;
    abilities[slot].name = def.name;
    abilities[slot].stamina_cost = def.stamina_cost;
    abilities[slot].cooldown_max = def.cooldown;
    abilities[slot].cooldown_timer = 0.0f;
}

bool Fighter::load_assets(SDL_Renderer* ren, PokemonType pkmn_type) {
    type = pkmn_type;
    if (type == PKMN_PIKACHU) {
        fly_sheet.destroy();
        if (!walk_sheet.load(ren, META_PIKACHU_WALK.filepath, META_PIKACHU_WALK.frame_w, META_PIKACHU_WALK.frame_h,
                             META_PIKACHU_WALK.anchor_x, META_PIKACHU_WALK.anchor_y)) return false;
        if (!attack_sheet.load(ren, META_PIKACHU_ATTACK.filepath, META_PIKACHU_ATTACK.frame_w, META_PIKACHU_ATTACK.frame_h,
                               META_PIKACHU_ATTACK.anchor_x, META_PIKACHU_ATTACK.anchor_y)) return false;
        if (!iron_tail_walk_sheet.load(ren, META_PIKACHU_IRON_TAIL_WALK.filepath, META_PIKACHU_IRON_TAIL_WALK.frame_w, META_PIKACHU_IRON_TAIL_WALK.frame_h,
                                       META_PIKACHU_IRON_TAIL_WALK.anchor_x, META_PIKACHU_IRON_TAIL_WALK.anchor_y)) return false;
        if (!iron_tail_attack_sheet.load(ren, META_PIKACHU_IRON_TAIL_ATTACK.filepath, META_PIKACHU_IRON_TAIL_ATTACK.frame_w, META_PIKACHU_IRON_TAIL_ATTACK.frame_h,
                                         META_PIKACHU_IRON_TAIL_ATTACK.anchor_x, META_PIKACHU_IRON_TAIL_ATTACK.anchor_y)) return false;
    } else if (type == PKMN_CHARIZARD) {
        iron_tail_walk_sheet.destroy();
        iron_tail_attack_sheet.destroy();
        if (!walk_sheet.load(ren, META_CHARIZARD_WALK.filepath, META_CHARIZARD_WALK.frame_w, META_CHARIZARD_WALK.frame_h,
                             META_CHARIZARD_WALK.anchor_x, META_CHARIZARD_WALK.anchor_y)) return false;
        if (!attack_sheet.load(ren, META_CHARIZARD_ATTACK.filepath, META_CHARIZARD_ATTACK.frame_w, META_CHARIZARD_ATTACK.frame_h,
                               META_CHARIZARD_ATTACK.anchor_x, META_CHARIZARD_ATTACK.anchor_y)) return false;
        if (!fly_sheet.load(ren, META_CHARIZARD_FLY.filepath, META_CHARIZARD_FLY.frame_w, META_CHARIZARD_FLY.frame_h,
                            META_CHARIZARD_FLY.anchor_x, META_CHARIZARD_FLY.anchor_y)) return false;
    } else if (type == PKMN_LUCARIO) {
        iron_tail_walk_sheet.destroy();
        iron_tail_attack_sheet.destroy();
        fly_sheet.destroy();
        shoot_sheet.destroy();
        if (!walk_sheet.load(ren, META_LUCARIO_WALK.filepath, META_LUCARIO_WALK.frame_w, META_LUCARIO_WALK.frame_h,
                             META_LUCARIO_WALK.anchor_x, META_LUCARIO_WALK.anchor_y)) return false;
        if (!attack_sheet.load(ren, META_LUCARIO_ATTACK.filepath, META_LUCARIO_ATTACK.frame_w, META_LUCARIO_ATTACK.frame_h,
                               META_LUCARIO_ATTACK.anchor_x, META_LUCARIO_ATTACK.anchor_y)) return false;
    } else if (type == PKMN_GRENINJA) {
        iron_tail_walk_sheet.destroy();
        iron_tail_attack_sheet.destroy();
        fly_sheet.destroy();
        if (!walk_sheet.load(ren, META_GRENINJA_WALK.filepath, META_GRENINJA_WALK.frame_w, META_GRENINJA_WALK.frame_h,
                             META_GRENINJA_WALK.anchor_x, META_GRENINJA_WALK.anchor_y)) return false;
        if (!attack_sheet.load(ren, META_GRENINJA_ATTACK.filepath, META_GRENINJA_ATTACK.frame_w, META_GRENINJA_ATTACK.frame_h,
                               META_GRENINJA_ATTACK.anchor_x, META_GRENINJA_ATTACK.anchor_y)) return false;
        if (!shoot_sheet.load(ren, META_GRENINJA_SHOOT.filepath, META_GRENINJA_SHOOT.frame_w, META_GRENINJA_SHOOT.frame_h,
                              META_GRENINJA_SHOOT.anchor_x, META_GRENINJA_SHOOT.anchor_y)) return false;
    }
    return true;
}

void Fighter::init(int fighter_id, PokemonType pkmn_type, float start_x, float start_y, bool dummy) {
    id = fighter_id;
    type = pkmn_type;
    is_dummy = dummy;
    state = STATE_IDLE;
    is_bench = false;
    tag_timer = 0.0f;

    x = start_x;
    y = start_y;
    z = 0.0f;
    iron_tail_jump_z = 0.0f;
    vx = 0;
    vy = 0;
    input_dx = 0;
    input_dy = 0;
    aim_angle = (id == 0) ? 0.0f : 3.14159f;
    pmd_dir = (id == 0) ? 2 : 6;

    dash_timer = 0;
    dash_cooldown_timer = 0;
    invuln_timer = 0;
    hitstun_timer = 0;
    attack_timer = 0;
    anim_timer = 0;
    anim_frame = 0;

    stamina = 100.0f;
    max_stamina = 100.0f;
    idle_regen_timer = 0.0f;
    ghost_timer = 0.0f;
    stamina_flash_timer = 0.0f;

    active_move = MOVE_NONE;
    move_phase = 0;
    move_phase_timer = 0.0f;
    flight_angle = 0.0f;
    primed_move = MOVE_NONE;
    primed_timer = 0.0f;
    is_flying = false;

    iron_tail_primed = false;
    iron_tail_striking = false;
    iron_tail_timer = 0.0f;
    wing_attack_windup = 0.0f;
    wing_attack_rush = 0.0f;
    agility_timer = 0.0f;
    is_grabbed = false;
    grabbed_by_id = -1;
    seismic_toss_timer = 0.0f;
    seismic_toss_center_x = 0.0f;
    seismic_toss_center_y = 0.0f;
    double_team_timer = 0.0f;
    clone_orbit_angle = 0.0f;
    clear_clones();

    protect_timer = 0.0f;
    protect_parry_window = 0.0f;
    shield_break_timer = 0.0f;

    // Status and dynamic stat stages
    status = STATUS_NONE;
    status_tick_timer = 0.0f;
    stage_atk = 0;
    stage_def = 0;
    stage_spa = 0;
    stage_spd = 0;

    blaze_active = false;
    blaze_aura_timer = 0.0f;

    for (int i = 0; i < 8; ++i) {
        dash_ghosts[i].alpha = 0.0f;
    }

    input_buffer = BufferedAction{};
    hit_cancel_timer = 0.0f;

    // Retrieve canonical stats and types from TypeChart
    TypeChart::get_species_types(type, type1, type2);
    base_stats = TypeChart::get_base_stats(type);

    hp = base_stats.hp;
    max_hp = base_stats.hp;
    display_lag_hp = (float)hp;

    radius = (type == PKMN_CHARIZARD) ? 28.0f : ((type == PKMN_GRENINJA) ? 24.0f : 23.0f);

    move_speed = 120.0f + (float)base_stats.speed * 1.74f;
    turn_traction = (base_stats.speed >= 100) ? 26.0f : ((base_stats.speed >= 70) ? 20.0f : 12.0f);
    dash_cooldown_max = 0.65f - ((float)base_stats.speed / 500.0f);
    if (dash_cooldown_max < 0.35f) dash_cooldown_max = 0.35f;

    // Default Moveset setup per species
    if (type == PKMN_PIKACHU) {
        movepool[0] = MOVE_QUICK_ATTACK;
        movepool[1] = MOVE_THUNDERBOLT;
        movepool[2] = MOVE_ELECTRO_BALL;
        movepool[3] = MOVE_IRON_TAIL;
        movepool[4] = MOVE_VOLT_TACKLE;
        movepool[5] = MOVE_AGILITY;
        movepool[6] = MOVE_DOUBLE_TEAM;
        movepool[7] = MOVE_PROTECT;
        movepool[8] = MOVE_THUNDER;
        movepool_count = 9;
        equip_move(0, MOVE_QUICK_ATTACK);
        equip_move(1, MOVE_THUNDERBOLT);
        equip_move(2, MOVE_THUNDER);
        equip_move(3, MOVE_IRON_TAIL);
    } else if (type == PKMN_CHARIZARD) {
        max_stamina = 120.0f;
        stamina = 120.0f;
        movepool[0] = MOVE_SCRATCH;
        movepool[1] = MOVE_FLAMETHROWER;
        movepool[2] = MOVE_FIRE_SPIN;
        movepool[3] = MOVE_WING_ATTACK;
        movepool[4] = MOVE_SCORCHING_SANDS;
        movepool[5] = MOVE_DRAGON_CLAW;
        movepool[6] = MOVE_EARTHQUAKE;
        movepool[7] = MOVE_SEISMIC_TOSS;
        movepool[8] = MOVE_PROTECT;
        movepool_count = 9;
        equip_move(0, MOVE_SCRATCH);
        equip_move(1, MOVE_FLAMETHROWER);
        equip_move(2, MOVE_FIRE_SPIN);
        equip_move(3, MOVE_WING_ATTACK);
    } else if (type == PKMN_LUCARIO) {
        // Lucario (PKMN_LUCARIO)
        movepool[0] = MOVE_FORCE_PALM;
        movepool[1] = MOVE_AURA_SPHERE;
        movepool[2] = MOVE_CLOSE_COMBAT;
        movepool[3] = MOVE_EXTREME_SPEED;
        movepool[4] = MOVE_BONE_RUSH;
        movepool[5] = MOVE_FLASH_CANNON;
        movepool[6] = MOVE_DRAGON_PULSE;
        movepool[7] = MOVE_PROTECT;
        movepool_count = 8;
        equip_move(0, MOVE_FORCE_PALM);
        equip_move(1, MOVE_AURA_SPHERE);
        equip_move(2, MOVE_CLOSE_COMBAT);
        equip_move(3, MOVE_EXTREME_SPEED);
    } else {
        // Greninja (PKMN_GRENINJA)
        movepool[0] = MOVE_WATER_SHURIKEN;
        movepool[1] = MOVE_NIGHT_SLASH;
        movepool[2] = MOVE_HYDRO_PUMP;
        movepool[3] = MOVE_AERIAL_ACE;
        movepool[4] = MOVE_DOUBLE_TEAM;
        movepool[5] = MOVE_PROTECT;
        movepool[6] = MOVE_QUICK_ATTACK;
        movepool_count = 7;
        equip_move(0, MOVE_WATER_SHURIKEN);
        equip_move(1, MOVE_NIGHT_SLASH);
        equip_move(2, MOVE_HYDRO_PUMP);
        equip_move(3, MOVE_AERIAL_ACE);
    }

    skid_timer = 0.0f;
    wall_splat_cooldown = 0.0f;
}

bool Fighter::can_act() const {
    if (is_bench) return false;
    if (state == STATE_TAG_RECALL || state == STATE_TAG_ENTRY) return false;
    if (is_grabbed) return false;
    if (shield_break_timer > 0.0f) return false;
    if (protect_timer > 0.0f) return false;
    if (hitstun_timer > 0.0f || dash_timer > 0.0f) return false;
    if (attack_timer > 0.0f && hit_cancel_timer <= 0.0f) return false;
    return true;
}

void Fighter::apply_status(StatusAilment new_status) {
    if (status != STATUS_NONE || hp <= 0) return;

    if (new_status == STATUS_POISON) {
        if (type1 == TYPE_POISON || type2 == TYPE_POISON || type1 == TYPE_STEEL || type2 == TYPE_STEEL) {
            return; // Immune
        }
        status = STATUS_POISON;
        status_tick_timer = 0.0f;
        g_projectiles.spawn_custom_text(x, y - 28, "POISONED!", 168, 85, 247, 50);
        g_projectiles.spawn_particles(x, y, 16, 168, 85, 247, 90.0f);
    } else if (new_status == STATUS_BURN) {
        if (type1 == TYPE_FIRE || type2 == TYPE_FIRE) {
            return; // Immune
        }
        status = STATUS_BURN;
        status_tick_timer = 0.0f;
        g_projectiles.spawn_custom_text(x, y - 28, "BURNED!", 249, 115, 22, 50);
        g_projectiles.spawn_particles(x, y, 16, 249, 115, 22, 90.0f);
    } else if (new_status == STATUS_PARALYSIS) {
        if (type1 == TYPE_ELECTRIC || type2 == TYPE_ELECTRIC) {
            return;
        }
        status = STATUS_PARALYSIS;
        g_projectiles.spawn_custom_text(x, y - 28, "PARALYZED!", 250, 204, 21, 50);
        g_projectiles.spawn_particles(x, y, 16, 250, 204, 21, 90.0f);
    }
}

void Fighter::clear_status() {
    status = STATUS_NONE;
    status_tick_timer = 0.0f;
}

void Fighter::modify_stat_stage(int stat_idx, int delta) {
    char buf[16];
    int* target_stage = &stage_atk;
    const char* stat_name = "ATK";

    if (stat_idx == 1) { target_stage = &stage_def; stat_name = "DEF"; }
    else if (stat_idx == 2) { target_stage = &stage_spa; stat_name = "SPA"; }
    else if (stat_idx == 3) { target_stage = &stage_spd; stat_name = "SPD"; }

    *target_stage += delta;
    if (*target_stage > 6) *target_stage = 6;
    if (*target_stage < -6) *target_stage = -6;

    if (delta > 0) {
        snprintf(buf, sizeof(buf), "%s+%d", stat_name, *target_stage);
        g_projectiles.spawn_custom_text(x, y - 28, buf, 234, 179, 8, 45);
    } else if (delta < 0) {
        snprintf(buf, sizeof(buf), "%s%d", stat_name, *target_stage);
        g_projectiles.spawn_custom_text(x, y - 28, buf, 96, 165, 250, 45);
    }
}

void Fighter::heal(int amount) {
    if (hp <= 0 || amount <= 0) return;
    hp += amount;
    if (hp > max_hp) hp = max_hp;
    display_lag_hp = (float)hp;

    char hbuf[16];
    snprintf(hbuf, sizeof(hbuf), "+%d", amount);
    g_projectiles.spawn_custom_text(x, y - 26, hbuf, 74, 222, 128, 55);
    g_projectiles.spawn_particles(x, y, 12, 74, 222, 128, 80.0f);
}

void Fighter::start_tag_recall() {
    state = STATE_TAG_RECALL;
    tag_timer = 0.35f;
    invuln_timer = 0.5f;
    vx = 0; vy = 0;

    // Reset stat stages upon recall
    stage_atk = 0;
    stage_def = 0;
    stage_spa = 0;
    stage_spd = 0;

    // Crimson recall light beam
    g_projectiles.spawn_shockwave(x, y, 60.0f, 0.25f, 239, 68, 68, 4);
    g_projectiles.spawn_custom_text(x, y - 28, "COME BACK!", 248, 113, 113, 40);
}

void Fighter::start_tag_entry() {
    state = STATE_TAG_ENTRY;
    tag_timer = 0.35f;
    invuln_timer = 0.4f;
    is_bench = false;
    vx = 0; vy = 0;

    // White/Cyan materialization burst
    g_projectiles.spawn_shockwave(x, y, 80.0f, 0.30f, 255, 255, 255, 5);
    g_projectiles.spawn_particles(x, y, 20, 255, 255, 255, 180.0f);

    char nbuf[32];
    snprintf(nbuf, sizeof(nbuf), "GO, %s!", TypeChart::get_species_name(type));
    g_projectiles.spawn_custom_text(x, y - 30, nbuf, 250, 204, 21, 50);
}

void Fighter::trigger_blaze() {
    if (blaze_active) return;
    blaze_active = true;
    blaze_aura_timer = 0.0f;

    // Climax Blaze ignition shockwaves
    g_projectiles.spawn_shockwave(x, y, 140.0f, 0.45f, 239, 68, 68, 7);
    g_projectiles.spawn_particles(x, y, 35, 245, 158, 11, 240.0f);
    g_projectiles.spawn_custom_text(x, y - 36, "BLAZE ACTIVATED!", 239, 68, 68, 70);
    g_projectiles.trigger_hitstop(0.12f, x, y);
}


void Fighter::take_damage(int amount, float kx, float ky, float effectiveness) {
    if (invuln_timer > 0.0f || is_bench) return;

    if (is_flying && wing_attack_rush > 0.0f) {
        amount = (int)roundf((float)amount * 0.65f);
        if (amount < 1) amount = 1;
        hp -= amount;
        if (hp < 0) hp = 0;
        invuln_timer = 0.20f;
        idle_regen_timer = 0.0f;
        bool crit = (amount >= 30);
        g_projectiles.spawn_damage_number(x, y, amount, crit, effectiveness);
        g_projectiles.spawn_particles(x, y, 16, 255, 230, 120, 200.0f);
        return;
    }

    hp -= amount;
    invuln_timer = 0.35f;
    hitstun_timer = 0.2f;
    state = STATE_HITSTUN;
    active_move = MOVE_NONE;
    move_phase = 0;
    move_phase_timer = 0.0f;
    is_grabbed = false;
    grabbed_by_id = -1;
    z = 0.0f;
    iron_tail_jump_z = 0.0f;
    iron_tail_striking = false;
    is_flying = false;
    wing_attack_windup = 0.0f;
    wing_attack_rush = 0.0f;
    double_team_timer = 0.0f;
    clear_clones();
    protect_timer = 0.0f;
    protect_parry_window = 0.0f;

    vx = kx;
    vy = ky;

    bool crit = (amount >= 30);
    g_projectiles.spawn_damage_number(x, y, amount, crit, effectiveness);
    g_projectiles.spawn_particles(x, y, 12, 255, 60, 60, 180.0f);

    idle_regen_timer = 0.0f;

    if (hp <= 0) {
        hp = 0;
    }
}

void Fighter::spawn_double_team() {
    double_team_timer = 4.2f;
    clone_orbit_angle = 0.0f;
    CombatLogger::record_clone_spawn(id, 4);

    g_projectiles.spawn_particles(x, y, 22, 250, 204, 21, 220.0f);
    g_projectiles.spawn_particles(x, y, 14, 56, 189, 248, 180.0f);
    g_projectiles.spawn_shockwave(x, y, 75.0f, 0.22f, 56, 189, 248, 5);

    for (int i = 0; i < MAX_CLONES; ++i) {
        clones[i].active = true;
        clones[i].scatter_angle = (float)i * 1.5707963f + 0.785398f;
        clones[i].scatter_dist = 135.0f;
        clones[i].target_offset_x = cosf(clones[i].scatter_angle) * clones[i].scatter_dist;
        clones[i].target_offset_y = sinf(clones[i].scatter_angle) * clones[i].scatter_dist;
        clones[i].current_offset_x = 0.0f;
        clones[i].current_offset_y = 0.0f;
        clones[i].x = x;
        clones[i].y = y;
        clones[i].pmd_dir = pmd_dir;
        clones[i].anim_frame = anim_frame;
        clones[i].alpha = 180.0f;
    }
}

void Fighter::clear_clones() {
    for (int i = 0; i < MAX_CLONES; ++i) {
        clones[i].active = false;
    }
}
