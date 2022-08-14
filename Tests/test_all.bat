@echo off

setlocal ENABLEDELAYEDEXPANSION

set num_args=0
for %%x in (%*) do set /A num_args+=1

if "%num_args%" LSS "3" (
    echo Command line format:
    echo.
    echo   test_all.bat build_path config golden_images_path
    echo.
    echo build_path         - path to the root of the build tree
    echo config             - configuration (Debug, Release, etc.^)
    echo golden_images_path - path to the golden images root folder
    echo.
    echo Example:
    echo   test_all.bat c:\Projects\DiligentEngine\build\Win64 Debug c:\Projects\DiligentEngine\GoldenImages
    echo.
    echo To capture golden images, replace 'compare' with 'capture' in the script below
    EXIT /B -1
)

set build_folder=%~1
shift

set config=%~1
shift

set golden_images_root_dir=%~1
shift

set FORMATTING_ERROR=0
set CORE_TEST_ERROR=0
set D3D11_ERROR=0
set D3D12_ERROR=0
set GL_ERROR=0
set VK_ERROR=0
set TOOLS_TEST_ERROR=0
set TOOLS_GPU_TEST_ERROR=0
set APP_ERROR=0

set CURR_DIR=%cd%

cd "%CURR_DIR%\..\DiligentCore\BuildTools\FormatValidation"
(call validate_format_win.bat) || set /a FORMATTING_ERROR+=1

cd "%CURR_DIR%\..\DiligentTools\BuildTools\FormatValidation"
(call validate_format_win.bat) || set /a FORMATTING_ERROR+=10

cd "%CURR_DIR%\..\DiligentFX\BuildTools\FormatValidation"
(call validate_format_win.bat) || set /a FORMATTING_ERROR+=100

cd "%CURR_DIR%\..\DiligentSamples\BuildTools\FormatValidation"
(call validate_format_win.bat) || set /a FORMATTING_ERROR+=1000

cd "%CURR_DIR%\..\DiligentCore\Tests\DiligentCoreTest\assets"
set CORE_TEST_EXE_PATH="%build_folder%\DiligentCore\Tests\DiligentCoreTest\%config%\DiligentCoreTest.exe"
(%CORE_TEST_EXE_PATH%) || set /a CORE_TEST_ERROR=%CORE_TEST_ERROR%+1

cd "%CURR_DIR%\..\DiligentTools\Tests\DiligentToolsTest\assets"
set TOOLS_TEST_EXE_PATH="%build_folder%\DiligentTools\Tests\DiligentToolsTest\%config%\DiligentToolsTest.exe"
(%TOOLS_TEST_EXE_PATH%) || set /a TOOLS_TEST_ERROR=%TOOLS_TEST_ERROR%+1


cd "%CURR_DIR%\..\DiligentCore\Tests\DiligentCoreAPITest\assets"
set API_TEST_EXE_PATH="%build_folder%\DiligentCore\Tests\DiligentCoreAPITest\%config%\DiligentCoreAPITest.exe"

(%API_TEST_EXE_PATH% --mode=d3d11) || set /a D3D11_ERROR=%D3D11_ERROR%+1
(%API_TEST_EXE_PATH% --mode=d3d11_sw) || set /a D3D11_ERROR=%D3D11_ERROR%+10

(%API_TEST_EXE_PATH% --mode=d3d12) || set /a D3D12_ERROR=%D3D12_ERROR%+1
(%API_TEST_EXE_PATH% --mode=d3d12_sw) || set /a D3D12_ERROR=%D3D12_ERROR%+10
(%API_TEST_EXE_PATH% --mode=d3d12 --shader_compiler=dxc --gtest_filter=-TextureCreation*) || set /a D3D12_ERROR=%D3D12_ERROR%+100
(%API_TEST_EXE_PATH% --mode=d3d12_sw --shader_compiler=dxc --gtest_filter=-TextureCreation*) || set /a D3D12_ERROR=%D3D12_ERROR%+1000

(%API_TEST_EXE_PATH% --mode=gl) || set /a GL_ERROR=%GL_ERROR%+1
(%API_TEST_EXE_PATH% --mode=gl --non_separable_progs --gtest_filter=-TextureCreation*) || set /a GL_ERROR=%GL_ERROR%+10

(%API_TEST_EXE_PATH% --mode=vk) || set /a VK_ERROR=%VK_ERROR%+1
(%API_TEST_EXE_PATH% --mode=vk --shader_compiler=dxc --gtest_filter=-TextureCreation*) || set /a VK_ERROR=%VK_ERROR%+10


cd "%CURR_DIR%\..\DiligentTools\Tests\DiligentToolsGPUTest\assets"
set TOOLS_GPU_TEST_EXE_PATH="%build_folder%\DiligentTools\Tests\DiligentToolsGPUTest\%config%\DiligentToolsGPUTest.exe"

(%TOOLS_GPU_TEST_EXE_PATH% --mode=d3d11) || set /a TOOLS_GPU_TEST_ERROR=%TOOLS_GPU_TEST_ERROR%+1
(%TOOLS_GPU_TEST_EXE_PATH% --mode=d3d11_sw) || set /a TOOLS_GPU_TEST_ERROR=%TOOLS_GPU_TEST_ERROR%+10
(%TOOLS_GPU_TEST_EXE_PATH% --mode=d3d12) || set /a TOOLS_GPU_TEST_ERROR=%TOOLS_GPU_TEST_ERROR%+100
(%TOOLS_GPU_TEST_EXE_PATH% --mode=d3d12_sw) || set /a TOOLS_GPU_TEST_ERROR=%TOOLS_GPU_TEST_ERROR%+1000
(%TOOLS_GPU_TEST_EXE_PATH% --mode=gl) || set /a TOOLS_GPU_TEST_ERROR=%TOOLS_GPU_TEST_ERROR%+10000
(%TOOLS_GPU_TEST_EXE_PATH% --mode=gl --non_separable_progs) || set /a TOOLS_GPU_TEST_ERROR=%TOOLS_GPU_TEST_ERROR%+100000
(%TOOLS_GPU_TEST_EXE_PATH% --mode=vk) || set /a TOOLS_GPU_TEST_ERROR=%TOOLS_GPU_TEST_ERROR%+1000000


cd "%CURR_DIR%"

echo.
echo.

(call test_samples.bat %build_folder% %config%  %golden_images_root_dir%) || set APP_ERROR=!ERRORLEVEL!

set FONT_RED=[91m
set FONT_GREEN=[92m
set FONT_DEFAULT=[0m

if "%FORMATTING_ERROR%"=="0" (@echo %FONT_GREEN%Format validation PASSED) else (@echo %FONT_RED%Format validation FAILED with code %FORMATTING_ERROR%)
if "%CORE_TEST_ERROR%"=="0" (@echo %FONT_GREEN%Core tests PASSED) else (@echo %FONT_RED%Core tests FAILED with code %CORE_TEST_ERROR%)

if "%D3D11_ERROR%"=="0" (@echo %FONT_GREEN%D3D11 tests PASSED) else (@echo %FONT_RED%D3D11 tests FAILED with code %D3D11_ERROR%)
if "%D3D12_ERROR%"=="0" (@echo %FONT_GREEN%D3D12 tests PASSED) else (@echo %FONT_RED%D3D12 tests FAILED with code %D3D12_ERROR%)
if "%GL_ERROR%"=="0" (@echo %FONT_GREEN%GL tests PASSED) else (@echo %FONT_RED%GL tests FAILED with code %GL_ERROR%)
if "%VK_ERROR%"=="0" (@echo %FONT_GREEN%Vk tests PASSED) else (@echo %FONT_RED%Vk tests FAILED with code %VK_ERROR%)

if "%TOOLS_TEST_ERROR%"=="0" (@echo %FONT_GREEN%Tools tests PASSED) else (@echo %FONT_RED%Tools tests FAILED with code %TOOLS_TEST_ERROR%)
if "%TOOLS_GPU_TEST_ERROR%"=="0" (@echo %FONT_GREEN%Tools GPU tests PASSED) else (@echo %FONT_RED%Tools GPU tests FAILED with code %TOOLS_GPU_TEST_ERROR%)

if "%APP_ERROR%"=="0" (@echo %FONT_GREEN%Sample app tests PASSED) else (@echo %FONT_RED%Sample app tests FAILED with code %APP_ERROR%)

@echo %FONT_DEFAULT%

exit /B %FORMATTING_ERROR%+%CORE_TEST_ERROR%+%D3D11_ERROR%+%D3D12_ERROR%+%GL_ERROR%+%VK_ERROR%+%TOOLS_TEST_ERROR%+%TOOLS_GPU_TEST_ERROR%+%APP_ERROR%
