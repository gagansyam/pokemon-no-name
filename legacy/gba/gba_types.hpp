#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

typedef u16 COLOR;

#define RGB15(r, g, b) ((COLOR)(((r) & 0x1F) | (((g) & 0x1F) << 5) | (((b) & 0x1F) << 10)))

// Common 15-bit colors
#define COLOR_BLACK   RGB15(0, 0, 0)
#define COLOR_WHITE   RGB15(31, 31, 31)
#define COLOR_RED     RGB15(31, 0, 0)
#define COLOR_GREEN   RGB15(0, 31, 0)
#define COLOR_BLUE    RGB15(0, 0, 31)
#define COLOR_YELLOW  RGB15(31, 31, 0)
#define COLOR_CYAN    RGB15(0, 31, 31)
#define COLOR_MAGENTA RGB15(31, 0, 31)
#define COLOR_GRAY    RGB15(15, 15, 15)
#define COLOR_ORANGE  RGB15(31, 15, 0)
#define COLOR_DARKGRAY RGB15(8, 8, 8)

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 160
