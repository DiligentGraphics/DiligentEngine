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


set TEST_REPORT=
set TESTS_FAILED=0

set FONT_RED=[91m
set FONT_GREEN=[92m
set FONT_WHITE=[97m
set FONT_DEFAULT=[0m

set CURR_DIR=%cd%

rem Formatting validation

cd "%CURR_DIR%\..\DiligentCore\BuildTools\FormatValidation"
call :run_tests "call validate_format_win.bat" "Core formatting validation"

cd "%CURR_DIR%\..\DiligentTools\BuildTools\FormatValidation"
call :run_tests "call validate_format_win.bat" "Tools formatting validation"

cd "%CURR_DIR%\..\DiligentFX\BuildTools\FormatValidation"
call :run_tests "call validate_format_win.bat" "FX formatting validation"

cd "%CURR_DIR%\..\DiligentSamples\BuildTools\FormatValidation"
call :run_tests "call validate_format_win.bat" "Samples formatting validation"
 

rem Core Tests
cd "%CURR_DIR%\..\DiligentCore\Tests\DiligentCoreTest\assets"
set CORE_TEST_EXE_PATH="%build_folder%\DiligentCore\Tests\DiligentCoreTest\%config%\DiligentCoreTest.exe"
call :run_tests "%CORE_TEST_EXE_PATH%" "Core tests"


rem Tools Tests
cd "%CURR_DIR%\..\DiligentTools\Tests\DiligentToolsTest\assets"
set TOOLS_TEST_EXE_PATH="%build_folder%\DiligentTools\Tests\DiligentToolsTest\%config%\DiligentToolsTest.exe"
call :run_tests "%TOOLS_TEST_EXE_PATH%" "Tools tests"


rem Core GPU tests

cd "%CURR_DIR%\..\DiligentCore\Tests\DiligentCoreAPITest\assets"
set API_TEST_EXE_PATH="%build_folder%\DiligentCore\Tests\DiligentCoreAPITest\%config%\DiligentCoreAPITest.exe"

call :run_tests "%API_TEST_EXE_PATH% --mode=d3d11" "Core GPU Tests D3D11"
call :run_tests "%API_TEST_EXE_PATH% --mode=d3d11_sw" "Core GPU Tests D3D11-SW"

call :run_tests "%API_TEST_EXE_PATH% --mode=d3d12" "Core GPU Tests D3D12"
call :run_tests "%API_TEST_EXE_PATH% --mode=d3d12_sw" "Core GPU Tests D3D12-SW"
call :run_tests "%API_TEST_EXE_PATH% --mode=d3d12 --shader_compiler=dxc --gtest_filter=-TextureCreation*" "Core GPU Tests D3D12 DXC"
call :run_tests "%API_TEST_EXE_PATH% --mode=d3d12_sw --shader_compiler=dxc --gtest_filter=-TextureCreation*" "Core GPU Tests D3D12-SW DXC"

call :run_tests "%API_TEST_EXE_PATH% --mode=gl" "Core GPU Tests GL"
call :run_tests "%API_TEST_EXE_PATH% --mode=gl --non_separable_progs --gtest_filter=-TextureCreation*" "Core GPU Tests GL Non-sep progs"

call :run_tests "%API_TEST_EXE_PATH% --mode=vk" "Core GPU Tests VK"
call :run_tests "%API_TEST_EXE_PATH% --mode=vk --shader_compiler=dxc --gtest_filter=-TextureCreation*" "Core GPU Tests VK DXC"


rem Tools GPU tests

cd "%CURR_DIR%\..\DiligentTools\Tests\DiligentToolsGPUTest\assets"
set TOOLS_GPU_TEST_EXE_PATH="%build_folder%\DiligentTools\Tests\DiligentToolsGPUTest\%config%\DiligentToolsGPUTest.exe"

call :run_tests "%TOOLS_GPU_TEST_EXE_PATH% --mode=d3d11" "Tools GPU Tests D3D11"
call :run_tests "%TOOLS_GPU_TEST_EXE_PATH% --mode=d3d11_sw" "Tools GPU Tests D3D11-SW"
call :run_tests "%TOOLS_GPU_TEST_EXE_PATH% --mode=d3d12" "Tools GPU Tests D3D12"
call :run_tests "%TOOLS_GPU_TEST_EXE_PATH% --mode=d3d12_sw" "Tools GPU Tests D3D12-SW"
call :run_tests "%TOOLS_GPU_TEST_EXE_PATH% --mode=gl" "Tools GPU Tests GL"
call :run_tests "%TOOLS_GPU_TEST_EXE_PATH% --mode=gl --non_separable_progs" "Tools GPU Tests GL Non-sep progs"
call :run_tests "%TOOLS_GPU_TEST_EXE_PATH% --mode=vk" "Tools GPU Tests VK"


cd "%CURR_DIR%"

echo.
echo.

(call test_samples.bat %build_folder% %config%  %golden_images_root_dir%) || set /a TESTS_FAILED=!TESTS_FAILED!+1

echo.
echo.

for %%X in (!TEST_REPORT!) do (
    echo %%~X
)

if "%TESTS_FAILED%" NEQ "0" (
    echo %FONT_RED%In total !TESTS_FAILED! test groups FAILED%FONT_DEFAULT%
)

exit /B !TESTS_FAILED!


:run_tests
    echo %FONT_WHITE%%~1%FONT_DEFAULT%

    (%~1) && (
        set STATUS=%FONT_GREEN%PASSED: %~2%FONT_DEFAULT%
    ) || (
        set STATUS=%FONT_RED%FAILED: %~2%FONT_DEFAULT%
        set /a TESTS_FAILED=!TESTS_FAILED!+1
    )
    set ERROR=!ERRORLEVEL!

    echo.
    echo !STATUS!
    echo.
    echo.
    echo.

    set TEST_REPORT=!TEST_REPORT! "!STATUS!"

    exit /B !ERROR!
