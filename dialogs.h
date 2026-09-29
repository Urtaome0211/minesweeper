// dialogs.h — 自定义难度对话框
#pragma once

#include <windows.h>

// 显示"自定义难度"对话框；返回 true 表示确定，并回填新的行列雷数
bool ShowCustomDialog(HWND parent, int& rows, int& cols, int& mines);
