// mainwindow.cpp — 主窗口实现（由 Win32 版 app.cpp 移植）
#include "mainwindow.h"

#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

#include "boardwidget.h"
#include "customdialog.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    // 原版样式：WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX（无最大化、不可调大小）
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);

    // 中央容器：菜单栏 + 棋盘（Qt 版菜单栏位于客户区内，属已知差异）
    auto* central = new QWidget(this);
    m_centralLayout = new QVBoxLayout(central);
    m_centralLayout->setContentsMargins(0, 0, 0, 0);
    m_centralLayout->setSpacing(0);
    m_menuBar = new QMenuBar(central);
    m_centralLayout->addWidget(m_menuBar);

    QMenu* gameMenu = m_menuBar->addMenu(QStringLiteral("游戏(&G)"));
    gameMenu->addAction(QStringLiteral("新游戏(&N)"), QKeySequence(Qt::Key_F2),
                        this, &MainWindow::newGame);
    gameMenu->addSeparator();
    gameMenu->addAction(QStringLiteral("初级(&B)"),    this, &MainWindow::chooseBeginner);
    gameMenu->addAction(QStringLiteral("中级(&I)"),    this, &MainWindow::chooseIntermediate);
    gameMenu->addAction(QStringLiteral("高级(&E)"),    this, &MainWindow::chooseExpert);
    gameMenu->addAction(QStringLiteral("自定义(&C)..."), this, &MainWindow::chooseCustom);
    QMenu* helpMenu = m_menuBar->addMenu(QStringLiteral("帮助(&H)"));
    helpMenu->addAction(QStringLiteral("关于(&A)..."), this, &MainWindow::showAbout);

    setCentralWidget(central);

    m_timerTick = new QTimer(this);
    m_timerTick->setInterval(1000);
    connect(m_timerTick, &QTimer::timeout, this, &MainWindow::onTick);
    m_timerTick->start();

    m_timerAnim = new QTimer(this);
    m_timerAnim->setInterval(16);
    connect(m_timerAnim, &QTimer::timeout, this, &MainWindow::onAnim);
    m_timerAnim->start();

    // 初始难度（同原版 WM_CREATE 时的 SetDifficulty）：设定标题并创建棋盘
    applyDifficulty(m_rows, m_cols, m_mines, m_diffName);
}

// ---- 难度 / 开新局 ----

void MainWindow::applyDifficulty(int rows, int cols, int mines, const QString& name) {
    m_rows     = rows;
    m_cols     = cols;
    m_mines    = mines;
    m_diffName = name;
    m_game.NewGame(rows, cols, mines);
    setWindowTitle(QStringLiteral("扫雷 - %1 (%2×%3, %4 雷)")
                       .arg(name).arg(rows).arg(cols).arg(mines));
    rebuildUi();
}

void MainWindow::rebuildUi() {
    if (m_board) {
        m_board->deleteLater();
        m_board = nullptr;
    }
    m_board = new BoardWidget(&m_game, this);
    connect(m_board, &BoardWidget::newGameRequested,
            this, &MainWindow::onBoardNewGameRequested);
    m_centralLayout->addWidget(m_board);
    fixWindowSize();
}

void MainWindow::fixWindowSize() {
    QSize bs = BoardWidget::boardSize(m_rows, m_cols);
    int mh = m_menuBar->height();
    if (mh <= 0) mh = m_menuBar->sizeHint().height();
    setFixedSize(bs.width(), mh + bs.height());
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (!m_sizeFixed) {   // 显示后菜单栏高度才最终确定，再校准一次
        m_sizeFixed = true;
        fixWindowSize();
    }
}

// ---- 菜单动作 ----

void MainWindow::newGame() {
    m_game.Restart();
    m_board->update();
}

void MainWindow::chooseBeginner()     { applyDifficulty(9, 9, 10, QStringLiteral("初级")); }
void MainWindow::chooseIntermediate() { applyDifficulty(16, 16, 40, QStringLiteral("中级")); }
void MainWindow::chooseExpert()       { applyDifficulty(16, 30, 99, QStringLiteral("高级")); }

void MainWindow::chooseCustom() {
    if (CustomDialog::getDifficulty(this, m_customRows, m_customCols, m_customMines))
        applyDifficulty(m_customRows, m_customCols, m_customMines, QStringLiteral("自定义"));
}

void MainWindow::showAbout() {
    QMessageBox::about(this, QStringLiteral("关于扫雷"),
                       QStringLiteral("扫雷 Minesweeper\n\nC++ / Qt 实现\n\n"
                                      "左键翻开，右键插旗，\n"
                                      "在数字上左右同按可快速翻开。"));
}

void MainWindow::onBoardNewGameRequested() {
    newGame();
}

// ---- 计时 / 动画 ----

void MainWindow::onTick() {
    m_game.Tick();
    if (m_game.State() == GameState::PLAYING)
        m_board->update(BoardWidget::headerRect(m_game.Cols()));
}

void MainWindow::onAnim() {
    if (m_board->hasActiveAnimation())
        m_board->update();
}

// ---- 演示/截图辅助（--demo 系列参数）----

void MainWindow::setDifficulty(int rows, int cols, int mines) {
    QString name = QStringLiteral("自定义");
    if (rows == 9 && cols == 9 && mines == 10)        name = QStringLiteral("初级");
    else if (rows == 16 && cols == 16 && mines == 40) name = QStringLiteral("中级");
    else if (rows == 16 && cols == 30 && mines == 99) name = QStringLiteral("高级");
    applyDifficulty(rows, cols, mines, name);
}

void MainWindow::flagFirstCell() {
    m_game.ToggleFlag(0, 0);
    m_board->update();
}

void MainWindow::revealMine() {
    for (int r = 0; r < m_game.Rows(); ++r)
        for (int c = 0; c < m_game.Cols(); ++c)
            if (m_game.Board()[r][c].mine && !m_game.Board()[r][c].revealed) {
                m_game.Reveal(r, c);
                m_board->update();
                return;
            }
}

void MainWindow::demoLoseSetup() {
    flagFirstCell();            // (0,0) 先插旗
    m_game.Reveal(1, 1);        // 首次翻开 → 布雷（(1,1) 安全区含 (0,0)，该旗必为错旗）
    revealMine();               // 踩雷 → 红叉 + 爆炸动画
    m_board->update();
}

void MainWindow::demoWinSetup() {
    m_game.Reveal(0, 0);        // 首次翻开（安全）
    for (int r = 0; r < m_game.Rows(); ++r)
        for (int c = 0; c < m_game.Cols(); ++c)
            if (!m_game.Board()[r][c].mine && !m_game.Board()[r][c].revealed)
                m_game.Reveal(r, c);   // 最后一个安全格翻开时触发胜利（自动插旗）
    m_board->update();
}

void MainWindow::demoSetupCustom(int rows, int cols, int mines) {
    auto* dlg = new CustomDialog(rows, cols, mines, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void MainWindow::demoCells(const QString& which) {
    // --demo-cells lost|won789|won046：注入与 harness2（GDI 地面真相）完全一致的
    // 整板状态，用于逐像素对比。布局表与 qt/shot/harness2.cpp 相同。
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
    m_game.NewGame(9, 9, 10);
    const char* table = which.startsWith(QLatin1Char('w')) ? won : lost;
    auto& board = m_game.Board();
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            Cell& cell = board[r][c];
            cell = Cell();
            char t = table[r * 9 + c];
            switch (t) {
                case 'H': break;                        // 隐藏
                case 'F': cell.flagged = true; break;   // 隐藏+旗
                case 'X': cell.flagged = true; break;   // 隐藏+旗+错叉（非雷）
                case 'M': cell.mine = true; cell.revealed = true; break;   // 已翻开的雷
                case 'D': cell.mine = true; cell.revealed = true; break;   // 踩中的红雷
                default:  cell.revealed = true; cell.adj = t - '0'; break; // 数字格
            }
        }
    }
    if (which == QLatin1String("lost"))
        m_game.InjectState(GameState::LOST, 3, 2, 123);
    else if (which == QLatin1String("won789"))
        m_game.InjectState(GameState::WON, -1, -1, 789);
    else
        m_game.InjectState(GameState::WON, -1, -1, 46);
    m_board->update();
}
