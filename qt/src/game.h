// game.h — 扫雷核心游戏逻辑（与界面无关，由 Win32 版移植，Qt 重构版）
#pragma once

#include <vector>

enum class GameState { PLAYING, WON, LOST };

struct Cell {
    bool mine     = false;
    bool revealed = false;
    bool flagged  = false;
    int  adj      = 0;   // 周围地雷数

    // 动画时间戳（毫秒，0 表示无动画）
    unsigned revealStamp = 0;   // 翻开弹出动画起点
    unsigned flagStamp   = 0;   // 插旗动画起点
    unsigned mineStamp   = 0;   // 地雷弹出动画起点
};

// 动画用毫秒时间戳（基于 std::chrono::steady_clock，取代原版的 GetTickCount）
unsigned NowMs();

class Game {
public:
    Game();

    // 开局 / 重开
    void NewGame(int rows, int cols, int mines);
    void Restart();

    // 玩家操作
    void Reveal(int r, int c);
    void Chord(int r, int c);
    void ToggleFlag(int r, int c);
    void Tick();               // 计时器走一秒

    // 状态查询
    const std::vector<std::vector<Cell>>& Board() const { return m_board; }
    // 仅供演示/测试注入整板状态（游戏逻辑本身不会调用）
    std::vector<std::vector<Cell>>& Board() { return m_board; }
    void InjectState(GameState st, int deadR, int deadC, int time) {
        m_state = st; m_deadR = deadR; m_deadC = deadC; m_time = time;
    }
    int       Rows()     const { return m_rows; }
    int       Cols()     const { return m_cols; }
    int       Mines()    const { return m_mines; }
    GameState State()    const { return m_state; }
    int       Time()     const { return m_time; }
    int       DeadRow()  const { return m_deadR; }
    int       DeadCol()  const { return m_deadC; }
    bool      IsRevealed(int r, int c) const;
    int       FlagCount() const;

private:
    void PlaceMines(int safeR, int safeC);
    void Lose(int r, int c);
    void Win();
    bool CheckWin() const;

    std::vector<std::vector<Cell>> m_board;
    int       m_rows, m_cols, m_mines;
    GameState m_state;
    bool      m_minesPlaced;
    int       m_time;
    int       m_deadR, m_deadC;
};
