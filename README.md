# Diligent Engine <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/diligentgraphics-logo.png" height=64 align="right" valign="middle">
**A Modern Cross-Platform Low-Level 3D Graphics Library** [![Tweet](https://img.shields.io/twitter/url/http/shields.io.svg?style=social)](https://twitter.com/intent/tweet?text=An%20easy-to-use%20cross-platform%20graphics%20library%20that%20takes%20full%20advantage%20of%20%23Direct3D12%20and%20%23VulkanAPI&url=https://github.com/DiligentGraphics/DiligentEngine)

[Diligent Engine](http://diligentgraphics.com/diligent-engine/) is a lightweight cross-platform graphics
API abstraction library and rendering framework. It is designed to take full advantage of Direct3D12, Vulkan
and Metal, while supporting older platforms via Direct3D11, OpenGL and OpenGLES. Diligent Engine exposes common
front-end  API and uses HLSL as universal shading language on all platforms and rendering back-ends. 
Platform-specific shader representations (GLSL, DX bytecode or SPIRV) can be used with corresponding back-ends.
The engine is intended to be used as graphics subsystem in a game engine or any other 3D application. 
It is distributed under [Apache 2.0 license](License.txt) and is free to use.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](License.txt)
[![Chat on gitter](https://badges.gitter.im/gitterHQ/gitter.png)](https://gitter.im/diligent-engine)
[![Chat on Discord](https://img.shields.io/discord/730091778081947680?logo=discord)](https://discord.gg/t7HGBK7)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/DiligentGraphics/DiligentEngine?svg=true)](https://ci.appveyor.com/project/DiligentGraphics/diligentengine)
[![Build Status](https://travis-ci.org/DiligentGraphics/DiligentEngine.svg?branch=master)](https://travis-ci.org/DiligentGraphics/DiligentEngine)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/978eebabb2fc438f9d736443b71127aa)](https://www.codacy.com/manual/DiligentGraphics/DiligentEngine?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=DiligentGraphics/DiligentEngine&amp;utm_campaign=Badge_Grade)

## Features

* Cross-platform
  * Exact same client code for all supported platforms and rendering backends
    * No `#if defined(_WIN32)` ... `#elif defined(LINUX)` ... `#elif defined(ANDROID)` ...
    * No `#if defined(D3D11)` ... `#elif defined(D3D12)` ... `#elif defined(OPENGL)` ...
  * Exact same HLSL shaders (VS, PS, GS, HS, DS, CS) run on all platforms and all back-ends 
* High performance
* Modular design
  * Components are clearly separated logically and physically and can be used as needed
  * Only take what you need for your project
* Clear and concise API
  * C/C++
  * Object-based
  * Stateless
* Key graphics features:
  * [Automatic shader resource binding](http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/) designed to leverage next-generation graphics APIs
  * Multithreaded command buffer generation
    * [50,000 draw calls at 300 fps](https://github.com/DiligentGraphics/DiligentEngine/tree/master/Projects/Asteroids) with D3D12/Vulkan backend
  * Multithreaded resource creation
  * [Automatic or explicit control over resource state transitions](http://diligentgraphics.com/2018/12/09/resource-state-management/)
  * Descriptor and memory management
  * Shader resource reflection
* Extensive validation and error reporting
* Modern c++ features to make the code fast and reliable
* Consistent high quality is ensured by continuous integration
  * Automated builds and unit testing
  * Source code formatting validation
  * Static analysis

## High-level Rendering components

* [Atmospheric light scattering post-effect](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/EpipolarLightScattering)
* [Tone mapping utilities](https://github.com/DiligentGraphics/DiligentFX/tree/master/Shaders/PostProcess/ToneMapping/public)
* [Physically-based GLTF2.0 renderer](https://github.com/DiligentGraphics/DiligentFX/tree/master/GLTF_PBR_Renderer)
* [Shadows](https://github.com/DiligentGraphics/DiligentFX/tree/master/Components#shadows)
* [Integration with Dear Imgui](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/ImguiDemo)
  [and Nuklear](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/NuklearDemo)

## Supported Plaforms and Low-Level Graphics APIs

| Platform                                                                                                                                        | D3D11              | D3D12              |  OpenGL            | Vulkan                                                                        | OpenGLES           |  Build Status                    |
| ----------------------------------------------------------------------------------------------------------------------------------------------- | ------------------ |------------------- | ------------------ | ----------------------------------------------------------------------------- | ------------------ | -------------------------------- |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/windows-logo.png" width=24 valign="middle"> Win32 (Windows desktop)| :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark:                                                            | -                  |  [![Build Status](https://ci.appveyor.com/api/projects/status/github/DiligentGraphics/DiligentEngine?svg=true)](https://ci.appveyor.com/project/DiligentGraphics/diligentengine) |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/uwindows-logo.png" width=24 valign="middle"> Universal Windows     | :heavy_check_mark: | :heavy_check_mark: | -                  | -                                                                             | -                  |  [![Build Status](https://ci.appveyor.com/api/projects/status/github/DiligentGraphics/DiligentEngine?svg=true)](https://ci.appveyor.com/project/DiligentGraphics/diligentengine) |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/linux-logo.png" width=24 valign="middle"> Linux                    | -                  | -                  | :heavy_check_mark: | :heavy_check_mark:                                                            | -                  |  [![Build Status](https://travis-ci.org/DiligentGraphics/DiligentEngine.svg?branch=master)](https://travis-ci.org/DiligentGraphics/DiligentEngine)      |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/macos-logo.png" width=24 valign="middle"> MacOS                    | -                  | -                  | :heavy_check_mark: | :heavy_check_mark: (via [MoltenVK](https://github.com/KhronosGroup/MoltenVK)) | -                  |  [![Build Status](https://travis-ci.org/DiligentGraphics/DiligentEngine.svg?branch=master)](https://travis-ci.org/DiligentGraphics/DiligentEngine)      |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/android-logo.png" width=24 valign="middle"> Android                | -                  | -                  | -                  | :heavy_check_mark:                                                            | :heavy_check_mark: |                                                                                                                                                         |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/apple-logo.png" width=24 valign="middle"> iOS                      | -                  | -                  | -                  | :heavy_check_mark: (via [MoltenVK](https://github.com/KhronosGroup/MoltenVK)) | :heavy_check_mark: |  [![Build Status](https://travis-ci.org/DiligentGraphics/DiligentEngine.svg?branch=master)](https://travis-ci.org/DiligentGraphics/DiligentEngine)      |

# Table of Contents

- [Cloning the Repository](#cloning)
  - [Repository Structure](#repository_structure)
- [Build and Run Instructions](#build_and_run)
  - [Win32](#build_and_run_win32)
  - [Universal Windows Platform](#build_and_run_uwp)
  - [Linux](#build_and_run_linux)
  - [Android](#build_and_run_android)
  - [MacOS](#build_and_run_macos)
  - [iOS](#build_and_run_ios)
  - [Integrating Diligent Engine with Existing Build System](#build_and_run_integration)
  - [Build Options](#build_option)
  - [Customizing Build](#build_and_run_customizing)
- [Getting started with the API](#getting_started)
- [Tutorials](#tutorials)
- [Samples](#samples)
- [Demos](#demos)
- [High-Level Rendering Components](#high_level_components)
- [License](#license)
- [Contributing](#contributing)
- [References](#references)
- [Release History](#release_history)

<a name="cloning"></a>
# Cloning the Repository

This is the master repository that contains four [submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules). To get the repository and all submodules, use the following command:

```
git clone --recursive https://github.com/DiligentGraphics/DiligentEngine.git
```

When updating existing repository, don't forget to update all submodules:

```
git pull
git submodule update --recursive
```

It is also a good idea to re-run CMake and perform clean rebuild after getting the latest version.

<a name="repository_structure"></a>
## Repository Structure

Master repository includes the following submodules:

* [Core](https://github.com/DiligentGraphics/DiligentCore) submodule implements 
  [Direct3D11](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineD3D11), 
  [Direct3D12](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineD3D12),
  [OpenGL/GLES](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineOpenGL), and
  [Vulkan](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineVulkan) back-ends.
  The module is self-contained and can be built by its own.
* [Tools](https://github.com/DiligentGraphics/DiligentTools) submodule contains 
  [texture loading library](https://github.com/DiligentGraphics/DiligentTools/tree/master/TextureLoader),
  [asset loading library](https://github.com/DiligentGraphics/DiligentTools/blob/master/AssetLoader),
  [dear imgui implementation](https://github.com/DiligentGraphics/DiligentTools/blob/master/Imgui), and
  [native application implementation](https://github.com/DiligentGraphics/DiligentTools/blob/master/NativeApp).
* [DiligentFX](https://github.com/DiligentGraphics/DiligentFX) is a high-level rendering framework that implements
  various rendering components. The module depends on Core and Tools modules.
* [Samples](https://github.com/DiligentGraphics/DiligentSamples) submodule contains tutorials and sample applications 
  intended to demonstrate the usage of the Diligent Engine API. The module depends on Core, Tools and DiligentFX modules.

<a name="build_and_run"></a>
# Build and Run Instructions

Diligent Engine uses [CMake](https://cmake.org/) as a cross-platform build tool. 
To start using cmake, download the [latest release](https://cmake.org/download/) (3.16 or later is required).
Another build prerequisite is [Python interpreter](https://www.python.org/downloads/) (3.0 or later is required).
If after following the instuctions below you have build/run issues, please take a look at [troubleshooting](Troubleshooting.md).

<a name="build_and_run_win32"></a>
## Win32

Build prerequisites:

* Windows SDK 10.0.17763.0 or later (10.0.19041.0 is required for mesh shaders)
* C++ build tools
* Visual C++ ATL Support

Use either CMake GUI or command line tool to generate build files. For example, to generate 
[Visual Studio 2019](https://www.visualstudio.com/vs/community) 64-bit solution and project files in *build/Win64* folder, 
navigate to the engine's root folder and run the following command:

```
cmake -S . -B ./build/Win64 -G "Visual Studio 16 2019" -A x64
```

You can generate Win32 solution that targets Win8.1 SDK using the following command:

```
cmake -D CMAKE_SYSTEM_VERSION=8.1 -S . -B ./build/Win64_8.1 -G "Visual Studio 16 2019" -A x64
```

If you use MinGW, you can generate the make files using the command below (note however that the functionalty
will be limited and that MinGW is not a recommended way to build the engine):

```
cmake -S . -B ./build/MinGW -D CMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
```

:warning: In current implementation, full path to cmake build folder **must not contain white spaces**.

To enable Vulkan validation layers, you will need to download [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) and add environemt
variable `VK_LAYER_PATH` that contains path to the *Bin* directory in VulkanSDK installation folder.

Open *DiligentEngine.sln* file in *build/Win64* folder, select configuration and build the engine. Set the desired project
as startup project (by default, GLTF Viewer will be selected) and run it. 

By default, sample and tutorial applications will show rendering backend selection dialog box. Use the following command line options to force
D3D11, D3D12, OpenGL, or Vulkan mode: **-mode D3D11**, **-mode D3D12**, **-mode GL**, or **-mode Vk**. If you want to run an application
outside of Visual Studio environment, the application's assets folder must be set as working directory. (For Visual Studio, this 
is automatically configured by CMake). Alternatively, you can navigate to the build target or install folder and run the executable from there.


<a name="build_and_run_uwp"></a>
## Universal Windows Platform

To generate build files for Universal Windows platform, you need to define the following two cmake variables:

* `CMAKE_SYSTEM_NAME=WindowsStore`
* `CMAKE_SYSTEM_VERSION=< Windows SDK Version >`

For example, to generate Visual Studio 2019 64-bit solution and project files in *build/UWP64* folder, run the following command
from the engine's root folder:

```
cmake -D CMAKE_SYSTEM_NAME=WindowsStore -D CMAKE_SYSTEM_VERSION=10.0 -S . -B ./build/UWP64 -G "Visual Studio 16 2019" -A x64
```

You can target specific SDK version by refining CMAKE_SYSTEM_VERSION, for instance:

```
cmake -D CMAKE_SYSTEM_NAME=WindowsStore -D CMAKE_SYSTEM_VERSION=10.0.16299.0 -S . -B ./build/UWP64 -G "Visual Studio 16 2019" -A x64
```

Set the desired project as startup project (by default, GLTF Viewer will be selected) and run it. 

By default, appplications will run in D3D12 mode. You can select D3D11 or D3D12 using the following command line options:
**-mode D3D11**, **-mode D3D12**.

Note: it is possible to generate solution that targets Windows 8.1 by defining CMAKE_SYSTEM_VERSION=8.1 cmake variable, but it will fail
to build as it will use Visual Studio 2013 (v120) toolset that lacks proper c++11 support.


<a name="build_and_run_linux"></a>
## Linux

Your Linux environment needs to be set up for c++ development. If it already is, make sure your c++ tools are up to date
as Diligent Engine uses modern c++ features (gcc/g++ 7 or later is recommended). You may need to install the following packages:

1. gcc, make and other essential c/c++ tools:

```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install build-essential
```

2. cmake

```
sudo apt-get install cmake
```

3. Other required packages:

```
sudo apt-get install libx11-dev
sudo apt-get install mesa-common-dev
sudo apt-get install mesa-utils
sudo apt-get install libgl-dev
sudo apt-get install python3-distutils
```

To configure Vulkan you will also need to:

* Install latest Vulkan drivers and libraries for your GPU
* Install [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)
  * To make sure that you system is properly configured you can try to build and run samples from the SDK

To generate make files for debug configuration, run the following CMake command from the engine's root folder:

```
cmake -S . -B ./build/Linux64 -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Debug"
```

To build the engine, run the following command:

```
cmake --build ./build/Linux64
```

The engine's root folder contains [Visual Studio Code](https://code.visualstudio.com/) settings files that configure
the IDE to build the engine. You can run applications directly from the IDE. To run an application from the command line,
the app's assets folder must be current directory.


<a name="build_and_run_android"></a>
## Android

Please make sure that your machine is set up for Android development. Download 
[Android Studio](https://developer.android.com/studio/index.html),
[install and configure the NDK and CMake](https://developer.android.com/studio/projects/install-ndk)
and other required tools. If you are not using CMake version bundled with Android Studio, make sure
your build files are [properly configured](https://developer.android.com/studio/projects/add-native-code.html#use_a_custom_cmake_version).
To verify that your environment is properly set up, try building the
[teapots sample](https://github.com/googlesamples/android-ndk/tree/master/teapots) as well as
[Vulkan Android tutorials](https://github.com/googlesamples/android-vulkan-tutorials).

Known issues:

* If native build does not find python executable, add `PYTHON_EXECUTABLE` variable to [CMake arguments in NativeApp's
  build.gradle file](https://github.com/DiligentGraphics/DiligentTools/blob/master/NativeApp/Android/build.gradle#L12):
  `-DPYTHON_EXECUTABLE=/Path/To/Your/Python36/python.exe`
* If native build messes up shader_list.h file, go to git and undo the changes.

Open *DiligentSamples/Android* folder with Android Studio to build and run tutorials and samples on Android.

By default, appplications will run in OpenGLES mode. To run them in Vulkan mode, add the following launch flags:
`--es mode vk` (in Android Studio, go to Run->Edit Configurations menu)

<a name="build_and_run_macos"></a>
## MacOS

After you clone the repo, run the following command from the engine's root folder to generate Xcode project
(you need to have [CMake](https://cmake.org/) installed on the system):

```
cmake -S . -B ./build/MacOS -G "Xcode"
```

The project will be located in `build/MacOS` folder.

### Configuring Vulkan Build Environment

By default there is no Vulkan implementation on MacOS. Diligent Engine loads Vulkan dynamically
and can use a Vulkan Portability implementation such as [MoltenVK](https://github.com/KhronosGroup/MoltenVK)
or [gfx-portability](https://github.com/gfx-rs/portability). Install [VulkanSDK](https://vulkan.lunarg.com/sdk/home#mac)
and make sure that your system is properly configured as described
[here](https://vulkan.lunarg.com/doc/view/latest/mac/getting_started.html#user-content-command-line).
In particular, you may need to define the following environment variables (assuming that Vulkan SDK is installed at
`~/LunarG/vulkansdk-macos` and you want to use MoltenVK):

```
export VULKAN_SDK=~/LunarG/vulkansdk-macos/macOS
export PATH=$VULKAN_SDK/bin:$PATH
export DYLD_LIBRARY_PATH=$VULKAN_SDK/lib:$DYLD_LIBRARY_PATH
export VK_ICD_FILENAMES=$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json
export VK_LAYER_PATH=$VULKAN_SDK/share/vulkan/explicit_layer.d
```

Note that environment variables set in the shell are not seen by the applications launched from Launchpad
or other desktop GUI. Thus to make sure that an application finds Vulkan libraries, it needs to be started from 
the command line. Due to the same reason, the xcode project file should also be opened from the shell using 
`open` command. With Xcode versions 7 and later, this behavior may need to be enabled first using the
following command:

```
defaults write com.apple.dt.Xcode UseSanitizedBuildSystemEnvironment -bool NO
```

Please refer to [this page](https://vulkan.lunarg.com/doc/sdk/latest/mac/getting_started.html) for more details.

:warning: `DYLD_LIBRARY_PATH` and `LD_LIBRARY_PATH` environment variables are ignored on MacOS unless
System Integrity Protection is disabled (which generally is not recommended). In order for executables to find the
Vulkan library, it must be in rpath. If `VULKAN_SDK` environment variable is set and points to correct location, Diligent
Engine will configure the rpath for all applications automatically.

Last tested LunarG SDK version: 1.2.154.0.

<a name="build_and_run_ios"></a>
## iOS

Run the command below from the engine's root folder to generate Xcode project configured for
[iOS build](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling-for-ios-tvos-or-watchos):

```cmake
cmake -S . -B ./build/iOS -DCMAKE_SYSTEM_NAME=iOS -G "Xcode"
```

If needed, you can provide iOS deployment target as well as other parameters, e.g.:

```cmake
cmake -S . -B ./build/iOS -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -G "Xcode"
```

Open Xcode project file in `build/IOS` folder and build the engine. To run the applications on an iOS device,
you will need to set appropriate development team in the project settings.

### Configuring Vulkan Build Environment

To enable Vulkan on iOS, download and install [VulkanSDK](https://vulkan.lunarg.com/sdk/home#mac). There is no Vulkan loader
on iOS, and Diligent Engine links directly with MoltenVK XCFramework (see
[MoltenVk install guide](https://github.com/KhronosGroup/MoltenVK/blob/master/Docs/MoltenVK_Runtime_UserGuide.md#install-as-static-framework-static-library-or-dynamic-library))
that implements Vulkan on Metal. To enable Vulkan in Diligent Engine on iOS, specify the path to Vulkan SDK 
when running CMake, for example (assuming that Vulkan SDK is installed at `/LunarG/vulkansdk-macos`):

```cmake
cmake -DCMAKE_SYSTEM_NAME=iOS -DVULKAN_SDK=/LunarG/vulkansdk-macos -S . -B ./build/iOS -G "Xcode"
```

By default, the engine links with MoltenVK XCFramework located in LunarG SDK. If this is not desired or an application wants
to use a framework from a specific location, it can provide the full path to the framework via `MoltenVK_FRAMEWORK` CMake variable.

Refer to [MoltenVK user guide](https://github.com/KhronosGroup/MoltenVK/blob/master/Docs/MoltenVK_Runtime_UserGuide.md#install)
for more information about MoltenVK installation and usage.

Last tested LunarG SDK version: 1.2.154.0.

<a name="build_and_run_integration"></a>
## Integrating Diligent Engine with Existing Build System

Diligent has modular structure, so for your project you can only use these 
submodules that implement the required functionality.
The diagram below shows the dependencies between modules.

```
  Core
   |
   +------>Tools----------.
   |        |             |
   |        V             |
   +------->FX---------.  |
   |                   |  |
   |                   V  V
   '----------------->Samples
```

### Your Project Uses Cmake

If your project uses CMake, adding Diligent Engine requires just few lines of code. 
Suppose that the directory structure looks like this:

```
|
+-DiligentCore
+-HelloDiligent.cpp
```

Then the following steps need to be done:
* Call `add_subdirectory(DiligentCore)`
* Add *DiligentCore* to the list of include directories
* Add dependencies on the targets implementing required rendering backends

Below is an example of a CMake file:

```cmake
cmake_minimum_required (VERSION 3.6)

project(HelloDiligent CXX)

add_subdirectory(DiligentCore)

add_executable(HelloDiligent WIN32 HelloDiligent.cpp)
target_compile_options(HelloDiligent PRIVATE -DUNICODE -DENGINE_DLL)
target_include_directories(HelloDiligent PRIVATE "DiligentCore")

target_link_libraries(HelloDiligent
PRIVATE
    Diligent-GraphicsEngineD3D11-shared
    Diligent-GraphicsEngineOpenGL-shared
    Diligent-GraphicsEngineD3D12-shared
    Diligent-GraphicsEngineVk-shared
)
copy_required_dlls(HelloDiligent)
```

`copy_required_dlls()` is a convenience function that copies shared libraries next to
the executable so that the system can find and load them. Alternatively, you can link against 
static (as well as shared) versions of libraries using `target_link_libraries()` command. In this case 
there is no need to explicitly add *DiligentCore* to the list of include directories as the targets export
all required include paths.
Please also take a look at getting started tutorials for 
[Windows](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial00_HelloWin32) and 
[Linux](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial00_HelloLinux).

### Your Project Does Not Use Cmake

If your project doesn't use CMake, it is recommended to build libraries with CMake and add them to your build system.
For Windows platforms, you can download the latest build artifacts from [appveyor](https://ci.appveyor.com/project/DiligentGraphics/diligentcore).

Global CMake installation directory is controlled by
[CMAKE_INTALL_PREFIX](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html) variable. 
Note that it defaults to `/usr/local` on UNIX and `c:/Program Files/${PROJECT_NAME}` on Windows, which may not
be what you want. Use `-D CMAKE_INSTALL_PREFIX=install` to use local `install` folder instead:

```
cmake -S . -B ./build/Win64 -D CMAKE_INSTALL_PREFIX=install -G "Visual Studio 16 2019" -A x64
```

To install libraries and header files, run the following CMake command from the build folder:

```cmake
cmake --build . --target install
```

DiligentCore installation directory will contain everything required to integrate the engine:

* *include* subdirectory will contain all required header files. Add this directory to your include search directories.
* *lib* subdirectory will contain static libraries.
* *bin* subdirectory will contain dynamic libraries.

An easier way is to link with dynamic libraries. When linking statically, you will need to list DiligentCore as well 
as all third-party libraries used by the engine. Besides that, you will also need to specify platform-specific system libraries. 
For example, for Windows platform, the list of libraries your project will need to link against may look like this:

```
DiligentCore.lib glslang.lib HLSL.lib OGLCompiler.lib OSDependent.lib spirv-cross-core.lib SPIRV.lib SPIRV-Tools-opt.lib SPIRV-Tools.lib glew-static.lib GenericCodeGen.lib MachineIndependent.lib vulkan-1.lib dxgi.lib d3d11.lib d3d12.lib d3dcompiler.lib opengl32.lib
```

Vulkan libraries can be found in [DiligentCore/ThirdParty/vulkan/libs](https://github.com/DiligentGraphics/DiligentCore/tree/master/ThirdParty/vulkan/libs) directory.

Diligent Engine headers require one of the following platform macros to be defined as `1`:
`PLATFORM_WIN32`, `PLATFORM_UNIVERSAL_WINDOWS`, `PLATFORM_ANDROID`, `PLATFORM_LINUX`, `PLATFORM_MACOS`, `PLATFORM_IOS`.

You can control which components of the engine you want to install using the following CMake options:
`DILIGENT_INSTALL_CORE`, `DILIGENT_INSTALL_FX`, `DILIGENT_INSTALL_SAMPLES`, and `DILIGENT_INSTALL_TOOLS`.

Another way to intergrate the engine is to generate build files (such as Visual Studio projects) and add them to your
build system. Build customization described below can help tweak the settings for your specific needs.


<a name="build_option"></a>
## Build Options

By default, all back-ends available on current platform are built. To disable specific back-ends,
use the following options: `DILIGENT_NO_DIRECT3D11`, `DILIGENT_NO_DIRECT3D12`, `DILIGENT_NO_OPENGL`,
`DILIGENT_NO_VULKAN`, `DILIGENT_NO_METAL`.
The options can be set through cmake UI or from the command line as in the example below:

```
cmake -D DILIGENT_NO_DIRECT3D11=TRUE -S . -B ./build/Win64 -G "Visual Studio 16 2019" -A x64
```

Additionally, individual engine components can be enabled or disabled using the following options:
`DILIGENT_BUILD_TOOLS`, `DILIGENT_BUILD_FX`, `DILIGENT_BUILD_SAMPLES`, `DILIGENT_BUILD_DEMOS`,
`DILIGENT_BUILD_UNITY_PLUGIN`. If you only want to build `SampleBase` project,
 you can use `DILIGENT_BUILD_SAMPLE_BASE_ONLY` option.

By default Vulkan back-end is linked with glslang that enables compiling HLSL and GLSL shaders to SPIRV at run time.
If run-time compilation is not required, glslang can be disabled with `DILIGENT_NO_GLSLANG` cmake option. 
Additionally, HLSL support in non-Direct3D backends can be disabled with `DILIGENT_NO_HLSL` option.
Enabling the options significantly reduces the size of Vulkan and OpenGL back-end binaries, which may be
especailly important for mobile applications.

Diligent Engine uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to ensure consistent
formatting throught the code base. The validation can be disabled using `DILIGENT_NO_FORMAT_VALIDATION`
CMake option. Note that any pool request will fail if formatting issues are found.

<a name="build_and_run_customizing"></a>
## Customizing Build

Diligent Engine allows clients to customize build settings by providing configuration script file that defines the following optional 
[cmake functions](https://cmake.org/cmake/help/latest/command/function.html):

* `custom_configure_build()` - defines global build properties such as build configurations, c/c++ compile flags, link flags etc.
* `custom_pre_configure_target()` - defines custom settings for every target in the build and is called before the engine's
									build system starts configuring the target.
* `custom_post_configure_target()` - called after the engine's build system has configured the target to let the client
									 override properties set by the engine.

The path to the configuration script should be provided through `BUILD_CONFIGURATION_FILE` variable when running 
cmake and must be relative to the cmake root folder, for example:

```
cmake -D BUILD_CONFIGURATION_FILE=BuildConfig.cmake -S . -B ./build/Win64 -G "Visual Studio 16 2019" -A x64
```

### Customizing global build settings with custom_configure_build() function

If defined, `custom_configure_build()` function is called before any build target is added. By default,
cmake defines the following four configurations: Debug, Release, RelWithDebInfo, MinSizeRel. If you want, 
you can define your own build configurations by setting `CMAKE_CONFIGURATION_TYPES` variable. For instance,
if you want to have only two configuration: Debug and ReleaseMT, add the following line to the `custom_configure_build()`
function:

```cmake
set(CMAKE_CONFIGURATION_TYPES Debug ReleaseMT CACHE STRING "Configuration types: Debug, ReleaseMT" FORCE)
```

The build system needs to know the list of debug and release (optimized) configurations, so the following
two variables must also be set when `CMAKE_CONFIGURATION_TYPES` variable is defined:

```cmake
set(DEBUG_CONFIGURATIONS DEBUG CACHE INTERNAL "" FORCE)
set(RELEASE_CONFIGURATIONS RELEASEMT CACHE INTERNAL "" FORCE)
```

Note that due to cmake specifics, configuration names listed in `DEBUG_CONFIGURATIONS` and `RELEASE_CONFIGURATIONS`
**must be capitalized**.

If you define any configuration other than four standard cmake ones, you also need to set the following variables, for every
new configuration:

* `CMAKE_C_FLAGS_<Config>` - c compile flags
* `CMAKE_CXX_FLAGS_<Config>` - c++ compile flags
* `CMAKE_EXE_LINKER_FLAGS_<Config>` - executable link flags
* `CMAKE_SHARED_LINKER_FLAGS_<Config>` - shared library link flags

For instance:

```cmake
set(CMAKE_C_FLAGS_RELEASEMT "/MT" CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASEMT "/MT" CACHE INTERNAL "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASEMT "/OPT:REF" CACHE INTERNAL "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELEASEMT "/OPT:REF" CACHE INTERNAL "" FORCE)
```

Below is an example of custom_configure_build() function:

```cmake
function(custom_configure_build)
    if(CMAKE_CONFIGURATION_TYPES)
        # Debug configurations
        set(DEBUG_CONFIGURATIONS DEBUG CACHE INTERNAL "" FORCE)
        # Release (optimized) configurations
        set(RELEASE_CONFIGURATIONS RELEASEMT CACHE INTERNAL "" FORCE)
        # CMAKE_CONFIGURATION_TYPES variable defines build configurations generated by cmake
        set(CMAKE_CONFIGURATION_TYPES Debug ReleaseMT CACHE STRING "Configuration types: Debug, ReleaseMT" FORCE)

        set(CMAKE_CXX_FLAGS_RELEASEMT "/MT" CACHE INTERNAL "" FORCE)
        set(CMAKE_C_FLAGS_RELEASEMT "/MT" CACHE INTERNAL "" FORCE)
        set(CMAKE_EXE_LINKER_FLAGS_RELEASEMT "/OPT:REF" CACHE INTERNAL "" FORCE)
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASEMT "/OPT:REF" CACHE INTERNAL "" FORCE)
    endif()
endfunction()
```


### Customizing individual target build settings with custom_pre_configure_target() and custom_post_configure_target() functions

If defined, `custom_pre_configure_target()` is called for every target created by the build system and
allows configuring target-specific properties.

By default, the build system sets some target properties. If `custom_pre_configure_target()` sets all required properties,
it can tell the build system that no further processing is required by setting `TARGET_CONFIGURATION_COMPLETE`
[parent scope](https://cmake.org/cmake/help/latest/command/set.html#set-normal-variable) variable to `TRUE`:

```cmake
set(TARGET_CONFIGURATION_COMPLETE TRUE PARENT_SCOPE)
```

The following is an example of `custom_pre_configure_target()` function:

```cmake
function(custom_pre_configure_target TARGET)
    set_target_properties(${TARGET} PROPERTIES
        STATIC_LIBRARY_FLAGS_RELEASEMT /LTCG
    )
    set(TARGET_CONFIGURATION_COMPLETE TRUE PARENT_SCOPE)   
endfunction()
```

If the client only needs to override some settings, it may define `custom_post_configure_target()` function that is called
after the engine has completed configuring the target, for example:

```cmake
function(custom_post_configure_target TARGET)
    set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 17
    )
endfunction()
```


<a name="getting_started"></a>
# Getting started with the API

Please refer to [this page](https://github.com/DiligentGraphics/DiligentCore#api-basics). Also, tutorials and samples listed below is a good place to start.


<a name="tutorials"></a>
# [Tutorials](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials)

| Tutorial   | Screenshot  | Description          |
|------------|-------------|----------------------|
| [01 - Hello Triangle](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial01_HelloTriangle) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial01_HelloTriangle/Screenshot.png) | This tutorial shows how to render simple triangle using Diligent Engine API. |
| [02 - Cube](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial02_Cube) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial02_Cube/Animation_Small.gif) | This tutorial demonstrates how to render an actual 3D object, a cube. It shows how to load shaders from files, create and use vertex, index and uniform buffers. |
| [03 - Texturing](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial03_Texturing) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial03_Texturing/Animation_Small.gif) | This tutorial demonstrates how to apply a texture to a 3D object. It shows how to load a texture from file, create shader resource binding object and how to sample a texture in the shader. |
| [03 - Texturing-C](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial03_Texturing-C) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial03_Texturing/Animation_Small.gif) | This tutorial is identical to Tutorial03, but is implemented using C API. |
| [04 - Instancing](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial04_Instancing) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial04_Instancing/Animation_Small.gif) | This tutorial demonstrates how to use instancing to render multiple copies of one object using unique transformation matrix for every copy. |
| [05 - Texture Array](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial05_TextureArray) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial05_TextureArray/Animation_Small.gif) | This tutorial demonstrates how to combine instancing with texture arrays to use unique texture for every instance. |
| [06 - Multithreading](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial06_Multithreading) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial06_Multithreading/Animation_Small.gif) | This tutorial shows how to generate command lists in parallel from multiple threads. |
| [07 - Geometry Shader](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial07_GeometryShader) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial07_GeometryShader/Animation_Small.gif) | This tutorial shows how to use geometry shader to render smooth wireframe. |
| [08 - Tessellation](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial08_Tessellation) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial08_Tessellation/Animation_Small.gif) | This tutorial shows how to use hardware tessellation to implement simple adaptive terrain rendering algorithm. |
| [09 - Quads](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial09_Quads) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial09_Quads/Animation_Small.gif) | This tutorial shows how to render multiple 2D quads, frequently swithcing textures and blend modes. |
| [10 - Data Streaming](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial10_DataStreaming) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial10_DataStreaming/Animation_Small.gif) | This tutorial shows dynamic buffer mapping strategy using `MAP_FLAG_DISCARD` and `MAP_FLAG_DO_NOT_SYNCHRONIZE` flags to efficiently stream varying amounts of data to GPU. |
| [11 - Resource Updates](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial11_ResourceUpdates) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial11_ResourceUpdates/Animation_Small.gif) | This tutorial demonstrates different ways to update buffers and textures in Diligent Engine and explains important internal details and performance implications related to each method. |
| [12 - Render Target](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial12_RenderTarget) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial12_RenderTarget/Animation_Small.gif) | This tutorial demonstrates how to render a 3d cube into an offscreen render target and do a simple post-processing effect. |
| [13 - Shadow Map](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial13_ShadowMap) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial13_ShadowMap/Animation_Small.gif) | This tutorial demonstrates how to render basic shadows using a shadow map. |
| [14 - Compute Shader](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial14_ComputeShader) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial14_ComputeShader/Animation_Small.gif) | This tutorial shows how to implement a simple particle simulation system using compute shaders. |
| [15 - Multiple Windows](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial15_MultipleWindows) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial15_MultipleWindows/Screenshot.png) | This tutorial demonstrates how to use Diligent Engine to render to multiple windows. |
| [16 - Bindless Resources](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial16_BindlessResources) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial16_BindlessResources/Animation_Small.gif) | This tutorial shows how to implement bindless resources, a technique that leverages dynamic shader resource indexing feature enabled by the next-gen APIs to significantly improve rendering performance. |
| [17 - MSAA](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial17_MSAA) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial17_MSAA/Animation_Small.gif) | This tutorial demonstrates how to use multisample anti-aliasing (MSAA) to make geometrical edges look smoother and more temporarily stable. |
| [18 - Queries](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial18_Queries) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial18_Queries/Animation_Small.gif) | This tutorial demonstrates how to use queries to retrieve various information about the GPU operation, such as the number of primitives rendered, command processing duration, etc. |
| [19 - Render Passes](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial19_RenderPasses) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial19_RenderPasses/Animation_Small.gif) | This tutorial demonstrates how to use the render passes API to implement simple deferred shading. |
| [20 - Mesh Shader](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial20_MeshShader) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial20_MeshShader/Animation_Small.gif) | This tutorial demonstrates how to use amplification and mesh shaders, the new programmable stages, to implement view frustum culling and object LOD calculation on the GPU. |

<a name="samples"></a>
# [Samples](https://github.com/DiligentGraphics/DiligentSamples)

| Sample     | Screenshot  | Description          |
|------------|-------------|----------------------|
| [Atmosphere Sample](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/Atmosphere) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/Atmosphere/Animation_Small.gif) | This sample demonstrates how to integrate [Epipolar Light Scattering](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/EpipolarLightScattering) post-processing effect into an application to render physically-based atmosphere. |
| [GLTF Viewer](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/GLTFViewer) | <img src="https://github.com/DiligentGraphics/DiligentFX/blob/master/GLTF_PBR_Renderer/screenshots/flight_helmet.jpg" width=240> | This sample demonstrates how to use the [Asset Loader](https://github.com/DiligentGraphics/DiligentTools/tree/master/AssetLoader) and [GLTF PBR Renderer](https://github.com/DiligentGraphics/DiligentFX/tree/master/GLTF_PBR_Renderer) to load and render GLTF models. |
| [Shadows](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/Shadows) | <img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/Shadows/Screenshot.jpg" width=240> | This sample demonstrates how to use the [Shadowing component](https://github.com/DiligentGraphics/DiligentFX/tree/master/Components#shadows) to render high-quality shadows. |
| [Dear ImGui Demo](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/ImguiDemo) | <img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/ImguiDemo/Screenshot.png" width=240> | This sample demonstrates the integration of the engine with [dear imgui](https://github.com/ocornut/imgui) UI library. |
| [Nuklear Demo](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/NuklearDemo) | <img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/NuklearDemo/Screenshot.png" width=240> | This sample demonstrates the integration of the engine with [nuklear](https://github.com/vurtun/nuklear) UI library. |
| [Hello AR](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Android/HelloAR) | <img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/Android/HelloAR/Screenshot.png" width=240> | This sample demonstrates how to use Diligent Engine in a basic Android AR application. |

<a name="demos"></a>
# Demos

| Project    | Screenshot  | Description          |
|------------|-------------|----------------------|
| [Asteroids Performance Benchmark](https://github.com/DiligentGraphics/DiligentEngine/tree/master/Projects/Asteroids) | ![](Projects/Asteroids/Screenshot.png) | This demo is designed to be a performance benchmark and is based on [this demo](https://software.intel.com/en-us/articles/asteroids-and-directx-12-performance-and-power-savings) developed by Intel. It renders 50,000 unique textured asteroids. Every asteroid is a combination of one of 1000 unique meshes and one of 10 unique textures. The sample uses original D3D11 and D3D12 native implementations, and adds implementation using Diligent Engine API to allow comparing performance of different rendering modes. |
| [Unity Integration Demo](https://github.com/DiligentGraphics/DiligentEngine/tree/master/unityplugin) | ![](unityplugin/GhostCubePlugin/Screenshot.png) | This project demonstrates integration of Diligent Engine with Unity |

<a name="high_level_components"></a>
# High-Level Rendering Components

High-level rendering functionality is implemented by [DiligentFX module](https://github.com/DiligentGraphics/DiligentFX).
The following components are now available:

* [Epipolar light scattering post-effect](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/EpipolarLightScattering)
<img src="https://github.com/DiligentGraphics/DiligentFX/blob/master/PostProcess/EpipolarLightScattering/media/LightScattering.png" width=240>

* [Tone mapping shader utilities](https://github.com/DiligentGraphics/DiligentFX/tree/master/Shaders/PostProcess/ToneMapping/public)

<a name="gltf_loader_and_renderer"></a>
* [GLTF2.0 Loader](https://github.com/DiligentGraphics/DiligentTools/tree/master/AssetLoader)
  and [Physically-based renderer with image-based lighting](https://github.com/DiligentGraphics/DiligentFX/tree/master/GLTF_PBR_Renderer).
  
|||
|-----------------|-----------------|
| ![](https://github.com/DiligentGraphics/DiligentFX/blob/master/GLTF_PBR_Renderer/screenshots/damaged_helmet.jpg) | ![](https://github.com/DiligentGraphics/DiligentFX/blob/master/GLTF_PBR_Renderer/screenshots/flight_helmet.jpg) |
| ![](https://github.com/DiligentGraphics/DiligentFX/blob/master/GLTF_PBR_Renderer/screenshots/mr_spheres.jpg)     | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/GLTFViewer/screenshots/cesium_man_large.gif)  |


* [Shadows](https://github.com/DiligentGraphics/DiligentFX/tree/master/Components#shadows)
<img src="https://github.com/DiligentGraphics/DiligentFX/blob/master/Components/media/Powerplant-Shadows.jpg" width=240>


<a name="products"></a>
# Products using Diligent Engine

We would appreciate it if you could send us a link in case your product uses Diligent Engine.

* [Vrmac Graphics](https://github.com/Const-me/Vrmac): A cross-platform graphics library for .NET  
  <img src="https://github.com/Const-me/Vrmac/blob/master/screenshots/Linux/TigerFullHD-1.png" width=480>
* Your product here (please submit a [PR](https://github.com/DiligentGraphics/DiligentEngine/pulls))!


<a name="license"></a>
# License

See [Apache 2.0 license](License.txt).

This project has some third-party dependencies, each of which may have independent licensing:

* Core module:
  * [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross): SPIRV parsing and cross-compilation tools.
  * [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers): SPIRV header files.
  * [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools): SPIRV optimization and validation tools.
  * [glslang](https://github.com/KhronosGroup/glslang): Khronos reference compiler and validator for GLSL, ESSL, and HLSL.
  * [glew](http://glew.sourceforge.net/): OpenGL Extension Wrangler Library.
* Tools module:
  * [libjpeg](http://libjpeg.sourceforge.net/): C library for reading and writing JPEG image files.
  * [libtiff](http://www.libtiff.org/): TIFF Library and Utilities.
  * [libpng](http://www.libpng.org/pub/png/libpng.html): Official PNG reference library.
  * [zlib](https://zlib.net/): A compression library.
  * [tinygltf](https://github.com/syoyo/tinygltf): A header only C++11 glTF 2.0 library.
  * [dear imgui](https://github.com/ocornut/imgui): A bloat-free immediate mode graphical user interface library.


<a name="contributing"></a>
# Contributing

To contribute your code, submit a [Pull Request](https://github.com/DiligentGraphics/DiligentEngine/pulls) 
to this repository. **Diligent Engine** is licensed under the [Apache 2.0 license](License.txt) that guarantees 
that code in the **DiligentEngine** repository is free of Intellectual Property encumbrances. In submitting code to
this repository, you are agreeing that the code is free of any Intellectual Property claims.  

Diligent Engine uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to ensure
consistent source code style throught the code base. The format is validated by appveyor and travis
for each commit and pull request, and the build will fail if any code formatting issue is found. Please refer
to [this page](https://github.com/DiligentGraphics/DiligentCore/blob/master/doc/code_formatting.md) for instructions
on how to set up clang-format and automatic code formatting.

<a name="references"></a>
# References

[API Reference](https://cdn.rawgit.com/DiligentGraphics/DiligentCore/4949ec8a/doc/html/index.html)


<a name="release_history"></a>
# Release History

See [Release History](ReleaseHistory.md)

------------------------------

[diligentgraphics.com](http://diligentgraphics.com)

[![Diligent Engine on Twitter](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/twitter.png)](https://twitter.com/diligentengine)
[![Diligent Engine on Facebook](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/facebook.png)](https://www.facebook.com/DiligentGraphics/)
