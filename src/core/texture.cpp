#include "texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

bool SpriteSheet::load(SDL_Renderer* ren, const char* filepath, int f_w, int f_h, int a_x, int a_y) {
    destroy();
    int comp;
    unsigned char* data = stbi_load(filepath, &width, &height, &comp, 4);
    if (!data) return false;

    frame_w = f_w;
    frame_h = f_h;
    num_frames = width / frame_w;
    num_dirs = height / frame_h;
    anchor_x = (a_x >= 0) ? a_x : (frame_w / 2);
    anchor_y = (a_y >= 0) ? a_y : (frame_h / 2);

    texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!texture) {
        stbi_image_free(data);
        return false;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(texture, nullptr, data, width * 4);
    stbi_image_free(data);
    return true;
}

void SpriteSheet::destroy() {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

void SpriteSheet::draw(SDL_Renderer* ren, int frame_idx, int dir_idx, int dest_x, int dest_y, float scale, int alpha, Uint8 r, Uint8 g, Uint8 b) const {
    if (!texture) return;
    frame_idx = frame_idx % num_frames;
    dir_idx = dir_idx % num_dirs;

    SDL_Rect src = { frame_idx * frame_w, dir_idx * frame_h, frame_w, frame_h };
    int dw = (int)(frame_w * scale);
    int dh = (int)(frame_h * scale);
    int ox = (int)(anchor_x * scale);
    int oy = (int)(anchor_y * scale);
    SDL_Rect dst = { dest_x - ox, dest_y - oy, dw, dh };

    SDL_SetTextureAlphaMod(texture, (Uint8)alpha);
    SDL_SetTextureColorMod(texture, r, g, b);
    SDL_RenderCopy(ren, texture, &src, &dst);
    if (r != 255 || g != 255 || b != 255) {
        SDL_SetTextureColorMod(texture, 255, 255, 255);
    }
}
