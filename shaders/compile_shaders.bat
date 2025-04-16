@echo off
setlocal enabledelayedexpansion

REM
set OUTPUT_DIR=bin

REM
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

REM
for %%f in (*.vert *.frag) do (
    echo Compiling %%f...
    glslangValidator.exe -V %%f -o "%OUTPUT_DIR%/%%~nxf.spv"
    if errorlevel 1 (
        echo [ERROR] Failed to compile %%f
    ) else (
        echo [SUCCESS] Compiled %%f to %OUTPUT_DIR%/%%~nxf.spv
    )
)

pause