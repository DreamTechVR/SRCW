@echo off
setlocal enabledelayedexpansion

echo === SRCW CppSDK Cleanup ===
echo Removes game-specific SDK files, keeps UE engine packages.
echo.

set SDK_DIR=SRCW\CppSDK\SDK

if not exist "%SDK_DIR%" (
    echo ERROR: %SDK_DIR% not found. Run this from the repo root.
    pause
    exit /b 1
)

set DELETED=0
set KEPT=0

for %%f in ("%SDK_DIR%\*.hpp" "%SDK_DIR%\*.cpp") do (
    set "NAME=%%~nxf"
    set DEL=0

    rem --- Game-specific prefixes (Sonic Racing CrossWorlds) ---
    if /i "!NAME:~0,4!"=="ABP_" set DEL=1
    if /i "!NAME:~0,5!"=="ABPT_" set DEL=1
    if /i "!NAME:~0,3!"=="AC_" set DEL=1
    if /i "!NAME:~0,3!"=="BP_" set DEL=1
    if /i "!NAME:~0,4!"=="BPC_" set DEL=1
    if /i "!NAME:~0,5!"=="BPFL_" set DEL=1
    if /i "!NAME:~0,4!"=="BPI_" set DEL=1
    if /i "!NAME:~0,3!"=="CR_" set DEL=1
    if /i "!NAME:~0,7!"=="DELETE_" set DEL=1
    if /i "!NAME:~0,3!"=="LS_" set DEL=1
    if /i "!NAME:~0,3!"=="PL_" set DEL=1
    if /i "!NAME:~0,4!"=="RTD_" set DEL=1
    if /i "!NAME:~0,3!"=="ST_" set DEL=1
    if /i "!NAME:~0,12!"=="UnionSystem_" set DEL=1
    if /i "!NAME:~0,6!"=="UNION_" set DEL=1
    if /i "!NAME:~0,9!"=="UnionRun_" set DEL=1
    if /i "!NAME:~0,8!"=="UnionUI_" set DEL=1
    if /i "!NAME:~0,9!"=="UnionLib_" set DEL=1
    if /i "!NAME:~0,4!"=="WBP_" set DEL=1
    if /i "!NAME:~0,18!"=="mariaUploadPlugin_" set DEL=1

    if !DEL!==1 (
        del "%%f"
        set /a DELETED+=1
    ) else (
        set /a KEPT+=1
    )
)

echo.
echo Deleted: !DELETED! game-specific files
echo Kept:    !KEPT! engine/framework files
echo.
echo CppSDK root files (UnrealContainers.hpp, etc.) are untouched.
pause
