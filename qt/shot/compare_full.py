# 整板对比：原版 vs Qt（Qt 板区向下偏移 menu_h=32）
import sys
sys.stdout.reconfigure(encoding="utf-8")
from PIL import Image
import numpy as np

def load(p):
    return np.asarray(Image.open(p).convert("RGB")).astype(int)

def cmp_pair(name, orig_p, qt_p, menu_h=32, x0=0, y0=0, x1=290, y1=360):
    o = load(orig_p)
    q = load(qt_p)
    o = o[y0:y1, x0:x1]
    q = q[y0 + menu_h:y1 + menu_h, x0:x1]
    diff = o != q
    npix = diff.any(axis=2).sum()
    total = o.shape[0] * o.shape[1]
    d = np.abs(o - q)
    # 按像素最大通道差分类
    dm = d.max(axis=2)
    m = diff.any(axis=2)
    h1 = ((dm == 1) & m).sum()
    h2 = ((dm == 2) & m).sum()
    hbig = ((dm > 2) & m).sum()
    print(f"{name}: 差异像素 {npix}/{total} ({100*npix/total:.2f}%)  通道差==1: {h1}, ==2: {h2}, >2: {hbig}")
    if hbig:
        # 找出差异大的像素坐标（前 20 个）
        m = (d.max(axis=2) > 2)
        ys, xs = np.where(m)
        print("  差异>2 的像素:", [(int(xs[i]), int(ys[i]), tuple(o[ys[i], xs[i]]), tuple(q[ys[i], xs[i]])) for i in range(min(20, len(xs)))])
    return npix

cmp_pair("initial 整板", "qt/shot/png/orig_initial.png", "qt/shot/png/qt_initial.png")
cmp_pair("flag 整板", "qt/shot/png/orig_flag.png", "qt/shot/png/qt_flag.png")

# 胜利笑脸 vs 地面真相
won = np.asarray(Image.open("face_won.bmp").convert("RGB")).astype(int)
qwin = load("qt/shot/png/qt_demo_win.png")
region = qwin[50:92, 124:166]
dd = (region != won)
print(f"won 笑脸 vs 地面真相: 差异像素 {dd.any(axis=2).sum()}/1764")
