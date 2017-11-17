@echo off
cd ..\assets
SET SAMPLE_PATH="..\..\..\build\Win32\bin\%~1\GhostCubeScene\GhostCubeScene.exe"

if not exist %SAMPLE_PATH% (
    echo Executable not found. Please build the solution for the selected configuration.
	exit /B 1
)

%SAMPLE_PATH% mode=%~2
