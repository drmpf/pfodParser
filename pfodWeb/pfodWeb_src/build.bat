@echo off
REM build.bat - Windows build script for pfodWeb
REM Builds standalone HTML files with inlined JavaScript
REM (c)2025 Forward Computing and Control Pty. Ltd.

echo ========================================
echo   pfodWeb Bundle Builder (Windows)
echo ========================================
echo.

REM Check if Node.js is installed
where node >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Node.js is not installed or not in PATH
    echo Please install Node.js from https://nodejs.org/
    echo.
    pause
    exit /b 1
)

REM Check if build script exists
if not exist build-bundle.js (
    echo ERROR: build-bundle.js not found
    echo Please ensure you are running this from the pfodWebServer directory
    echo.
    pause
    exit /b 1
)

echo Building standalone HTML files...
echo.

REM Run the build script
node build-bundle.js

if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo   Build Successful!
    echo ========================================
    echo.
    echo Output files are in the dist/ folder:
    echo   - index.html
    echo   - pfodWeb.html
    echo   - pfodWebDebug.html
    echo.
    echo To test:
    echo   1. Open dist folder
    echo   2. Double-click index.html
    echo.
    echo To distribute:
    echo   - Copy all files from dist/ folder
    echo   - Or create ZIP: dist-standalone.zip
    echo.
) else (
    echo.
    echo ========================================
    echo   Build Failed!
    echo ========================================
    echo.
    echo Please check the error messages above
    echo and ensure all source files exist.
    echo.
)

pause
