@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "ROOT=%~dp0"
set "IDF_DIR=G:\esp-idf-5.5.4"
if not exist "%IDF_DIR%\export.bat" (echo SAFE STOP: ESP-IDF 5.5.4 not found. & exit /b 2)
where git >nul 2>&1
if errorlevel 1 (echo SAFE STOP: git missing. & exit /b 2)
if not exist "F:\build\tmp" mkdir "F:\build\tmp"
call "%IDF_DIR%\export.bat"
if errorlevel 1 (echo SAFE STOP: IDF activation failed. & exit /b 2)
python -u "%ROOT%scripts\build_from_source.py" --public-root "F:\build\tmp\PanicPlayerTab5_Public" --report-out "F:\build\tmp\PANIC_R5Y_PUBLIC_SOURCE_BUILD_SEND_THIS.zip"
set "RC=%ERRORLEVEL%"
echo Result: F:\build\tmp\PANIC_R5Y_PUBLIC_SOURCE_BUILD_SEND_THIS.zip
exit /b %RC%
