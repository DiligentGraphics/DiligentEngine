@echo off
cd ..\.
SET SAMPLE_PATH="..\..\..\..\build\Win32\bin\%~1\GhostCubeScene\GhostCubeScene.exe"

if not exist %SAMPLE_PATH% (
	rem echo Executable not found in the diligentsamples\build\Win32 directory. Checking parent directory.
    SET SAMPLE_PATH="..\"%SAMPLE_PATH%
)

if not exist %SAMPLE_PATH% (
    echo Executable not found. Please build the solution for the selected configuration.
	exit /B 1
)

%SAMPLE_PATH% mode=%~2
