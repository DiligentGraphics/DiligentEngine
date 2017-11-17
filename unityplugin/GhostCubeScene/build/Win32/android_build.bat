@echo off
SET LOCAL_PATH=%~dp0

SET ENGINE_ROOT=%LOCAL_PATH%/../../../..
chdir /d "%ENGINE_ROOT%/build/Android"
call build_all.bat

chdir /d %LOCAL_PATH%

call android update project -p . --target android-19
call ant debug
call adb install -r ./bin/UnityEmulatorNativeActivity-debug.apk
call adb shell am start -a android.intent.action.MAIN -n com.DiligentGraphics.UnityEmulator/com.DiligentGraphics.UnityEmulator.UnityEmulatorNativeActivity
