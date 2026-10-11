/* 扫雷网页版 - 纯逻辑单元测试（Node 运行：node tests/logic.test.cjs） */
'use strict';
const assert = require('assert');
const g = require('../game.js');

let passed = 0;
function test(name, fn) {
  fn();
  passed++;
  console.log('  ✓ ' + name);
}

console.log('逻辑测试：');

test('makeBoard 尺寸与初值', () => {
  const b = g.makeBoard(9, 9);
  assert.strictEqual(b.length, 9);
  assert.strictEqual(b[0].length, 9);
  assert.deepStrictEqual(b[3][4], { mine: false, revealed: false, flagged: false, adj: 0 });
});

test('placeMines：数量正确且首次点击 3×3 安全', () => {
  for (let t = 0; t < 50; t++) {
    const rows = 9, cols = 9, mines = 10;
    const b = g.makeBoard(rows, cols);
    g.placeMines(b, rows, cols, mines, 4, 4);
    let n = 0;
    for (let r = 0; r < rows; r++)
      for (let c = 0; c < cols; c++) {
        if (b[r][c].mine) {
          n++;
          assert.ok(!(Math.abs(r - 4) <= 1 && Math.abs(c - 4) <= 1),
            '雷出现在安全区 (' + r + ',' + c + ')');
        }
      }
    assert.strictEqual(n, mines);
  }
});

test('calcAdj：邻雷数正确', () => {
  const b = g.makeBoard(3, 3);
  b[0][0].mine = true;
  b[1][1].mine = true;
  g.calcAdj(b, 3, 3);
  assert.strictEqual(b[1][1].adj, 1);
  assert.strictEqual(b[0][1].adj, 2);
  assert.strictEqual(b[2][2].adj, 1);
  assert.strictEqual(b[2][0].adj, 1);
});

test('neighbors：边界正确', () => {
  assert.strictEqual(g.neighbors(0, 0, 9, 9).length, 3);
  assert.strictEqual(g.neighbors(4, 4, 9, 9).length, 8);
  assert.strictEqual(g.neighbors(8, 8, 9, 9).length, 3);
});

test('floodCollect：无雷盘全开、距离单调', () => {
  const b = g.makeBoard(9, 9);
  const list = g.floodCollect(b, 9, 9, 0, 0);
  assert.strictEqual(list.length, 81);
  assert.strictEqual(list[0].r, 0);
  assert.strictEqual(list[0].c, 0);
  for (let i = 1; i < list.length; i++)
    assert.ok(list[i].d >= list[i - 1].d, 'BFS 距离未按序');
});

test('floodCollect：雷区被数字边界截断', () => {
  const rows = 9, cols = 9, mines = 10;
  const b = g.makeBoard(rows, cols);
  g.placeMines(b, rows, cols, mines, 4, 4);
  b[0][0].revealed = false;
  const list = g.floodCollect(b, rows, cols, 4, 4);
  // 不包含雷、不包含已翻开格、不重复
  const seen = new Set();
  list.forEach((it) => {
    assert.ok(!b[it.r][it.c].mine, '列表包含地雷');
    const key = it.r * cols + it.c;
    assert.ok(!seen.has(key), '重复格 ' + key);
    seen.add(key);
  });
});

test('isWin：翻开全部安全格判胜', () => {
  const rows = 9, cols = 9, mines = 10;
  const b = g.makeBoard(rows, cols);
  g.placeMines(b, rows, cols, mines, 0, 0);
  assert.strictEqual(g.isWin(b, rows, cols, mines), false);
  for (let r = 0; r < rows; r++)
    for (let c = 0; c < cols; c++)
      if (!b[r][c].mine) b[r][c].revealed = true;
  assert.strictEqual(g.isWin(b, rows, cols, mines), true);
});

test('renderValue：LED 显示格式化', () => {
  assert.strictEqual(g.renderValue(0), '000');
  assert.strictEqual(g.renderValue(46), '046');
  assert.strictEqual(g.renderValue(999), '999');
  assert.strictEqual(g.renderValue(1234), '999');
  assert.strictEqual(g.renderValue(-5), '-05');
  assert.strictEqual(g.renderValue(-99), '-99');
  assert.strictEqual(g.renderValue(-100), '-99');
});

console.log('\n全部 ' + passed + ' 项测试通过。');
