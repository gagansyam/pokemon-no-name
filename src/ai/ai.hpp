#pragma once

#include "fighter.hpp"

enum AIState {
    AI_STATE_SPACING,
    AI_STATE_STRAFE,
    AI_STATE_APPROACH,
    AI_STATE_ATTACK,
    AI_STATE_EVADE,
    AI_STATE_RETREAT
};

enum AIStance {
    AI_STANCE_NEUTRAL,   // Standard probing & circle-strafing (200px - 380px)
    AI_STANCE_RUSHDOWN,  // Aggressive gap-close & point-blank infighting (40px - 140px)
    AI_STANCE_RECHARGE   // Tactical retreat & stamina recovery (420px+)
};

class BattleAI {
public:
    AIMode mode = AI_MODE_BOSS;
    AIState state = AI_STATE_SPACING;
    AIStance stance = AI_STANCE_NEUTRAL;

    float stance_timer = 0.0f;
    float stance_decision_timer = 0.0f;
    float decision_timer = 0.0f;
    float attack_cooldown_timer = 0.0f;
    float reaction_timer = 0.0f;
    float strafe_timer = 0.0f;
    float strafe_direction = 1.0f; // +1.0 = clockwise, -1.0 = counter-clockwise
    float evade_lock_timer = 0.0f;
    float ai_dodge_cooldown_timer = 0.0f;

    // Incoming threat tracking
    bool threat_detected = false;
    float threat_dodge_x = 0.0f;
    float threat_dodge_y = 0.0f;

    // Human telegraph reaction lag & relaxed cadence for Casual mode
    float human_reaction_timer = 0.0f;
    float casual_cadence_timer = 0.0f;

    // Double Team decoy illusion tracking
    int ai_target_clone_idx = -1; // -1 = real player, 0..MAX_CLONES-1 = clone index
    float ai_retarget_timer = 0.0f;

    // Hit-Confirm true combo state machine
    bool hit_confirm_active = false;
    int combo_stage = 0;
    float combo_timer = 0.0f;
    MoveId combo_last_move = MOVE_NONE;

    void init(AIMode ai_mode = AI_MODE_BOSS);
    void update(Fighter* cpu, Fighter* player, float dt);


    // Whiff Punish & Hit-Confirm Combo Triggers
    bool evaluate_whiff_punish(Fighter* cpu, Fighter* player, float dist);
    bool execute_hit_confirm_combo(Fighter* cpu, Fighter* player, float dist, float target_x, float target_y);

private:
    void update_tactical_stance(Fighter* cpu, Fighter* player, float dt, float dist);
    void evaluate_threats(Fighter* cpu, Fighter* player, float dt);
    void handle_movement_and_spacing(Fighter* cpu, Fighter* player, float dt, float dist, float dx, float dy);
    void select_attack(Fighter* cpu, Fighter* player, float dist, float target_x, float target_y);
    void apply_arena_steering(Fighter* cpu, float& out_dx, float& out_dy);
    void apply_hazard_avoidance(Fighter* cpu, float& out_dx, float& out_dy);
};

extern BattleAI g_ai_p1;
extern BattleAI g_ai_p2;
extern BattleAI& g_ai;

