@echo off
REM Windows batch file to gzip JavaScript and HTML files individually
REM Creates separate .gz files for each source file

echo Compressing JavaScript and HTML files with gzip...
echo.

REM Check if gzip is available
where gzip >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: gzip command not found!
    echo Please install gzip or use Windows Subsystem for Linux
    echo You can download gzip for Windows from: http://gnuwin32.sourceforge.net/packages/gzip.htm
    echo OR you can use 7zip on each file -- https://www.7-zip.org/ and then change the extensions to .js.gz or .html.gz 
    pause
    exit /b 1
)

REM JavaScript files
set js_files=pfodWebMouse.js webTranslator.js DrawingManager.js drawingDataProcessor.js redraw.js pfodWebDebug.js mergeAndRedraw.js pfodWeb.js displayTextUtils.js

echo Compressing JavaScript files:
for %%f in (%js_files%) do (
    if exist "%%f" (
        echo   Compressing %%f...
        gzip -c "%%f" > "%%f.gz"
        if exist "%%f.gz" (
            echo     Created %%f.gz
        ) else (
            echo     Failed to create %%f.gz
        )
    ) else (
        echo     Warning: %%f not found
    )
)

echo.

REM HTML files
set html_files=index.html localIndex.html pfodWebDebug.html pfodWeb.html

echo Compressing HTML files:
for %%f in (%html_files%) do (
    if exist "%%f" (
        echo   Compressing %%f...
        gzip -c "%%f" > "%%f.gz"
        if exist "%%f.gz" (
            echo     Created %%f.gz
        ) else (
            echo     Failed to create %%f.gz
        )
    ) else (
        echo     Warning: %%f not found
    )
)

echo.
echo Gzip compression completed!
echo.

REM Show results
echo Created .gz files:
dir /b *.js.gz *.html.gz 2>nul
if %errorlevel% neq 0 (
    echo No .gz files found
) else (
    echo.
    echo File sizes comparison:
    for %%f in (*.js *.html) do (
        if exist "%%f.gz" (
            echo %%f vs %%f.gz
        )
    )
)

echo.
pause