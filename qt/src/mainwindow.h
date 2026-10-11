// mainwindow.h — 主窗口（对应原版 app.cpp 的窗口、菜单与计时器职责）
#pragma once

#include <QMainWindow>

#include "game.h"

class QMenuBar;
class QTimer;
class QVBoxLayout;
class BoardWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    // ---- 演示/截图辅助入口（由 --demo 系列参数调用）----
    void demoSetupCustom(int rows, int cols, int mines);  // 打开自定义难度对话框（非模态）
    void demoLoseSetup();                                 // 摆成"将输"：错旗 + 踩雷
    void demoWinSetup();                                  // 摆成"将赢"：翻完安全格触发胜利
    void demoCells(const QString& which);                 // 注入测试整板（lost/won789/won046）
    void setDifficulty(int rows, int cols, int mines);    // 直接设定难度（演示用）
    void flagFirstCell();                                 // 给 (0,0) 插旗（演示用）
    void revealMine();                                    // 翻开一枚地雷（演示用）

protected:
    void showEvent(QShowEvent* event) override;   // 首次显示后按真实菜单栏高度定死窗口尺寸

private slots:
    void newGame();
    void chooseBeginner();
    void chooseIntermediate();
    void chooseExpert();
    void chooseCustom();
    void showAbout();
    void onBoardNewGameRequested();   // 笑脸点击 → 新游戏
    void onTick();                    // 1s：计时 + 刷新头部
    void onAnim();                    // 16ms：有动画时刷新棋盘

private:
    void applyDifficulty(int rows, int cols, int mines, const QString& name);
    void rebuildUi();
    void fixWindowSize();

    Game         m_game;
    BoardWidget* m_board          = nullptr;
    QMenuBar*    m_menuBar        = nullptr;
    QVBoxLayout* m_centralLayout  = nullptr;
    QTimer*      m_timerTick      = nullptr;   // 1s 计时
    QTimer*      m_timerAnim      = nullptr;   // 16ms 动画

    int     m_rows = 9, m_cols = 9, m_mines = 10;
    QString m_diffName = QStringLiteral("初级");
    int     m_customRows = 16, m_customCols = 16, m_customMines = 40;
    bool    m_sizeFixed = false;
};
