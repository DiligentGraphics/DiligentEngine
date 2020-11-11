rem @echo off

set ERROR=0

if "%PLATFORM_NAME%"=="Windows" (
    "%1\Tests\DiligentToolsTest\%CONFIGURATION%\DiligentToolsTest.exe" || set ERROR=1
)

exit /B %ERROR% REM use /B to exit the current batch script context, and not the command prompt process
