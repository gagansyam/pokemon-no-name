#pragma once

#include <SDL2/SDL.h>

#define ARENA_WIDTH   2400
#define ARENA_HEIGHT  1600
#define ARENA_MIN_X   120.0f
#define ARENA_MAX_X   2280.0f
#define ARENA_MIN_Y   140.0f
#define ARENA_MAX_Y   1460.0f

enum DecalType {
    DECAL_TECTONIC_CRACK,  // Earthquake permanent fault fissures
    DECAL_IMPACT_CRATER,   // Iron Tail floor slam, heavy physical impacts, Seismic Toss
    DECAL_SCORCH_MARK,     // Thunderbolt blast, Flamethrower / Fire Spin scorch
    DECAL_SAND_FURROW      // Scorching Sands excavated trench beneath feet
};

struct GroundDecal {
    bool active = false;
    DecalType type = DECAL_IMPACT_CRATER;
    float x = 0.0f;
    float y = 0.0f;
    float radius = 20.0f;
    float angle = 0.0f;
    int seed = 0;
    Uint8 r = 30;
    Uint8 g = 20;
    Uint8 b = 10;
    Uint8 alpha = 220;
    float ttl = 30.0f;
    float max_ttl = 30.0f;
};

enum HazardType {
    HAZARD_NONE = 0,
    HAZARD_TOXIC_SPIKES,
    HAZARD_STEALTH_ROCK
};

struct ActiveHazard {
    bool active = false;
    HazardType type = HAZARD_TOXIC_SPIKES;
    int target_team = 1; // 0 = Ash side (left), 1 = Paul side (right)
    float x = 0.0f;
    float y = 0.0f;
    float pulse_timer = 0.0f;
};

class Arena {
public:
    static constexpr int MAX_GROUND_DECALS = 512;
    static constexpr int MAX_HAZARDS = 64;

    GroundDecal decals[MAX_GROUND_DECALS];
    int next_decal_idx = 0;

    ActiveHazard hazards[MAX_HAZARDS];
    int hazard_count = 0;

    float anim_timer = 0.0f;

    void init();
    void update(float dt);
    void render(SDL_Renderer* ren, float cam_x, float cam_y, int view_w, int view_h);
    void clamp(float& x, float& y, float radius);

    void add_decal(DecalType type, float x, float y, float radius, float angle = 0.0f,
                   Uint8 r = 30, Uint8 g = 20, Uint8 b = 10, Uint8 alpha = 220, float ttl = 30.0f);
    void clear_decals();

    void add_toxic_spikes(int target_team, float center_x, float center_y);
    void clear_hazards(int target_team);
    bool check_hazard_collision(float x, float y, float z, int team_id, HazardType& out_type);
};

extern Arena g_arena;
