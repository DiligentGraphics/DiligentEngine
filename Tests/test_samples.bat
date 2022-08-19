@echo off

setlocal ENABLEDELAYEDEXPANSION
setlocal

set num_args=0
for %%x in (%*) do set /A num_args+=1

if "%num_args%" LSS "3" (
    echo Command line format:
    echo.
    echo   test_samples.bat build_path config golden_images_path [golden_images_mode]
    echo.
    echo build_path         - path to the root of the build tree
    echo config             - configuration (Debug, Release, etc.^)
    echo golden_images_path - path to the golden images root folder
    echo golden_images_mode - optional golden images compare mode (capture or compare^)
    echo.
    echo Optional variables:
    echo   ADDITIONAL_TEST_APPS_D3D11 - Additional test applications to run in D3D11 mode
    echo   ADDITIONAL_TEST_APPS_D3D12 - Additional test applications to run in D3D12 mode
    echo   ADDITIONAL_TEST_APPS_GL    - Additional test applications to run in GL mode
    echo   ADDITIONAL_TEST_APPS_VK    - Additional test applications to run in Vk mode
    echo.
    echo Example:
    echo   test_samples.bat c:\Projects\DiligentEngine\build\Win64 Debug c:\Projects\DiligentTestData\GoldenImages compare
    echo.
    echo To capture golden images, use 'capture'
    EXIT /B 1
)

set DILIGENT_BUILD_DIR=%~1
shift

set DILIGENT_BUILD_TYPE=%~1
shift

rem golden_images_root_dir is used by ProcessGoldenImages.bat
set golden_images_root_dir=%~1
shift

set golden_images_mode=%~1
shift

if "%golden_images_mode%" == "" (
    set golden_images_mode=compare
)

set APP_ERROR=0

set CURR_DIR=%cd%

cd "%CURR_DIR%\..\DiligentSamples\Tests"

set GOLDEN_IMAGE_WIDTH=1024
set GOLDEN_IMAGE_HEIGHT=768

set FONT_RED=[91m
set FONT_GREEN=[92m
set FONT_DEFAULT=[0m

set TESTS_FAILED=0
set TEST_STATUS=

rem When passing ADDITIONAL_TEST_APPS_D3D11 as parameter, only the first app is used
set ADDITIONAL_TEST_APPS=%ADDITIONAL_TEST_APPS_D3D11%
call :run_tests "--mode d3d11"

set ADDITIONAL_TEST_APPS=%ADDITIONAL_TEST_APPS_D3D12%
call :run_tests "--mode d3d12"

set ADDITIONAL_TEST_APPS=%ADDITIONAL_TEST_APPS_GL%
call :run_tests "--mode gl"
call :run_tests "--mode gl --non_separable_progs 1"

set ADDITIONAL_TEST_APPS=%ADDITIONAL_TEST_APPS_VK%
call :run_tests  "--mode vk"

cd "%CURR_DIR%"

echo.
echo.

for %%X in (!TEST_STATUS!) do (
    echo %%~X
)

if "%TESTS_FAILED%" NEQ "0" (
    echo %FONT_RED%In total !TESTS_FAILED! tests FAILED%FONT_DEFAULT%
)

exit /B !TESTS_FAILED!


:run_tests
    (call ProcessGoldenImages.bat %golden_images_root_dir% %golden_images_mode% "%~1") && (
        set TEST_STATUS=!TEST_STATUS! "%FONT_GREEN%PASSED: Sample tests [%~1]%FONT_DEFAULT%"
    ) || (
        set TEST_STATUS=!TEST_STATUS! "%FONT_RED%FAILED: Sample tests [%~1]%FONT_DEFAULT%"
        set /a TESTS_FAILED=!TESTS_FAILED!+!ERRORLEVEL!
    )

    exit /B !ERRORLEVEL!
