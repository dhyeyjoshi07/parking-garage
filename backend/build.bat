@echo off
rem ============================================================================
rem  Parking Garage — Windows Build Script (CMD / PowerShell)
rem ============================================================================

echo ===================================================
echo   Building Parking Garage Backend on Windows
echo ===================================================

where gcc >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] 'gcc' was not found in your PATH.
    echo Please install MinGW, w64devkit, or MSYS2 GCC.
    echo Tip: You can install GCC via winget:
    echo   winget install w64devkit
    echo.
    pause
    exit /b 1
)

echo [1/2] Compiling backend executable (parking.exe)...
gcc -Wall -Wextra -g -Iinclude -o parking.exe src/circular_queue.c src/priority_queue.c src/stack.c src/linked_list.c src/hash_table.c src/billing.c src/json_helpers.c src/garage.c src/main.c

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation of parking.exe failed.
    exit /b 1
)
echo [OK] Successfully built parking.exe!

echo [2/2] Compiling unit tests (test_all.exe)...
gcc -Wall -Wextra -g -Iinclude -o test_all.exe src/circular_queue.c src/priority_queue.c src/stack.c src/linked_list.c src/hash_table.c src/billing.c tests/test_all.c

if %ERRORLEVEL% neq 0 (
    echo [WARNING] Compilation of test_all.exe failed.
) else (
    echo [OK] Successfully built test_all.exe!
    echo.
    echo Running unit tests:
    test_all.exe
)

echo.
echo ===================================================
echo   Build complete! You can now start the server:
echo   cd ..
echo   python server.py
echo ===================================================
