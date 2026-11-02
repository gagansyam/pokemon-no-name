#pragma once

#include <SDL2/SDL.h>

struct SpriteSheet {
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
    int frame_w = 0;
    int frame_h = 0;
    int num_frames = 0;
    int num_dirs = 8;
    int anchor_x = 0;
    int anchor_y = 0;

    bool load(SDL_Renderer* ren, const char* filepath, int f_w, int f_h, int a_x = -1, int a_y = -1);
    void destroy();
    void draw(SDL_Renderer* ren, int frame_idx, int dir_idx, int dest_x, int dest_y, float scale = 2.0f, int alpha = 255) const {
        draw(ren, frame_idx, dir_idx, dest_x, dest_y, scale, alpha, 255, 255, 255);
    }
    void draw(SDL_Renderer* ren, int frame_idx, int dir_idx, int dest_x, int dest_y, float scale, int alpha, Uint8 r, Uint8 g, Uint8 b) const;
};
