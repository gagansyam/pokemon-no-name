#pragma once

#include "gba_types.hpp"

// Display Control Register (0x04000000)
#define REG_DISPCNT     (*(volatile u16*)0x04000000)
#define REG_DISPSTAT    (*(volatile u16*)0x04000004)
#define REG_VCOUNT      (*(volatile u16*)0x04000006)

#define MODE_0          0x0000
#define MODE_1          0x0001
#define MODE_2          0x0002
#define MODE_3          0x0003
#define MODE_4          0x0004
#define MODE_5          0x0005

#define BG0_ENABLE      0x0100
#define BG1_ENABLE      0x0200
#define BG2_ENABLE      0x0400
#define BG3_ENABLE      0x0800
#define OBJ_ENABLE      0x1000
#define OBJ_1D_MAP      0x0040

// Background Control Registers
#define REG_BG0CNT      (*(volatile u16*)0x04000008)
#define REG_BG1CNT      (*(volatile u16*)0x0400000A)
#define REG_BG2CNT      (*(volatile u16*)0x0400000C)
#define REG_BG3CNT      (*(volatile u16*)0x0400000E)

#define REG_BG0HOFS     (*(volatile u16*)0x04000010)
#define REG_BG0VOFS     (*(volatile u16*)0x04000012)
#define REG_BG1HOFS     (*(volatile u16*)0x04000014)
#define REG_BG1VOFS     (*(volatile u16*)0x04000016)

// Keypad Input Register (Active LOW: 0 = pressed, 1 = released)
#define REG_KEYINPUT    (*(volatile u16*)0x04000130)

#define KEY_A           (1 << 0)
#define KEY_B           (1 << 1)
#define KEY_SELECT      (1 << 2)
#define KEY_START       (1 << 3)
#define KEY_RIGHT       (1 << 4)
#define KEY_LEFT        (1 << 5)
#define KEY_UP          (1 << 6)
#define KEY_DOWN        (1 << 7)
#define KEY_R           (1 << 8)
#define KEY_L           (1 << 9)
#define KEY_MASK        0x03FF

// DMA 3 Registers
#define REG_DMA3SAD     (*(volatile u32*)0x040000D4)
#define REG_DMA3DAD     (*(volatile u32*)0x040000D8)
#define REG_DMA3CNT     (*(volatile u32*)0x040000DC)

#define DMA_ENABLE      0x80000000
#define DMA_TIMING_IMM  0x00000000
#define DMA_16          0x00000000
#define DMA_32          0x04000000

// Memory Mappings
#define MEM_PALETTE_BG  ((volatile COLOR*)0x05000000)
#define MEM_PALETTE_OBJ ((volatile COLOR*)0x05000200)
#define MEM_VRAM        ((volatile u16*)0x06000000)
#define MEM_OAM         ((volatile OBJ_ATTR*)0x07000000)

// Character Blocks (each 16KB)
#define CHAR_BLOCK(n)   ((volatile u16*)(0x06000000 + ((n) * 0x4000)))
// Screen Blocks (each 2KB)
#define SCREEN_BLOCK(n) ((volatile u16*)(0x06000000 + ((n) * 0x0800)))
// Sprite Tiles in VRAM (Char Block 4)
#define SPRITE_VRAM     ((volatile u16*)0x06010000)

// GBA Hardware Sprite (OAM) Attribute Structure
struct OBJ_ATTR {
    u16 attr0;
    u16 attr1;
    u16 attr2;
    u16 fill;
};

// Attr0 constants
#define ATTR0_Y_MASK        0x00FF
#define ATTR0_REGULAR       0x0000
#define ATTR0_AFFINE        0x0100
#define ATTR0_HIDE          0x0200
#define ATTR0_DOUBLE        0x0300
#define ATTR0_4BPP          0x0000
#define ATTR0_8BPP          0x2000
#define ATTR0_SQUARE        0x0000
#define ATTR0_WIDE          0x4000
#define ATTR0_TALL          0x8000

// Attr1 constants
#define ATTR1_X_MASK        0x01FF
#define ATTR1_HFLIP         0x1000
#define ATTR1_VFLIP         0x2000
#define ATTR1_SIZE_8        0x0000 // 8x8 (Square), 16x8 (Wide), 8x16 (Tall)
#define ATTR1_SIZE_16       0x4000 // 16x16, 32x8, 8x32
#define ATTR1_SIZE_32       0x8000 // 32x32, 32x16, 16x32
#define ATTR1_SIZE_64       0xC000 // 64x64, 64x32, 32x64

// Attr2 constants
#define ATTR2_TILE_MASK     0x03FF
#define ATTR2_PRIO(n)       (((n) & 3) << 10)
#define ATTR2_PALETTE(n)    (((n) & 15) << 12)
