// game.cpp — 扫雷核心游戏逻辑实现（由 Win32 版移植，Qt 重构版）
#include "game.h"

#include <chrono>
#include <cstdlib>
#include <utility>

namespace {
    constexpr unsigned kWaveStagger = 10;   // 翻开波浪：每格距离延迟(ms)
    constexpr unsigned kMineStagger = 35;   // 失败时地雷依次弹出延迟(ms)
    constexpr unsigned kWinStagger  = 30;   // 胜利插旗依次延迟(ms)
}

unsigned NowMs() {
    using namespace std::chrono;
    return (unsigned)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

Game::Game()
    : m_rows(9), m_cols(9), m_mines(10),
      m_state(GameState::PLAYING), m_minesPlaced(false),
      m_time(0), m_deadR(-1), m_deadC(-1) {
    m_board.assign(m_rows, std::vector<Cell>(m_cols));
}

void Game::NewGame(int rows, int cols, int mines) {
    m_rows  = rows;
    m_cols  = cols;
    m_mines = mines;
    Restart();
}

void Game::Restart() {
    m_board.assign(m_rows, std::vector<Cell>(m_cols));
    m_state       = GameState::PLAYING;
    m_minesPlaced = false;
    m_time        = 0;
    m_deadR = m_deadC = -1;
}

void Game::PlaceMines(int safeR, int safeC) {
    int placed = 0;
    while (placed < m_mines) {
        int r = std::rand() % m_rows;
        int c = std::rand() % m_cols;
        if (m_board[r][c].mine) continue;
        // 首次点击处及其周围 8 格不放雷，保证开局安全
        if (std::abs(r - safeR) <= 1 && std::abs(c - safeC) <= 1) continue;
        m_board[r][c].mine = true;
        ++placed;
    }
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            int n = 0;
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    int nr = r + dr, nc = c + dc;
                    if (nr < 0 || nr >= m_rows || nc < 0 || nc >= m_cols) continue;
                    if (m_board[nr][nc].mine) ++n;
                }
            m_board[r][c].adj = n;
        }
}

void Game::Lose(int r, int c) {
    m_state = GameState::LOST;
    m_deadR = r;
    m_deadC = c;
    unsigned now = NowMs();
    m_board[r][c].revealed  = true;
    m_board[r][c].mineStamp = now;   // 踩中的雷最先弹出
    for (int rr = 0; rr < m_rows; ++rr)
        for (int cc = 0; cc < m_cols; ++cc)
            if (m_board[rr][cc].mine && !m_board[rr][cc].flagged) {
                m_board[rr][cc].revealed  = true;   // 翻开其余未被标记的雷
                m_board[rr][cc].mineStamp = now + (unsigned)((std::abs(rr - r) + std::abs(cc - c)) * kMineStagger);
            }
}

bool Game::CheckWin() const {
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c)
            if (!m_board[r][c].mine && !m_board[r][c].revealed)
                return false;
    return true;
}

void Game::Win() {
    m_state = GameState::WON;
    unsigned now = NowMs();
    int seq = 0;
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c)
            if (m_board[r][c].mine) {
                m_board[r][c].flagged   = true;   // 胜利自动插满旗
                m_board[r][c].flagStamp = now + (unsigned)(seq * kWinStagger);
                ++seq;
            }
}

void Game::Reveal(int r, int c) {
    if (m_state == GameState::LOST || m_state == GameState::WON) return;
    if (r < 0 || r >= m_rows || c < 0 || c >= m_cols) return;
    Cell& cell = m_board[r][c];
    if (cell.flagged || cell.revealed) return;

    if (!m_minesPlaced) {
        PlaceMines(r, c);
        m_minesPlaced = true;
    }

    if (cell.mine) { Lose(r, c); return; }

    // 洪泛展开（按曼哈顿距离设置波浪延迟，形成从点击点扩散的动画）
    unsigned now = NowMs();
    std::vector<std::pair<int, int>> stack;
    stack.push_back({r, c});
    while (!stack.empty()) {
        auto [rr, cc] = stack.back();
        stack.pop_back();
        Cell& cur = m_board[rr][cc];
        if (cur.revealed || cur.flagged) continue;
        cur.revealed     = true;
        cur.revealStamp  = now + (unsigned)((std::abs(rr - r) + std::abs(cc - c)) * kWaveStagger);
        if (cur.adj == 0) {
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    int nr = rr + dr, nc = cc + dc;
                    if (nr < 0 || nr >= m_rows || nc < 0 || nc >= m_cols) continue;
                    Cell& nb = m_board[nr][nc];
                    if (!nb.revealed && !nb.flagged) stack.push_back({nr, nc});
                }
        }
    }
    if (CheckWin()) Win();
}

void Game::Chord(int r, int c) {
    if (m_state != GameState::PLAYING) return;
    if (r < 0 || r >= m_rows || c < 0 || c >= m_cols) return;
    if (!m_board[r][c].revealed) return;

    int flags = 0;
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc) {
            int nr = r + dr, nc = c + dc;
            if (nr < 0 || nr >= m_rows || nc < 0 || nc >= m_cols) continue;
            if (m_board[nr][nc].flagged) ++flags;
        }
    if (flags != m_board[r][c].adj) return;

    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc) {
            int nr = r + dr, nc = c + dc;
            if (nr < 0 || nr >= m_rows || nc < 0 || nc >= m_cols) continue;
            if (!m_board[nr][nc].revealed && !m_board[nr][nc].flagged)
                Reveal(nr, nc);
        }
}

void Game::ToggleFlag(int r, int c) {
    if (m_state != GameState::PLAYING) return;
    if (r < 0 || r >= m_rows || c < 0 || c >= m_cols) return;
    Cell& cell = m_board[r][c];
    if (cell.revealed) return;
    cell.flagged = !cell.flagged;
    if (cell.flagged) cell.flagStamp = NowMs();   // 插旗弹出动画
    else              cell.flagStamp = 0;
}

void Game::Tick() {
    if (m_state == GameState::PLAYING && m_time < 999)
        ++m_time;
}

bool Game::IsRevealed(int r, int c) const {
    return r >= 0 && r < m_rows && c >= 0 && c < m_cols && m_board[r][c].revealed;
}

int Game::FlagCount() const {
    int n = 0;
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c)
            if (m_board[r][c].flagged) ++n;
    return n;
}
