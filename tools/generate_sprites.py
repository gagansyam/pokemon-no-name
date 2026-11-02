#!/usr/bin/env python3
"""
generate_sprites.py
Generates GBA 4bpp sprite tile data and palettes into src/sprites.hpp and src/sprites.cpp.
"""

import sys

def pack_4bpp(pixels_8x8):
    """Packs an 8x8 matrix of pixel palette indices (0-15) into 32 bytes of 4bpp GBA tile data."""
    tile = bytearray(32)
    for row in range(8):
        for col_pair in range(4):
            p0 = pixels_8x8[row][col_pair * 2] & 0x0F
            p1 = pixels_8x8[row][col_pair * 2 + 1] & 0x0F
            # Low nibble is left pixel, high nibble is right pixel
            tile[row * 4 + col_pair] = p0 | (p1 << 4)
    return tile

def pack_sprite_16x16(matrix):
    """Packs a 16x16 matrix (16 strings/lists of 16 ints) into 4 tiles (128 bytes) in 1D order."""
    data = bytearray()
    # In 1D mapping: top-left (tile 0), top-right (tile 1), bot-left (tile 2), bot-right (tile 3)
    for ty in range(2):
        for tx in range(2):
            tile_pixels = []
            for y in range(8):
                row = []
                for x in range(8):
                    row.append(matrix[ty * 8 + y][tx * 8 + x])
                tile_pixels.append(row)
            data.extend(pack_4bpp(tile_pixels))
    return data

def pack_sprite_32x32(matrix):
    """Packs a 32x32 matrix into 16 tiles (512 bytes) in 1D order."""
    data = bytearray()
    for ty in range(4):
        for tx in range(4):
            tile_pixels = []
            for y in range(8):
                row = []
                for x in range(8):
                    row.append(matrix[ty * 8 + y][tx * 8 + x])
                tile_pixels.append(row)
            data.extend(pack_4bpp(tile_pixels))
    return data

# --- Pixel Art Matrices ---

def make_pikachu_front():
    # 32x32 Pikachu front facing
    m = [[0]*32 for _ in range(32)]
    # Ears
    for y in range(2, 8):
        # Left ear pointing up-left
        m[y][8 - y//2] = 8; m[y][9 - y//2] = 8; m[y][10 - y//2] = 1
        # Right ear pointing up-right
        m[y][23 + y//2] = 8; m[y][22 + y//2] = 8; m[y][21 + y//2] = 1
    for y in range(8, 14):
        m[y][5] = 1; m[y][6] = 1; m[y][7] = 2
        m[y][26] = 1; m[y][25] = 1; m[y][24] = 2

    # Head circle
    for y in range(10, 22):
        for x in range(9, 23):
            dx = x - 15.5
            dy = y - 15.5
            if dx*dx + dy*dy <= 38:
                m[y][x] = 1
            if dx*dx + dy*dy > 30 and dx*dx + dy*dy <= 38:
                m[y][x] = 2 # shadow

    # Eyes
    m[15][12] = 8; m[16][12] = 8; m[15][13] = 9 # left eye with white shine
    m[15][19] = 8; m[16][19] = 8; m[15][18] = 9 # right eye with white shine
    m[17][15] = 8; m[17][16] = 8 # nose
    m[19][14] = 10; m[19][15] = 10; m[19][16] = 10; m[19][17] = 10 # mouth

    # Red cheeks
    for y in range(17, 21):
        for x in range(9, 13):
            m[y][x] = 4
        for x in range(19, 23):
            m[y][x] = 4

    # Body
    for y in range(21, 30):
        for x in range(10, 22):
            dx = x - 15.5
            dy = (y - 25.5) * 1.2
            if dx*dx + dy*dy <= 32:
                m[y][x] = 1
            if dy > 3 or dx > 4 or dx < -4:
                if m[y][x] == 1: m[y][x] = 2

    # Feet
    for x in range(11, 14): m[30][x] = 2; m[31][x] = 3
    for x in range(18, 21): m[30][x] = 2; m[31][x] = 3

    # Tail (lightning bolt on side)
    m[18][24] = 1; m[18][25] = 1; m[17][26] = 1; m[17][27] = 1
    m[16][26] = 1; m[16][27] = 1; m[15][28] = 1; m[14][28] = 1
    m[13][29] = 1; m[12][29] = 1; m[11][30] = 1; m[10][30] = 1

    return m

def make_pikachu_side():
    # 32x32 Pikachu facing East (Side)
    m = [[0]*32 for _ in range(32)]
    # Ears
    for y in range(3, 8):
        m[y][10 - y//2] = 8; m[y][11 - y//2] = 1
        m[y][14 - y//2] = 8; m[y][15 - y//2] = 1
    for y in range(8, 13):
        m[y][8] = 1; m[y][9] = 2
        m[y][12] = 1; m[y][13] = 2

    # Head
    for y in range(10, 22):
        for x in range(10, 24):
            dx = (x - 16.5) * 0.9
            dy = y - 15.5
            if dx*dx + dy*dy <= 34:
                m[y][x] = 1
            if dx*dx + dy*dy > 26 and dx*dx + dy*dy <= 34:
                m[y][x] = 2

    # Snout & Eye
    m[15][20] = 8; m[16][20] = 8; m[15][19] = 9
    m[17][23] = 8; m[18][22] = 10
    # Red cheek
    for y in range(17, 20):
        for x in range(17, 20):
            m[y][x] = 4

    # Body
    for y in range(21, 30):
        for x in range(9, 22):
            dx = (x - 14.5) * 0.9
            dy = (y - 25.5) * 1.1
            if dx*dx + dy*dy <= 32:
                m[y][x] = 1
            if dx < -3 or dy > 3:
                if m[y][x] == 1: m[y][x] = 2

    # Brown stripes on back
    for y in range(23, 25):
        m[y][9] = 6; m[y][10] = 6
    for y in range(26, 28):
        m[y][10] = 6; m[y][11] = 6

    # Feet
    for x in range(10, 13): m[30][x] = 2; m[31][x] = 3
    for x in range(17, 21): m[30][x] = 2; m[31][x] = 3

    # Big lightning bolt tail pointing up-left
    for y in range(12, 16):
        m[y][4] = 1; m[y][5] = 1
    for y in range(15, 19):
        m[y][6] = 1; m[y][7] = 1
    for y in range(18, 23):
        m[y][8] = 6; m[y][9] = 7

    return m

def make_pikachu_back():
    # 32x32 Pikachu back facing
    m = [[0]*32 for _ in range(32)]
    # Ears
    for y in range(2, 8):
        m[y][8 - y//2] = 8; m[y][9 - y//2] = 8; m[y][10 - y//2] = 1
        m[y][23 + y//2] = 8; m[y][22 + y//2] = 8; m[y][21 + y//2] = 1
    for y in range(8, 14):
        m[y][5] = 1; m[y][6] = 2; m[y][7] = 3
        m[y][26] = 1; m[y][25] = 2; m[y][24] = 3

    # Head
    for y in range(10, 22):
        for x in range(9, 23):
            dx = x - 15.5
            dy = y - 15.5
            if dx*dx + dy*dy <= 38:
                m[y][x] = 1
            if dx*dx + dy*dy > 30 and dx*dx + dy*dy <= 38:
                m[y][x] = 2

    # Body
    for y in range(21, 30):
        for x in range(10, 22):
            dx = x - 15.5
            dy = (y - 25.5) * 1.2
            if dx*dx + dy*dy <= 32:
                m[y][x] = 1
            if dy > 3 or dx > 4 or dx < -4:
                if m[y][x] == 1: m[y][x] = 2

    # Two prominent brown stripes across the back
    for x in range(12, 20):
        m[23][x] = 6; m[24][x] = 7
        m[26][x] = 6; m[27][x] = 7

    # Tail attached at base
    m[22][15] = 6; m[22][16] = 6
    m[19][16] = 1; m[19][17] = 1; m[18][17] = 1; m[18][18] = 1
    m[16][18] = 1; m[16][19] = 1; m[15][19] = 1; m[15][20] = 1
    m[13][20] = 1; m[13][21] = 1; m[12][21] = 1; m[11][21] = 1

    return m

def make_charizard_front():
    # 32x32 Charizard front facing
    m = [[0]*32 for _ in range(32)]
    # Horns
    m[3][10] = 9; m[4][11] = 9; m[5][12] = 3
    m[3][21] = 9; m[4][20] = 9; m[5][19] = 3

    # Wings (spreading out wide)
    for y in range(4, 16):
        # Left wing
        for x in range(2, 9):
            if x >= 9 - (y - 4):
                m[y][x] = 7 # teal inner wing
        m[y][2] = 1 # orange wing ridge
        # Right wing
        for x in range(23, 30):
            if x <= 22 + (y - 4):
                m[y][x] = 7 # teal inner wing
        m[y][29] = 1

    # Head
    for y in range(6, 15):
        for x in range(11, 21):
            dx = x - 15.5
            dy = y - 10.5
            if dx*dx + dy*dy <= 20:
                m[y][x] = 1
            if dy > 2: m[y][x] = 3

    # Eyes & Snout
    m[9][13] = 12; m[9][18] = 12 # teal/blue eyes
    m[11][13] = 9; m[11][18] = 9 # white fangs
    m[12][14] = 4; m[12][15] = 4; m[12][16] = 4; m[12][17] = 4 # mouth shadow

    # Neck and Body
    for y in range(15, 28):
        for x in range(10, 22):
            dx = (x - 15.5) * 0.9
            dy = (y - 21.5) * 0.8
            if dx*dx + dy*dy <= 30:
                m[y][x] = 1
            if dx*dx + dy*dy <= 14:
                m[y][x] = 5 # Cream yellow belly

    # Claws / Feet
    for x in range(9, 13): m[28][x] = 1; m[29][x] = 9 # left foot & white claws
    for x in range(19, 23): m[28][x] = 1; m[29][x] = 9 # right foot & white claws

    # Tail with flame on bottom left
    m[24][7] = 1; m[24][8] = 1; m[25][6] = 1; m[25][7] = 1
    m[26][4] = 11; m[26][5] = 10; m[27][3] = 11; m[27][4] = 10 # flame!
    m[28][4] = 11; m[25][4] = 11

    return m

def make_charizard_side():
    # 32x32 Charizard facing East (Side)
    m = [[0]*32 for _ in range(32)]
    # Horn
    m[4][10] = 9; m[5][11] = 9; m[6][12] = 1

    # Wing extending backwards
    for y in range(5, 18):
        for x in range(3, 12):
            if (x - 6)*(x - 6) + (y - 11)*(y - 11) <= 25:
                m[y][x] = 7 # teal wing
        m[y][3] = 1 # orange rim

    # Head and long snout
    for y in range(7, 16):
        for x in range(12, 25):
            dx = (x - 17.5) * 0.7
            dy = y - 11.5
            if dx*dx + dy*dy <= 18:
                m[y][x] = 1
    # Snout / Jaw
    m[11][23] = 1; m[11][24] = 1; m[12][24] = 1
    m[10][20] = 12 # eye
    m[13][22] = 9; m[13][23] = 9 # fangs

    # Body
    for y in range(16, 28):
        for x in range(10, 22):
            dx = (x - 15.5) * 0.9
            dy = (y - 21.5) * 0.8
            if dx*dx + dy*dy <= 28:
                m[y][x] = 1
            if x > 17 and y < 26:
                m[y][x] = 5 # cream belly

    # Feet
    for x in range(11, 15): m[28][x] = 3; m[29][x] = 9
    for x in range(16, 20): m[28][x] = 1; m[29][x] = 9

    # Tail sweeping back with flame
    for x in range(4, 11): m[25][x] = 1; m[26][x] = 3
    m[23][2] = 11; m[23][3] = 10; m[24][2] = 11; m[24][3] = 10
    m[22][3] = 11; m[25][2] = 11

    return m

def make_charizard_back():
    # 32x32 Charizard facing North (Back)
    m = [[0]*32 for _ in range(32)]
    # Horns
    m[3][10] = 9; m[4][11] = 9; m[5][12] = 3
    m[3][21] = 9; m[4][20] = 9; m[5][19] = 3

    # Giant wings spread out
    for y in range(4, 17):
        for x in range(1, 10):
            if x >= 9 - (y - 4): m[y][x] = 1 # back of wings is orange!
        for x in range(22, 31):
            if x <= 22 + (y - 4): m[y][x] = 1

    # Head back
    for y in range(6, 15):
        for x in range(11, 21):
            dx = x - 15.5
            dy = y - 10.5
            if dx*dx + dy*dy <= 20:
                m[y][x] = 1
            if dy > 2: m[y][x] = 3

    # Back spine / Body
    for y in range(15, 28):
        for x in range(10, 22):
            dx = (x - 15.5) * 0.9
            dy = (y - 21.5) * 0.8
            if dx*dx + dy*dy <= 30:
                m[y][x] = 1
            if x == 15 or x == 16:
                m[y][x] = 3 # dark spinal ridge

    # Feet
    for x in range(9, 13): m[28][x] = 3; m[29][x] = 4
    for x in range(19, 23): m[28][x] = 3; m[29][x] = 4

    # Tail and flame
    for y in range(25, 29): m[y][15] = 1; m[y][16] = 3
    m[29][14] = 11; m[29][15] = 10; m[30][14] = 11; m[30][15] = 10

    return m

# --- Projectiles (16x16) ---

def make_thunderbolt():
    # 16x16 Thunderbolt zigzag
    m = [[0]*16 for _ in range(16)]
    path = [
        (2, 7), (3, 7), (3, 8),
        (4, 8), (4, 9), (5, 9), (5, 10),
        (6, 10), (6, 9), (7, 8), (7, 7), # jagged bend
        (8, 7), (8, 8), (9, 8), (9, 9),
        (10, 9), (10, 10), (11, 10), (12, 11), (13, 11), (14, 12)
    ]
    for y, x in path:
        m[y][x] = 1 # pure white core
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                ny, nx = y + dy, x + dx
                if 0 <= ny < 16 and 0 <= nx < 16 and m[ny][nx] == 0:
                    m[ny][nx] = 2 # yellow outer aura
    return m

def make_electro_ball():
    # 16x16 Electro Ball glowing sphere
    m = [[0]*16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            d2 = (x - 7.5)**2 + (y - 7.5)**2
            if d2 <= 6:
                m[y][x] = 1 # white core
            elif d2 <= 16:
                m[y][x] = 2 # bright yellow
            elif d2 <= 28:
                m[y][x] = 4 # cyan spark ring
    return m

def make_flamethrower():
    # 16x16 Flame puff
    m = [[0]*16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            d2 = (x - 7.5)**2 + (y - 7.5)**2
            if d2 <= 6:
                m[y][x] = 1 # white hot core
            elif d2 <= 14:
                m[y][x] = 2 # bright yellow
            elif d2 <= 26:
                m[y][x] = 3 # bright orange
            elif d2 <= 38:
                m[y][x] = 4 # deep red perimeter
    return m

def make_fire_spin():
    # 32x32 Fire vortex trap
    m = [[0]*32 for _ in range(32)]
    for y in range(32):
        for x in range(32):
            d2 = (x - 15.5)**2 + (y - 15.5)**2
            if 16 <= d2 <= 140:
                # Swirling ring
                angle = (x * y) % 7
                if d2 <= 40:
                    m[y][x] = 2 if angle < 4 else 1
                elif d2 <= 90:
                    m[y][x] = 3 if angle < 4 else 2
                else:
                    m[y][x] = 4 if angle < 3 else 0
    return m

def make_claw_scratch():
    # 16x16 Melee claw slash marks
    m = [[0]*16 for _ in range(16)]
    for i in range(3, 13):
        m[i][i - 1] = 1 # slash 1
        m[i][i] = 3
        m[i][i + 2] = 1 # slash 2
        m[i][i + 3] = 3
    return m

def make_impact_spark():
    # 16x16 Impact starburst / hit spark
    m = [[0]*16 for _ in range(16)]
    for i in range(2, 14):
        m[7][i] = 1; m[8][i] = 2 # horizontal beam
        m[i][7] = 1; m[i][8] = 2 # vertical beam
        if 4 <= i <= 11:
            m[i][i] = 1 # diagonal 1
            m[i][15 - i] = 1 # diagonal 2
    return m

# --- 8x8 Font & HUD Tiles ---

FONT_PATTERNS = {
    'A': [0x18, 0x24, 0x42, 0x7E, 0x42, 0x42, 0x00],
    'B': [0x7C, 0x42, 0x7C, 0x42, 0x7C, 0x00, 0x00],
    'C': [0x3C, 0x42, 0x40, 0x42, 0x3C, 0x00, 0x00],
    'D': [0x78, 0x44, 0x42, 0x44, 0x78, 0x00, 0x00],
    'E': [0x7E, 0x40, 0x78, 0x40, 0x7E, 0x00, 0x00],
    'F': [0x7E, 0x40, 0x78, 0x40, 0x40, 0x00, 0x00],
    'G': [0x3C, 0x42, 0x4E, 0x42, 0x3C, 0x00, 0x00],
    'H': [0x42, 0x42, 0x7E, 0x42, 0x42, 0x00, 0x00],
    'I': [0x3E, 0x08, 0x08, 0x08, 0x3E, 0x00, 0x00],
    'J': [0x1E, 0x04, 0x04, 0x44, 0x38, 0x00, 0x00],
    'K': [0x44, 0x48, 0x70, 0x48, 0x44, 0x00, 0x00],
    'L': [0x40, 0x40, 0x40, 0x40, 0x7E, 0x00, 0x00],
    'M': [0x66, 0x7E, 0x5A, 0x42, 0x42, 0x00, 0x00],
    'N': [0x42, 0x62, 0x52, 0x4A, 0x46, 0x00, 0x00],
    'O': [0x3C, 0x42, 0x42, 0x42, 0x3C, 0x00, 0x00],
    'P': [0x7C, 0x42, 0x7C, 0x40, 0x40, 0x00, 0x00],
    'Q': [0x3C, 0x42, 0x42, 0x4A, 0x34, 0x02, 0x00],
    'R': [0x7C, 0x42, 0x7C, 0x48, 0x44, 0x00, 0x00],
    'S': [0x3C, 0x40, 0x3C, 0x02, 0x3C, 0x00, 0x00],
    'T': [0x7E, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00],
    'U': [0x42, 0x42, 0x42, 0x42, 0x3C, 0x00, 0x00],
    'V': [0x42, 0x42, 0x24, 0x24, 0x18, 0x00, 0x00],
    'W': [0x42, 0x42, 0x5A, 0x7E, 0x24, 0x00, 0x00],
    'X': [0x42, 0x24, 0x18, 0x24, 0x42, 0x00, 0x00],
    'Y': [0x42, 0x24, 0x18, 0x08, 0x08, 0x00, 0x00],
    'Z': [0x7E, 0x04, 0x18, 0x20, 0x7E, 0x00, 0x00],
    '0': [0x3C, 0x46, 0x4A, 0x52, 0x3C, 0x00, 0x00],
    '1': [0x18, 0x28, 0x08, 0x08, 0x3E, 0x00, 0x00],
    '2': [0x3C, 0x42, 0x0C, 0x30, 0x7E, 0x00, 0x00],
    '3': [0x7C, 0x04, 0x38, 0x04, 0x7C, 0x00, 0x00],
    '4': [0x48, 0x48, 0x7E, 0x08, 0x08, 0x00, 0x00],
    '5': [0x7E, 0x40, 0x7C, 0x02, 0x7C, 0x00, 0x00],
    '6': [0x3C, 0x40, 0x7C, 0x42, 0x3C, 0x00, 0x00],
    '7': [0x7E, 0x04, 0x08, 0x10, 0x20, 0x00, 0x00],
    '8': [0x3C, 0x42, 0x3C, 0x42, 0x3C, 0x00, 0x00],
    '9': [0x3C, 0x42, 0x3E, 0x02, 0x3C, 0x00, 0x00],
    '!': [0x18, 0x18, 0x18, 0x00, 0x18, 0x00, 0x00],
    ':': [0x00, 0x18, 0x00, 0x18, 0x00, 0x00, 0x00],
    '.': [0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00],
    '-': [0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00],
    ' ': [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
}

def make_font_tile(char):
    pat = FONT_PATTERNS.get(char, [0]*7)
    tile_pixels = [[0]*8 for _ in range(8)]
    for r in range(min(7, len(pat))):
        val = pat[r]
        for c in range(8):
            if val & (1 << (7 - c)):
                tile_pixels[r][c] = 1 # White font pixel
    return pack_4bpp(tile_pixels)

def make_bar_tile(color_idx):
    tile_pixels = [[0]*8 for _ in range(8)]
    for r in range(8):
        for c in range(8):
            if r == 0 or r == 7 or c == 0 or c == 7:
                tile_pixels[r][c] = 2 # Black border
            else:
                tile_pixels[r][c] = color_idx
    return pack_4bpp(tile_pixels)

def make_btn_tile(letter):
    tile_pixels = [[0]*8 for _ in range(8)]
    for r in range(8):
        for c in range(8):
            if r == 0 or r == 7 or c == 0 or c == 7:
                tile_pixels[r][c] = 2 # border
            else:
                tile_pixels[r][c] = 8 # dark background
    pat = FONT_PATTERNS.get(letter, [0]*7)
    for r in range(min(5, len(pat))):
        val = pat[r]
        for c in range(6):
            if val & (1 << (7 - c)):
                tile_pixels[r + 1][c + 1] = 1 # White letter
    return pack_4bpp(tile_pixels)

# --- Main Generator ---

def generate():
    pikachu_front = pack_sprite_32x32(make_pikachu_front())
    pikachu_side  = pack_sprite_32x32(make_pikachu_side())
    pikachu_back  = pack_sprite_32x32(make_pikachu_back())

    charizard_front = pack_sprite_32x32(make_charizard_front())
    charizard_side  = pack_sprite_32x32(make_charizard_side())
    charizard_back  = pack_sprite_32x32(make_charizard_back())

    thunderbolt   = pack_sprite_16x16(make_thunderbolt())
    electro_ball  = pack_sprite_16x16(make_electro_ball())
    flamethrower  = pack_sprite_16x16(make_flamethrower())
    fire_spin     = pack_sprite_32x32(make_fire_spin())
    claw_scratch  = pack_sprite_16x16(make_claw_scratch())
    impact_spark  = pack_sprite_16x16(make_impact_spark())

    # HUD Bar Tiles
    bar_empty  = make_bar_tile(9) # Dark empty
    bar_green  = make_bar_tile(3) # Green HP
    bar_yellow = make_bar_tile(6) # Yellow Stamina
    bar_red    = make_bar_tile(5) # Red HP

    btn_a = make_btn_tile('A')
    btn_b = make_btn_tile('B')
    btn_l = make_btn_tile('L')
    btn_r = make_btn_tile('R')

    # Font tiles
    font_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!:. - "
    font_bytes = bytearray()
    for ch in font_chars:
        font_bytes.extend(make_font_tile(ch))

    # Build header
    with open("src/sprites.hpp", "w") as f:
        f.write("""#pragma once

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
""")

    # Build source
    with open("src/sprites.cpp", "w") as f:
        f.write("""#include "sprites.hpp"
#include "gba_regs.hpp"

// 16-color Palettes for GBA (15-bit BGR format)
static const COLOR PALETTE_DATA[5][16] = {
    // 0: Pikachu Palette
    {
        RGB15(0, 0, 0),         // 0: Transparent
        RGB15(31, 28, 4),       // 1: Primary Yellow
        RGB15(26, 22, 2),       // 2: Shadow Yellow
        RGB15(20, 16, 2),       // 3: Dark Shadow
        RGB15(28, 4, 4),        // 4: Red Cheek
        RGB15(20, 2, 2),        // 5: Red Shadow
        RGB15(15, 8, 2),        // 6: Brown Stripe
        RGB15(10, 4, 1),        // 7: Dark Brown
        RGB15(2, 2, 2),         // 8: Black (Eyes, ear tips)
        RGB15(31, 31, 31),      // 9: White (Highlights)
        RGB15(31, 16, 18),      // 10: Pink Mouth
        RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0)
    },
    // 1: Charizard Palette
    {
        RGB15(0, 0, 0),         // 0: Transparent
        RGB15(30, 14, 2),       // 1: Primary Orange
        RGB15(31, 18, 6),       // 2: Light Orange
        RGB15(22, 8, 2),        // 3: Shadow Orange
        RGB15(15, 4, 1),        // 4: Deep Shadow
        RGB15(30, 27, 14),      // 5: Cream Belly
        RGB15(24, 21, 10),      // 6: Cream Shadow
        RGB15(4, 20, 18),       // 7: Teal Wing Inner
        RGB15(2, 14, 12),       // 8: Teal Shadow
        RGB15(30, 30, 30),      // 9: White Claws/Horns
        RGB15(31, 30, 8),       // 10: Flame Yellow
        RGB15(31, 8, 2),        // 11: Flame Red
        RGB15(4, 20, 30),       // 12: Eye Blue
        RGB15(2, 2, 2),         // 13: Black Pupil
        RGB15(0, 0, 0), RGB15(0, 0, 0)
    },
    // 2: Electric FX Palette
    {
        RGB15(0, 0, 0),         // 0: Transparent
        RGB15(31, 31, 31),      // 1: Pure White core
        RGB15(31, 31, 0),       // 2: Electric Yellow
        RGB15(31, 31, 16),      // 3: Light Yellow
        RGB15(10, 28, 31),      // 4: Electric Cyan
        RGB15(4, 16, 24),       // 5: Electric Deep Blue
        RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0),
        RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0)
    },
    // 3: Fire FX Palette
    {
        RGB15(0, 0, 0),         // 0: Transparent
        RGB15(31, 31, 20),      // 1: White-Yellow Core
        RGB15(31, 26, 0),       // 2: Bright Yellow
        RGB15(31, 14, 0),       // 3: Bright Orange
        RGB15(26, 4, 0),        // 4: Deep Red
        RGB15(16, 2, 0),        // 5: Dark Maroon
        RGB15(8, 8, 8),         // 6: Smoke Gray
        RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0),
        RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0)
    },
    // 4: UI / HUD Palette
    {
        RGB15(0, 0, 0),         // 0: Transparent
        RGB15(31, 31, 31),      // 1: White
        RGB15(0, 0, 0),         // 2: Black border
        RGB15(4, 28, 6),        // 3: Health Green
        RGB15(10, 31, 12),      // 4: Health Green Light
        RGB15(28, 4, 4),        // 5: Health Red Low
        RGB15(30, 26, 2),       // 6: Stamina Yellow
        RGB15(31, 30, 10),      // 7: Stamina Yellow Light
        RGB15(8, 12, 18),       // 8: Cooldown Blue/Gray
        RGB15(5, 5, 5),         // 9: Dark Frame
        RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0), RGB15(0, 0, 0)
    }
};
""")

        def write_byte_array(name, data):
            f.write(f"\nstatic const u8 {name}[{len(data)}] = {{\n    ")
            for i, b in enumerate(data):
                f.write(f"0x{b:02X}, ")
                if (i + 1) % 16 == 0:
                    f.write("\n    ")
            f.write("\n};\n")

        write_byte_array("PIKACHU_FRONT_TILES", pikachu_front)
        write_byte_array("PIKACHU_SIDE_TILES", pikachu_side)
        write_byte_array("PIKACHU_BACK_TILES", pikachu_back)

        write_byte_array("CHARIZARD_FRONT_TILES", charizard_front)
        write_byte_array("CHARIZARD_SIDE_TILES", charizard_side)
        write_byte_array("CHARIZARD_BACK_TILES", charizard_back)

        write_byte_array("THUNDERBOLT_TILES", thunderbolt)
        write_byte_array("ELECTRO_BALL_TILES", electro_ball)
        write_byte_array("FLAMETHROWER_TILES", flamethrower)
        write_byte_array("CLAW_SCRATCH_TILES", claw_scratch)
        write_byte_array("IMPACT_SPARK_TILES", impact_spark)
        write_byte_array("FIRE_SPIN_TILES", fire_spin)

        write_byte_array("BAR_EMPTY_TILES", bar_empty)
        write_byte_array("BAR_GREEN_TILES", bar_green)
        write_byte_array("BAR_YELLOW_TILES", bar_yellow)
        write_byte_array("BAR_RED_TILES", bar_red)

        write_byte_array("BTN_A_TILES", btn_a)
        write_byte_array("BTN_B_TILES", btn_b)
        write_byte_array("BTN_L_TILES", btn_l)
        write_byte_array("BTN_R_TILES", btn_r)

        write_byte_array("FONT_TILES", font_bytes)

        f.write("""
static void copy_tiles(u32 tile_offset, const u8* src, u32 byte_count) {
    volatile u16* dest = (volatile u16*)(0x06010000 + (tile_offset * 32));
    for (u32 i = 0; i < byte_count; i += 2) {
        u16 val = (u16)src[i] | ((u16)src[i + 1] << 8);
        dest[i / 2] = val;
    }
}

int get_font_tile(char c) {
    if (c >= 'A' && c <= 'Z') return TILE_FONT_BASE + (c - 'A');
    if (c >= 'a' && c <= 'z') return TILE_FONT_BASE + (c - 'a');
    if (c >= '0' && c <= '9') return TILE_FONT_BASE + 26 + (c - '0');
    if (c == '!') return TILE_FONT_BASE + 36;
    if (c == ':') return TILE_FONT_BASE + 37;
    if (c == '.') return TILE_FONT_BASE + 38;
    if (c == '-') return TILE_FONT_BASE + 39;
    return TILE_FONT_BASE + 40; // space
}

void load_all_sprites_and_palettes() {
    // 1. Copy OBJ Palettes
    volatile COLOR* pal_dest = MEM_PALETTE_OBJ;
    for (int bank = 0; bank < 5; ++bank) {
        for (int c = 0; c < 16; ++c) {
            pal_dest[bank * 16 + c] = PALETTE_DATA[bank][c];
        }
    }

    // 2. Copy Sprite Tiles to Char Block 4 (0x06010000)
    copy_tiles(TILE_PIKACHU_FRONT,   PIKACHU_FRONT_TILES,   sizeof(PIKACHU_FRONT_TILES));
    copy_tiles(TILE_PIKACHU_SIDE,    PIKACHU_SIDE_TILES,    sizeof(PIKACHU_SIDE_TILES));
    copy_tiles(TILE_PIKACHU_BACK,    PIKACHU_BACK_TILES,    sizeof(PIKACHU_BACK_TILES));

    copy_tiles(TILE_CHARIZARD_FRONT, CHARIZARD_FRONT_TILES, sizeof(CHARIZARD_FRONT_TILES));
    copy_tiles(TILE_CHARIZARD_SIDE,  CHARIZARD_SIDE_TILES,  sizeof(CHARIZARD_SIDE_TILES));
    copy_tiles(TILE_CHARIZARD_BACK,  CHARIZARD_BACK_TILES,  sizeof(CHARIZARD_BACK_TILES));

    copy_tiles(TILE_THUNDERBOLT,     THUNDERBOLT_TILES,     sizeof(THUNDERBOLT_TILES));
    copy_tiles(TILE_ELECTRO_BALL,    ELECTRO_BALL_TILES,    sizeof(ELECTRO_BALL_TILES));
    copy_tiles(TILE_FLAMETHROWER,    FLAMETHROWER_TILES,    sizeof(FLAMETHROWER_TILES));
    copy_tiles(TILE_CLAW_SCRATCH,    CLAW_SCRATCH_TILES,    sizeof(CLAW_SCRATCH_TILES));
    copy_tiles(TILE_IMPACT_SPARK,    IMPACT_SPARK_TILES,    sizeof(IMPACT_SPARK_TILES));
    copy_tiles(TILE_FIRE_SPIN,       FIRE_SPIN_TILES,       sizeof(FIRE_SPIN_TILES));

    copy_tiles(TILE_BAR_EMPTY,       BAR_EMPTY_TILES,       sizeof(BAR_EMPTY_TILES));
    copy_tiles(TILE_BAR_GREEN,       BAR_GREEN_TILES,       sizeof(BAR_GREEN_TILES));
    copy_tiles(TILE_BAR_YELLOW,      BAR_YELLOW_TILES,      sizeof(BAR_YELLOW_TILES));
    copy_tiles(TILE_BAR_RED,         BAR_RED_TILES,         sizeof(BAR_RED_TILES));

    copy_tiles(TILE_BTN_A,           BTN_A_TILES,           sizeof(BTN_A_TILES));
    copy_tiles(TILE_BTN_B,           BTN_B_TILES,           sizeof(BTN_B_TILES));
    copy_tiles(TILE_BTN_L,           BTN_L_TILES,           sizeof(BTN_L_TILES));
    copy_tiles(TILE_BTN_R,           BTN_R_TILES,           sizeof(BTN_R_TILES));

    copy_tiles(TILE_FONT_BASE,       FONT_TILES,            sizeof(FONT_TILES));
}
""")

    print("[generate_sprites] Successfully created src/sprites.hpp and src/sprites.cpp")

if __name__ == "__main__":
    generate()
