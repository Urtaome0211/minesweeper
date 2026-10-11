/* ============================================================
   扫雷 - 现代网页版
   游戏逻辑 + 界面胶水（纯 HTML/CSS/JS，无依赖，file:// 可直接运行）
   上半部分为纯逻辑，可在 Node 下单元测试（tests/logic.test.cjs）
   ============================================================ */
(function () {
  'use strict';

  /* ==================== 纯逻辑 ==================== */

  function makeBoard(rows, cols) {
    const b = [];
    for (let r = 0; r < rows; r++) {
      const row = [];
      for (let c = 0; c < cols; c++)
        row.push({ mine: false, revealed: false, flagged: false, adj: 0 });
      b.push(row);
    }
    return b;
  }

  // 布雷：避开 (safeR, safeC) 及其周围 8 格（首次点击安全）
  function placeMines(board, rows, cols, mines, safeR, safeC) {
    let placed = 0;
    while (placed < mines) {
      const r = (Math.random() * rows) | 0;
      const c = (Math.random() * cols) | 0;
      if (board[r][c].mine) continue;
      if (Math.abs(r - safeR) <= 1 && Math.abs(c - safeC) <= 1) continue;
      board[r][c].mine = true;
      placed++;
    }
    calcAdj(board, rows, cols);
  }

  function calcAdj(board, rows, cols) {
    for (let r = 0; r < rows; r++)
      for (let c = 0; c < cols; c++) {
        let n = 0;
        for (let dr = -1; dr <= 1; dr++)
          for (let dc = -1; dc <= 1; dc++) {
            if (!dr && !dc) continue;
            const rr = r + dr, cc = c + dc;
            if (rr >= 0 && rr < rows && cc >= 0 && cc < cols && board[rr][cc].mine) n++;
          }
        board[r][c].adj = n;
      }
  }

  function neighbors(r, c, rows, cols) {
    const out = [];
    for (let dr = -1; dr <= 1; dr++)
      for (let dc = -1; dc <= 1; dc++) {
        if (!dr && !dc) continue;
        const rr = r + dr, cc = c + dc;
        if (rr >= 0 && rr < rows && cc >= 0 && cc < cols) out.push({ r: rr, c: cc });
      }
    return out;
  }

  // 洪泛展开：返回按 BFS 距离排序的待翻开列表 [{r,c,d}]，供波浪动画错峰
  function floodCollect(board, rows, cols, r, c) {
    const out = [{ r, c, d: 0 }];
    const seen = new Set([r * cols + c]);
    const queue = [{ r, c, d: 0 }];
    while (queue.length) {
      const cur = queue.shift();
      for (let dr = -1; dr <= 1; dr++)
        for (let dc = -1; dc <= 1; dc++) {
          if (!dr && !dc) continue;
          const rr = cur.r + dr, cc = cur.c + dc;
          if (rr < 0 || rr >= rows || cc < 0 || cc >= cols) continue;
          const key = rr * cols + cc;
          if (seen.has(key)) continue;
          const cell = board[rr][cc];
          if (cell.flagged || cell.revealed) continue;
          seen.add(key);
          const d = cur.d + 1;
          out.push({ r: rr, c: cc, d });
          if (cell.adj === 0) queue.push({ r: rr, c: cc, d });
        }
    }
    return out;
  }

  function isWin(board, rows, cols, mines) {
    let revealed = 0;
    for (let r = 0; r < rows; r++)
      for (let c = 0; c < cols; c++)
        if (!board[r][c].mine && board[r][c].revealed) revealed++;
    return revealed === rows * cols - mines;
  }

  // LED 计数器显示值：上限 999，下限 -99，负号占一位
  function renderValue(v) {
    if (v > 999) v = 999;
    if (v < -99) v = -99;
    if (v < 0) return '-' + String(Math.min(99, -v)).padStart(2, '0');
    return String(v).padStart(3, '0');
  }

  // 七段数码管段位表（a..g）
  const SEG = {
    '0': 'abcdef', '1': 'bc', '2': 'abged', '3': 'abgcd', '4': 'fgbc',
    '5': 'afgcd', '6': 'afgedc', '7': 'abc', '8': 'abcdefg', '9': 'abcdfg',
    '-': 'g'
  };

  if (typeof module !== 'undefined' && module.exports) {
    module.exports = { makeBoard, placeMines, calcAdj, neighbors, floodCollect, isWin, renderValue, SEG };
  }
  if (typeof document === 'undefined') return;

  /* ==================== 界面 ==================== */

  const $ = (id) => document.getElementById(id);
  const boardEl = $('board'), card = $('card'), smiley = $('smiley');
  const mineCounter = $('mineCounter'), timeCounter = $('timeCounter');
  const SVG_NS = 'http://www.w3.org/2000/svg';

  const LEVELS = {
    beginner:     { rows: 9,  cols: 9,  mines: 10, name: '初级' },
    intermediate: { rows: 16, cols: 16, mines: 40, name: '中级' },
    expert:       { rows: 16, cols: 30, mines: 99, name: '高级' }
  };

  let rows = 9, cols = 9, mines = 10;
  let board = null, state = 'idle', firstDone = false, flags = 0, dead = null;
  let startedAt = null, timeShown = 0, timerId = null;
  let timers = [];
  let custom = { rows: 16, cols: 16, mines: 40 };
  let curLevel = 'beginner';
  let pressedCell = null;

  const schedule = (fn, ms) => timers.push(setTimeout(fn, ms));
  function clearTimers() { timers.forEach(clearTimeout); timers = []; }

  /* ---------- LED 计数器 ---------- */

  const SEG_RECT = {
    a: [2, 0, 10, 2], b: [12, 2, 2, 9], c: [12, 13, 2, 9], d: [2, 22, 10, 2],
    e: [0, 13, 2, 9], f: [0, 2, 2, 9], g: [2, 11, 10, 2]
  };

  function buildCounter(el) {
    for (let d = 0; d < 3; d++) {
      const svg = document.createElementNS(SVG_NS, 'svg');
      svg.setAttribute('viewBox', '0 0 14 24');
      for (const k in SEG_RECT) {
        const rect = document.createElementNS(SVG_NS, 'rect');
        rect.setAttribute('x', SEG_RECT[k][0]);
        rect.setAttribute('y', SEG_RECT[k][1]);
        rect.setAttribute('width', SEG_RECT[k][2]);
        rect.setAttribute('height', SEG_RECT[k][3]);
        rect.setAttribute('rx', '1');
        rect.setAttribute('class', 'seg');
        rect.dataset.seg = k;
        svg.appendChild(rect);
      }
      el.appendChild(svg);
    }
  }

  function setCounter(el, value) {
    const s = renderValue(value);
    el.querySelectorAll('svg').forEach((svg, i) => {
      const on = SEG[s[i]] || '';
      svg.querySelectorAll('.seg').forEach((seg) => {
        seg.classList.toggle('on', on.indexOf(seg.dataset.seg) >= 0);
      });
    });
  }

  function updateCounters() {
    setCounter(mineCounter, mines - flags);
    setCounter(timeCounter, timeShown);
  }

  /* ---------- 图标 ---------- */

  function makeIcon(symbolId, cls) {
    const svg = document.createElementNS(SVG_NS, 'svg');
    svg.setAttribute('class', cls);
    const use = document.createElementNS(SVG_NS, 'use');
    use.setAttribute('href', '#' + symbolId);
    use.setAttribute('xlink:href', '#' + symbolId);
    svg.appendChild(use);
    return svg;
  }
  const flagIco = () => makeIcon('ic-flag', 'flag-ico');
  const mineIco = () => makeIcon('ic-mine', 'mine-ico');
  const wrongIco = () => makeIcon('ic-wrong', 'wrong-ico');

  /* ---------- 棋盘 ---------- */

  function buildBoard() {
    boardEl.innerHTML = '';
    boardEl.style.setProperty('--rows', rows);
    boardEl.style.setProperty('--cols', cols);
    for (let r = 0; r < rows; r++)
      for (let c = 0; c < cols; c++) {
        const div = document.createElement('div');
        div.className = 'cell hidden';
        div.dataset.r = r;
        div.dataset.c = c;
        boardEl.appendChild(div);
      }
  }

  const cellEl = (r, c) => boardEl.children[r * cols + c];

  function fitBoard() {
    const availW = window.innerWidth - 90;
    const availH = window.innerHeight - 250;
    const cell = Math.max(18, Math.min(34, Math.floor(Math.min(availW / cols, availH / rows))));
    document.documentElement.style.setProperty('--cell', cell + 'px');
  }

  function difficultyLabel() {
    const lv = LEVELS[curLevel];
    const name = lv ? lv.name : '自定义';
    return name + ' · ' + rows + '×' + cols + ' · ' + mines + ' 雷';
  }

  function newGame(r, c, m) {
    clearTimers();
    rows = r; cols = c; mines = m;
    board = makeBoard(rows, cols);
    state = 'playing'; firstDone = false; flags = 0; dead = null;
    startedAt = null; timeShown = 0;
    if (timerId) { clearInterval(timerId); timerId = null; }
    pressedCell = null;
    card.classList.remove('lost');
    boardEl.classList.remove('lost', 'won');
    smiley.className = 'smiley';
    buildBoard();
    updateCounters();
    fitBoard();
    $('difficultyLabel').textContent = difficultyLabel();
    updateChecks();
  }

  function updateChecks() {
    document.querySelectorAll('.menu-item').forEach((item) => {
      const check = item.querySelector('.check');
      if (check) check.textContent = item.dataset.act === curLevel ? '✓' : '';
    });
  }

  /* ---------- 计时 ---------- */

  function startTimer() {
    startedAt = Date.now();
    timerId = setInterval(() => {
      const t = Math.min(999, Math.floor((Date.now() - startedAt) / 1000));
      if (t !== timeShown) { timeShown = t; setCounter(timeCounter, t); }
      if (t >= 999) { clearInterval(timerId); timerId = null; }
    }, 250);
  }

  function stopTimer() {
    if (timerId) { clearInterval(timerId); timerId = null; }
  }

  /* ---------- 揭开 / 插旗 / 同按 ---------- */

  function animateReveal(list) {
    list.forEach((it) => {
      schedule(() => {
        const el = cellEl(it.r, it.c);
        el.classList.remove('hidden');
        el.classList.add('revealed', 'anim-pop');
        const bc = board[it.r][it.c];
        if (bc.adj > 0) {
          el.textContent = bc.adj;
          el.classList.add('n' + bc.adj);
        }
      }, it.d * 14);
    });
  }

  function revealAt(r, c, baseDist) {
    const list = floodCollect(board, rows, cols, r, c);
    list.forEach((it) => {
      it.d += baseDist;
      board[it.r][it.c].revealed = true;
    });
    animateReveal(list);
    return list.length ? list[list.length - 1].d : baseDist;
  }

  function reveal(r, c) {
    if (state !== 'playing') return;
    const cell = board[r][c];
    if (cell.revealed || cell.flagged) return;
    if (!firstDone) {
      firstDone = true;
      placeMines(board, rows, cols, mines, r, c);
      startTimer();
    }
    if (cell.mine) return lose(r, c);
    const maxD = revealAt(r, c, 0);
    schedule(() => { if (state === 'playing' && isWin(board, rows, cols, mines)) win(); }, maxD * 14 + 160);
  }

  function toggleFlag(r, c) {
    if (state !== 'playing') return;
    const cell = board[r][c];
    if (cell.revealed) return;
    cell.flagged = !cell.flagged;
    flags += cell.flagged ? 1 : -1;
    const el = cellEl(r, c);
    el.classList.remove('anim-flag');
    void el.offsetWidth;               // 重启动画
    el.classList.add('anim-flag');
    if (cell.flagged) {
      el.classList.add('flagged');
      el.appendChild(flagIco());
    } else {
      el.classList.remove('flagged');
      const f = el.querySelector('.flag-ico');
      if (f) f.remove();
    }
    updateCounters();
  }

  function chord(r, c) {
    if (state !== 'playing') return;
    const cell = board[r][c];
    if (!cell.revealed || cell.adj === 0) return;
    let nf = 0;
    const nbs = neighbors(r, c, rows, cols);
    nbs.forEach((n) => { if (board[n.r][n.c].flagged) nf++; });
    if (nf !== cell.adj) return;
    let deadCell = null;
    const toReveal = [];
    nbs.forEach((n) => {
      const bc = board[n.r][n.c];
      if (bc.revealed || bc.flagged) return;
      if (bc.mine) { if (!deadCell) deadCell = n; }
      else toReveal.push(n);
    });
    if (deadCell) return lose(deadCell.r, deadCell.c);
    toReveal.forEach((n) => {
      if (state === 'playing' && !board[n.r][n.c].revealed) revealAt(n.r, n.c, 1);
    });
    schedule(() => { if (state === 'playing' && isWin(board, rows, cols, mines)) win(); }, 200);
  }

  /* ---------- 胜负 ---------- */

  function lose(r, c) {
    state = 'lost';
    stopTimer();
    dead = { r, c };
    smiley.className = 'smiley dead';
    boardEl.classList.add('lost');
    card.classList.remove('lost');
    void card.offsetWidth;             // 重触发失败抖动
    card.classList.add('lost');

    // 死格：红底 + 抖动
    const dEl = cellEl(r, c);
    dEl.classList.remove('hidden');
    dEl.classList.add('revealed', 'mine', 'dead', 'anim-shake');
    dEl.appendChild(mineIco());

    // 错旗：立即打红叉
    for (let rr = 0; rr < rows; rr++)
      for (let cc = 0; cc < cols; cc++) {
        const bc = board[rr][cc];
        if (bc.flagged && !bc.mine) {
          const el = cellEl(rr, cc);
          el.classList.add('wrong', 'anim-flag');
          el.appendChild(wrongIco());
        }
      }

    // 其余雷：逐颗错峰爆开
    let i = 0;
    for (let rr = 0; rr < rows; rr++)
      for (let cc = 0; cc < cols; cc++) {
        const bc = board[rr][cc];
        if (bc.mine && !bc.flagged && !(rr === r && cc === c)) {
          const el = cellEl(rr, cc);
          schedule(() => {
            el.classList.remove('hidden');
            el.classList.add('revealed', 'mine', 'anim-explode');
            el.appendChild(mineIco());
          }, 150 + i++ * 35);
        }
      }
  }

  function win() {
    if (state !== 'playing') return;
    state = 'won';
    stopTimer();
    smiley.className = 'smiley cool bounce';
    boardEl.classList.add('won');
    // 剩余地雷自动插旗，错峰弹跳
    let i = 0;
    for (let r = 0; r < rows; r++)
      for (let c = 0; c < cols; c++) {
        const bc = board[r][c];
        if (bc.mine && !bc.flagged) {
          const el = cellEl(r, c);
          schedule(() => {
            bc.flagged = true;
            flags++;
            el.classList.remove('anim-flag');
            void el.offsetWidth;
            el.classList.add('anim-flag', 'flagged');
            el.appendChild(flagIco());
            setCounter(mineCounter, mines - flags);
          }, 80 + i++ * 30);
        }
      }
  }

  /* ---------- 事件 ---------- */

  boardEl.addEventListener('mousedown', (e) => {
    if (state !== 'playing') return;
    const cell = e.target.closest('.cell');
    if (!cell) return;
    const r = +cell.dataset.r, c = +cell.dataset.c;
    smiley.classList.add('surprised');
    if (e.buttons & 2) {                        // 右键：插旗
      e.preventDefault();
      pressedCell = null;
      toggleFlag(r, c);
    } else if ((e.buttons & 3) === 3) {         // 左右同按：快速翻开
      pressedCell = null;
      chord(r, c);
    } else if (e.button === 0) {                // 左键按下：记录，抬起时揭开
      pressedCell = { r, c };
    }
  });

  boardEl.addEventListener('mouseup', (e) => {
    if (e.button !== 0) return;
    smiley.classList.remove('surprised');
    if (!pressedCell) return;
    const cell = e.target.closest('.cell');
    if (cell &&
        +cell.dataset.r === pressedCell.r &&
        +cell.dataset.c === pressedCell.c) {
      reveal(pressedCell.r, pressedCell.c);
    }
    pressedCell = null;
  });

  boardEl.addEventListener('mouseleave', () => {
    smiley.classList.remove('surprised');
    pressedCell = null;
  });

  boardEl.addEventListener('contextmenu', (e) => e.preventDefault());

  smiley.addEventListener('click', () => newGame(rows, cols, mines));

  /* ---------- 菜单 ---------- */

  function closeMenus() {
    document.querySelectorAll('.menu.open').forEach((m) => m.classList.remove('open'));
  }

  document.querySelectorAll('.menu').forEach((menu) => {
    menu.querySelector('.menu-title').addEventListener('click', (e) => {
      e.stopPropagation();
      const wasOpen = menu.classList.contains('open');
      closeMenus();
      if (!wasOpen) menu.classList.add('open');
    });
  });

  document.addEventListener('click', closeMenus);

  function setLevel(name) {
    curLevel = name;
    const lv = LEVELS[name];
    newGame(lv.rows, lv.cols, lv.mines);
  }

  document.querySelectorAll('.menu-item').forEach((item) => {
    item.addEventListener('click', (e) => {
      e.stopPropagation();
      closeMenus();
      switch (item.dataset.act) {
        case 'new':          newGame(rows, cols, mines); break;
        case 'beginner':     setLevel('beginner'); break;
        case 'intermediate': setLevel('intermediate'); break;
        case 'expert':       setLevel('expert'); break;
        case 'custom':       openCustom(); break;
        case 'about':        openAbout(); break;
      }
    });
  });

  /* ---------- 对话框 ---------- */

  function openCustom() {
    $('customError').textContent = '';
    $('inRows').value = custom.rows;
    $('inCols').value = custom.cols;
    $('inMines').value = custom.mines;
    $('customOverlay').classList.remove('hidden');
    $('inRows').focus();
    $('inRows').select();
  }

  function closeCustom() { $('customOverlay').classList.add('hidden'); }

  function shakeCustom() {
    const d = $('customDialog');
    d.classList.remove('shake');
    void d.offsetWidth;
    d.classList.add('shake');
  }

  function applyCustom() {
    const r = parseInt($('inRows').value, 10);
    const c = parseInt($('inCols').value, 10);
    const m = parseInt($('inMines').value, 10);
    const err = $('customError');
    if (isNaN(r) || r < 5 || r > 40 || isNaN(c) || c < 5 || c > 60) {
      err.textContent = '输入有误：行数需在 5–40 之间，列数需在 5–60 之间。';
      shakeCustom();
      return;
    }
    if (isNaN(m) || m < 1 || m > r * c - 1) {
      err.textContent = '地雷数量需在 1–' + (r * c - 1) + ' 之间。';
      shakeCustom();
      return;
    }
    custom = { rows: r, cols: c, mines: m };
    closeCustom();
    setLevel('custom');
  }

  $('customOk').addEventListener('click', applyCustom);
  $('customCancel').addEventListener('click', closeCustom);

  $('inRows').addEventListener('keydown', (e) => { if (e.key === 'Enter') applyCustom(); });
  $('inCols').addEventListener('keydown', (e) => { if (e.key === 'Enter') applyCustom(); });
  $('inMines').addEventListener('keydown', (e) => { if (e.key === 'Enter') applyCustom(); });

  function openAbout() { $('aboutOverlay').classList.remove('hidden'); }
  function closeAbout() { $('aboutOverlay').classList.add('hidden'); }
  $('aboutOk').addEventListener('click', closeAbout);

  document.querySelectorAll('.overlay').forEach((ov) => {
    ov.addEventListener('mousedown', (e) => {
      if (e.target === ov) {                     // 点背景关闭
        ov.classList.add('hidden');
      }
    });
  });

  /* ---------- 键盘 / 缩放 ---------- */

  document.addEventListener('keydown', (e) => {
    if (e.key === 'F2') {
      e.preventDefault();
      newGame(rows, cols, mines);
    } else if (e.key === 'Escape') {
      closeMenus();
      closeCustom();
      closeAbout();
    }
  });

  window.addEventListener('resize', fitBoard);

  /* ---------- 内置自测（index.html?test=1，供无头浏览器验证） ---------- */

  if (location.search.indexOf('test') >= 0) {
    const results = [];
    const check = (name, ok) => results.push((ok ? 'PASS ' : 'FAIL ') + name);
    window.addEventListener('load', () => {
      check('初始 81 格', boardEl.children.length === 81);
      check('初始状态 playing', state === 'playing');
      reveal(0, 0);
      check('首次揭开无雷', firstDone && !board[0][0].mine);
      check('计时启动', startedAt !== null);
      let mr = -1, mc = -1;
      for (let r = 0; r < rows && mr < 0; r++)
        for (let c = 0; c < cols; c++)
          if (board[r][c].mine) { mr = r; mc = c; break; }
      reveal(mr, mc);
      check('失败状态 lost', state === 'lost');
      check('笑脸 dead', smiley.classList.contains('dead'));
      check('棋盘 lost', boardEl.classList.contains('lost'));
      check('死格红底', cellEl(mr, mc).classList.contains('dead'));
      check('死格雷图标', !!cellEl(mr, mc).querySelector('.mine-ico'));
      newGame(9, 9, 10);
      reveal(0, 0);
      for (let r = 0; r < rows; r++)
        for (let c = 0; c < cols; c++)
          if (!board[r][c].mine && !board[r][c].revealed) reveal(r, c);
      setTimeout(() => {
        check('胜利状态 won', state === 'won');
        check('笑脸 cool', smiley.classList.contains('cool'));
        check('棋盘 won', boardEl.classList.contains('won'));
        newGame(9, 9, 10);
        toggleFlag(2, 2);
        check('插旗逻辑', board[2][2].flagged === true);
        check('旗子图标', !!cellEl(2, 2).querySelector('.flag-ico'));
        const report = document.createElement('div');
        report.id = 'testResult';
        report.textContent = results.join(' | ');
        document.body.appendChild(report);
        document.title = results.every((s) => s.indexOf('PASS') === 0)
          ? 'TEST-ALL-PASS-' + results.length : 'TEST-FAIL';
      }, 600);
    });
  }

  /* ---------- 启动 ---------- */

  buildCounter(mineCounter);
  buildCounter(timeCounter);
  newGame(9, 9, 10);
})();
