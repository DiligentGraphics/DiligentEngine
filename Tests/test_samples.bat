@echo off

setlocal ENABLEDELAYEDEXPANSION

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
    echo Example:
    echo   test_samples.bat c:\Projects\DiligentEngine\build\Win64 Debug c:\Projects\DiligentTestData\GoldenImages compare
    echo.
    echo To capture golden images, use 'capture'
    EXIT /B -1
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
(call ProcessGoldenImages.bat %golden_images_root_dir% %golden_images_mode% "--mode d3d11" "--mode d3d12" "--mode gl" "--mode gl --non_separable_progs 1" "--mode vk") || set APP_ERROR=!ERRORLEVEL!

cd "%CURR_DIR%"

exit /B %APP_ERROR%
