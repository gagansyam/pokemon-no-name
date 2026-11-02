#pragma once

#include "types.hpp"
#include <SDL2/SDL.h>

class Fighter;

class Camera {
public:
    float x = 0.0f;
    float y = 0.0f;
    float render_x = 0.0f;
    float render_y = 0.0f;
    float zoom = 1.0f;
    float target_zoom = 1.0f;
    bool dynamic_enabled = true;
    float view_w = 1280.0f;
    float view_h = 720.0f;

    void reset(float initial_x, float initial_y, float initial_zoom = 1.0f);
    void update(float dt, const Fighter& p1, const Fighter& p2,
                float mouse_scr_x, float mouse_scr_y,
                int screen_w, int screen_h,
                int match_state, float ko_focus_x, float ko_focus_y,
                bool is_spectator_or_locked, float hitstop_timer, float screen_shake);
};

extern Camera g_camera;
