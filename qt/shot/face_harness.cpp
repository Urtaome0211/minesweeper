// face_harness.cpp — 用与原版完全相同的 render.cpp 代码离线渲染笑脸到 DIB，
// 得到 GDI 的"地面真相"，供 Qt 版逐像素对齐。编译：
//   clang++ face_harness.cpp render.cpp game.cpp -lgdi32 -luser32 -o face_harness.exe
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstring>

#include "game.h"
#include "render.h"
#include "render.cpp"   // 同 TU 包含，以直接调用匿名命名空间内的 DrawSmiley

static void SaveBmp(const char* path, void* bits, int w, int h) {
    BITMAPFILEHEADER bf = {};
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(bi);
    bi.biWidth = w;
    bi.biHeight = -h;  // top-down
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bf.bfType = 0x4D42;
    bf.bfOffBits = sizeof(bf) + sizeof(bi);
    bf.bfSize = bf.bfOffBits + w * h * 4;
    FILE* f = nullptr;
    fopen_s(&f, path, "wb");
    if (!f) return;
    fwrite(&bf, sizeof(bf), 1, f);
    fwrite(&bi, sizeof(bi), 1, f);
    fwrite(bits, w * h * 4, 1, f);
    fclose(f);
}

int main() {
    RenderInit();
    const int S = 42;
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = S;
    bmi.bmiHeader.biHeight = -S;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HDC mem = CreateCompatibleDC(nullptr);
    HBITMAP bmp = CreateDIBSection(mem, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    SelectObject(mem, bmp);

    RECT rc = {0, 0, S, S};
    RECT full = {0, 0, S, S};
    GameState states[3] = {GameState::PLAYING, GameState::LOST, GameState::WON};
    const char* names[3] = {"face_playing.bmp", "face_lost.bmp", "face_won.bmp"};
    for (int i = 0; i < 3; i++) {
        HBRUSH bg = CreateSolidBrush(RGB(1, 2, 3));
        FillRect(mem, &full, bg);
        DeleteObject(bg);
        DrawSmiley(mem, rc, states[i], false);
        SaveBmp(names[i], bits, S, S);
    }
    DeleteObject(bmp);
    DeleteDC(mem);
    RenderCleanup();
    printf("done\n");
    return 0;
}
