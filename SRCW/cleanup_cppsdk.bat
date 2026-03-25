@echo off
echo === SRCW CppSDK Cleanup ===
echo This removes game-specific SDK files, keeping only engine-level files.
echo.

set SDK_DIR=SRCW\CppSDK\SDK

echo Keeping: Basic.*, CoreUObject_*, Engine_*
echo Removing everything else from %SDK_DIR%...
echo.

for %%f in (%SDK_DIR%\*.hpp %SDK_DIR%\*.cpp) do (
    echo %%~nxf | findstr /i /b "Basic\. CoreUObject_ Engine_" >nul 2>&1
    if errorlevel 1 (
        del "%%f"
        echo   Deleted: %%~nxf
    ) else (
        echo   Kept:    %%~nxf
    )
)

echo.
echo Done! Only engine-level files remain.
echo CppSDK root files (UnrealContainers.hpp, PropertyFixup.hpp, etc.) are untouched.
pause
