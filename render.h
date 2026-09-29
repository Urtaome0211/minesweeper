// render.h — 棋盘绘制（GDI）
#pragma once

#include <windows.h>
#include "game.h"

// 布局常量（像素）
constexpr int CELL     = 30;
constexpr int HEADER_H = 62;
constexpr int PAD      = 10;
constexpr int GAP      = 8;

// GDI 资源生命周期（窗口创建/销毁时调用）
void RenderInit();
void RenderCleanup();

// 布局辅助
void ComputeClientSize(int rows, int cols, int& width, int& height);
RECT CellRect(int r, int c);
RECT HeaderRect(int cols);
RECT SmileyRect(int cols);
bool BoardHit(POINT pt, int rows, int cols, int& r, int& c);

// 主绘制入口（含双缓冲）
void RenderClient(HDC hdc, const Game& game, bool smileDown, const RECT& client);

// 是否还有动画进行中（用于驱动动画重绘）
bool RenderHasActiveAnimation(const Game& game);
