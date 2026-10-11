@echo off
setlocal
rem ============================================================
rem  Build Qt minesweeper (Qt 6.12.0 llvm-mingw_64 kit + Ninja)
rem  Run from qt\; output: build\minesweeper-qt.exe
rem  Deploy dir: dist\ (windeployqt collects all Qt deps)
rem ============================================================
cd /d "%~dp0"

set "QT_ROOT=D:\Qt\6.12.0\llvm-mingw_64"
set "TOOLCHAIN=D:\Qt\Tools\llvm-mingw2217_64\bin"
set "CMAKE_BIN=D:\Qt\Tools\CMake_64\bin"
set "NINJA_BIN=D:\Qt\Tools\Ninja"

set "PATH=%TOOLCHAIN%;%CMAKE_BIN%;%NINJA_BIN%;%QT_ROOT%\bin;%PATH%"

echo === configure ===
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_ROOT%
if errorlevel 1 exit /b 1

echo === build ===
cmake --build build
if errorlevel 1 exit /b 1

echo === deploy ===
if exist dist rmdir /s /q dist
mkdir dist
copy /y build\minesweeper-qt.exe dist\ >nul
copy /y "%QT_ROOT%\bin\Qt6Core.dll"    dist\ >nul
copy /y "%QT_ROOT%\bin\Qt6Gui.dll"     dist\ >nul
copy /y "%QT_ROOT%\bin\Qt6Widgets.dll" dist\ >nul
copy /y "%QT_ROOT%\bin\libc++.dll"     dist\ >nul
copy /y "%QT_ROOT%\bin\libunwind.dll"  dist\ >nul
mkdir dist\platforms
copy /y "%QT_ROOT%\plugins\platforms\qwindows.dll" dist\platforms\ >nul
mkdir dist\styles
copy /y "%QT_ROOT%\plugins\styles\qmodernwindowsstyle.dll" dist\styles\ >nul

echo.
echo === done ===
echo exe: dist\minesweeper-qt.exe
endlocal
