// harness2.cpp — 用与原版完全相同的 render.cpp 代码离线渲染"地面真相"：
//   1) 数字 1..8 的整格合成图（翻开格渐变+边框+数字），30x30 不透明；
//   2) 地雷 R=2..7 的黑色(含高光)/红色变体，14x14，背景 (1,2,3) 作透明哨兵；
//   3) 错旗红叉 X，30x30 透明底；
//   4) 计数器若干值（诊断用）；
//   5) 三张完整棋盘（lost / won789 / won46），290x360，与 Qt --demo-cells 布局一致。
// 编译：
//   clang++ harness2.cpp render.cpp game.cpp -lgdi32 -luser32 -o harness2.exe
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>

#include "game.h"
#include "render.h"
#include "render.cpp"   // 同 TU 包含，直接调用其内部函数

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

// 建立一个 WxH 的 32bpp DIB 并把视口原点移到 (ox,oy)（画在 (x,y) 的点落在缓冲 (x-ox,y-oy)）
struct Dib {
    HDC dc = nullptr;
    HBITMAP bmp = nullptr;
    void* bits = nullptr;
    int w = 0, h = 0, ox = 0, oy = 0;
    bool Init(int W, int H, int ox_, int oy_) {
        w = W; h = H; ox = ox_; oy = oy_;
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = W;
        bmi.bmiHeader.biHeight = -H;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        dc = CreateCompatibleDC(nullptr);
        bmp = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!dc || !bmp) return false;
        SelectObject(dc, bmp);
        SetViewportOrgEx(dc, ox, oy, nullptr);
        return true;
    }
    void Fill(int r, int g, int b) {
        // FillRect 走设备坐标，视口原点偏移时会被裁剪 —— 先复位再恢复
        POINT old;
        SetViewportOrgEx(dc, 0, 0, &old);
        RECT rc = {0, 0, w, h};
        HBRUSH br = CreateSolidBrush(RGB(r, g, b));
        FillRect(dc, &rc, br);
        DeleteObject(br);
        SetViewportOrgEx(dc, old.x, old.y, nullptr);
    }
    ~Dib() { if (bmp) DeleteObject(bmp); if (dc) DeleteDC(dc); }
};

// ---- 数字整格合成图 ----
static void RenderDigitComposites() {
    Dib d;
    d.Init(30, 30, -10, -80);
    RECT rc = {10, 80, 40, 110};
    char name[64];
    for (int n = 1; n <= 8; n++) {
        FillGradientV(d.dc, rc, kColOpenTop, kColOpenBot);
        FrameRect(d.dc, &rc, g_brushBorder);
        DrawNumber(d.dc, rc, n);
        sprintf_s(name, "cell_digit%d.bmp", n);
        SaveBmp(name, d.bits, 30, 30);
    }
}

// ---- 地雷 R 变体 ----
static void RenderMines() {
    Dib d;
    d.Init(14, 14, -18, -88);
    char name[64];
    for (int R = 2; R <= 7; R++) {
        for (int red = 0; red <= 1; red++) {
            d.Fill(1, 2, 3);
            HGDIOBJ ob = SelectObject(d.dc, red ? g_brushRed : g_brushBlack);
            HGDIOBJ op = SelectObject(d.dc, GetStockObject(NULL_PEN));
            Ellipse(d.dc, 25 - R, 95 - R, 25 + R, 95 + R);
            SelectObject(d.dc, ob);
            if (!red) {
                int hl = R > 3 ? R / 3 : 1;
                HGDIOBJ ow = SelectObject(d.dc, g_brushWhite);
                Ellipse(d.dc, 25 - R + hl, 95 - R + hl, 25 - hl, 95 - hl);
                SelectObject(d.dc, ow);
            }
            SelectObject(d.dc, op);
            sprintf_s(name, "cell_mine_%s_R%d.bmp", red ? "red" : "black", R);
            SaveBmp(name, d.bits, 14, 14);
        }
    }
}

// ---- 错旗红叉 ----
static void RenderWrongX() {
    Dib d;
    d.Init(30, 30, -10, -80);
    d.Fill(1, 2, 3);
    RECT rc = {10, 80, 40, 110};
    DrawWrongFlag(d.dc, rc);
    SaveBmp("cell_wrongx.bmp", d.bits, 30, 30);
}

// ---- 计数器（诊断） ----
static void RenderCounters() {
    Dib d;
    d.Init(62, 42, -16, -18);
    RECT rc = {16, 18, 78, 60};
    int vals[] = {-99, -5, 0, 46, 123, 789, 999};
    const char* names[] = {"counter_minus99.bmp", "counter_minus5.bmp", "counter_0.bmp",
                           "counter_046.bmp", "counter_123.bmp", "counter_789.bmp", "counter_999.bmp"};
    for (int i = 0; i < 7; i++) {
        DrawCounter(d.dc, rc, vals[i]);
        SaveBmp(names[i], d.bits, 62, 42);
    }
}

// ---- 整板 ----
struct TestCell { bool mine = false, revealed = false, flagged = false; int adj = 0; };

static void ParseBoard(const char* table, TestCell* cells) {
    // table: 81 字符，逐格。H=隐藏 F=隐藏+旗 X=隐藏+旗+错叉(非雷)
    // M=已翻开的雷 D=踩中的红雷 0..8=已翻开的数字格
    for (int i = 0; i < 81; i++) {
        char t = table[i];
        TestCell& c = cells[i];
        c = TestCell();
        switch (t) {
            case 'H': break;
            case 'F': c.flagged = true; break;
            case 'X': c.flagged = true; break;
            case 'M': c.mine = true; c.revealed = true; break;
            case 'D': c.mine = true; c.revealed = true; break;
            default:  c.revealed = true; c.adj = t - '0'; break;
        }
    }
}

static void RenderBoard(const TestCell* cells, GameState state, int deadR, int deadC,
                        int mines, int flags, int time, const char* path) {
    const int rows = 9, cols = 9;
    const int W = 2 * 10 + cols * 30, H = 2 * 10 + 62 + 8 + rows * 30;
    Dib d;
    d.Init(W, H, 0, 0);
    RECT client = {0, 0, W, H};
    FillGradientV(d.dc, client, kColBgTop, kColBgBot);

    RECT panel = HeaderRect(cols);
    FillGradientV(d.dc, panel, kColPanelTop, kColPanelBot);
    FrameRect(d.dc, &panel, g_brushBorder);
    RECT lc = {panel.left + 6, panel.top + 8, panel.left + 68, panel.top + 50};
    DrawCounter(d.dc, lc, mines - flags);
    DrawSmiley(d.dc, SmileyRect(cols), state, false);
    RECT rc2 = {panel.right - 68, panel.top + 8, panel.right - 6, panel.top + 50};
    DrawCounter(d.dc, rc2, time);

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            const TestCell& cell = cells[r * cols + c];
            RECT rc = CellRect(r, c);
            if (!cell.revealed) {
                DrawHiddenCell(d.dc, rc);
                if (cell.flagged) DrawFlag(d.dc, rc, 1.0f);
                if (state == GameState::LOST && cell.flagged && !cell.mine)
                    DrawWrongFlag(d.dc, rc);
            } else {
                FillGradientV(d.dc, rc, kColOpenTop, kColOpenBot);
                FrameRect(d.dc, &rc, g_brushBorder);
                if (cell.mine) DrawMine(d.dc, rc, (r == deadR && c == deadC), 1.0f);
                else if (cell.adj > 0) DrawNumber(d.dc, rc, cell.adj);
            }
        }
    }
    SaveBmp(path, d.bits, W, H);
}

static void RenderBoards() {
    const char* lost =
        "012345678"
        "HFFFFFFFF"
        "FX0000000"
        "0MDM00000"
        "00M0M0000"
        "FFFFHHHHH"
        "FHHHH0000"
        "000000000"
        "00000000H";
    const char* won =
        "123456780"
        "FFFFFFFFF"
        "F00000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000";
    TestCell cells[81];

    ParseBoard(lost, cells);
    // 旗子数：行1有8，行2有2(F+X)，行5有4，行6有1(FM) → 15；雷数 10 → 左计数器 -5
    RenderBoard(cells, GameState::LOST, 3, 2, 10, 15, 123, "board_lost.bmp");

    ParseBoard(won, cells);
    RenderBoard(cells, GameState::WON, -1, -1, 10, 10, 789, "board_won789.bmp");
    RenderBoard(cells, GameState::WON, -1, -1, 10, 10, 46, "board_won046.bmp");
}

int main() {
    RenderInit();
    RenderDigitComposites();
    RenderMines();
    RenderWrongX();
    RenderCounters();
    RenderBoards();
    RenderCleanup();
    printf("done\n");
    return 0;
}
