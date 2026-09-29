// dialogs.cpp — 自定义难度对话框实现（内存对话框模板，无需 .rc 资源）
#include "dialogs.h"

#include <vector>

namespace {

constexpr int IDC_ROWS  = 1001;
constexpr int IDC_COLS  = 1002;
constexpr int IDC_MINES = 1003;

struct DlgData {
    int rows, cols, mines;
};

void AppendWord(std::vector<BYTE>& b, WORD w) {
    b.push_back((BYTE)(w & 0xFF));
    b.push_back((BYTE)(w >> 8));
}
void AppendDword(std::vector<BYTE>& b, DWORD d) {
    AppendWord(b, (WORD)(d & 0xFFFF));
    AppendWord(b, (WORD)(d >> 16));
}
void AppendWStr(std::vector<BYTE>& b, const wchar_t* s) {
    for (; *s; ++s) AppendWord(b, (WORD)*s);
    AppendWord(b, 0);
}
void Align4(std::vector<BYTE>& b) {
    while (b.size() % 4) b.push_back(0);
}

std::vector<BYTE> BuildTemplate() {
    std::vector<BYTE> b;

    // DLGTEMPLATE 头部（逐字段写入，避免结构体对齐填充问题）
    AppendDword(b, WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_SETFONT | DS_CENTER); // style
    AppendDword(b, 0);              // dwExtendedStyle
    AppendWord(b, 8);               // cdit = 3 标签 + 3 编辑框 + 2 按钮
    AppendWord(b, 0);               // x
    AppendWord(b, 0);               // y
    AppendWord(b, 160);             // cx
    AppendWord(b, 82);              // cy
    AppendWord(b, 0);               // menu
    AppendWord(b, 0);               // windowClass
    AppendWStr(b, L"自定义难度");    // title
    AppendWord(b, 9);               // 字号
    AppendWStr(b, L"MS Shell Dlg"); // 字体名

    auto AddItem = [&](WORD id, WORD cls, const wchar_t* text,
                       DWORD style, int x, int y, int cx, int cy) {
        Align4(b);
        AppendDword(b, style);
        AppendDword(b, 0);          // exStyle
        AppendWord(b, (WORD)x);
        AppendWord(b, (WORD)y);
        AppendWord(b, (WORD)cx);
        AppendWord(b, (WORD)cy);
        AppendWord(b, id);
        AppendWord(b, 0xFFFF);
        AppendWord(b, cls);         // 控件类（序号）
        if (text) AppendWStr(b, text);
        else       AppendWord(b, 0);
        AppendWord(b, 0);           // 附加创建数据长度
    };

    // 标签
    AddItem(0, 0x0082, L"行数:",    WS_CHILD | WS_VISIBLE, 12, 14, 40, 12);
    AddItem(0, 0x0082, L"列数:",    WS_CHILD | WS_VISIBLE, 12, 30, 40, 12);
    AddItem(0, 0x0082, L"地雷数:",  WS_CHILD | WS_VISIBLE, 12, 46, 40, 12);
    // 编辑框
    AddItem(IDC_ROWS,  0x0081, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER, 56, 12, 65, 12);
    AddItem(IDC_COLS,  0x0081, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER, 56, 28, 65, 12);
    AddItem(IDC_MINES, 0x0081, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER, 56, 44, 65, 12);
    // 按钮
    AddItem(IDOK,     0x0080, L"确定", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 28, 62, 45, 14);
    AddItem(IDCANCEL, 0x0080, L"取消", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 88, 62, 45, 14);

    return b;
}

INT_PTR CALLBACK DlgProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_INITDIALOG: {
        DlgData* d = reinterpret_cast<DlgData*>(l);
        SetWindowLongPtr(h, DWLP_USER, reinterpret_cast<LONG_PTR>(d));
        wchar_t buf[16];
        wsprintf(buf, L"%d", d->rows);  SetDlgItemText(h, IDC_ROWS, buf);
        wsprintf(buf, L"%d", d->cols);  SetDlgItemText(h, IDC_COLS, buf);
        wsprintf(buf, L"%d", d->mines); SetDlgItemText(h, IDC_MINES, buf);
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case IDOK: {
            DlgData* d = reinterpret_cast<DlgData*>(GetWindowLongPtr(h, DWLP_USER));
            int r  = (int)GetDlgItemInt(h, IDC_ROWS, nullptr, FALSE);
            int c  = (int)GetDlgItemInt(h, IDC_COLS, nullptr, FALSE);
            int mn = (int)GetDlgItemInt(h, IDC_MINES, nullptr, FALSE);
            if (r < 5 || r > 40 || c < 5 || c > 60) {
                MessageBox(h, L"行数范围 5–40，列数范围 5–60。", L"输入有误", MB_ICONWARNING);
                return TRUE;
            }
            int maxm = r * c - 1;
            if (mn < 1 || mn > maxm) {
                wchar_t msg[128];
                wsprintf(msg, L"地雷数量需在 1–%d 之间。", maxm);
                MessageBox(h, msg, L"输入有误", MB_ICONWARNING);
                return TRUE;
            }
            d->rows = r;
            d->cols = c;
            d->mines = mn;
            EndDialog(h, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(h, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

} // namespace

bool ShowCustomDialog(HWND parent, int& rows, int& cols, int& mines) {
    DlgData data = { rows, cols, mines };
    std::vector<BYTE> tmpl = BuildTemplate();
    INT_PTR res = DialogBoxIndirectParam(
        GetModuleHandle(nullptr),
        reinterpret_cast<const DLGTEMPLATE*>(tmpl.data()),
        parent, DlgProc, reinterpret_cast<LPARAM>(&data));
    if (res == IDOK) {
        rows  = data.rows;
        cols  = data.cols;
        mines = data.mines;
        return true;
    }
    return false;
}
