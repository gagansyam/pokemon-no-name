#pragma once

#include "gba_types.hpp"

// Tile indices in Sprite VRAM (Char Block 4, 1D mapping)
// 32x32 sprite = 16 tiles
// 16x16 sprite = 4 tiles

#define TILE_PIKACHU_FRONT      0
#define TILE_PIKACHU_SIDE       16
#define TILE_PIKACHU_BACK       32

#define TILE_CHARIZARD_FRONT    48
#define TILE_CHARIZARD_SIDE     64
#define TILE_CHARIZARD_BACK     80

#define TILE_THUNDERBOLT        96
#define TILE_ELECTRO_BALL       100
#define TILE_FLAMETHROWER       104
#define TILE_CLAW_SCRATCH       108
#define TILE_IMPACT_SPARK       112
#define TILE_FIRE_SPIN          116 // 16 tiles (116..131)

// HUD Bar Tiles
#define TILE_BAR_EMPTY          132
#define TILE_BAR_GREEN          133
#define TILE_BAR_YELLOW         134
#define TILE_BAR_RED            135

// Button Badge Tiles
#define TILE_BTN_A              136
#define TILE_BTN_B              137
#define TILE_BTN_L              138
#define TILE_BTN_R              139

// Font Base Tile
#define TILE_FONT_BASE          144

#define PAL_PIKACHU     0
#define PAL_CHARIZARD   1
#define PAL_ELECTRIC    2
#define PAL_FIRE        3
#define PAL_UI          4

void load_all_sprites_and_palettes();
int get_font_tile(char c);
