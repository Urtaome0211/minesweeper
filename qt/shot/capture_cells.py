# capture_cells.py — 运行 Qt 版 --demo-cells 三种模式，截图并与 harness2 GDI 地面真相整板 BMP 对比
import subprocess
import sys
import time

import numpy as np
from PIL import Image

sys.stdout.reconfigure(encoding="utf-8")
from shotlib import capture, wait_window, move_to, focus

EXE = r"D:\dsh\project\minesweeper\qt\build\minesweeper-qt.exe"
MENU_H = 32  # Qt 菜单栏高度（板区在客户区内向下偏移）


def run(kind):
    subprocess.Popen([EXE, "--demo-cells", kind])
    hwnd = wait_window("扫雷 - 初级")
    if not hwnd:
        print(f"[{kind}] 未找到窗口")
        return None
    move_to(hwnd, 120, 120)  # 移开任务栏/屏幕边缘，避免截到被遮挡的底角
    focus(hwnd)
    time.sleep(1.2)  # 等待 300ms 注入 + 绘制
    img = capture(hwnd, f"qt/shot/png/cells_{kind}.png")
    subprocess.run(["taskkill", "/F", "/IM", "minesweeper-qt.exe"],
                   capture_output=True)
    time.sleep(0.3)
    return np.asarray(img.convert("RGB")).astype(int)


def cmp(kind):
    from PIL import Image
    q = np.asarray(Image.open(f"qt/shot/png/cells_{kind}.png").convert("RGB")).astype(int)
    o = np.asarray(Image.open(f"qt/shot/board_{kind}.bmp").convert("RGB")).astype(int)
    q = q[MENU_H:MENU_H + 360, 0:290]
    diff = (o != q).any(axis=2)
    # DWM 窗口圆角：底角 ~8px 圆弧区被窗口框架裁掉（原版窗口截图同位置像素一致，属窗口框架而非游戏渲染）
    arc = np.zeros_like(diff)
    arc[352:360, 0:8] = True
    arc[352:360, 282:290] = True
    in_arc = (diff & arc).sum()
    npix = (diff & ~arc).sum()
    print(f"[{kind}] 整板差异像素 {npix}/104400 ({100 * npix / 104400:.3f}%)  （窗口圆角区 {in_arc} 已排除）")
    if npix:
        ys, xs = np.where(diff)
        for i in range(min(30, len(xs))):
            x, y = int(xs[i]), int(ys[i])
            print(f"    ({x},{y}) 原 {tuple(o[y, x])} / Qt {tuple(q[y, x])}")
        Image.fromarray((diff * 255).astype(np.uint8)).save(f"qt/shot/png/diff_{kind}.png")
    return npix


if __name__ == "__main__":
    subprocess.run(["taskkill", "/F", "/IM", "minesweeper-qt.exe"], capture_output=True)
    time.sleep(0.3)
    total = 0
    for k in ("lost", "won789", "won046"):
        run(k)
        total += cmp(k)
    print("总差异:", total)
