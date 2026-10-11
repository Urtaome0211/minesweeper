# capture_screens.py — 双版本多模式截图（原版 Win32 vs Qt 版）
"""启动 exe → 定位窗口 → （可选）模拟鼠标 → 截取客户区 PNG 到 qt/shot/png/。"""
import os
import subprocess
import time

from shotlib import (
    find_window, wait_window, move_to, focus, capture, client_screen_rect,
    lclick, rclick,
)

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "png")
os.makedirs(OUT, exist_ok=True)

ORIG_EXE = os.path.join(ROOT, "minesweeper.exe")
QT_EXE = os.path.join(ROOT, "qt", "dist", "minesweeper-qt.exe")

CELL, PAD, HEADER_H, GAP = 30, 10, 62, 8
BOARD_H = 2 * PAD + HEADER_H + GAP + 9 * CELL  # 9x9 板高 360


def cell_center(r, c):
    return (PAD + c * CELL + CELL // 2, PAD + HEADER_H + GAP + r * CELL + CELL // 2)


def launch(exe, args=None):
    return subprocess.Popen([exe] + (args or []), cwd=os.path.dirname(exe))


def kill(proc):
    try:
        proc.terminate()
    except Exception:
        pass


def shoot(exe, args, name, pre=None, settle=1.0, menu_offset=False):
    """启动、截图、退出。pre(hwnd, menu_h) 在截图前执行（模拟鼠标）。"""
    proc = launch(exe, args)
    hwnd = wait_window("扫雷")
    if not hwnd:
        print(f"[失败] 未找到窗口: {exe}")
        kill(proc)
        return None
    move_to(hwnd, 120, 120)
    focus(hwnd)
    menu_h = 0
    if menu_offset:
        _l, _t, _r, _b = client_screen_rect(hwnd)
        menu_h = (_b - _t) - BOARD_H
    if pre:
        pre(hwnd, menu_h)
    time.sleep(settle)
    path = os.path.join(OUT, name)
    img = capture(hwnd, path)
    print(f"已保存 {path}  ({img.size[0]}x{img.size[1]})")
    kill(proc)
    return img


def main():
    print("== 原版 Win32 ==")
    # 初始棋盘
    shoot(ORIG_EXE, None, "orig_initial.png")
    # 插旗 (0,0)
    def flag_pre(hwnd, mh):
        x, y = cell_center(0, 0)
        rclick(hwnd, x, y + mh)
    shoot(ORIG_EXE, None, "orig_flag.png", pre=flag_pre, settle=0.8)
    # 失败局面：错旗(0,0) + 首点(1,1) + 逐格点开直至踩雷
    def loss_pre(hwnd, mh):
        x, y = cell_center(0, 0)
        rclick(hwnd, x, y + mh)
        x, y = cell_center(1, 1)
        lclick(hwnd, x, y + mh)
        time.sleep(0.5)
        for r in range(9):
            for c in range(9):
                if (r, c) == (0, 0):
                    continue
                x, y = cell_center(r, c)
                lclick(hwnd, x, y + mh)
                time.sleep(0.12)
    shoot(ORIG_EXE, None, "orig_loss.png", pre=loss_pre, settle=2.8)

    print("== Qt 版 ==")
    shoot(QT_EXE, None, "qt_initial.png", menu_offset=True)
    def qt_flag_pre(hwnd, mh):
        x, y = cell_center(0, 0)
        rclick(hwnd, x, y + mh)
    shoot(QT_EXE, None, "qt_flag.png", pre=qt_flag_pre, settle=0.8, menu_offset=True)
    shoot(QT_EXE, ["--demo-lose"], "qt_demo_lose.png", settle=4.0, menu_offset=True)
    shoot(QT_EXE, ["--demo-win"], "qt_demo_win.png", settle=4.5, menu_offset=True)

    print("== 自定义难度对话框 ==")
    proc = launch(QT_EXE, ["--demo-custom"])
    hwnd = wait_window("扫雷")
    time.sleep(1.0)
    dlg = find_window("自定义难度")
    if dlg:
        move_to(hwnd, 120, 120)
        focus(dlg)
        time.sleep(0.5)
        p1 = os.path.join(OUT, "qt_custom_dialog.png")
        p2 = os.path.join(OUT, "qt_custom_main.png")
        capture(dlg, p1)
        capture(hwnd, p2)
        print(f"已保存 {p1}")
        print(f"已保存 {p2}")
    else:
        print("[失败] 未找到自定义难度对话框窗口")
    kill(proc)


if __name__ == "__main__":
    main()
