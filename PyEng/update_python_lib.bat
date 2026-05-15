@echo off
copy /Y D:\CantorAI\out\build\x64-Debug\bin\pyeng.dll C:\Python\Python312\Lib\xlang.pyd
copy /Y D:\CantorAI\out\build\x64-Debug\bin\pyeng.pdb C:\Python\Python312\Lib\pyeng.pdb
<nul set /p="D:\CantorAI\out\build\x64-Debug\bin" > C:\Python\Python312\Lib\xlang_engine_path.txt
echo Updated python lib successfully.
