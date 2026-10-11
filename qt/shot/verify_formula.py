# 验证统一公式：v = top + floor(delta * (y*65536 + h - 1) / (h * 65536))
import sys
sys.stdout.reconfigure(encoding="utf-8")
from PIL import Image
import numpy as np

im = np.asarray(Image.open("qt/shot/png/orig_initial.png").convert("RGB")).astype(int)
plate = np.asarray(Image.open("face_playing.bmp").convert("RGB")).astype(int)

def pred(top, bot, y, h):
    d = bot - top
    return top + (d * (y * 65536 + h - 1)) // (h * 65536)   # Python // = floor

cases = [
    ("bg",    (206,228,248), (147,190,230), 360, [(283, y, y) for y in range(1, 359)]),
    ("cell",  (255,255,255), (199,221,244),  30, [(15, 80 + y, y) for y in range(1, 29)]),
    ("panel", (232,242,253), (181,210,238),  62, [(100, 10 + y, y) for y in range(1, 61)]),
]
total = bad = 0
for name, top, bot, h, src in cases:
    n = 0
    for x, absy, y in src:
        obs = im[absy, x]
        for ch in range(3):
            p = pred(top[ch], bot[ch], y, h)
            total += 1
            if p != obs[ch]:
                bad += 1; n += 1
                if n <= 4:
                    print(f"  {name} y={absy} ch={ch}: pred {p} obs {obs[ch]}")
    print(f"{name}: {n} bad")
for y in range(1, 41):
    obs = plate[y, 2]
    for ch in range(3):
        p = pred(255, (199, 221, 244)[ch], y, 42)
        total += 1
        if p != obs[ch]:
            bad += 1
            print(f"  plate y={y} ch={ch}: pred {p} obs {obs[ch]}")
print(f"TOTAL: {bad}/{total} 不一致")
