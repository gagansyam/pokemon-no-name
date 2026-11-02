#!/usr/bin/env python3
from PIL import Image
import numpy as np

def process_walk_sheet():
    img = Image.open("assets/sprites/pikachu_walk.png").convert("RGBA")
    arr = np.array(img).copy()

    for d in range(5):
        for f in range(4):
            f_arr = arr[d*40:(d+1)*40, f*32:(f+1)*32]
            mask = np.zeros((40, 32), dtype=bool)
            if d == 0:
                for y in range(4, 9):
                    for x in range(22, 28):
                        if f_arr[y, x, 3] > 0: mask[y, x] = True
            elif d == 1:
                for y in range(2, 23):
                    for x in range(0, 12):
                        if f_arr[y, x, 3] > 0: mask[y, x] = True
            elif d == 2:
                for y in range(2, 23):
                    for x in range(0, 12):
                        if f_arr[y, x, 3] > 0: mask[y, x] = True
            elif d == 3:
                for y in range(6, 25):
                    for x in range(0, 14):
                        if f_arr[y, x, 3] > 0: mask[y, x] = True
            elif d == 4:
                for y in range(10, 25):
                    for x in range(6, 14):
                        if f_arr[y, x, 3] > 0: mask[y, x] = True

            # Recolor tail pixels to hardened chrome / steel
            for y in range(40):
                for x in range(32):
                    if mask[y, x]:
                        c = f_arr[y, x]
                        if c[0] == 255 and c[1] == 247:
                            # Gleaming chrome / platinum
                            f_arr[y, x] = [238, 246, 255, 255]
                        elif c[0] == 223 and c[1] == 183:
                            # Polished metallic silver
                            f_arr[y, x] = [175, 195, 222, 255]
                        elif c[0] == 167:
                            # Slate steel base
                            f_arr[y, x] = [110, 132, 162, 255]
                        elif c[0] == 0:
                            # Gunmetal steel outline
                            f_arr[y, x] = [60, 72, 92, 255]

    # Mirror dirs 1, 2, 3 to 7, 6, 5
    for f in range(4):
        d3 = arr[3*40:4*40, f*32:(f+1)*32]
        arr[5*40:6*40, f*32:(f+1)*32] = np.fliplr(d3)
        d2 = arr[2*40:3*40, f*32:(f+1)*32]
        arr[6*40:7*40, f*32:(f+1)*32] = np.fliplr(d2)
        d1 = arr[1*40:2*40, f*32:(f+1)*32]
        arr[7*40:8*40, f*32:(f+1)*32] = np.fliplr(d1)

    out = Image.fromarray(arr)
    out.save("assets/sprites/pikachu_iron_tail_walk.png")
    print("Generated assets/sprites/pikachu_iron_tail_walk.png")

def process_attack_sheet():
    img = Image.open("assets/sprites/pikachu_attack.png").convert("RGBA")
    arr = np.array(img).copy()

    for d in range(5):
        for f in range(10):
            f_arr = arr[d*80:(d+1)*80, f*80:(f+1)*80]
            crop = img.crop((f*80, d*80, (f+1)*80, (d+1)*80))
            bbox = crop.getbbox()
            if not bbox: continue
            mask = np.zeros((80, 80), dtype=bool)

            if d == 0:
                # Tail tip behind right ear
                for y in range(bbox[1], bbox[1] + 12):
                    for x in range(bbox[0] + 10, bbox[2]):
                        if f_arr[y, x, 3] > 0: mask[y, x] = True
            elif d in [1, 2, 3]:
                # Tail on left side of Pikachu
                tail_width = 13 if f < 3 else 15
                tail_max_x = bbox[0] + tail_width
                for y in range(bbox[1], bbox[3]):
                    for x in range(bbox[0], min(80, tail_max_x)):
                        if f_arr[y, x, 3] > 0: mask[y, x] = True
            elif d == 4:
                # Back view: tail is on left half of Pikachu back
                mid_x = (bbox[0] + bbox[2]) // 2
                for y in range(bbox[1] + 8, bbox[3]):
                    for x in range(bbox[0] + 2, mid_x + 1):
                        if f_arr[y, x, 3] > 0: mask[y, x] = True

            # Recolor tail pixels to hardened chrome / steel
            for y in range(80):
                for x in range(80):
                    if mask[y, x]:
                        c = f_arr[y, x]
                        if c[0] == 255 and c[1] == 247:
                            f_arr[y, x] = [238, 246, 255, 255]
                        elif c[0] == 223 and c[1] == 183:
                            f_arr[y, x] = [175, 195, 222, 255]
                        elif c[0] == 167:
                            f_arr[y, x] = [110, 132, 162, 255]
                        elif c[0] == 0:
                            f_arr[y, x] = [60, 72, 92, 255]

    # Mirror dirs 1, 2, 3 to 7, 6, 5
    for f in range(10):
        d3 = arr[3*80:4*80, f*80:(f+1)*80]
        arr[5*80:6*80, f*80:(f+1)*80] = np.fliplr(d3)
        d2 = arr[2*80:3*80, f*80:(f+1)*80]
        arr[6*80:7*80, f*80:(f+1)*80] = np.fliplr(d2)
        d1 = arr[1*80:2*80, f*80:(f+1)*80]
        arr[7*80:8*80, f*80:(f+1)*80] = np.fliplr(d1)

    out = Image.fromarray(arr)
    out.save("assets/sprites/pikachu_iron_tail_attack.png")
    print("Generated assets/sprites/pikachu_iron_tail_attack.png")

if __name__ == "__main__":
    process_walk_sheet()
    process_attack_sheet()
