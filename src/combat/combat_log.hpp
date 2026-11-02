#pragma once

#include <string>
#include <vector>
#include "types.hpp"
#include "moves.hpp"

struct MoveLogEntry {
    MoveId move_id = MOVE_NONE;
    int uses = 0;              // Times ability was activated / cast
    int hits = 0;              // Total individual hits connected
    int successful_casts = 0;  // Casts where at least 1 hit landed
    int total_damage = 0;      // Cumulative damage dealt by this move
    int max_single_hit = 0;    // Highest single impact
    bool is_finisher = false;  // Did this move land the decisive K.O. blow?
};

struct FighterCombatLog {
    int fighter_id = 0;
    PokemonType pokemon_type = PKMN_PIKACHU;
    bool is_player = true;
    AIMode ai_mode = AI_MODE_BOSS;

    int total_damage_dealt = 0;
    int total_damage_taken = 0;
    int total_casts = 0;
    int total_hits = 0;
    int wall_splat_damage = 0;
    int parries_performed = 0;
    int clones_spawned = 0;
    int clones_popped = 0;

    MoveLogEntry moves[8];
    int move_count = 0;

    MoveLogEntry* find_or_add_move(MoveId mid);
    const MoveLogEntry* find_move(MoveId mid) const;

    float get_accuracy() const {
        if (total_casts <= 0) return 0.0f;
        int successful = 0;
        for (int i = 0; i < move_count; ++i) {
            successful += moves[i].successful_casts;
        }
        float acc = ((float)successful / (float)total_casts) * 100.0f;
        return (acc > 100.0f) ? 100.0f : acc;
    }
};

struct MatchLog {
    bool valid = false;
    MatchType match_type = MATCH_TYPE_PLAYER_VS_CPU;
    AimMode aim_mode = AIM_MODE_FREE;
    bool dynamic_cam = true;
    float match_duration = 0.0f;
    int winner_id = -1; // 0 = P1, 1 = P2, -1 = In progress / draw
    MoveId finisher_move = MOVE_NONE;
    char finisher_name[32] = {0};

    FighterCombatLog p1;
    FighterCombatLog p2;
};

class CombatLogger {
public:
    static void init();
    static void start_match(MatchType match_type,
                            PokemonType p1_type, PokemonType p2_type,
                            const MoveId p1_moves[4], const MoveId p2_moves[4],
                            AIMode p1_ai, AIMode p2_ai,
                            AimMode aim_mode, bool dynamic_cam);

    static void update(float dt);
    static void record_move_use(int fighter_id, MoveId move_id);
    static void record_move_hit(int attacker_id, MoveId move_id, int damage, bool is_ko = false);
    static void record_wall_splat(int attacker_id, int damage, bool is_ko = false);
    static void record_parry(int defender_id);
    static void record_clone_spawn(int fighter_id, int count = 4);
    static void record_clone_pop(int fighter_id);
    static void end_match(int winner_id, MoveId finisher = MOVE_NONE);

    static const MatchLog& get_active_log();
    static const MatchLog& get_last_completed_log();
    static bool has_completed_log();
    static bool is_match_active();

    static void dump_to_file(const char* filepath = "battle_log.txt");
    static void format_log(const MatchLog& log, char* buffer, size_t max_len);
};
