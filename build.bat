@echo off
rem Build Minesweeper (MSVC toolchain)
call "D:\VS2026\VC\Auxiliary\Build\vcvars64.bat"
cl /nologo /O2 /std:c++17 /EHsc /W3 /utf-8 /DUNICODE /D_UNICODE game.cpp render.cpp dialogs.cpp app.cpp /Fe:minesweeper.exe user32.lib gdi32.lib msimg32.lib
