@echo off

REM Build xlang — the combined build lives one level up in D:\CantorAI\out\build\x64-Debug
REM Mirrors D:\CantorAI\build_all.bat

call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat" x64
cmake --build ..\out\build\x64-Debug

REM for debugging specific targets
REM cmake --build ..\out\build\x64-Debug --target xlang_eng
