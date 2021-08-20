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
    echo   test_samples.bat c:\Projects\DiligentEngine\build\Win64 Debug c:\Projects\DiligentEngine\GoldenImages compare
    echo.
    echo To capture golden images, use 'capture'
    EXIT /B -1
)

set build_folder=%~1
shift

set config=%1
shift

rem golden_images_root_dir is used by ProcessGoldenImages
set golden_images_root_dir=%~1
shift

set golden_images_mode=%~1
shift

if "%golden_images_mode%" == "" (
    set golden_images_mode=compare
)

set APP_ERROR=0

set CURR_DIR=%cd%

cd "%CURR_DIR%\..\DiligentSamples\Tests\GoldenImages"
set golden_img_width=1024
set golden_img_height=768
(call ProcessGoldenImages.bat %build_folder% %config% %golden_images_mode% "-mode d3d11" "-mode d3d12" "-mode gl" "-mode gl -non_separable_progs 1" "-mode vk") || set APP_ERROR=!ERRORLEVEL!

cd "%CURR_DIR%"

set FONT_RED=[91m
set FONT_GREEN=[92m
set FONT_DEFAULT=[0m

if "%APP_ERROR%"=="0" (@echo %FONT_GREEN%Sample app tests PASSED) else (@echo %FONT_RED%Sample app tests FAILED with code %APP_ERROR%)

@echo %FONT_DEFAULT%

exit /B %APP_ERROR%
