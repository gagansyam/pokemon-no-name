#pragma once

#include "texture.hpp"
#include "moves.hpp"
#include "types.hpp"

int angle_to_pmd_dir(float rad);

enum FighterState {
    STATE_IDLE,
    STATE_MOVE,
    STATE_DASH,
    STATE_ATTACK,
    STATE_HITSTUN,
    STATE_TAG_RECALL, // Disappearing into Pokéball red light beam
    STATE_TAG_ENTRY   // Materializing from white Pokéball burst
};

struct AbilitySlot {
    MoveId move_id = MOVE_NONE;
    const char* name = "";
    int stamina_cost = 20;
    float cooldown_max = 1.0f;
    float cooldown_timer = 0.0f;
};

enum BufferType {
    BUFFER_NONE = 0,
    BUFFER_MOVE,
    BUFFER_DASH
};

struct BufferedAction {
    BufferType type = BUFFER_NONE;
    int slot = -1;
    float dir_x = 0.0f;
    float dir_y = 0.0f;
    float target_x = 0.0f;
    float target_y = 0.0f;
    float ttl = 0.0f; // Time To Live (0.12s ~ 7 frames)
};

constexpr float DASH_STAMINA_COST = 30.0f;
constexpr float DASH_DURATION = 0.28f;
constexpr float DASH_RECOVERY_COOLDOWN = 0.45f;

struct IllusionClone {
    bool active = false;
    float x = 0.0f;
    float y = 0.0f;
    float target_offset_x = 0.0f;
    float target_offset_y = 0.0f;
    float current_offset_x = 0.0f;
    float current_offset_y = 0.0f;
    float scatter_angle = 0.0f;
    float scatter_dist = 135.0f;
    int pmd_dir = 0;
    int anim_frame = 0;
    float alpha = 160.0f;
};

class Fighter {
public:
    int id = 0; // 0 = Player/Ash active, 1 = Dummy/Paul active
    PokemonType type = PKMN_PIKACHU;
    FighterState state = STATE_IDLE;
    bool is_dummy = false;

    // Team & Bench Management
    int team_id = 0;            // 0 = Ash, 1 = Paul
    int team_slot = 0;          // 0..5
    bool is_bench = false;      // True if waiting on bench
    float tag_timer = 0.0f;     // Transition timer for tag recall/entry

    // Pokemon RPG Type & Stats
    ElementType type1 = TYPE_ELECTRIC;
    ElementType type2 = TYPE_NONE;
    PokemonStats base_stats;

    // Status Ailments & Stat Stages
    StatusAilment status = STATUS_NONE;
    float status_tick_timer = 0.0f;
    int stage_atk = 0; // -6..+6
    int stage_def = 0;
    int stage_spa = 0;
    int stage_spd = 0;

    // Form & Ability Triggers
    bool blaze_active = false;
    float blaze_aura_timer = 0.0f;

    // Dynamic Lagging HP Bar (Fighting game damage lag)
    float display_lag_hp = 600.0f;

    float x = 0, y = 0;
    float z = 0.0f; // Unified vertical elevation in px off ground
    float vx = 0, vy = 0;
    float input_dx = 0, input_dy = 0;
    float aim_angle = 0; // Radians to mouse
    int pmd_dir = 0; // 0..7

    int hp = 600;
    int max_hp = 600;
    float stamina = 100.0f;
    float max_stamina = 100.0f;
    float idle_regen_timer = 0.0f;

    float radius = 24.0f;
    float move_speed = 320.0f;
    float turn_traction = 24.0f;
    float skid_timer = 0.0f;
    float wall_splat_cooldown = 0.0f;
    float dash_timer = 0.0f;
    float dash_cooldown_timer = 0.0f;
    float dash_cooldown_max = 0.45f;
    float invuln_timer = 0.0f;
    float hitstun_timer = 0.0f;
    float attack_timer = 0.0f;
    float attack_duration = 0.0f;

    // Animation tracking
    float anim_timer = 0.0f;
    int anim_frame = 0;

    // Dash Afterimage / Ghost Trail
    struct DashGhost {
        float x = 0, y = 0;
        int pmd_dir = 0;
        int anim_frame = 0;
        float alpha = 0.0f;
        Uint8 r = 255;
        Uint8 g = 255;
        Uint8 b = 255;
    };
    DashGhost dash_ghosts[8];
    float ghost_timer = 0.0f;
    float stamina_flash_timer = 0.0f;

    AbilitySlot abilities[4];
    MoveId movepool[12];
    int movepool_count = 0;

    // Input Buffer & Hit-Cancel State
    BufferedAction input_buffer;
    float hit_cancel_timer = 0.0f;

    // Generalized Move State
    MoveId active_move = MOVE_NONE;
    int move_phase = 0;
    float move_phase_timer = 0.0f;
    float flight_angle = 0.0f;
    MoveId primed_move = MOVE_NONE;
    float primed_timer = 0.0f;
    bool is_flying = false;

    // Backward compatibility & move state tracking
    bool iron_tail_primed = false;
    bool iron_tail_striking = false;
    float iron_tail_timer = 0.0f;
    float iron_tail_jump_z = 0.0f;
    float wing_attack_windup = 0.0f;
    float wing_attack_rush = 0.0f;
    float agility_timer = 0.0f;
    bool is_grabbed = false;
    int grabbed_by_id = -1;
    float seismic_toss_timer = 0.0f;
    float seismic_toss_center_x = 0.0f;
    float seismic_toss_center_y = 0.0f;

    // Double Team optical clone illusions
    static constexpr int MAX_CLONES = 4;
    IllusionClone clones[MAX_CLONES];
    float double_team_timer = 0.0f;
    float clone_orbit_angle = 0.0f;

    // Protect & Perfect Parry State
    float protect_timer = 0.0f;
    float protect_parry_window = 0.0f;
    float shield_break_timer = 0.0f;
    bool is_protecting() const { return protect_timer > 0.0f; }

    SpriteSheet walk_sheet;
    SpriteSheet attack_sheet;
    SpriteSheet shoot_sheet;
    SpriteSheet fly_sheet;
    SpriteSheet iron_tail_walk_sheet;
    SpriteSheet iron_tail_attack_sheet;

    bool load_assets(SDL_Renderer* ren, PokemonType pkmn_type);
    void init(int fighter_id, PokemonType pkmn_type, float start_x, float start_y, bool dummy);
    void equip_move(int slot, MoveId move_id);
    void update(float dt, Fighter* opponent);
    void update(float dt) { update(dt, nullptr); }
    void render(SDL_Renderer* ren, float cam_x, float cam_y);
    void render_preview(SDL_Renderer* ren, int center_x, int center_y, int dir, int frame, float scale = 2.4f) const;

    bool can_act() const;
    void buffer_ability(int slot, float target_x = 0.0f, float target_y = 0.0f);
    void buffer_dash(float dir_x, float dir_y);
    void process_buffered_action(Fighter* opponent);

    void try_dash(float dir_x, float dir_y);
    void use_ability(int slot, Fighter* opponent, float target_x = 0.0f, float target_y = 0.0f);
    void take_damage(int amount, float kx, float ky, float effectiveness = 1.0f);
    void heal(int amount);
    bool check_hits(Fighter* opponent);

    void apply_status(StatusAilment new_status);
    void clear_status();
    void modify_stat_stage(int stat_idx, int delta); // 0=atk, 1=def, 2=spa, 3=spd

    void start_tag_recall();
    void start_tag_entry();
    void trigger_blaze();

    void spawn_double_team();
    void clear_clones();
};
