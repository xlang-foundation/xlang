@echo off
setlocal

REM Get the Python site-packages path
for /f "delims=" %%i in ('python -c "import site; print(site.getsitepackages()[-1])"') do set "SITE_PACKAGES=%%i"

REM Define source directory as the current directory
set "SOURCE_DIR=%cd%"

REM Print out the site-packages directory
echo Site-Packages Directory: %SITE_PACKAGES%

REM Define destination directory
set "DEST_DIR=%SITE_PACKAGES%\xlang"

REM Create destination directory if it doesn't exist
if not exist "%DEST_DIR%" (
    mkdir "%DEST_DIR%"
)

REM Copy .dll and .pdb files
copy "%SOURCE_DIR%\pyeng.dll" "%DEST_DIR%"
copy "%SOURCE_DIR%\pyeng.pdb" "%DEST_DIR%"
copy "%SOURCE_DIR%\xlang_*.dll" "%DEST_DIR%"
copy "%SOURCE_DIR%\xlang_*.pdb" "%DEST_DIR%"

REM Copy pyeng.dll and rename to xlang.pyd
copy "%SOURCE_DIR%\pyeng.dll" "%SITE_PACKAGES%\xlang.pyd"

REM Copy pyeng.pdb and rename to xlang.pdb
copy "%SOURCE_DIR%\pyeng.pdb" "%SITE_PACKAGES%\xlang.pdb"

endlocal


