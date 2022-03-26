### Build fails
  
* Make sure your code is up-to-date. When updating existing repository, don't forget to update all submodules:

```
git pull
git submodule update --recursive
```

* Try to [get clean version](https://github.com/DiligentGraphics/DiligentEngine#cloning-the-repository)

* Make sure your build environment is up-to-date and properly configured:
  * When building with Visual Studio, make sure you use Windows SDK 10.0.17763.0 or later,
    have C++ build tools and Visual C++ ATL Support installed.
  * When building for UWP, make sure you have UWP build tools.
  * When building for Android, make sure all your tools are up to date, you have
    [NDK and CMake installed](https://developer.android.com/studio/projects/install-ndk).
    If you are not using CMake version bundled with Android Studio, make sure your build files are
    [properly configured](https://developer.android.com/studio/projects/add-native-code.html#use_a_custom_cmake_version).
  * When using gcc, make sure the compiler version is at least 7.4.
  * gcc 9, 10 and 11 seemingly produce invalid code in release configurations that causes crash on startup.
    Use gcc 7 or 8 or clang.
  * Make sure you build the project with c++14 features enabled.

* When including Diligent headers, make sure that exactly one of `PLATFORM_WIN32`,
  `PLATFORM_UNIVERSAL_WINDOWS`, `PLATFORM_ANDROID`, `PLATFORM_LINUX`, `PLATFORM_MACOS`, and
  `PLATFORM_IOS` macros is defined as `1`.

* When building on Windows, generating Visual Studio project files is the recommended way.
  Other IDEs such as Visual Studio Code or CLion may need extra configuration to properly set up
  the build environment.

* If on Windows you get long path error, try cloning the project to a folder with shorter name
  such as `c:/git/DiligentEngine`.

* If on MacOS CMake fails to find the compiler, run the following command:
  ```cmake
  sudo xcode-select --reset
  ```  

### Projects don't run

* When running from the command line, make sure that the project's `assets` folder is set as working directory
* Try using different backends: use `-mode d3d11` or `-mode gl` command line options 
