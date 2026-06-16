@echo off
REM Batch file to run multiple instances of Remere's Map Editor
REM This batch file assumes it's placed in the same directory as the RME executable

echo Remere's Map Editor - Multiple Instances Launcher
echo =================================================
echo.
echo This batch file allows you to run multiple instances of the map editor simultaneously.
echo.

REM Find the RME executable
set RME_EXE=IdlerMapEditor.exe
if not exist %RME_EXE% (
    set RME_EXE=IdlerMapEditor.exe
)

if not exist %RME_EXE% (
    echo ERROR: Could not find the map editor executable.
    echo Please place this batch file in the same directory as the map editor.
    echo.
    pause
    exit /b
)

REM Launch the editor with the force-multi-instance parameter
start "" "%RME_EXE%" -force-multi-instance

echo Successfully launched a new instance of Remere's Map Editor.
echo You can run this batch file again to launch more instances.
echo.
echo Press any key to exit...
pause > nul 