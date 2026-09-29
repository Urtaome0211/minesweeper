// app.cpp — 程序入口、窗口与消息处理
// 注：UNICODE/_UNICODE 由编译参数统一定义（见 build.bat）
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <cstdlib>
#include <ctime>

#include "game.h"
#include "render.h"
#include "dialogs.h"

namespace {

constexpr int TIMER_ID   = 1;   // 1 秒计时
constexpr int TIMER_ANIM = 2;   // 动画驱动（约 60fps）

constexpr int IDM_NEW    = 100;
constexpr int IDM_BEGIN  = 101;
constexpr int IDM_INTER  = 102;
constexpr int IDM_EXPERT = 103;
constexpr int IDM_CUSTOM = 104;
constexpr int IDM_ABOUT  = 105;

const DWORD kStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

HINSTANCE g_hInst;
HWND      g_hwnd;
Game      g_game;
bool      g_smileDown   = false;
int       g_customRows  = 16;
int       g_customCols  = 16;
int       g_customMines = 40;

const Difficulty g_presets[] = {
    { 9,  9, 10, L"初级" },
    { 16, 16, 40, L"中级" },
    { 16, 30, 99, L"高级" },
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// 依据当前棋盘尺寸调整窗口大小并触发重绘
void ResetWindowAndInvalidate() {
    int cw, ch;
    ComputeClientSize(g_game.Rows(), g_game.Cols(), cw, ch);
    RECT rc = { 0, 0, cw, ch };
    AdjustWindowRectEx(&rc, kStyle, TRUE, 0);
    SetWindowPos(g_hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                 SWP_NOMOVE | SWP_NOZORDER);
    InvalidateRect(g_hwnd, nullptr, TRUE);
}

void NewGame() {
    g_game.Restart();
    ResetWindowAndInvalidate();
}

void SetDifficulty(int idx) {
    const Difficulty& d = (idx < 3)
        ? g_presets[idx]
        : Difficulty{ g_customRows, g_customCols, g_customMines, L"自定义" };
    g_game.NewGame(d.rows, d.cols, d.mines);
    wchar_t title[64];
    wsprintf(title, L"扫雷 - %s (%d×%d, %d 雷)", d.name, d.rows, d.cols, d.mines);
    SetWindowText(g_hwnd, title);
    ResetWindowAndInvalidate();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        RenderInit();

        // 菜单
        HMENU menu = CreateMenu();
        HMENU game = CreatePopupMenu();
        AppendMenu(game, MF_STRING, IDM_NEW,    L"新游戏(&N)\tF2");
        AppendMenu(game, MF_SEPARATOR, 0, nullptr);
        AppendMenu(game, MF_STRING, IDM_BEGIN,  L"初级(&B)");
        AppendMenu(game, MF_STRING, IDM_INTER,  L"中级(&I)");
        AppendMenu(game, MF_STRING, IDM_EXPERT, L"高级(&E)");
        AppendMenu(game, MF_STRING, IDM_CUSTOM, L"自定义(&C)...");
        AppendMenu(menu, MF_POPUP, (UINT_PTR)game, L"游戏(&G)");
        HMENU help = CreatePopupMenu();
        AppendMenu(help, MF_STRING, IDM_ABOUT, L"关于(&A)...");
        AppendMenu(menu, MF_POPUP, (UINT_PTR)help, L"帮助(&H)");
        SetMenu(hwnd, menu);

        SetTimer(hwnd, TIMER_ID, 1000, nullptr);
        SetTimer(hwnd, TIMER_ANIM, 16, nullptr);
        return 0;
    }

    case WM_TIMER:
        if (wParam == TIMER_ID) {
            g_game.Tick();
            if (g_game.State() == GameState::PLAYING) {
                RECT rc = HeaderRect(g_game.Cols());
                InvalidateRect(hwnd, &rc, FALSE);
            }
        } else if (wParam == TIMER_ANIM) {
            if (RenderHasActiveAnimation(g_game))
                InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT client;
        GetClientRect(hwnd, &client);
        RenderClient(hdc, g_game, g_smileDown, client);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_PRINTCLIENT: {
        RECT client;
        GetClientRect(hwnd, &client);
        RenderClient((HDC)wParam, g_game, g_smileDown, client);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        SetFocus(hwnd);
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT sr = SmileyRect(g_game.Cols());
        if (PtInRect(&sr, pt)) {
            g_smileDown = true;
            SetCapture(hwnd);
            InvalidateRect(hwnd, &sr, FALSE);
            return 0;
        }
        int r, c;
        if (BoardHit(pt, g_game.Rows(), g_game.Cols(), r, c)) {
            if ((GetKeyState(VK_RBUTTON) & 0x8000) && g_game.IsRevealed(r, c))
                g_game.Chord(r, c);
            else
                g_game.Reveal(r, c);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (g_smileDown) {
            g_smileDown = false;
            ReleaseCapture();
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT sr = SmileyRect(g_game.Cols());
            if (PtInRect(&sr, pt)) NewGame();
            else InvalidateRect(hwnd, &sr, FALSE);
        }
        return 0;
    }

    case WM_CAPTURECHANGED:
        g_smileDown = false;
        return 0;

    case WM_RBUTTONDOWN: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        int r, c;
        if (BoardHit(pt, g_game.Rows(), g_game.Cols(), r, c)) {
            if ((GetKeyState(VK_LBUTTON) & 0x8000) && g_game.IsRevealed(r, c))
                g_game.Chord(r, c);
            else
                g_game.ToggleFlag(r, c);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_F2) NewGame();
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_NEW:    NewGame(); break;
        case IDM_BEGIN:  SetDifficulty(0); break;
        case IDM_INTER:  SetDifficulty(1); break;
        case IDM_EXPERT: SetDifficulty(2); break;
        case IDM_CUSTOM:
            if (ShowCustomDialog(hwnd, g_customRows, g_customCols, g_customMines))
                SetDifficulty(3);
            break;
        case IDM_ABOUT:
            MessageBox(hwnd,
                       L"扫雷 Minesweeper\n\nC++ / Win32 实现\n\n"
                       L"左键翻开，右键插旗，\n"
                       L"在数字上左右同按可快速翻开。",
                       L"关于扫雷", MB_OK | MB_ICONINFORMATION);
            break;
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        KillTimer(hwnd, TIMER_ANIM);
        RenderCleanup();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

} // namespace

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInst = hInst;
    std::srand((unsigned)std::time(nullptr));

    WNDCLASSW wc = {};
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"MinesweeperWnd";
    if (!RegisterClassW(&wc)) return 1;

    // 初始大小（初级 9×9）
    int cw, ch;
    ComputeClientSize(g_game.Rows(), g_game.Cols(), cw, ch);
    RECT rc = { 0, 0, cw, ch };
    AdjustWindowRectEx(&rc, kStyle, TRUE, 0);

    g_hwnd = CreateWindowExW(0, L"MinesweeperWnd", L"扫雷 - 初级 (9×9, 10 雷)",
                             kStyle, CW_USEDEFAULT, CW_USEDEFAULT,
                             rc.right - rc.left, rc.bottom - rc.top,
                             nullptr, nullptr, hInst, nullptr);
    if (!g_hwnd) return 1;

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
