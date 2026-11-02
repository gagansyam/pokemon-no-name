#include "combat_log.hpp"
#include "moves.hpp"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <algorithm>

static MatchLog s_active_log;
static MatchLog s_last_completed_log;
static bool s_has_completed_log = false;

MoveLogEntry* FighterCombatLog::find_or_add_move(MoveId mid) {
    if (mid == MOVE_NONE) return nullptr;
    for (int i = 0; i < move_count; ++i) {
        if (moves[i].move_id == mid) {
            return &moves[i];
        }
    }
    if (move_count < 8) {
        moves[move_count].move_id = mid;
        moves[move_count].uses = 0;
        moves[move_count].hits = 0;
        moves[move_count].successful_casts = 0;
        moves[move_count].total_damage = 0;
        moves[move_count].max_single_hit = 0;
        moves[move_count].is_finisher = false;
        return &moves[move_count++];
    }
    return nullptr;
}

const MoveLogEntry* FighterCombatLog::find_move(MoveId mid) const {
    for (int i = 0; i < move_count; ++i) {
        if (moves[i].move_id == mid) {
            return &moves[i];
        }
    }
    return nullptr;
}

void CombatLogger::init() {
    s_active_log = MatchLog();
    s_last_completed_log = MatchLog();
    s_has_completed_log = false;
}

void CombatLogger::start_match(MatchType match_type,
                              PokemonType p1_type, PokemonType p2_type,
                              const MoveId p1_moves[4], const MoveId p2_moves[4],
                              AIMode p1_ai, AIMode p2_ai,
                              AimMode aim_mode, bool dynamic_cam) {
    s_active_log = MatchLog();
    s_active_log.valid = true;
    s_active_log.match_type = match_type;
    s_active_log.aim_mode = aim_mode;
    s_active_log.dynamic_cam = dynamic_cam;
    s_active_log.match_duration = 0.0f;
    s_active_log.winner_id = -1;
    s_active_log.finisher_move = MOVE_NONE;
    s_active_log.finisher_name[0] = '\0';

    // P1 Setup
    s_active_log.p1.fighter_id = 0;
    s_active_log.p1.pokemon_type = p1_type;
    s_active_log.p1.is_player = (match_type != MATCH_TYPE_SPECTATOR);
    s_active_log.p1.ai_mode = p1_ai;
    s_active_log.p1.move_count = 0;
    for (int i = 0; i < 4; ++i) {
        if (p1_moves[i] != MOVE_NONE) {
            s_active_log.p1.find_or_add_move(p1_moves[i]);
        }
    }

    // P2 Setup
    s_active_log.p2.fighter_id = 1;
    s_active_log.p2.pokemon_type = p2_type;
    s_active_log.p2.is_player = false;
    s_active_log.p2.ai_mode = p2_ai;
    s_active_log.p2.move_count = 0;
    for (int i = 0; i < 4; ++i) {
        if (p2_moves[i] != MOVE_NONE) {
            s_active_log.p2.find_or_add_move(p2_moves[i]);
        }
    }
}

void CombatLogger::update(float dt) {
    if (s_active_log.valid && s_active_log.winner_id == -1) {
        s_active_log.match_duration += dt;
    }
}

void CombatLogger::record_move_use(int fighter_id, MoveId move_id) {
    if (!s_active_log.valid) return;
    FighterCombatLog& f = (fighter_id == 0) ? s_active_log.p1 : s_active_log.p2;
    f.total_casts++;
    MoveLogEntry* m = f.find_or_add_move(move_id);
    if (m) {
        m->uses++;
    }
}

void CombatLogger::record_move_hit(int attacker_id, MoveId move_id, int damage, bool is_ko) {
    if (!s_active_log.valid) return;
    FighterCombatLog& atk = (attacker_id == 0) ? s_active_log.p1 : s_active_log.p2;
    FighterCombatLog& def = (attacker_id == 0) ? s_active_log.p2 : s_active_log.p1;

    atk.total_hits++;
    atk.total_damage_dealt += damage;
    def.total_damage_taken += damage;

    MoveLogEntry* m = atk.find_or_add_move(move_id);
    if (m) {
        m->hits++;
        m->total_damage += damage;
        if (damage > m->max_single_hit) {
            m->max_single_hit = damage;
        }
        // At least one hit landed for this move
        if (m->successful_casts < m->uses) {
            m->successful_casts++;
        }
        if (is_ko) {
            m->is_finisher = true;
            s_active_log.finisher_move = move_id;
            const MoveDef& mdef = MoveRegistry::get(move_id);
            snprintf(s_active_log.finisher_name, sizeof(s_active_log.finisher_name), "%s", mdef.name);
        }
    }
}

void CombatLogger::record_wall_splat(int attacker_id, int damage, bool is_ko) {
    if (!s_active_log.valid) return;
    FighterCombatLog& atk = (attacker_id == 0) ? s_active_log.p1 : s_active_log.p2;
    FighterCombatLog& def = (attacker_id == 0) ? s_active_log.p2 : s_active_log.p1;

    atk.wall_splat_damage += damage;
    atk.total_damage_dealt += damage;
    def.total_damage_taken += damage;

    if (is_ko && s_active_log.finisher_move == MOVE_NONE) {
        snprintf(s_active_log.finisher_name, sizeof(s_active_log.finisher_name), "Arena Wall Collision Impact");
    }
}

void CombatLogger::record_parry(int defender_id) {
    if (!s_active_log.valid) return;
    FighterCombatLog& f = (defender_id == 0) ? s_active_log.p1 : s_active_log.p2;
    f.parries_performed++;
}

void CombatLogger::record_clone_spawn(int fighter_id, int count) {
    if (!s_active_log.valid) return;
    FighterCombatLog& f = (fighter_id == 0) ? s_active_log.p1 : s_active_log.p2;
    f.clones_spawned += count;
}

void CombatLogger::record_clone_pop(int fighter_id) {
    if (!s_active_log.valid) return;
    FighterCombatLog& f = (fighter_id == 0) ? s_active_log.p1 : s_active_log.p2;
    f.clones_popped++;
}

void CombatLogger::end_match(int winner_id, MoveId finisher) {
    if (!s_active_log.valid) return;
    s_active_log.winner_id = winner_id;

    if (finisher != MOVE_NONE && s_active_log.finisher_move == MOVE_NONE) {
        s_active_log.finisher_move = finisher;
        const MoveDef& mdef = MoveRegistry::get(finisher);
        snprintf(s_active_log.finisher_name, sizeof(s_active_log.finisher_name), "%s", mdef.name);

        FighterCombatLog& winner = (winner_id == 0) ? s_active_log.p1 : s_active_log.p2;
        MoveLogEntry* m = winner.find_or_add_move(finisher);
        if (m) m->is_finisher = true;
    }

    // Ensure successful casts sanity clamp
    for (int i = 0; i < s_active_log.p1.move_count; ++i) {
        MoveLogEntry& m = s_active_log.p1.moves[i];
        if (m.hits > 0 && m.successful_casts == 0) {
            m.successful_casts = std::min(m.hits, m.uses > 0 ? m.uses : 1);
        }
    }
    for (int i = 0; i < s_active_log.p2.move_count; ++i) {
        MoveLogEntry& m = s_active_log.p2.moves[i];
        if (m.hits > 0 && m.successful_casts == 0) {
            m.successful_casts = std::min(m.hits, m.uses > 0 ? m.uses : 1);
        }
    }

    s_last_completed_log = s_active_log;
    s_has_completed_log = true;

    // Automatically export to battle_log.txt and append history
    dump_to_file("battle_log.txt");
}

const MatchLog& CombatLogger::get_active_log() {
    return s_active_log;
}

const MatchLog& CombatLogger::get_last_completed_log() {
    return s_last_completed_log;
}

bool CombatLogger::has_completed_log() {
    return s_has_completed_log;
}

bool CombatLogger::is_match_active() {
    return s_active_log.valid && (s_active_log.winner_id == -1);
}

void CombatLogger::format_log(const MatchLog& log, char* buf, size_t max_len) {
    if (!log.valid) {
        snprintf(buf, max_len, "No battle log records available.\n");
        return;
    }

    const char* p1_name = TypeChart::get_species_name(log.p1.pokemon_type);
    const char* p2_name = TypeChart::get_species_name(log.p2.pokemon_type);
    const char* m_type_str = (log.match_type == MATCH_TYPE_SPECTATOR) ? "AI vs AI Spectator" : "Player vs CPU";

    const char* winner_str = "DRAW / IN PROGRESS";
    if (log.winner_id == 0) {
        winner_str = (log.match_type == MATCH_TYPE_SPECTATOR) ? "Player 1 (CPU AI) Victory" : "Player 1 Victory";
    } else if (log.winner_id == 1) {
        winner_str = (log.match_type == MATCH_TYPE_SPECTATOR) ? "Player 2 (CPU AI) Victory" : "CPU Opponent Victory";
    }

    time_t raw_time = time(nullptr);
    struct tm* time_info = localtime(&raw_time);
    char time_str[64] = "N/A";
    if (time_info) {
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);
    }

    int offset = 0;
    offset += snprintf(buf + offset, max_len - offset,
        "================================================================================\n"
        "                     POKEMON REAL-TIME BATTLE - MATCH LOG\n"
        "================================================================================\n"
        "Timestamp:       %s\n"
        "Match Type:      %s\n"
        "Aiming Mode:     %s\n"
        "Camera Mode:     %s\n"
        "Match Duration:  %.2f seconds\n"
        "Match Outcome:   %s\n"
        "Fatal Finisher:  %s\n"
        "--------------------------------------------------------------------------------\n\n",
        time_str,
        m_type_str,
        (log.aim_mode == AIM_MODE_LOCK_ON) ? "Target Lock-On" : "Free Mouse Aim",
        log.dynamic_cam ? "Dynamic Adaptive" : "Fixed 1.0X",
        log.match_duration,
        winner_str,
        (log.finisher_name[0] != '\0') ? log.finisher_name : "None"
    );

    // P1 Section
    offset += snprintf(buf + offset, max_len - offset,
        "--- COMBATANT 1: %s (%s) ---\n"
        "Total Damage Dealt:  %d DMG\n"
        "Total Damage Taken:  %d DMG\n"
        "Total Actions Cast:  %d\n"
        "Total Hits Landed:   %d\n"
        "Overall Accuracy:    %.1f%%\n"
        "Wall Splat Damage:   %d DMG\n"
        "Perfect Parries:     %d\n"
        "Decoys Spawned/Popped: %d / %d\n\n"
        "%-20s %-8s %-8s %-10s %-12s %-10s %-16s\n"
        "--------------------------------------------------------------------------------\n",
        p1_name,
        log.p1.is_player ? "Player 1" : "CPU AI",
        log.p1.total_damage_dealt,
        log.p1.total_damage_taken,
        log.p1.total_casts,
        log.p1.total_hits,
        log.p1.get_accuracy(),
        log.p1.wall_splat_damage,
        log.p1.parries_performed,
        log.p1.clones_spawned,
        log.p1.clones_popped,
        "MOVE", "USES", "HITS", "ACCURACY", "TOTAL DMG", "MAX HIT", "STATUS"
    );

    for (int i = 0; i < log.p1.move_count; ++i) {
        const MoveLogEntry& m = log.p1.moves[i];
        const MoveDef& def = MoveRegistry::get(m.move_id);
        float acc = (m.uses > 0) ? (((float)m.successful_casts / (float)m.uses) * 100.0f) : 0.0f;
        char acc_str[16];
        snprintf(acc_str, sizeof(acc_str), "%.1f%%", acc);

        char dmg_str[16];
        snprintf(dmg_str, sizeof(dmg_str), "%d DMG", m.total_damage);

        char max_str[16];
        snprintf(max_str, sizeof(max_str), "%d DMG", m.max_single_hit);

        const char* status = "";
        if (m.is_finisher) status = "[K.O. FINISHER]";
        else if (def.category == CATEGORY_STATUS) status = "[UTILITY]";

        offset += snprintf(buf + offset, max_len - offset,
            "%-20s %-8d %-8d %-10s %-12s %-10s %-16s\n",
            def.name, m.uses, m.hits, acc_str, dmg_str, max_str, status
        );
    }

    // P2 Section
    offset += snprintf(buf + offset, max_len - offset,
        "\n--- COMBATANT 2: %s (CPU Opponent) ---\n"
        "Total Damage Dealt:  %d DMG\n"
        "Total Damage Taken:  %d DMG\n"
        "Total Actions Cast:  %d\n"
        "Total Hits Landed:   %d\n"
        "Overall Accuracy:    %.1f%%\n"
        "Wall Splat Damage:   %d DMG\n"
        "Perfect Parries:     %d\n"
        "Decoys Spawned/Popped: %d / %d\n\n"
        "%-20s %-8s %-8s %-10s %-12s %-10s %-16s\n"
        "--------------------------------------------------------------------------------\n",
        p2_name,
        log.p2.total_damage_dealt,
        log.p2.total_damage_taken,
        log.p2.total_casts,
        log.p2.total_hits,
        log.p2.get_accuracy(),
        log.p2.wall_splat_damage,
        log.p2.parries_performed,
        log.p2.clones_spawned,
        log.p2.clones_popped,
        "MOVE", "USES", "HITS", "ACCURACY", "TOTAL DMG", "MAX HIT", "STATUS"
    );

    for (int i = 0; i < log.p2.move_count; ++i) {
        const MoveLogEntry& m = log.p2.moves[i];
        const MoveDef& def = MoveRegistry::get(m.move_id);
        float acc = (m.uses > 0) ? (((float)m.successful_casts / (float)m.uses) * 100.0f) : 0.0f;
        char acc_str[16];
        snprintf(acc_str, sizeof(acc_str), "%.1f%%", acc);

        char dmg_str[16];
        snprintf(dmg_str, sizeof(dmg_str), "%d DMG", m.total_damage);

        char max_str[16];
        snprintf(max_str, sizeof(max_str), "%d DMG", m.max_single_hit);

        const char* status = "";
        if (m.is_finisher) status = "[K.O. FINISHER]";
        else if (def.category == CATEGORY_STATUS) status = "[UTILITY]";

        offset += snprintf(buf + offset, max_len - offset,
            "%-20s %-8d %-8d %-10s %-12s %-10s %-16s\n",
            def.name, m.uses, m.hits, acc_str, dmg_str, max_str, status
        );
    }

    snprintf(buf + offset, max_len - offset,
        "================================================================================\n"
    );
}

void CombatLogger::dump_to_file(const char* filepath) {
    if (!s_last_completed_log.valid) return;
    char buffer[4096];
    format_log(s_last_completed_log, buffer, sizeof(buffer));

    // Overwrite the latest match log
    FILE* fp = fopen(filepath, "w");
    if (fp) {
        fputs(buffer, fp);
        fclose(fp);
    }

    // Append to continuous match history
    FILE* hist = fopen("battle_history.log", "a");
    if (hist) {
        fputs(buffer, hist);
        fputs("\n\n", hist);
        fclose(hist);
    }
}
