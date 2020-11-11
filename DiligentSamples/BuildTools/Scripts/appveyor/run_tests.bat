@echo off

if "%PLATFORM_NAME%"=="Windows" (
    if "%PLATFORM%" == "x64" (
        ProcessGoldenImages.bat %1 %CONFIGURATION% %2 "-mode d3d11 -adapter sw" "-mode d3d12 -adapter sw"
    ) else (
        rem  There appear to be bugs in 32-bit D3D12 WARP that cause shader compilation errors in
        rem  Tutorial14 and Atmosphere sample, so only run tests in d3d11 mode
        ProcessGoldenImages.bat %1 %CONFIGURATION% %2 "-mode d3d11 -adapter sw"
    )
)

if "%PLATFORM_NAME%"=="Windows8.1" (
    ProcessGoldenImages.bat %1 %CONFIGURATION% %2 "-mode d3d11 -adapter sw"
)
