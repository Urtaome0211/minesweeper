# 扫雷 Qt 重构版

将原 Win32 API + GDI 版扫雷整体重构为 Qt 6 版本，完整还原 Windows 7 蓝色 Aero
视觉风格与全部动画（波浪翻开、插旗弹出、地雷依次爆炸、胜利自动插旗）。

## 目录结构

```
qt/
├── CMakeLists.txt        CMake 工程（Qt 6.5+，C++17）
├── build-qt.bat          一键构建 + windeployqt 部署脚本
├── src/
│   ├── main.cpp          QApplication 入口（含 --demo 演示参数）
│   ├── mainwindow.h/cpp  主窗口：菜单、计时器、难度切换（对应原 app.cpp）
│   ├── boardwidget.h/cpp 棋盘绘制与交互（QPainter，对应原 render.cpp）
│   ├── customdialog.h/cpp 自定义难度对话框（QDialog，对应原 dialogs.cpp）
│   ├── game.h/cpp        核心逻辑（无平台依赖，对应原 game.cpp）
│   ├── cellpix.h         GDI 烘焙像素（数字/地雷/红叉，由 bake_cells.py 生成）
│   └── facepix.h         GDI 烘焙像素（三种笑脸，由 extract_face.py 生成）
├── shot/                 像素级验证工具（harness2.cpp 离屏渲染 + 对比脚本）
│   └── png/              两版本各模式截图
└── dist/                 （构建产物）可直接双击运行的完整目录
```

## 构建

前置要求（本机已具备）：

- Qt 6.12.0（`D:\Qt\6.12.0\llvm-mingw_64` 套件）
- 配套工具链 `D:\Qt\Tools\llvm-mingw2217_64`（clang++ 22.1.7，libc++ 运行时）
- CMake（`D:\Qt\Tools\CMake_64`）与 Ninja（`D:\Qt\Tools\Ninja`）

在 `qt\` 目录下双击运行 `build-qt.bat`（或在终端执行）：

```bat
build-qt.bat
```

产物：`qt\build\minesweeper-qt.exe`（调试用）与 `qt\dist\`（完整部署目录，
手工收集最小依赖：Qt6Core/Gui/Widgets、qwindows/qmodernwindowsstyle 插件与
libc++/libunwind 运行库，可整体拷贝分发）。

也可以手工执行同样的 CMake 命令：

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_PREFIX_PATH=D:\Qt\6.12.0\llvm-mingw_64
cmake --build build
```

## 玩法与操作

与原版一致：

- 左键翻开，右键插旗/取消插旗
- 在已翻开的数字上左右同按（Chord）快速翻开周围
- 点击笑脸或按 `F2` 重新开始
- 首次点击处及其周围 8 格保证安全
- 计时上限 999 秒；计数器可显示负数（下限 -99）
- 失败：显示全部地雷，踩中的雷标红，插错的旗打红叉
- 胜利：自动为所有地雷插旗，笑脸变为墨镜

## 演示参数（截图/验证用）

| 参数 | 效果 |
| --- | --- |
| `--demo-lose` | 300ms 后自动摆出"错旗 + 踩雷"的失败局面（含爆炸动画） |
| `--demo-win` | 300ms 后自动翻完安全格，触发胜利插旗动画 |
| `--demo-custom` | 300ms 后打开自定义难度对话框（非模态） |
| `--demo-cells <kind>` | 注入固定棋局（`lost` / `won789` / `won046`），供像素级对比验证 |

## 还原度验证（像素级）

原版与 Qt 版同屏截图后逐像素对比（对比脚本在 `qt/shot/`，以原版 GDI
离屏渲染为地面真相）：

| 场景 | 差异像素 |
| --- | --- |
| 初始棋盘整板（104400 像素） | **0** |
| 插旗棋盘整板 | **0** |
| 失败棋局（错旗红叉、黑雷、踩中红雷、数字 0–8） | **0**（游戏区域） |
| 胜利棋局 ×2（计时 789 / 46、计数器、墨镜笑脸） | **0**（游戏区域） |
| 胜利笑脸 vs GDI 地面真相（1764 像素） | **0** |

> 说明：
> - GDI 的渐变、七段数码管、笑脸、旗子、红叉、地雷与数字字形在 Qt 中无法
>   用矢量绘制得到相同像素，因此由 `qt/shot/harness2.cpp` 用原版 GDI 代码
>   离屏烘焙成 `qt/src/cellpix.h`、`qt/src/facepix.h`，Qt 版直接按像素
>   blit，保证所见即所得。
> - 两版本窗口的**左下/右下 DWM 圆角**（约 8px 圆弧，属 Windows 窗口框架，
>   两个版本同样存在）不计入对比。
> - 动画中间帧（格子翻开缩放、地雷弹出缩放）按原版 `ScaleRect` 的中心缩放
>   语义实现，静止帧与地面真相完全一致。

## 与原版的差异（有意为之）

1. **菜单栏位置**：原版使用 Windows 原生非客户区菜单；Qt 版菜单栏由 Qt 绘制，
   位于窗口客户区顶部，因此窗口总高度比原版多约一条菜单栏，棋盘本体仍为
   1:1 像素还原。
2. **关于对话框**：文案中的"实现"标注改为 `C++ / Qt 实现`。
3. 原 Win32 版全部文件保留在项目根目录，不受影响。
