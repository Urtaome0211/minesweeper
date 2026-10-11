// boardwidget.cpp — 棋盘绘制与交互实现（QPainter，由 Win32 GDI 版 render.cpp 移植）
#include "boardwidget.h"

#include "cellpix.h"
#include "facepix.h"

#include <QFocusEvent>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>

#include <cmath>

namespace {

// ---- 布局常量（像素，与原版一致）----
constexpr int CELL     = 30;
constexpr int HEADER_H = 62;
constexpr int PAD      = 10;
constexpr int GAP      = 8;

// ---- 配色（Windows 7 蓝色风格）----
const QColor kColBgTop(206, 228, 248);   // 棋盘背景渐变(上)
const QColor kColBgBot(147, 190, 230);   // 棋盘背景渐变(下)
const QColor kColCellTop(255, 255, 255); // 未翻开格子(上, 高光)
const QColor kColCellBot(199, 221, 244); // 未翻开格子(下)
const QColor kColOpenTop(219, 233, 250); // 翻开格子(上)
const QColor kColOpenBot(199, 221, 244); // 翻开格子(下)
const QColor kColPanelTop(232, 242, 253); // 顶部面板(上)
const QColor kColPanelBot(181, 210, 238); // 顶部面板(下)
const QColor kColBorder(141, 174, 208);  // 格子边框

// ---- 动画时长(ms) ----
constexpr unsigned kPopMs     = 160;   // 翻开/地雷弹出
constexpr unsigned kFlagPopMs = 120;   // 插旗弹出
constexpr unsigned kAnimWin   = 200;   // 判定动画是否结束的窗口

// ---- 渐变 / 缩放辅助 ----
// 复刻 GDI GradientFill 的逐行定点插值：
//   y=0 时为首色原值；否则 v(y) = top + floor(delta * (y*65536 + h - 1) / (h * 65536))
// 已对照原版截图 bg/cell/panel/plate 四组渐变共 1458 个样本验证 0 差异。
int gradStep(int top, int bot, int y, int h) {
    if (y == 0) return top;
    qint64 num = qint64(bot - top) * (qint64(y) * 65536 + h - 1);
    qint64 den = qint64(h) * 65536;
    qint64 q = num / den, r = num % den;
    if (r < 0) --q;   // 向负无穷取整（floor），对应算术右移
    return top + int(q);
}

void fillGradientV(QPainter& p, const QRect& rc, const QColor& top, const QColor& bottom) {
    const int h = rc.height();
    const int x = rc.left(), w = rc.width(), y0 = rc.top();
    for (int y = 0; y < h; ++y) {
        p.fillRect(x, y0 + y, w, 1,
                   QColor(gradStep(top.red(), bottom.red(), y, h),
                          gradStep(top.green(), bottom.green(), y, h),
                          gradStep(top.blue(), bottom.blue(), y, h)));
    }
}

void frameRect(QPainter& p, const QRect& rc) {
    p.setPen(QPen(kColBorder, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(rc).adjusted(0.5, 0.5, -0.5, -0.5));
}

QRect scaleRect(const QRect& rc, float s) {
    int dw = int(rc.width() * (1.0f - s) * 0.5f);
    int dh = int(rc.height() * (1.0f - s) * 0.5f);
    return rc.adjusted(dw, dh, -dw, -dh);
}

// ---- 七段数码管 ----
enum Seg {
    SEG_A = 1 << 0, SEG_B = 1 << 1, SEG_C = 1 << 2, SEG_D = 1 << 3,
    SEG_E = 1 << 4, SEG_F = 1 << 5, SEG_G = 1 << 6
};

const int g_segDigits[10] = {
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

void segRect(QPainter& p, int x, int y, int w, int h) {
    p.fillRect(x, y, w, h, Qt::red);
}

void drawDigit7(QPainter& p, int x, int y, int w, int h, int digit) {
    int t   = w >= 20 ? 4 : 3;
    int mid = h / 2;
    int m   = g_segDigits[digit];
    if (m & SEG_A) segRect(p, x + t, y,               w - 2 * t, t);
    if (m & SEG_F) segRect(p, x,     y + t,           t,          mid - t);
    if (m & SEG_B) segRect(p, x + w - t, y + t,       t,          mid - t);
    if (m & SEG_G) segRect(p, x + t, y + mid - t / 2, w - 2 * t, t);
    if (m & SEG_E) segRect(p, x,     y + mid,         t,          h - mid - t);
    if (m & SEG_C) segRect(p, x + w - t, y + mid,     t,          h - mid - t);
    if (m & SEG_D) segRect(p, x + t, y + h - t,       w - 2 * t, t);
}

void drawMinus7(QPainter& p, int x, int y, int w, int h) {
    int t = w >= 20 ? 4 : 3;
    segRect(p, x + t, y + h / 2 - t / 2, w - 2 * t, t);
}

// ---- 元素绘制 ----
void drawHiddenCell(QPainter& p, const QRect& rc) {
    fillGradientV(p, rc, kColCellTop, kColCellBot);
    frameRect(p, rc);
}

void drawFlag(QPainter& p, const QRect& rc, float s) {
    QRect r = scaleRect(rc, s);
    // 对应 GDI 的 RECT 排他边界：right/bottom 不含端点
    int rightEx = r.left() + r.width(), botEx = r.top() + r.height();
    int cx = (r.left() + rightEx) / 2;
    int top = r.top() + 5, bot = botEx - 5;
    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::NoBrush);
    // GDI LineTo 不含终点像素 → 终点各回退 1
    p.drawLine(cx, top, cx, bot - 1);
    p.drawLine(cx - 5, bot - 1, cx + 4, bot - 1);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::red);
    // GDI Polygon 扫描线填充：排除顶边/右边，包含左边/底边
    for (int yy = top + 1; yy <= top + 7; ++yy) {
        double lx = (yy <= top + 4)
            ? (cx + 1) - 7.0 * (yy - top) / 4.0
            : (cx - 6) + 7.0 * (yy - (top + 4)) / 4.0;
        int x0 = (int)std::ceil(lx);
        p.fillRect(x0, yy, (cx + 1) - x0, 1, Qt::red);
    }
}

// ---- GDI 地面真相烘焙图缓存 ----
const QImage& digitCompositePix(int n) {          // n = 1..8，整格合成图（30x30 不透明）
    static QImage cache[8];
    static bool init = false;
    if (!init) {
        for (int i = 0; i < 8; ++i)
            cache[i] = QImage(reinterpret_cast<const uchar*>(cellpix::digitPix(i + 1)),
                              cellpix::digitW, cellpix::digitH, QImage::Format_ARGB32);
        init = true;
    }
    return cache[n - 1];
}

const QImage& mineImage(bool red, int R) {        // R = 2..7，14x14 透明底
    static QImage cache[2][6];
    static bool init = false;
    if (!init) {
        for (int i = 0; i < 6; ++i) {
            cache[0][i] = QImage(reinterpret_cast<const uchar*>(cellpix::minePix(false, i + 2)),
                                 cellpix::mineW, cellpix::mineH, QImage::Format_ARGB32);
            cache[1][i] = QImage(reinterpret_cast<const uchar*>(cellpix::minePix(true, i + 2)),
                                 cellpix::mineW, cellpix::mineH, QImage::Format_ARGB32);
        }
        init = true;
    }
    return cache[red ? 1 : 0][R - 2];
}

const QImage& wrongXImage() {                     // 30x30 透明底
    static QImage img;
    static bool init = false;
    if (!init) {
        img = QImage(reinterpret_cast<const uchar*>(cellpix::wrongX),
                     cellpix::wrongW, cellpix::wrongH, QImage::Format_ARGB32);
        init = true;
    }
    return img;
}

void drawWrongFlag(QPainter& p, const QRect& rc) {
    p.drawImage(rc.left(), rc.top(), wrongXImage());
}

void drawMine(QPainter& p, const QRect& rc, bool red, float s) {
    int R = int(7.0f * s);
    if (R < 1) return;
    // GDI 用排他边界取中心：缩放时中心恒为格内 (25,95)
    int cx = rc.left() + rc.width() / 2;
    int cy = rc.top() + rc.height() / 2;
    const QImage& img = mineImage(red, R);
    p.drawImage(QRect(cx - R, cy - R, 2 * R, 2 * R), img,
                QRect(cellpix::mineW / 2 - R, cellpix::mineH / 2 - R, 2 * R, 2 * R));
}

void drawNumber(QPainter& p, const QRect& rc, int n, float s) {
    if (s >= 1.0f) {
        // 定格帧：整格合成图与 GDI 逐像素一致（渐变+边框+数字）
        p.drawImage(rc.left(), rc.top(), digitCompositePix(n));
        return;
    }
    // 弹出动画过渡帧（数字中心位置恒定，与 GDI 相同；字形近似即可）
    static const QColor colors[9] = {
        QColor(),             // 0 未使用
        QColor(0, 0, 255),    // 1 蓝
        QColor(0, 128, 0),    // 2 绿
        QColor(255, 0, 0),    // 3 红
        QColor(0, 0, 128),    // 4 深蓝
        QColor(128, 0, 0),    // 5 深红
        QColor(0, 128, 128),  // 6 青
        QColor(0, 0, 0),      // 7 黑
        QColor(128, 128, 128) // 8 灰
    };
    static const QFont font = [] {
        QFont f(QStringLiteral("Tahoma"));
        f.setPixelSize(20);
        f.setBold(true);
        return f;
    }();
    p.setPen(colors[n]);
    p.setFont(font);
    p.drawText(rc, Qt::AlignCenter, QString::number(n));
}

void drawCounter(QPainter& p, const QRect& rc, int value) {
    int v = value < -99 ? -99 : (value > 999 ? 999 : value);
    p.fillRect(rc, Qt::black);
    frameRect(p, rc);

    int digitW = 19, digitH = 34;
    int totalW = 3 * digitW;
    int x0 = rc.left() + (rc.width() - totalW) / 2;
    int y0 = rc.top() + (rc.height() - digitH) / 2;

    int absv = v < 0 ? -v : v;
    int hundreds = absv / 100 % 10;
    int tens     = absv / 10 % 10;
    int ones     = absv % 10;

    if (v < 0) {
        drawMinus7(p, x0, y0, digitW, digitH);
        drawDigit7(p, x0 + digitW, y0, digitW, digitH, tens);
        drawDigit7(p, x0 + 2 * digitW, y0, digitW, digitH, ones);
    } else {
        drawDigit7(p, x0, y0, digitW, digitH, hundreds);
        drawDigit7(p, x0 + digitW, y0, digitW, digitH, tens);
        drawDigit7(p, x0 + 2 * digitW, y0, digitW, digitH, ones);
    }
}

// 三种状态笑脸的烘焙像素（来自 GDI 地面真相，逐像素一致）
const QImage& smileyFacePix(GameState state) {
    static QImage cache[3];
    static const uint8_t* src[3] = {facePlaying, faceLost, faceWon};
    static bool init = false;
    if (!init) {
        init = true;
        for (int s = 0; s < 3; ++s) {
            cache[s] = QImage(42, 42, QImage::Format_ARGB32_Premultiplied);
            cache[s].fill(0);
            for (int y = 0; y < 42; ++y)
                for (int x = 0; x < 42; ++x) {
                    int v = (src[s][y * 11 + (x >> 2)] >> ((x & 3) * 2)) & 3;
                    QRgb c = v == 1 ? 0xFFFFFF00u : v == 2 ? 0xFF000000u
                                   : v == 3 ? 0xFFFFFFFFu : 0u;
                    if (c) cache[s].setPixel(x, y, c);
                }
        }
    }
    int idx = state == GameState::LOST ? 1 : state == GameState::WON ? 2 : 0;
    return cache[idx];
}

void drawSmiley(QPainter& p, const QRect& rc, GameState state, bool down) {
    // 蓝色光泽按钮（按下时渐变对调，与原版一致）
    fillGradientV(p, rc, down ? kColCellBot : kColCellTop, down ? kColCellTop : kColCellBot);
    frameRect(p, rc);
    p.drawImage(rc.left(), rc.top(), smileyFacePix(state));
}

void drawRevealed(QPainter& p, const QRect& rc, const Cell& cell, const Game& game,
                  int r, int c, float s, unsigned now) {
    QRect inner = scaleRect(rc, s);
    fillGradientV(p, inner, kColOpenTop, kColOpenBot);
    frameRect(p, inner);

    if (cell.mine) {
        float ms = 1.0f;
        if (cell.mineStamp) {
            int age = (int)(now - cell.mineStamp);
            if (age < 0) ms = 0.0f;
            else if (age < (int)kPopMs) ms = 0.4f + 0.6f * age / (float)kPopMs;
        }
        drawMine(p, inner, (r == game.DeadRow() && c == game.DeadCol()), ms);
    } else if (cell.adj > 0) {
        if (s >= 0.8f) drawNumber(p, inner, cell.adj, s);   // 弹出接近完成再显示数字
    }
}

void drawCell(QPainter& p, int r, int c, const Game& game) {
    QRect rc = BoardWidget::cellRect(r, c);
    const Cell& cell = game.Board()[r][c];
    unsigned now = NowMs();

    if (!cell.revealed) {
        drawHiddenCell(p, rc);
        if (cell.flagged) {
            float s = 1.0f;
            if (cell.flagStamp) {
                int age = (int)(now - cell.flagStamp);
                if (age < (int)kFlagPopMs) s = 0.6f + 0.4f * (age < 0 ? 0 : age) / (float)kFlagPopMs;
            }
            drawFlag(p, rc, s);
        }
        if (game.State() == GameState::LOST && cell.flagged && !cell.mine)
            drawWrongFlag(p, rc);   // 标错的旗子打叉
    } else {
        if (cell.revealStamp) {
            int age = (int)(now - cell.revealStamp);
            if (age < 0) { drawHiddenCell(p, rc); return; }   // 波浪尚未到达
            float s = (age >= (int)kPopMs) ? 1.0f : (0.55f + 0.45f * age / (float)kPopMs);
            drawRevealed(p, rc, cell, game, r, c, s, now);
        } else {
            drawRevealed(p, rc, cell, game, r, c, 1.0f, now);
        }
    }
}

void drawHeader(QPainter& p, const Game& game, bool smileDown) {
    QRect panel = BoardWidget::headerRect(game.Cols());
    fillGradientV(p, panel, kColPanelTop, kColPanelBot);
    frameRect(p, panel);

    QRect lc(panel.left() + 6, panel.top() + 8, 62, 42);
    drawCounter(p, lc, game.Mines() - game.FlagCount());

    drawSmiley(p, BoardWidget::smileyRect(game.Cols()), game.State(), smileDown);

    QRect rc(panel.left() + panel.width() - 68, panel.top() + 8, 62, 42);
    drawCounter(p, rc, game.Time());
}

void drawGame(QPainter& p, const QRect& client, const Game& game, bool smileDown) {
    fillGradientV(p, client, kColBgTop, kColBgBot);
    drawHeader(p, game, smileDown);
    for (int r = 0; r < game.Rows(); ++r)
        for (int c = 0; c < game.Cols(); ++c)
            drawCell(p, r, c, game);
}

} // namespace

// ---- BoardWidget ----

BoardWidget::BoardWidget(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game) {
    setFocusPolicy(Qt::ClickFocus);
    setFixedSize(boardSize(m_game->Rows(), m_game->Cols()));
}

QSize BoardWidget::boardSize(int rows, int cols) {
    return QSize(2 * PAD + cols * CELL, 2 * PAD + HEADER_H + GAP + rows * CELL);
}

QRect BoardWidget::headerRect(int cols) {
    return QRect(PAD, PAD, cols * CELL, HEADER_H);
}

QRect BoardWidget::smileyRect(int cols) {
    int cx = PAD + cols * CELL / 2;
    return QRect(cx - 21, PAD + 8, 42, 42);
}

QRect BoardWidget::cellRect(int r, int c) {
    return QRect(PAD + c * CELL, PAD + HEADER_H + GAP + r * CELL, CELL, CELL);
}

bool BoardWidget::boardHit(const QPoint& pt, int rows, int cols, int& r, int& c) {
    int bx = PAD, by = PAD + HEADER_H + GAP;
    if (pt.x() < bx || pt.y() < by) return false;
    int cc = (pt.x() - bx) / CELL;
    int rr = (pt.y() - by) / CELL;
    if (rr < 0 || rr >= rows || cc < 0 || cc >= cols) return false;
    r = rr;
    c = cc;
    return true;
}

bool BoardWidget::hasActiveAnimation() const {
    unsigned now = NowMs();
    for (const auto& row : m_game->Board())
        for (const Cell& cell : row) {
            if (cell.revealStamp && (int)(now - cell.revealStamp) < (int)kAnimWin) return true;
            if (cell.flagStamp   && (int)(now - cell.flagStamp)   < (int)kAnimWin) return true;
            if (cell.mineStamp   && (int)(now - cell.mineStamp)   < (int)kAnimWin) return true;
        }
    return false;
}

void BoardWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    drawGame(p, rect(), *m_game, m_smileDown);
}

void BoardWidget::mousePressEvent(QMouseEvent* event) {
    QPoint pt = event->position().toPoint();
    if (event->button() == Qt::LeftButton) {
        if (smileyRect(m_game->Cols()).contains(pt)) {
            m_smileDown = true;
            grabMouse();   // 对应原版 SetCapture：拖出窗口外也能收到释放事件
            update(smileyRect(m_game->Cols()));
            return;
        }
        int r, c;
        if (boardHit(pt, m_game->Rows(), m_game->Cols(), r, c)) {
            if ((event->buttons() & Qt::RightButton) && m_game->IsRevealed(r, c))
                m_game->Chord(r, c);
            else
                m_game->Reveal(r, c);
            update();
        }
    } else if (event->button() == Qt::RightButton) {
        int r, c;
        if (boardHit(pt, m_game->Rows(), m_game->Cols(), r, c)) {
            if ((event->buttons() & Qt::LeftButton) && m_game->IsRevealed(r, c))
                m_game->Chord(r, c);
            else
                m_game->ToggleFlag(r, c);
            update();
        }
    }
    QWidget::mousePressEvent(event);
}

void BoardWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_smileDown) {
        m_smileDown = false;
        releaseMouse();
        QRect sr = smileyRect(m_game->Cols());
        if (sr.contains(event->position().toPoint()))
            emit newGameRequested();
        else
            update(sr);
    }
    QWidget::mouseReleaseEvent(event);
}

void BoardWidget::focusOutEvent(QFocusEvent* event) {
    if (m_smileDown) {
        m_smileDown = false;
        releaseMouse();
        update(smileyRect(m_game->Cols()));
    }
    QWidget::focusOutEvent(event);
}
