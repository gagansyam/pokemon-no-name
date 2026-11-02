#pragma once

#include "fighter.hpp"
#include "ai.hpp"
#include "combat_log.hpp"

enum MatchState {
    MATCH_IN_PROGRESS = 0,
    MATCH_CINEMATIC_KO = 1,
    MATCH_VICTORY = 2,
    MATCH_DEFEAT = 3
};

enum GameScreen {
    SCREEN_START_MENU = 0,
    SCREEN_BATTLE = 1
};

class UI {
public:
    void render_start_menu(
        SDL_Renderer* ren,
        const Fighter& p1_preview, const Fighter& p2_preview,
        PokemonType p1_type, PokemonType p2_type,
        const MoveId p1_moves[4], const MoveId p2_moves[4],
        int p1_selected_slot, int p2_selected_slot, int active_panel,
        MatchType match_type, AIMode p1_ai_mode, AIMode p2_ai_mode,
        AimMode aim_mode, bool dynamic_cam,
        float mouse_x, float mouse_y,
        int screen_w, int screen_h,
        float anim_timer,
        bool show_log_modal = false,
        bool show_settings_modal = false,
        int drafter_target = 0,
        int drafter_slot = 0
    );

    void render_post_match_analytics(
        SDL_Renderer* ren,
        const MatchLog& log,
        int screen_w, int screen_h,
        bool in_menu = false
    );

    void render_battle_hud(
        SDL_Renderer* ren, const Fighter& player, const Fighter& dummy,
        float mouse_scr_x, float mouse_scr_y, float cam_x, float cam_y,
        int screen_w, int screen_h, bool locked_on = false,
        bool ai_active = true, int match_state = 0,
        MatchType match_type = MATCH_TYPE_PLAYER_VS_CPU,
        AIMode p1_ai_mode = AI_MODE_BOSS, AIMode p2_ai_mode = AI_MODE_BOSS,
        bool spectator_paused = false, float sim_speed = 1.0f,
        float cam_zoom = 1.0f, bool dynamic_cam_enabled = true,
        AimMode aim_mode = AIM_MODE_FREE,
        const Fighter* p1_team = nullptr, int p1_active_idx = 0, int p1_team_size = 6,
        const Fighter* p2_team = nullptr, int p2_active_idx = 0, int p2_team_size = 6,
        float match_timer = 0.0f
    );

    void render_cinematic_ko(
        SDL_Renderer* ren, float focus_x, float focus_y,
        float cam_x, float cam_y, float cam_zoom,
        float ko_timer, int screen_w, int screen_h
    );

    void render(SDL_Renderer* ren, const Fighter& player, const Fighter& dummy,
                float mouse_scr_x, float mouse_scr_y, float cam_x, float cam_y,
                int screen_w, int screen_h, bool locked_on = false,
                bool ai_active = true, int match_state = 0,
                bool moveset_menu_open = false, int selected_slot = 0);
};

extern UI g_ui;
