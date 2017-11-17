@echo off

SETLOCAL
SET LOCAL_PATH=%~dp0

SET ENGINE_ROOT=%LOCAL_PATH%/../..
SET CORE_ROOT=%ENGINE_ROOT%/diligentcore
SET TOOLS_ROOT=%ENGINE_ROOT%/diligenttools
SET SAMPLES_ROOT=%ENGINE_ROOT%/diligentsamples

call "%LOCAL_PATH%call_ndk_build.bat" "%CORE_ROOT%/External/Android/ndk_helper/build"
call "%LOCAL_PATH%call_ndk_build.bat" "%CORE_ROOT%/Platforms/Basic/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%CORE_ROOT%/Common/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%CORE_ROOT%/Platforms/Android/build/Windows"
call "%LOCAL_PATH%call_ndk_build.bat" "%TOOLS_ROOT%/External/zlib-1.2.8/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%TOOLS_ROOT%/External/libtiff-4.0.3/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%TOOLS_ROOT%/External/lpng-1.6.17/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%TOOLS_ROOT%/External/libjpeg-9a/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%TOOLS_ROOT%/External/lua-5.2.3/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%CORE_ROOT%/Graphics/GraphicsEngine/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%CORE_ROOT%/Graphics/GraphicsTools/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%CORE_ROOT%/Graphics/HLSL2GLSLConverterLib/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%CORE_ROOT%/Graphics/GraphicsEngineOpenGL/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%TOOLS_ROOT%/TextureLoader/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%TOOLS_ROOT%/RenderScript/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%SAMPLES_ROOT%/External/TwBarLib/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%SAMPLES_ROOT%/Samples/SampleBase/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%SAMPLES_ROOT%/Samples/AntTweakBar/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%SAMPLES_ROOT%/Samples/Atmosphere/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%ENGINE_ROOT%/unityplugin/UnityEmulator/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%ENGINE_ROOT%/unityplugin/GhostCubeScene/build/Win32"
call "%LOCAL_PATH%call_ndk_build.bat" "%ENGINE_ROOT%/unityplugin/GhostCubePlugin/PluginSource/build/Win32"

chdir /d %LOCAL_PATH%

ENDLOCAL
