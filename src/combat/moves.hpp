#pragma once

#include <SDL2/SDL.h>
#include "types.hpp"

class Fighter;

enum MoveId {
    MOVE_NONE = 0,
    MOVE_QUICK_ATTACK,
    MOVE_THUNDERBOLT,
    MOVE_ELECTRO_BALL,
    MOVE_IRON_TAIL,
    MOVE_SCRATCH,
    MOVE_FLAMETHROWER,
    MOVE_FIRE_SPIN,
    MOVE_WING_ATTACK,
    MOVE_SCORCHING_SANDS,
    MOVE_DRAGON_CLAW,
    MOVE_VOLT_TACKLE,
    MOVE_DRAGON_RAGE,
    MOVE_EARTHQUAKE,
    MOVE_SEISMIC_TOSS,
    MOVE_AGILITY,
    MOVE_DOUBLE_TEAM,
    MOVE_PROTECT,
    MOVE_AURA_SPHERE,
    MOVE_FORCE_PALM,
    MOVE_CLOSE_COMBAT,
    MOVE_BONE_RUSH,
    MOVE_EXTREME_SPEED,
    MOVE_FLASH_CANNON,
    MOVE_DRAGON_PULSE,
    MOVE_THUNDER,
    MOVE_WATER_SHURIKEN,
    MOVE_NIGHT_SLASH,
    MOVE_HYDRO_PUMP,
    MOVE_AERIAL_ACE,
    MOVE_COUNT
};

struct MoveDef {
    MoveId id = MOVE_NONE;
    const char* name = "";
    ElementType element_type = TYPE_NORMAL;
    MoveCategory category = CATEGORY_PHYSICAL;
    int base_power = 40;
    int stamina_cost = 20;
    float cooldown = 1.0f;
    float min_range = 0.0f;
    float max_range = 300.0f;
    float ai_stamina_reserve = 15.0f; // Minimum stamina AI retains after use
    bool is_targeted = false;        // Uses mouse/target coordinate (e.g. Thunderbolt, Fire Spin)
    void (*execute)(Fighter* user, Fighter* target, float target_x, float target_y) = nullptr;
};

class MoveRegistry {
public:
    static void init();
    static const MoveDef& get(MoveId id);
    static bool execute(MoveId id, Fighter* user, Fighter* target, float target_x = 0.0f, float target_y = 0.0f);
    static void update_active_move(Fighter* user, float dt, Fighter* opponent = nullptr);
};
