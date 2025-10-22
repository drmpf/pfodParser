@echo off
setlocal enabledelayedexpansion

:: Batch script to zip all files in current directory individually using 7-Zip
:: Each file gets compressed with .gz extension added to original filename
:: Output files are placed in ../data directory

echo Starting individual file compression with 7-Zip...
echo.

:: Create ../data directory if it doesn't exist
if not exist "..\data" (
    echo Creating ../data directory...
    mkdir ..\data
    echo.
)

:: Check if 7z.exe is available in PATH
where 7z.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: 7z.exe not found in PATH
    echo Please install 7-Zip and ensure it's in your system PATH
    echo Or modify this script to point to the full path of 7z.exe
    pause
    exit /b 1
)

:: Counter for processed files
set /a count=0

:: Process each file in the current directory
for %%f in (*) do (
    :: Skip batch files, shell scripts, markdown files, directories, and already compressed files
    if /i not "%%~xf"==".bat" (
        if /i not "%%~xf"==".sh" (
            if /i not "%%~xf"==".md" (
                if /i not "%%~xf"==".gz" (
                if not exist "%%f\" (
                echo Compressing: %%f

                :: Use 7-Zip to create gzip compressed file in ../data directory
                :: -tgzip specifies gzip format
                :: -mx9 sets maximum compression level
                7z.exe a -tgzip -mx9 "..\data\%%f.gz" "%%f"

                if !errorlevel! equ 0 (
                    echo   Success: %%f.gz created
                    set /a count+=1
                ) else (
                    echo   ERROR: Failed to compress %%f
                )
                echo.
                    )
                )
            )
        )
    )
)

echo.
echo Compression complete!
echo Total files processed: !count!
echo.

:: Post-compression cleanup
echo Performing post-compression cleanup...

:: Delete build-bundle.js.gz if it exists
if exist "..\data\build-bundle.js.gz" (
    echo Deleting build-bundle.js.gz...
    del "..\data\build-bundle.js.gz"
)

:: Delete index.html.gz if it exists
if exist "..\data\index.html.gz" (
    echo Deleting index.html.gz...
    del "..\data\index.html.gz"
)

:: Rename data_index.html.gz to index.html.gz if it exists
if exist "..\data\data_index.html.gz" (
    echo Renaming data_index.html.gz to index.html.gz...
    ren "..\data\data_index.html.gz" "index.html.gz"
)

echo Cleanup complete!
echo.
pause