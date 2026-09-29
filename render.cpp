// render.cpp — 棋盘绘制实现（Windows 7 蓝色 Aero 风格 + 动画）
#include "render.h"

#include <cmath>

namespace {

// ---- 配色（Windows 7 蓝色风格）----
const COLORREF kColBgTop    = RGB(206, 228, 248);   // 棋盘背景渐变(上)
const COLORREF kColBgBot    = RGB(147, 190, 230);   // 棋盘背景渐变(下)
const COLORREF kColCellTop  = RGB(255, 255, 255);   // 未翻开格子(上, 高光)
const COLORREF kColCellBot  = RGB(199, 221, 244);   // 未翻开格子(下)
const COLORREF kColOpenTop  = RGB(219, 233, 250);   // 翻开格子(上)
const COLORREF kColOpenBot  = RGB(199, 221, 244);   // 翻开格子(下)
const COLORREF kColPanelTop = RGB(232, 242, 253);   // 顶部面板(上)
const COLORREF kColPanelBot = RGB(181, 210, 238);   // 顶部面板(下)
const COLORREF kColBorder   = RGB(141, 174, 208);   // 格子边框

// ---- 动画时长(ms) ----
constexpr unsigned kPopMs     = 160;   // 翻开/地雷弹出
constexpr unsigned kFlagPopMs = 120;   // 插旗弹出
constexpr unsigned kAnimWin   = 200;   // 判定动画是否结束的窗口

HBRUSH g_brushBlack, g_brushRed, g_brushYellow, g_brushWhite, g_brushBorder;
HPEN   g_penBlack, g_penRed;
HFONT  g_fontNum;

// ---- 渐变 / 缩放辅助 ----
void FillGradientV(HDC hdc, const RECT& rc, COLORREF top, COLORREF bottom) {
    TRIVERTEX v[2];
    v[0].x = rc.left;  v[0].y = rc.top;
    v[0].Red   = GetRValue(top)    << 8;
    v[0].Green = GetGValue(top)    << 8;
    v[0].Blue  = GetBValue(top)    << 8;
    v[0].Alpha = 0x0000;
    v[1].x = rc.right; v[1].y = rc.bottom;
    v[1].Red   = GetRValue(bottom) << 8;
    v[1].Green = GetGValue(bottom) << 8;
    v[1].Blue  = GetBValue(bottom) << 8;
    v[1].Alpha = 0x0000;
    GRADIENT_RECT gr = { 0, 1 };
    GradientFill(hdc, v, 2, &gr, 1, GRADIENT_FILL_RECT_V);
}

RECT ScaleRect(const RECT& rc, float s) {
    int dw = (int)((rc.right - rc.left) * (1.0f - s) * 0.5f);
    int dh = (int)((rc.bottom - rc.top) * (1.0f - s) * 0.5f);
    RECT r;
    r.left   = rc.left + dw;
    r.top    = rc.top + dh;
    r.right  = rc.right - dw;
    r.bottom = rc.bottom - dh;
    return r;
}

// ---- 七段数码管 ----
enum Seg {
    SEG_A = 1 << 0, SEG_B = 1 << 1, SEG_C = 1 << 2, SEG_D = 1 << 3,
    SEG_E = 1 << 4, SEG_F = 1 << 5, SEG_G = 1 << 6
};

const BYTE g_segDigits[10] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,              // 0
    SEG_B | SEG_C,                                              // 1
    SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,                      // 2
    SEG_A | SEG_B | SEG_G | SEG_C | SEG_D,                      // 3
    SEG_F | SEG_G | SEG_B | SEG_C,                              // 4
    SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,                      // 5
    SEG_A | SEG_F | SEG_G | SEG_E | SEG_C | SEG_D,              // 6
    SEG_A | SEG_B | SEG_C,                                      // 7
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,      // 8
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,              // 9
};

void SegRect(HDC hdc, int x, int y, int w, int h) {
    RECT r = { x, y, x + w, y + h };
    FillRect(hdc, &r, g_brushRed);
}

void DrawDigit7(HDC hdc, int x, int y, int w, int h, int digit) {
    int t   = w >= 20 ? 4 : 3;
    int mid = h / 2;
    BYTE m  = g_segDigits[digit];
    if (m & SEG_A) SegRect(hdc, x + t, y,             w - 2 * t, t);
    if (m & SEG_F) SegRect(hdc, x,     y + t,         t,          mid - t);
    if (m & SEG_B) SegRect(hdc, x + w - t, y + t,     t,          mid - t);
    if (m & SEG_G) SegRect(hdc, x + t, y + mid - t / 2, w - 2 * t, t);
    if (m & SEG_E) SegRect(hdc, x,     y + mid,       t,          h - mid - t);
    if (m & SEG_C) SegRect(hdc, x + w - t, y + mid,   t,          h - mid - t);
    if (m & SEG_D) SegRect(hdc, x + t, y + h - t,     w - 2 * t, t);
}

void DrawMinus7(HDC hdc, int x, int y, int w, int h) {
    int t = w >= 20 ? 4 : 3;
    SegRect(hdc, x + t, y + h / 2 - t / 2, w - 2 * t, t);
}

// ---- 元素绘制 ----
void DrawHiddenCell(HDC hdc, RECT rc) {
    FillGradientV(hdc, rc, kColCellTop, kColCellBot);
    FrameRect(hdc, &rc, g_brushBorder);
}

void DrawFlag(HDC hdc, RECT rc, float s) {
    RECT r = ScaleRect(rc, s);
    int cx = (r.left + r.right) / 2;
    int top = r.top + 5, bot = r.bottom - 5;
    HGDIOBJ old = SelectObject(hdc, g_penBlack);
    MoveToEx(hdc, cx, top, nullptr); LineTo(hdc, cx, bot);
    MoveToEx(hdc, cx - 5, bot - 1, nullptr); LineTo(hdc, cx + 5, bot - 1);
    SelectObject(hdc, old);
    HGDIOBJ ob = SelectObject(hdc, g_brushRed);
    HGDIOBJ op = SelectObject(hdc, GetStockObject(NULL_PEN));
    POINT tri[3] = { {cx + 1, top}, {cx + 1, top + 8}, {cx - 6, top + 4} };
    Polygon(hdc, tri, 3);
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
}

void DrawWrongFlag(HDC hdc, RECT rc) {
    HGDIOBJ old = SelectObject(hdc, g_penRed);
    MoveToEx(hdc, rc.left + 6, rc.top + 6, nullptr);
    LineTo(hdc, rc.right - 6, rc.bottom - 6);
    MoveToEx(hdc, rc.right - 6, rc.top + 6, nullptr);
    LineTo(hdc, rc.left + 6, rc.bottom - 6);
    SelectObject(hdc, old);
}

void DrawMine(HDC hdc, RECT rc, bool red, float s) {
    int cx = (rc.left + rc.right) / 2, cy = (rc.top + rc.bottom) / 2;
    int R = (int)(7.0f * s);
    if (R < 1) return;
    HGDIOBJ ob = SelectObject(hdc, red ? g_brushRed : g_brushBlack);
    HGDIOBJ op = SelectObject(hdc, GetStockObject(NULL_PEN));
    Ellipse(hdc, cx - R, cy - R, cx + R, cy + R);
    SelectObject(hdc, ob);
    if (!red) {
        // 高光（光泽球体）
        int hl = R > 3 ? R / 3 : 1;
        HGDIOBJ ow = SelectObject(hdc, g_brushWhite);
        Ellipse(hdc, cx - R + hl, cy - R + hl, cx - hl, cy - hl);
        SelectObject(hdc, ow);
    }
    SelectObject(hdc, op);
}

void DrawNumber(HDC hdc, RECT rc, int n) {
    static const COLORREF colors[9] = {
        0,
        RGB(0, 0, 255),     // 1 蓝
        RGB(0, 128, 0),     // 2 绿
        RGB(255, 0, 0),     // 3 红
        RGB(0, 0, 128),     // 4 深蓝
        RGB(128, 0, 0),     // 5 深红
        RGB(0, 128, 128),   // 6 青
        RGB(0, 0, 0),       // 7 黑
        RGB(128, 128, 128)  // 8 灰
    };
    SetTextColor(hdc, colors[n]);
    SetBkMode(hdc, TRANSPARENT);
    HGDIOBJ old = SelectObject(hdc, g_fontNum);
    wchar_t s[2] = { (wchar_t)(L'0' + n), 0 };
    DrawText(hdc, s, 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
}

void DrawCounter(HDC hdc, RECT rc, int value) {
    int v = value < -99 ? -99 : (value > 999 ? 999 : value);
    FillRect(hdc, &rc, g_brushBlack);
    FrameRect(hdc, &rc, g_brushBorder);

    int digitW = 19, digitH = 34;
    int totalW = 3 * digitW;
    int x0 = rc.left + ((rc.right - rc.left) - totalW) / 2;
    int y0 = rc.top + (rc.bottom - rc.top - digitH) / 2;

    int absv = v < 0 ? -v : v;
    int hundreds = absv / 100 % 10;
    int tens     = absv / 10 % 10;
    int ones     = absv % 10;

    if (v < 0) {
        DrawMinus7(hdc, x0, y0, digitW, digitH);
        DrawDigit7(hdc, x0 + digitW, y0, digitW, digitH, tens);
        DrawDigit7(hdc, x0 + 2 * digitW, y0, digitW, digitH, ones);
    } else {
        DrawDigit7(hdc, x0, y0, digitW, digitH, hundreds);
        DrawDigit7(hdc, x0 + digitW, y0, digitW, digitH, tens);
        DrawDigit7(hdc, x0 + 2 * digitW, y0, digitW, digitH, ones);
    }
}

void DrawSmiley(HDC hdc, RECT rc, GameState state, bool down) {
    // 蓝色光泽按钮
    FillGradientV(hdc, rc, down ? kColCellBot : kColCellTop, down ? kColCellTop : kColCellBot);
    FrameRect(hdc, &rc, g_brushBorder);

    int cx = (rc.left + rc.right) / 2, cy = (rc.top + rc.bottom) / 2;
    HGDIOBJ ob = SelectObject(hdc, g_brushYellow);
    HGDIOBJ op = SelectObject(hdc, g_penBlack);
    Ellipse(hdc, cx - 11, cy - 11, cx + 11, cy + 11);
    // 高光
    SelectObject(hdc, g_brushWhite);
    SelectObject(hdc, GetStockObject(NULL_PEN));
    Ellipse(hdc, cx - 7, cy - 9, cx - 1, cy - 4);
    SelectObject(hdc, g_penBlack);

    if (state == GameState::LOST) {
        MoveToEx(hdc, cx - 7, cy - 5, nullptr); LineTo(hdc, cx - 3, cy - 1);
        MoveToEx(hdc, cx - 3, cy - 5, nullptr); LineTo(hdc, cx - 7, cy - 1);
        MoveToEx(hdc, cx + 3, cy - 5, nullptr); LineTo(hdc, cx + 7, cy - 1);
        MoveToEx(hdc, cx + 7, cy - 5, nullptr); LineTo(hdc, cx + 3, cy - 1);
        Arc(hdc, cx - 6, cy + 3, cx + 6, cy + 11, cx - 6, cy + 7, cx + 6, cy + 7);
    } else if (state == GameState::WON) {
        HGDIOBJ ob2 = SelectObject(hdc, g_brushBlack);
        HGDIOBJ op2 = SelectObject(hdc, GetStockObject(NULL_PEN));
        Rectangle(hdc, cx - 8, cy - 5, cx - 1, cy + 1);
        Rectangle(hdc, cx + 1, cy - 5, cx + 8, cy + 1);
        SelectObject(hdc, ob2);
        SelectObject(hdc, op2);
        MoveToEx(hdc, cx - 1, cy - 2, nullptr); LineTo(hdc, cx + 1, cy - 2);
        Arc(hdc, cx - 6, cy + 1, cx + 6, cy + 9, cx + 6, cy + 5, cx - 6, cy + 5);
    } else {
        HGDIOBJ ob2 = SelectObject(hdc, g_brushBlack);
        HGDIOBJ op2 = SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, cx - 6, cy - 4, cx - 3, cy - 1);
        Ellipse(hdc, cx + 3, cy - 4, cx + 6, cy - 1);
        SelectObject(hdc, ob2);
        SelectObject(hdc, op2);
        Arc(hdc, cx - 6, cy + 1, cx + 6, cy + 9, cx + 6, cy + 5, cx - 6, cy + 5);
    }
    SelectObject(hdc, op);
}

void DrawRevealed(HDC hdc, RECT rc, const Cell& cell, const Game& game, int r, int c, float s, unsigned now) {
    RECT inner = ScaleRect(rc, s);
    FillGradientV(hdc, inner, kColOpenTop, kColOpenBot);
    FrameRect(hdc, &inner, g_brushBorder);

    if (cell.mine) {
        float ms = 1.0f;
        if (cell.mineStamp) {
            int age = (int)(now - cell.mineStamp);
            if (age < 0) ms = 0.0f;
            else if (age < (int)kPopMs) ms = 0.4f + 0.6f * age / (float)kPopMs;
        }
        DrawMine(hdc, inner, (r == game.DeadRow() && c == game.DeadCol()), ms);
    } else if (cell.adj > 0) {
        if (s >= 0.8f) DrawNumber(hdc, inner, cell.adj);   // 弹出接近完成再显示数字
    }
}

void DrawCell(HDC hdc, int r, int c, const Game& game) {
    RECT rc = CellRect(r, c);
    const Cell& cell = game.Board()[r][c];
    unsigned now = GetTickCount();

    if (!cell.revealed) {
        DrawHiddenCell(hdc, rc);
        if (cell.flagged) {
            float s = 1.0f;
            if (cell.flagStamp) {
                int age = (int)(now - cell.flagStamp);
                if (age < (int)kFlagPopMs) s = 0.6f + 0.4f * (age < 0 ? 0 : age) / (float)kFlagPopMs;
            }
            DrawFlag(hdc, rc, s);
        }
        if (game.State() == GameState::LOST && cell.flagged && !cell.mine)
            DrawWrongFlag(hdc, rc);   // 标错的旗子打叉
    } else {
        if (cell.revealStamp) {
            int age = (int)(now - cell.revealStamp);
            if (age < 0) { DrawHiddenCell(hdc, rc); return; }   // 波浪尚未到达
            float s = (age >= (int)kPopMs) ? 1.0f : (0.55f + 0.45f * age / (float)kPopMs);
            DrawRevealed(hdc, rc, cell, game, r, c, s, now);
        } else {
            DrawRevealed(hdc, rc, cell, game, r, c, 1.0f, now);
        }
    }
}

void DrawHeader(HDC hdc, const Game& game, bool smileDown) {
    RECT panel = HeaderRect(game.Cols());
    FillGradientV(hdc, panel, kColPanelTop, kColPanelBot);
    FrameRect(hdc, &panel, g_brushBorder);

    RECT lc = { panel.left + 6, panel.top + 8, panel.left + 68, panel.top + 50 };
    DrawCounter(hdc, lc, game.Mines() - game.FlagCount());

    DrawSmiley(hdc, SmileyRect(game.Cols()), game.State(), smileDown);

    RECT rc = { panel.right - 68, panel.top + 8, panel.right - 6, panel.top + 50 };
    DrawCounter(hdc, rc, game.Time());
}

void DrawGame(HDC hdc, const RECT& client, const Game& game, bool smileDown) {
    FillGradientV(hdc, client, kColBgTop, kColBgBot);
    DrawHeader(hdc, game, smileDown);
    for (int r = 0; r < game.Rows(); ++r)
        for (int c = 0; c < game.Cols(); ++c)
            DrawCell(hdc, r, c, game);
}

} // namespace

void RenderInit() {
    g_brushBlack  = CreateSolidBrush(RGB(0, 0, 0));
    g_brushRed    = CreateSolidBrush(RGB(255, 0, 0));
    g_brushYellow = CreateSolidBrush(RGB(255, 255, 0));
    g_brushWhite  = CreateSolidBrush(RGB(255, 255, 255));
    g_brushBorder = CreateSolidBrush(kColBorder);
    g_penBlack    = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    g_penRed      = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
    g_fontNum     = CreateFont(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Tahoma");
}

void RenderCleanup() {
    DeleteObject(g_brushBlack);
    DeleteObject(g_brushRed);
    DeleteObject(g_brushYellow);
    DeleteObject(g_brushWhite);
    DeleteObject(g_brushBorder);
    DeleteObject(g_penBlack);
    DeleteObject(g_penRed);
    DeleteObject(g_fontNum);
}

void ComputeClientSize(int rows, int cols, int& width, int& height) {
    width  = 2 * PAD + cols * CELL;
    height = 2 * PAD + HEADER_H + GAP + rows * CELL;
}

RECT CellRect(int r, int c) {
    RECT rc;
    rc.left   = PAD + c * CELL;
    rc.top    = PAD + HEADER_H + GAP + r * CELL;
    rc.right  = rc.left + CELL;
    rc.bottom = rc.top + CELL;
    return rc;
}

RECT HeaderRect(int cols) {
    RECT rc = { PAD, PAD, PAD + cols * CELL, PAD + HEADER_H };
    return rc;
}

RECT SmileyRect(int cols) {
    RECT rc;
    int cx = PAD + cols * CELL / 2;
    rc.left   = cx - 21;
    rc.top    = PAD + 8;
    rc.right  = cx + 21;
    rc.bottom = PAD + 8 + 42;
    return rc;
}

bool BoardHit(POINT pt, int rows, int cols, int& r, int& c) {
    int bx = PAD, by = PAD + HEADER_H + GAP;
    if (pt.x < bx || pt.y < by) return false;
    int cc = (pt.x - bx) / CELL;
    int rr = (pt.y - by) / CELL;
    if (rr < 0 || rr >= rows || cc < 0 || cc >= cols) return false;
    r = rr;
    c = cc;
    return true;
}

bool RenderHasActiveAnimation(const Game& game) {
    unsigned now = GetTickCount();
    const auto& board = game.Board();
    for (const auto& row : board)
        for (const Cell& cell : row) {
            if (cell.revealStamp && (int)(now - cell.revealStamp) < (int)kAnimWin) return true;
            if (cell.flagStamp   && (int)(now - cell.flagStamp)   < (int)kAnimWin) return true;
            if (cell.mineStamp   && (int)(now - cell.mineStamp)   < (int)kAnimWin) return true;
        }
    return false;
}

void RenderClient(HDC hdc, const Game& game, bool smileDown, const RECT& client) {
    int w = client.right - client.left;
    int h = client.bottom - client.top;
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldb = (HBITMAP)SelectObject(mem, bmp);
    DrawGame(mem, client, game, smileDown);
    BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldb);
    DeleteObject(bmp);
    DeleteDC(mem);
}
