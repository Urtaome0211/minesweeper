// boardwidget.h — 棋盘绘制与交互（QPainter，还原 Win7 蓝色 Aero 风格与动画）
#pragma once

#include <QWidget>

#include "game.h"

class BoardWidget : public QWidget {
    Q_OBJECT
public:
    explicit BoardWidget(Game* game, QWidget* parent = nullptr);

    // 是否还有动画进行中（用于驱动动画重绘）
    bool hasActiveAnimation() const;

    // 布局辅助（窗口据此调整大小、局部刷新）
    static QSize boardSize(int rows, int cols);
    static QRect headerRect(int cols);
    static QRect smileyRect(int cols);
    static QRect cellRect(int r, int c);
    static bool boardHit(const QPoint& pt, int rows, int cols, int& r, int& c);

signals:
    void newGameRequested();   // 点击笑脸按钮

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    Game* m_game;
    bool  m_smileDown = false;
};
