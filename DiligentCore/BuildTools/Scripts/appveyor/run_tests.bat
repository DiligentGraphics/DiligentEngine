rem @echo off

set RUN_API_CORE_TEST=false
set RUN_CORE_API_D3D11_TEST=false
set RUN_CORE_API_D3D12_TEST=false
set ERROR=0

if "%PLATFORM_NAME%"=="Windows" (
    set RUN_API_CORE_TEST=true
    set RUN_CORE_API_D3D11_TEST=true
    set RUN_CORE_API_D3D12_TEST=true
)

if "%PLATFORM_NAME%"=="Windows8.1" (
    set RUN_API_CORE_TEST=true
    set RUN_CORE_API_D3D11_TEST=true
)

if "%RUN_API_CORE_TEST%"=="true" (
    "%1\Tests\DiligentCoreTest\%CONFIGURATION%\DiligentCoreTest.exe" || set ERROR=1
)

if "%RUN_CORE_API_D3D11_TEST%"=="true" (
    ("%1\Tests\DiligentCoreAPITest\%CONFIGURATION%\DiligentCoreAPITest.exe" --mode=d3d11_sw) || set /a ERROR=%ERROR%+10
)

if "%RUN_CORE_API_D3D12_TEST%"=="true" (
    ("%1\Tests\DiligentCoreAPITest\%CONFIGURATION%\DiligentCoreAPITest.exe" --mode=d3d12_sw) || set /a ERROR=%ERROR%+100
	("%1\Tests\DiligentCoreAPITest\%CONFIGURATION%\DiligentCoreAPITest.exe" --mode=d3d12_sw --shader_compiler=dxc) || set /a ERROR=%ERROR%+1000
)

exit /B %ERROR% REM use /B to exit the current batch script context, and not the command prompt process
