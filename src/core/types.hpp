#pragma once

#include <SDL2/SDL.h>
#include <cmath>
#include <algorithm>

enum ElementType {
    TYPE_NONE = 0,
    TYPE_NORMAL,
    TYPE_FIRE,
    TYPE_WATER,
    TYPE_ELECTRIC,
    TYPE_GRASS,
    TYPE_ICE,
    TYPE_FIGHTING,
    TYPE_POISON,
    TYPE_GROUND,
    TYPE_FLYING,
    TYPE_PSYCHIC,
    TYPE_BUG,
    TYPE_ROCK,
    TYPE_GHOST,
    TYPE_DRAGON,
    TYPE_STEEL,
    TYPE_DARK,
    TYPE_FAIRY,
    TYPE_COUNT
};

enum PokemonType {
    PKMN_PIKACHU = 0,
    PKMN_CHARIZARD = 1,
    PKMN_LUCARIO = 2,
    PKMN_GRENINJA = 3,
    PKMN_COUNT = 4
};

enum StatusAilment {
    STATUS_NONE = 0,
    STATUS_POISON,     // PSN: 4% max HP DoT, purple particles
    STATUS_BURN,       // BRN: 3% max HP DoT, -30% physical attack, flame sparks
    STATUS_PARALYSIS   // PAR: -40% speed, brief movement glitching
};

enum AIMode {
    AI_MODE_DUMMY = 0,
    AI_MODE_CASUAL = 1,
    AI_MODE_BOSS = 2
};

enum MatchType {
    MATCH_TYPE_PLAYER_VS_CPU = 0,
    MATCH_TYPE_SPECTATOR = 1
};

enum AimMode {
    AIM_MODE_FREE = 0,
    AIM_MODE_LOCK_ON = 1
};

enum MoveCategory {
    CATEGORY_PHYSICAL,
    CATEGORY_SPECIAL,
    CATEGORY_STATUS
};

struct PokemonStats {
    int hp = 600;
    int attack = 80;
    int defense = 50;
    int sp_attack = 90;
    int sp_defense = 50;
    int speed = 100;
};

class TypeChart {
public:
    // Single-type defender multiplier lookup (Gen 9 standard)
    static float get_single_multiplier(ElementType atk, ElementType def) {
        if (atk == TYPE_NONE || def == TYPE_NONE) return 1.0f;

        float mult = 1.0f;

        switch (atk) {
            case TYPE_NORMAL:
                if (def == TYPE_ROCK || def == TYPE_STEEL) mult = 0.5f;
                else if (def == TYPE_GHOST) mult = 0.0f;
                break;

            case TYPE_FIRE:
                if (def == TYPE_GRASS || def == TYPE_ICE || def == TYPE_BUG || def == TYPE_STEEL) mult = 2.0f;
                else if (def == TYPE_FIRE || def == TYPE_WATER || def == TYPE_ROCK || def == TYPE_DRAGON) mult = 0.5f;
                break;

            case TYPE_WATER:
                if (def == TYPE_FIRE || def == TYPE_GROUND || def == TYPE_ROCK) mult = 2.0f;
                else if (def == TYPE_WATER || def == TYPE_GRASS || def == TYPE_DRAGON) mult = 0.5f;
                break;

            case TYPE_ELECTRIC:
                if (def == TYPE_WATER || def == TYPE_FLYING) mult = 2.0f;
                else if (def == TYPE_ELECTRIC || def == TYPE_GRASS || def == TYPE_DRAGON) mult = 0.5f;
                else if (def == TYPE_GROUND) mult = 0.0f;
                break;

            case TYPE_GRASS:
                if (def == TYPE_WATER || def == TYPE_GROUND || def == TYPE_ROCK) mult = 2.0f;
                else if (def == TYPE_FIRE || def == TYPE_GRASS || def == TYPE_POISON || def == TYPE_FLYING ||
                         def == TYPE_BUG || def == TYPE_DRAGON || def == TYPE_STEEL) mult = 0.5f;
                break;

            case TYPE_ICE:
                if (def == TYPE_GRASS || def == TYPE_GROUND || def == TYPE_FLYING || def == TYPE_DRAGON) mult = 2.0f;
                else if (def == TYPE_FIRE || def == TYPE_WATER || def == TYPE_ICE || def == TYPE_STEEL) mult = 0.5f;
                break;

            case TYPE_FIGHTING:
                if (def == TYPE_NORMAL || def == TYPE_ICE || def == TYPE_ROCK || def == TYPE_DARK || def == TYPE_STEEL) mult = 2.0f;
                else if (def == TYPE_POISON || def == TYPE_FLYING || def == TYPE_PSYCHIC || def == TYPE_BUG || def == TYPE_FAIRY) mult = 0.5f;
                else if (def == TYPE_GHOST) mult = 0.0f;
                break;

            case TYPE_POISON:
                if (def == TYPE_GRASS || def == TYPE_FAIRY) mult = 2.0f;
                else if (def == TYPE_POISON || def == TYPE_GROUND || def == TYPE_ROCK || def == TYPE_GHOST) mult = 0.5f;
                else if (def == TYPE_STEEL) mult = 0.0f;
                break;

            case TYPE_GROUND:
                if (def == TYPE_FIRE || def == TYPE_ELECTRIC || def == TYPE_POISON || def == TYPE_ROCK || def == TYPE_STEEL) mult = 2.0f;
                else if (def == TYPE_GRASS || def == TYPE_BUG) mult = 0.5f;
                else if (def == TYPE_FLYING) mult = 0.0f;
                break;

            case TYPE_FLYING:
                if (def == TYPE_GRASS || def == TYPE_FIGHTING || def == TYPE_BUG) mult = 2.0f;
                else if (def == TYPE_ELECTRIC || def == TYPE_ROCK || def == TYPE_STEEL) mult = 0.5f;
                break;

            case TYPE_PSYCHIC:
                if (def == TYPE_FIGHTING || def == TYPE_POISON) mult = 2.0f;
                else if (def == TYPE_PSYCHIC || def == TYPE_STEEL) mult = 0.5f;
                else if (def == TYPE_DARK) mult = 0.0f;
                break;

            case TYPE_BUG:
                if (def == TYPE_GRASS || def == TYPE_PSYCHIC || def == TYPE_DARK) mult = 2.0f;
                else if (def == TYPE_FIRE || def == TYPE_FIGHTING || def == TYPE_POISON || def == TYPE_FLYING ||
                         def == TYPE_GHOST || def == TYPE_STEEL || def == TYPE_FAIRY) mult = 0.5f;
                break;

            case TYPE_ROCK:
                if (def == TYPE_FIRE || def == TYPE_ICE || def == TYPE_FLYING || def == TYPE_BUG) mult = 2.0f;
                else if (def == TYPE_FIGHTING || def == TYPE_GROUND || def == TYPE_STEEL) mult = 0.5f;
                break;

            case TYPE_GHOST:
                if (def == TYPE_PSYCHIC || def == TYPE_GHOST) mult = 2.0f;
                else if (def == TYPE_DARK) mult = 0.5f;
                else if (def == TYPE_NORMAL) mult = 0.0f;
                break;

            case TYPE_DRAGON:
                if (def == TYPE_DRAGON) mult = 2.0f;
                else if (def == TYPE_STEEL) mult = 0.5f;
                else if (def == TYPE_FAIRY) mult = 0.0f;
                break;

            case TYPE_STEEL:
                if (def == TYPE_ICE || def == TYPE_ROCK || def == TYPE_FAIRY) mult = 2.0f;
                else if (def == TYPE_FIRE || def == TYPE_WATER || def == TYPE_ELECTRIC || def == TYPE_STEEL) mult = 0.5f;
                break;

            case TYPE_DARK:
                if (def == TYPE_PSYCHIC || def == TYPE_GHOST) mult = 2.0f;
                else if (def == TYPE_FIGHTING || def == TYPE_DARK || def == TYPE_FAIRY) mult = 0.5f;
                break;

            case TYPE_FAIRY:
                if (def == TYPE_FIGHTING || def == TYPE_DRAGON || def == TYPE_DARK) mult = 2.0f;
                else if (def == TYPE_FIRE || def == TYPE_POISON || def == TYPE_STEEL) mult = 0.5f;
                break;

            default:
                break;
        }

        return mult;
    }

    // Dual-type defender multiplier calculation
    static float get_multiplier(ElementType atk, ElementType def1, ElementType def2 = TYPE_NONE) {
        float m1 = get_single_multiplier(atk, def1);
        float m2 = (def2 != TYPE_NONE && def2 != def1) ? get_single_multiplier(atk, def2) : 1.0f;
        return m1 * m2;
    }

    static const char* get_type_name(ElementType type) {
        switch (type) {
            case TYPE_NORMAL:   return "NORMAL";
            case TYPE_FIRE:     return "FIRE";
            case TYPE_WATER:    return "WATER";
            case TYPE_ELECTRIC: return "ELECTRIC";
            case TYPE_GRASS:    return "GRASS";
            case TYPE_ICE:      return "ICE";
            case TYPE_FIGHTING: return "FIGHT";
            case TYPE_POISON:   return "POISON";
            case TYPE_GROUND:   return "GROUND";
            case TYPE_FLYING:   return "FLYING";
            case TYPE_PSYCHIC:  return "PSYCHIC";
            case TYPE_BUG:      return "BUG";
            case TYPE_ROCK:     return "ROCK";
            case TYPE_GHOST:    return "GHOST";
            case TYPE_DRAGON:   return "DRAGON";
            case TYPE_STEEL:    return "STEEL";
            case TYPE_DARK:     return "DARK";
            case TYPE_FAIRY:    return "FAIRY";
            default:            return "NONE";
        }
    }

    static void get_type_color(ElementType type, Uint8& r, Uint8& g, Uint8& b) {
        switch (type) {
            case TYPE_NORMAL:   r = 168; g = 167; b = 122; break; // A8A77A
            case TYPE_FIRE:     r = 238; g = 129; b = 48;  break; // EE8130
            case TYPE_WATER:    r = 99;  g = 144; b = 240; break; // 6390F0
            case TYPE_ELECTRIC: r = 247; g = 208; b = 44;  break; // F7D02C
            case TYPE_GRASS:    r = 122; g = 199; b = 76;  break; // 7AC74C
            case TYPE_ICE:      r = 150; g = 217; b = 214; break; // 96D9D6
            case TYPE_FIGHTING: r = 194; g = 46;  b = 40;  break; // C22E28
            case TYPE_POISON:   r = 163; g = 62;  b = 161; break; // A33EA1
            case TYPE_GROUND:   r = 226; g = 191; b = 101; break; // E2BF65
            case TYPE_FLYING:   r = 169; g = 143; b = 243; break; // A98FF3
            case TYPE_PSYCHIC:  r = 249; g = 85;  b = 135; break; // F95587
            case TYPE_BUG:      r = 166; g = 185; b = 26;  break; // A6B91A
            case TYPE_ROCK:     r = 182; g = 161; b = 54;  break; // B6A136
            case TYPE_GHOST:    r = 115; g = 87;  b = 151; break; // 735797
            case TYPE_DRAGON:   r = 111; g = 53;  b = 252; break; // 6F35FC
            case TYPE_STEEL:    r = 183; g = 183; b = 206; break; // B7B7CE
            case TYPE_DARK:     r = 112; g = 87;  b = 70;  break; // 705746
            case TYPE_FAIRY:    r = 214; g = 133; b = 173; break; // D685AD
            default:            r = 148; g = 163; b = 184; break; // Slate
        }
    }

    // Action-Calibrated Real-Time Multiplier
    static float get_action_multiplier(float type_mult) {
        if (type_mult <= 0.001f) return 0.0f; // Immune is strictly 0
        if (type_mult > 1.0f) {
            return 1.0f + (type_mult - 1.0f) * 0.40f;
        } else if (type_mult < 1.0f) {
            return 1.0f - (1.0f - type_mult) * 0.60f;
        }
        return 1.0f;
    }

    static float get_stage_multiplier(int stage) {
        if (stage > 6) stage = 6;
        if (stage < -6) stage = -6;
        if (stage > 0) return (2.0f + (float)stage) / 2.0f;
        if (stage < 0) return 2.0f / (2.0f - (float)stage);
        return 1.0f;
    }

    static const char* get_species_name(PokemonType ptype) {
        switch (ptype) {
            case PKMN_PIKACHU:   return "PIKACHU";
            case PKMN_CHARIZARD: return "CHARIZARD";
            case PKMN_LUCARIO:   return "LUCARIO";
            case PKMN_GRENINJA:  return "GRENINJA";
            default:             return "UNKNOWN";
        }
    }

    static void get_species_types(PokemonType ptype, ElementType& t1, ElementType& t2) {
        switch (ptype) {
            case PKMN_PIKACHU:   t1 = TYPE_ELECTRIC; t2 = TYPE_NONE; break;
            case PKMN_CHARIZARD: t1 = TYPE_FIRE;     t2 = TYPE_FLYING; break;
            case PKMN_LUCARIO:   t1 = TYPE_FIGHTING; t2 = TYPE_STEEL; break;
            case PKMN_GRENINJA:  t1 = TYPE_WATER;    t2 = TYPE_DARK; break;
            default:             t1 = TYPE_NORMAL;   t2 = TYPE_NONE; break;
        }
    }

    static PokemonStats get_base_stats(PokemonType ptype) {
        switch (ptype) {
            case PKMN_PIKACHU:   return { 540, 80, 50, 90, 55, 110 };
            case PKMN_CHARIZARD: return { 660, 105, 80, 110, 85, 100 };
            case PKMN_LUCARIO:   return { 600, 110, 70, 115, 70, 90 };
            case PKMN_GRENINJA:  return { 540, 95, 67, 103, 71, 122 };
            default:             return { 600, 80, 50, 90, 50, 100 };
        }
    }

    // Real-Time Normalized Damage Calculator with STAB, Stat Stages & Status scaling
    static int calculate_damage(int base_power, MoveCategory cat, ElementType move_type,
                                const PokemonStats& atk_stats, ElementType atk_type1, ElementType atk_type2,
                                const PokemonStats& def_stats, ElementType def_type1, ElementType def_type2,
                                int atk_stage = 0, int def_stage = 0, bool is_burned = false, bool is_blaze = false,
                                float* out_mult = nullptr) {
        if (base_power <= 0) {
            if (out_mult) *out_mult = 1.0f;
            return 0;
        }

        // 1. Offense vs Defense Stat Ratio with Dynamic Stages
        float off = 100.0f;
        float def = 100.0f;
        if (cat == CATEGORY_PHYSICAL) {
            off = (float)atk_stats.attack * get_stage_multiplier(atk_stage);
            if (is_burned) off *= 0.70f; // Burn decreases physical attack
            def = (float)def_stats.defense * get_stage_multiplier(def_stage);
        } else if (cat == CATEGORY_SPECIAL) {
            off = (float)atk_stats.sp_attack * get_stage_multiplier(atk_stage);
            def = (float)def_stats.sp_defense * get_stage_multiplier(def_stage);
        }
        if (def < 10.0f) def = 10.0f;

        float stat_ratio = powf(off / def, 0.75f);

        // 2. STAB (Same-Type Attack Bonus): 1.25x in real-time engine
        float stab = 1.0f;
        if (move_type != TYPE_NONE && (move_type == atk_type1 || move_type == atk_type2)) {
            stab = 1.25f;
        }

        // 3. Blaze Ability Multiplier (1.6x for Fire-type moves when active)
        float ability_mult = 1.0f;
        if (is_blaze && move_type == TYPE_FIRE) {
            ability_mult = 1.60f;
        }

        // 4. Type Effectiveness Multiplier
        float type_mult = get_multiplier(move_type, def_type1, def_type2);
        if (out_mult) *out_mult = type_mult;

        if (type_mult <= 0.001f) {
            return 0; // Immune
        }

        float action_mult = get_action_multiplier(type_mult);

        // 5. Final Damage Computation
        float raw = (float)base_power * stat_ratio * stab * ability_mult * action_mult;
        int final_dmg = (int)roundf(raw);
        if (final_dmg < 1) final_dmg = 1;

        return final_dmg;
    }

    static int calculate_damage(int base_power, MoveCategory cat, ElementType move_type,
                                const PokemonStats& atk_stats, ElementType atk_type1, ElementType atk_type2,
                                const PokemonStats& def_stats, ElementType def_type1, ElementType def_type2,
                                float* out_mult) {
        return calculate_damage(base_power, cat, move_type, atk_stats, atk_type1, atk_type2, def_stats, def_type1, def_type2, 0, 0, false, false, out_mult);
    }
};
