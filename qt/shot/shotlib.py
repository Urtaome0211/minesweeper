# shotlib.py — 截图与鼠标模拟辅助（ctypes + PIL.ImageGrab）
"""Win32 窗口查找/客户区截图/鼠标消息模拟，供 capture_screens.py 与 compare.py 使用。"""
import ctypes
import time
from ctypes import wintypes

from PIL import ImageGrab

user32 = ctypes.windll.user32

# ---------------------------------------------------------------- 窗口查找

_titles = []


def _enum_cb(hwnd, _lparam):
    if user32.IsWindowVisible(hwnd):
        length = user32.GetWindowTextLengthW(hwnd)
        if length:
            buf = ctypes.create_unicode_buffer(length + 1)
            user32.GetWindowTextW(hwnd, buf, length + 1)
            _titles.append((hwnd, buf.value))
    return True


_WNDENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def find_window(title_prefix):
    _titles.clear()
    user32.EnumWindows(_WNDENUMPROC(_enum_cb), 0)
    for hwnd, title in _titles:
        if title.startswith(title_prefix):
            return hwnd
    return None


def wait_window(title_prefix, timeout=15.0):
    t0 = time.time()
    while time.time() - t0 < timeout:
        hwnd = find_window(title_prefix)
        if hwnd:
            return hwnd
        time.sleep(0.1)
    return None


# ---------------------------------------------------------------- 几何与截图

class RECT(ctypes.Structure):
    _fields_ = [("left", ctypes.c_long), ("top", ctypes.c_long),
                ("right", ctypes.c_long), ("bottom", ctypes.c_long)]


def client_screen_rect(hwnd):
    r = RECT()
    user32.GetClientRect(hwnd, ctypes.byref(r))
    pt = wintypes.POINT(0, 0)
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    return (pt.x, pt.y, pt.x + r.right, pt.y + r.bottom)


def move_to(hwnd, x, y):
    user32.SetWindowPos(hwnd, 0, x, y, 0, 0, 0x0001 | 0x0004)  # SWP_NOSIZE | SWP_NOZORDER


def focus(hwnd):
    user32.SetForegroundWindow(hwnd)
    time.sleep(0.15)


def capture(hwnd, out_png=None):
    box = client_screen_rect(hwnd)
    img = ImageGrab.grab(bbox=box)
    if out_png:
        img.save(out_png)
    return img


# ---------------------------------------------------------------- 鼠标模拟

def _click(hwnd, x, y, button, down):
    msg = 0x0201 if button == "left" else 0x0204   # WM_LBUTTONDOWN / WM_RBUTTONDOWN
    if not down:
        msg += 1                                    # ...UP
    wparam = 0x0001 if button == "left" else 0x0002  # MK_LBUTTON / MK_RBUTTON
    lparam = (y << 16) | (x & 0xFFFF)
    user32.PostMessageW(hwnd, msg, wparam, lparam)
    time.sleep(0.05)


def lclick(hwnd, x, y):
    _click(hwnd, x, y, "left", True)
    _click(hwnd, x, y, "left", False)


def rclick(hwnd, x, y):
    _click(hwnd, x, y, "right", True)
    _click(hwnd, x, y, "right", False)
