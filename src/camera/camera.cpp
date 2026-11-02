#include "camera.hpp"
#include "fighter.hpp"
#include "arena.hpp"
#include <cmath>
#include <cstdlib>

Camera g_camera;

void Camera::reset(float initial_x, float initial_y, float initial_zoom) {
    x = initial_x;
    y = initial_y;
    render_x = initial_x;
    render_y = initial_y;
    zoom = initial_zoom;
    target_zoom = initial_zoom;
    dynamic_enabled = true;
}

void Camera::update(float dt, const Fighter& p1, const Fighter& p2,
                    float mouse_scr_x, float mouse_scr_y,
                    int screen_w, int screen_h,
                    int match_state, float ko_focus_x, float ko_focus_y,
                    bool is_spectator_or_locked, float hitstop_timer, float screen_shake) {
    (void)dt;

    float p1_p2_dx = p2.x - p1.x;
    float p1_p2_dy = p2.y - p1.y;
    float p1_p2_dist = sqrtf(p1_p2_dx * p1_p2_dx + p1_p2_dy * p1_p2_dy);

    if (!dynamic_enabled) {
        target_zoom = 1.0f;
    } else if (match_state == 1 /* MATCH_CINEMATIC_KO */) {
        target_zoom = 1.35f; // Dramatic punch-in zoom on KO victim
    } else {
        // Distance-based dynamic zoom: 1.25x in close melee (< 180px) down to 0.72x in wide retreat (> 1000px)
        float dist_norm = (p1_p2_dist - 180.0f) / 820.0f;
        if (dist_norm < 0.0f) dist_norm = 0.0f;
        if (dist_norm > 1.0f) dist_norm = 1.0f;
        float base_zoom = 1.25f - dist_norm * (1.25f - 0.72f);

        // Contextual hitstop punch-in bump
        float hitstop_zoom = 0.0f;
        if (hitstop_timer > 0.0f) {
            float prog = hitstop_timer / 0.085f;
            if (prog > 1.0f) prog = 1.0f;
            hitstop_zoom = prog * 0.10f;
        }

        target_zoom = base_zoom + hitstop_zoom;
    }

    // Smooth zoom dampening
    zoom += (target_zoom - zoom) * 0.08f;
    if (zoom < 0.65f) zoom = 0.65f;
    if (zoom > 1.45f) zoom = 1.45f;

    // Visible world dimensions at current zoom
    view_w = (float)screen_w / zoom;
    view_h = (float)screen_h / zoom;

    // Camera focus center in world coordinates
    float target_center_x, target_center_y;
    if (match_state == 1 /* MATCH_CINEMATIC_KO */) {
        target_center_x = ko_focus_x;
        target_center_y = ko_focus_y;
    } else if (is_spectator_or_locked) {
        target_center_x = (p1.x + p2.x) * 0.5f;
        target_center_y = (p1.y + p2.y) * 0.5f;
    } else {
        float cur_mouse_wx = x + mouse_scr_x / zoom;
        float cur_mouse_wy = y + mouse_scr_y / zoom;
        target_center_x = p1.x * 0.65f + p2.x * 0.35f + (cur_mouse_wx - p1.x) * 0.12f;
        target_center_y = p1.y * 0.65f + p2.y * 0.35f + (cur_mouse_wy - p1.y) * 0.12f;
    }

    // Top-left camera world position from center
    float target_cam_x = target_center_x - view_w * 0.5f;
    float target_cam_y = target_center_y - view_h * 0.5f;

    // Clamping within arena bounds
    float max_cam_x = (float)ARENA_WIDTH - view_w;
    float max_cam_y = (float)ARENA_HEIGHT - view_h;
    if (max_cam_x < 0.0f) {
        target_cam_x = ((float)ARENA_WIDTH - view_w) * 0.5f;
    } else {
        if (target_cam_x < 0.0f) target_cam_x = 0.0f;
        if (target_cam_x > max_cam_x) target_cam_x = max_cam_x;
    }

    if (max_cam_y < 0.0f) {
        target_cam_y = ((float)ARENA_HEIGHT - view_h) * 0.5f;
    } else {
        if (target_cam_y < 0.0f) target_cam_y = 0.0f;
        if (target_cam_y > max_cam_y) target_cam_y = max_cam_y;
    }

    x += (target_cam_x - x) * 0.10f;
    y += (target_cam_y - y) * 0.10f;

    // Camera Screen Shake calculation
    render_x = x;
    render_y = y;
    if (screen_shake > 0.0f) {
        float mag = screen_shake * 36.0f;
        if (hitstop_timer > 0.0f) mag *= 1.4f;
        render_x += ((float)(rand() % 100) - 50.0f) * 0.02f * mag;
        render_y += ((float)(rand() % 100) - 50.0f) * 0.02f * mag;
    }
}
