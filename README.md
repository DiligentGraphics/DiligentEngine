# Diligent Engine <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/diligentgraphics-logo.png" height=64 align="right" valign="middle">
**A Modern Cross-Platform Low-Level 3D Graphics Library and Rendering Framework**
[![Tweet](https://img.shields.io/twitter/url/http/shields.io.svg?style=social)](https://twitter.com/intent/tweet?text=An%20easy-to-use%20cross-platform%20graphics%20library%20that%20takes%20full%20advantage%20of%20%23Direct3D12%20and%20%23VulkanAPI&url=https://github.com/DiligentGraphics/DiligentEngine)

[Diligent Engine](http://diligentgraphics.com/diligent-engine/) is a lightweight cross-platform graphics
API abstraction library and rendering framework. It is designed to take full advantage of Direct3D12, Vulkan
and Metal, while supporting older platforms via Direct3D11, OpenGL and OpenGLES. Diligent Engine exposes common
front-end  API and uses HLSL as universal shading language on all platforms and rendering back-ends. 
Platform-specific shader representations (GLSL, MSL, DX bytecode or SPIRV) can be used with corresponding back-ends.
The engine is intended to be used as graphics subsystem in a game engine or any other 3D application. 
It is distributed under [Apache 2.0 license](License.txt) and is free to use.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](License.txt)
[![Chat on Discord](https://img.shields.io/discord/730091778081947680?logo=discord)](https://discord.gg/t7HGBK7)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/github/DiligentGraphics/DiligentEngine?svg=true)](https://ci.appveyor.com/project/DiligentGraphics/diligentengine)

## Supported Platforms and Low-Level Graphics APIs

| Platform                                                                                                                                     | D3D11              | D3D12              |  OpenGL/GLES       | Vulkan                          | Metal                           |  Build Status                    |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ------------------ |------------------- | ------------------ | ------------------------------- | ------------------------------- | -------------------------------- |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/windows-logo.png" width=24 valign="middle"> Windows             | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark:              | -                               |  [![Build Status](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-windows.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-windows.yml?query=branch%3Amaster) |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/uwindows-logo.png" width=24 valign="middle"> Universal Windows  | :heavy_check_mark: | :heavy_check_mark: | -                  | -                               | -                               |  [![Build Status](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-windows.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-windows.yml?query=branch%3Amaster) |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/linux-logo.png" width=24 valign="middle"> Linux                 | -                  | -                  | :heavy_check_mark: | :heavy_check_mark:              | -                               |  [![Build Status](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-linux.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-linux.yml?query=branch%3Amaster) |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/android-logo.png" width=24 valign="middle"> Android             | -                  | -                  | :heavy_check_mark: | :heavy_check_mark:              | -                               |  [![Build Status](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-android.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-android.yml?query=branch%3Amaster) |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/macos-logo.png" width=24 valign="middle"> MacOS                 | -                  | -                  | :heavy_check_mark: | :heavy_check_mark: <sup>1</sup> | :heavy_check_mark: <sup>2</sup> |  [![Build Status](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-apple.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-apple.yml?query=branch%3Amaster) |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/apple-logo.png" width=24 valign="middle"> iOS                   | -                  | -                  | :heavy_check_mark: | :heavy_check_mark: <sup>1</sup> | :heavy_check_mark: <sup>2</sup> |  [![Build Status](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-apple.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-apple.yml?query=branch%3Amaster) |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/tvos-logo.png" width=24 valign="middle"> tvOS                   | -                  | -                  | -                  | :heavy_check_mark: <sup>1</sup> | :heavy_check_mark: <sup>2</sup> |  [![Build Status](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-apple.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-apple.yml?query=branch%3Amaster) |
| <img src="https://github.com/DiligentGraphics/DiligentCore/blob/master/media/emscripten-logo.png" width=24 valign="middle"> Emscripten       | -                  | -                  | :heavy_check_mark: | -                               | -                               |  [![Build Status](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-emscripten.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentEngine/actions/workflows/build-emscripten.yml?query=branch%3Amaster) |


<sup>1</sup> Vulkan API is not natively supported on MacOS, iOS and tvOS platforms and requires a Vulkan portability implementation such as [MoltenVK](https://github.com/KhronosGroup/MoltenVK)
or [gfx-portability](https://github.com/gfx-rs/portability).

<sup>2</sup> Available under commercial license - please contact us for details.

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
  * C/C++/C#
  * Object-based
  * Stateless
* Key graphics features:
  * [Automatic shader resource binding](http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/) designed to leverage next-generation graphics APIs
  * Multithreaded command buffer generation
  * Multithreaded resource creation
  * [Automatic or explicit control over resource state transitions](http://diligentgraphics.com/2018/12/09/resource-state-management/)
  * Descriptor and memory management
  * Shader resource reflection
  * Async compute and multiple command queues
  * Ray-tracing, mesh shaders, tile shaders, bindless resources, variable rate shading, sparse resources,
    wave operations, and other state-of-the-art capabilities
* JSON-based render state description language and state packaging tool
* Extensive validation and error reporting
* Modern c++ features to make the code fast and reliable
* Consistent high quality is ensured by continuous integration
  * Automated builds and unit testing
  * Source code formatting validation
  * Static analysis


Minimum supported low-level API versions:
* OpenGL 4.1
* OpenGLES 3.0
* WebGL 2.0
* Direct3D11.1
* Direct3D12 with SDK version 10.0.17763.0
* Vulkan 1.0
* Metal 1.0


## High-level Rendering components

* [Screen-Space Reflections](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/ScreenSpaceReflection)
* [Screen-Space Ambient Occlusion](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/ScreenSpaceAmbientOcclusion)
* [Depth of Field](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/DepthOfField)
* [Bloom](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/Bloom)
* [Temporal Anti-Aliasing](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/TemporalAntiAliasing)
* [Atmospheric light scattering post-effect](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/EpipolarLightScattering)
* [Tone mapping utilities](https://github.com/DiligentGraphics/DiligentFX/tree/master/Shaders/PostProcess/ToneMapping/public)
* [PBR renderer](https://github.com/DiligentGraphics/DiligentFX/tree/master/PBR)
* [Hydrogent](https://github.com/DiligentGraphics/DiligentFX/tree/master/Hydrogent), an implementation of the Hydra rendering API in Diligent Engine.
* [Shadows](https://github.com/DiligentGraphics/DiligentFX/tree/master/Components#shadows)
* [Integration with Dear Imgui](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/ImguiDemo)
  [and Nuklear](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/NuklearDemo)


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
  - [Emscripten](#build_and_run_emscripten)
  - [Integrating Diligent Engine with Existing Build System](#build_and_run_integration)
  - [Build Options](#build_option)
  - [Customizing Build](#build_and_run_customizing)
- [Getting started with the API](#getting_started)
- [Render State Notation](#render_state_notation)
- [Tutorials](#tutorials)
- [Samples](#samples)
- [High-Level Rendering Components](#high_level_components)
- [Products Using Diligent Engine](#products-using-diligent-engine)
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
  [dear imgui implementation](https://github.com/DiligentGraphics/DiligentTools/blob/master/Imgui),
  [native application implementation](https://github.com/DiligentGraphics/DiligentTools/blob/master/NativeApp),
  [Diligent render state notation parser](https://github.com/DiligentGraphics/DiligentTools/tree/master/RenderStateNotation) and
  [offline render state packaging tool](https://github.com/DiligentGraphics/DiligentTools/tree/master/RenderStatePackager).
* [DiligentFX](https://github.com/DiligentGraphics/DiligentFX) is a high-level rendering framework that implements
  various rendering components. The module depends on Core and Tools modules.
* [Samples](https://github.com/DiligentGraphics/DiligentSamples) submodule contains tutorials and sample applications 
  intended to demonstrate the usage of the Diligent Engine API. The module depends on Core, Tools and DiligentFX modules.

<a name="build_and_run"></a>
# Build and Run Instructions

Diligent Engine uses [CMake](https://cmake.org/) as a cross-platform build tool. 
To start using cmake, download the [latest release](https://cmake.org/download/) (3.20 or later is required).
Another build prerequisite is [Python interpreter](https://www.python.org/downloads/) (3.0 or later is required).
If after following the instructions below you have build/run issues, please take a look at [troubleshooting](Troubleshooting.md).

<a name="build_and_run_win32"></a>
## Win32

Build prerequisites:

* Windows SDK 10.0.17763.0 or later (10.0.19041.0 is required for mesh shaders)
* C++ build tools
* Visual C++ ATL Support

.NET support requires .NET SDK 6.0 or later.

Use either CMake GUI or command line tool to generate build files. For example, to generate 
[Visual Studio 2022](https://visualstudio.microsoft.com/) 64-bit solution and project files in *build/Win64* folder, 
navigate to the engine's root folder and run the following command:

```
cmake -S . -B ./build/Win64 -G "Visual Studio 17 2022" -A x64
```

You can generate Win32 solution that targets Win8.1 SDK using the following command:

```
cmake -D CMAKE_SYSTEM_VERSION=8.1 -S . -B ./build/Win64_8.1 -G "Visual Studio 17 2022" -A x64
```

If you use MinGW, you can generate the make files using the command below (note however that the functionality
will be limited and that MinGW is not a recommended way to build the engine):

```
cmake -S . -B ./build/MinGW -D CMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
```

:warning: In current implementation, full path to cmake build folder **must not contain white spaces**.

To enable Vulkan validation layers, you will need to download the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) and add environment
variable `VK_LAYER_PATH` that contains the path to the *Bin* directory in VulkanSDK installation folder.

Open *DiligentEngine.sln* file in *build/Win64* folder, select configuration and build the engine. Set the desired project
as startup project (by default, GLTF Viewer will be selected) and run it. 

By default, sample and tutorial applications will show rendering backend selection dialog box. Use the following command line options to force
D3D11, D3D12, OpenGL, or Vulkan mode: **--mode d3d11**, **--mode d3d12**, **--mode gl**, or **--mode vk**. If you want to run an application
outside of Visual Studio environment, the application's assets folder must be set as working directory. (For Visual Studio, this 
is automatically configured by CMake). Alternatively, you can navigate to the build target or install folder and run the executable from there.


<a name="build_and_run_uwp"></a>
## Universal Windows Platform

To generate build files for Universal Windows platform, you need to define the following two cmake variables:

* `CMAKE_SYSTEM_NAME=WindowsStore`
* `CMAKE_SYSTEM_VERSION=< Windows Version >`

For example, to generate Visual Studio 2022 64-bit solution and project files in *build/UWP64* folder, run the following command
from the engine's root folder:

```
cmake -D CMAKE_SYSTEM_NAME=WindowsStore -D CMAKE_SYSTEM_VERSION=10.0 -S . -B ./build/UWP64 -G "Visual Studio 17 2022" -A x64
```

Set the desired project as startup project (by default, GLTF Viewer will be selected) and run it. 

By default, applications will run in D3D12 mode. You can select D3D11 or D3D12 using the following command line options:
**--mode d3d11**, **--mode d3d12**.

Note: it is possible to generate solution that targets Windows 8.1 by defining CMAKE_SYSTEM_VERSION=8.1 cmake variable, but it will fail
to build as it will use Visual Studio 2013 (v120) toolset that lacks proper c++14 support.


<a name="build_and_run_linux"></a>
## Linux

Your Linux environment needs to be set up for c++ development. If it already is, make sure your c++ tools are up to date
as Diligent Engine uses modern c++ features (clang 10 or later is recommended).

:warning: gcc 9 and above seemingly produces invalid binary code with O2 and O3 optimization levels. To avoid crashes,
optimization level is downgraded to O1 in release configurations. It is recommended to use clang or gcc 7 or 8.

You may need to install the following packages:

1. gcc, clang, make and other essential c/c++ tools:

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
sudo apt-get install libgl1-mesa-dev
sudo apt-get install libxrandr-dev
sudo apt-get install libxinerama-dev
sudo apt-get install libxcursor-dev
sudo apt-get install libxi-dev
```

To configure Vulkan you will also need to:

* Install latest Vulkan drivers and libraries for your GPU
* Install [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)
  * To make sure that you system is properly configured you can try to build and run samples from the SDK

To generate make files for debug configuration, run the following CMake command from the engine's root folder:

```
cmake -S . -B ./build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Debug"
```

To build the engine, run the following command:

```
cmake --build ./build
```

The engine's root folder contains [Visual Studio Code](https://code.visualstudio.com/) settings files that configure
the IDE to build the engine. You can run applications directly from the IDE. To run an application from the command line,
the app's assets folder must be current directory.


<a name="build_and_run_android"></a>
## Android

Please make sure that your machine is set up for Android development. Download 
[Android Studio](https://developer.android.com/studio/index.html),
[install and configure the NDK and CMake](https://developer.android.com/studio/projects/install-ndk)
and other required tools. NDK r24 or later is required. If you are not using CMake version bundled with Android Studio, make sure
your build files are [properly configured](https://developer.android.com/studio/projects/add-native-code.html#use_a_custom_cmake_version).
To verify that your environment is properly set up, try building the
[teapots sample](https://github.com/googlesamples/android-ndk/tree/master/teapots) as well as
[Vulkan Android tutorials](https://github.com/googlesamples/android-vulkan-tutorials).

Open *DiligentSamples/Android* folder with Android Studio to build and run tutorials and samples on Android.

By default, applications will run in Vulkan mode. To run them in Vulkan mode, add the following launch flags:
`--es mode gl` (in Android Studio, go to Run->Edit Configurations menu)

<a name="build_and_run_macos"></a>
## MacOS

Prerequisites:

* Xcode 14 or later
* Vulkan SDK 1.3.268.1 or later to enable Vulkan

After you clone the repo, run the following command from the engine's root folder to generate Xcode project:

```
cmake -S . -B ./build/MacOS -G "Xcode"
```

The project will be located in `build/MacOS` folder.

Note that if CMake fails to find the compiler, you may need to run the following command:

```
sudo xcode-select --reset
```

### Configuring Vulkan Build Environment

By default there is no Vulkan implementation on MacOS. Diligent Engine loads Vulkan dynamically
and can use a Vulkan Portability implementation such as [MoltenVK](https://github.com/KhronosGroup/MoltenVK)
or [gfx-portability](https://github.com/gfx-rs/portability). Install [VulkanSDK](https://vulkan.lunarg.com/sdk/home#mac)
and make sure that your system is properly configured as described
[here](https://vulkan.lunarg.com/doc/view/latest/mac/getting_started.html#user-content-sdk-system-paths).
In particular, you may need to define the following environment variables (assuming that Vulkan SDK is installed at
`/Users/MyName/VulkanSDK/1.3.268.1` and you want to use MoltenVK):

```
export VULKAN_SDK=/Users/MyName/VulkanSDK/1.3.268.1/macOS
export PATH=$VULKAN_SDK/bin:$PATH
export DYLD_LIBRARY_PATH=$VULKAN_SDK/lib:$DYLD_LIBRARY_PATH
export VK_ADD_LAYER_PATH=$VULKAN_SDK/share/vulkan/explicit_layer.d
export VK_ICD_FILENAMES=$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json
export VK_DRIVER_FILES=$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json
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

Latest tested Vulkan SDK version: 1.3.268.1.

:warning: There are known issues with later versions of the SDK, so it is recommended to use the latest tested version.

<a name="build_and_run_ios"></a>
## iOS

Prerequisites:

* Xcode 14 or later
* Vulkan SDK 1.3.268.1 or later to enable Vulkan

Run the command below from the engine's root folder to generate Xcode project configured for
[iOS build](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling-for-ios-tvos-or-watchos):

```cmake
cmake -S . -B ./build/iOS -DCMAKE_SYSTEM_NAME=iOS -G "Xcode"
```

If needed, you can provide iOS deployment target (13.0 or later is required) as well as other parameters, e.g.:

```cmake
cmake -S . -B ./build/iOS -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 -G "Xcode"
```

:warning: To build for iPhone simulator, use the `iphonesimulator` system root. You may also use the
`CMAKE_OSX_ARCHITECTURES` variable to specify target architecture, for example:

```cmake
cmake -S . -B ./build/iOSSim -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64 -G "Xcode"
```

Open Xcode project file in `build/IOS` folder and build the engine. To run the applications on an iOS device,
you will need to set appropriate development team in the project settings.

### Configuring Vulkan Build Environment

To enable Vulkan on iOS, download and install the [VulkanSDK](https://vulkan.lunarg.com/sdk/home#mac). There is no Vulkan loader
on iOS, and Diligent Engine links directly with MoltenVK XCFramework (see
[MoltenVk install guide](https://github.com/KhronosGroup/MoltenVK/blob/master/Docs/MoltenVK_Runtime_UserGuide.md#install-moltenvk-as-a-universal-xcframework))
that implements Vulkan on Metal. To enable Vulkan in Diligent Engine on iOS, specify the path to Vulkan SDK 
when running CMake, for example (assuming that Vulkan SDK is installed at `/Users/MyName/VulkanSDK/1.3.268.1`):

```cmake
cmake -DCMAKE_SYSTEM_NAME=iOS -DVULKAN_SDK=/Users/MyName/VulkanSDK/1.3.268.1 -S . -B ./build/iOS -G "Xcode"
```

By default, the engine links with MoltenVK XCFramework located in Vulkan SDK. If this is not desired or an application wants
to use a specific library, it can provide the full path to the library via `MOLTENVK_LIBRARY` CMake variable.

Refer to [MoltenVK user guide](https://github.com/KhronosGroup/MoltenVK/blob/master/Docs/MoltenVK_Runtime_UserGuide.md#install)
for more information about MoltenVK installation and usage.

Latest tested Vulkan SDK version: 1.3.268.1.

:warning: There are known issues with later versions of the SDK, so it is recommended to use the latest tested version.

<a name="build_and_run_emscripten"></a>
## Emscripten
Build prerequisites:

* Emscripten SDK 2.0.30
* Ninja 1.10.2

To activate PATH and other environment variables in the current terminal
````bash
source ${PATH_TO_EMSDK}/emsdk/emsdk_env.sh
````
:warning: On Windows, run `${PATH_TO_EMSDK}/emsdk/emsdk_env.bat` instead of `source ${PATH_TO_EMSDK}/emsdk/emsdk_env.sh`


To generate project, run the following CMake command from the engine's root folder:

```cmake
emcmake cmake -S . -B ./build/Emscripten -G "Ninja"
```

To build the engine, run the following command:

```cmake
cmake --build ./build/Emscripten
```

To test emscripten applications, run a basic web server

```bash
cd ./build/Emscripten
python -m http.server
```

Open a browser, and navigate to `http://localhost:8000`

For example, the demo will be available at 
```
http://localhost:8000/DiligentSamples/Tutorials/Tutorial01_HelloTriangle/Tutorial01_HelloTriangle.html
```

<a name="build_and_run_integration"></a>
## Integrating Diligent Engine with Existing Build System

Diligent has modular structure, so for your project you can only use those 
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

Don't forget to recursively initialize submodules if you are adding Diligent repos
as submodules to your project.

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
* Add dependencies on the targets implementing required rendering backends

Below is an example of a CMake file:

```cmake
cmake_minimum_required (VERSION 3.6)

project(HelloDiligent CXX)

add_subdirectory(DiligentCore)

add_executable(HelloDiligent WIN32 HelloDiligent.cpp)
target_compile_options(HelloDiligent PRIVATE -DUNICODE -DENGINE_DLL)

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
the executable so that the system can find and load them. 
Please also take a look at getting started tutorials for 
[Windows](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial00_HelloWin32) and 
[Linux](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial00_HelloLinux).

#### Using FetchContent

You can use [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html) to download Diligent Engine modules.
The only caveat is that you need to specify the source directory for each module to be the same as the module name,
so that header files can be found. Below is an example of a CMake file that uses FetchContent:

```cmake
cmake_minimum_required (VERSION 3.6)

project(HelloDiligent CXX)

include(FetchContent)
FetchContent_Declare(
    DiligentCore
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentCore.git
    SOURCE_DIR _deps/DiligentCore
)
FetchContent_Declare(
    DiligentTools
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentTools.git
    SOURCE_DIR _deps/DiligentTools
)
FetchContent_Declare(
    DiligentFX
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentFX.git
    SOURCE_DIR _deps/DiligentFX
)
FetchContent_MakeAvailable(DiligentCore DiligentTools DiligentFX)

add_executable(HelloDiligent WIN32 HelloDiligent.cpp)
target_include_directories(HelloDiligent
PRIVATE
    ${diligentcore_SOURCE_DIR}
    ${diligenttools_SOURCE_DIR}
    ${diligentfx_SOURCE_DIR}
)

target_compile_definitions(HelloDiligent PRIVATE UNICODE)

target_link_libraries(HelloDiligent
PRIVATE
    Diligent-BuildSettings
    Diligent-GraphicsEngineD3D11-shared
    Diligent-GraphicsEngineD3D12-shared
    Diligent-GraphicsEngineOpenGL-shared
    Diligent-GraphicsEngineVk-shared
    DiligentFX
)
copy_required_dlls(HelloDiligent)
```

### Your Project Does Not Use Cmake

If your project doesn't use CMake, it is recommended to build libraries with CMake and add them to your build system.
You can download the latest build artifacts from [GitHub](https://github.com/DiligentGraphics/DiligentCore/actions?query=branch%3Amaster).

Global CMake installation directory is controlled by
[CMAKE_INTALL_PREFIX](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html) variable. 
Note that it defaults to `/usr/local` on UNIX and `c:/Program Files/${PROJECT_NAME}` on Windows, which may not
be what you want. Use `-D CMAKE_INSTALL_PREFIX=install` to use local `install` folder instead:

```
cmake -S . -B ./build/Win64 -D CMAKE_INSTALL_PREFIX=install -G "Visual Studio 17 2022" -A x64
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
DiligentCore.lib glslang.lib HLSL.lib OGLCompiler.lib OSDependent.lib spirv-cross-core.lib SPIRV.lib SPIRV-Tools-opt.lib SPIRV-Tools.lib glew-static.lib GenericCodeGen.lib MachineIndependent.lib dxgi.lib d3d11.lib d3d12.lib d3dcompiler.lib opengl32.lib
```

Diligent Engine headers require one of the following platform macros to be defined as `1`:
`PLATFORM_WIN32`, `PLATFORM_UNIVERSAL_WINDOWS`, `PLATFORM_ANDROID`, `PLATFORM_LINUX`, `PLATFORM_MACOS`, `PLATFORM_IOS`.

You can control which components of the engine you want to install using the following CMake options:
`DILIGENT_INSTALL_CORE`, `DILIGENT_INSTALL_FX`, `DILIGENT_INSTALL_SAMPLES`, and `DILIGENT_INSTALL_TOOLS`.

Another way to integrate the engine is to generate build files (such as Visual Studio projects) and add them to your
build system. Build customization described below can help tweak the settings for your specific needs.


<a name="build_option"></a>
## Build Options

Available CMake options are summarized in the table below:

| Option                                  |Default value|     Description                                              |
|-----------------------------------------|-------------|--------------------------------------------------------------|
| `DILIGENT_NO_DIRECT3D11`                |    No       | Do not build Direct3D11 backend                              |
| `DILIGENT_NO_DIRECT3D12`                |    No       | Do not build Direct3D12 backend                              |
| `DILIGENT_NO_OPENGL`                    |    No       | Do not build OpenGL backend                                  |
| `DILIGENT_NO_VULKAN`                    |    No       | Do not build Vulkan backend                                  |
| `DILIGENT_NO_METAL`                     |    No       | Do not build Metal backend                                   |
| `DILIGENT_NO_ARCHIVER`                  |    No       | Do not build Archiver                                        |
| `DILIGENT_NO_RENDER_STATE_PACKAGER`     |    No       | Do not build Render State Packager tool                      |
| `DILIGENT_ENABLE_DRACO`                 |    No       | Enable Draco compression support in GLTF loader              |
| `DILIGENT_BUILD_TOOLS`                  |    Yes      | Build Tools module                                           |
| `DILIGENT_BUILD_FX`                     |    Yes      | Build FX module                                              |
| `DILIGENT_BUILD_SAMPLES`                |    Yes      | Build Samples module                                         |
| `DILIGENT_BUILD_SAMPLE_BASE_ONLY`       |    No       | Build only SampleBase project and no other samples/tutorials |
| `DILIGENT_BUILD_TESTS`                  |    No       | Build Unit Tests                                             |
| `DILIGENT_NO_GLSLANG`                   |    No       | Do not build GLSLang and SPRIV-Tools                         |
| `DILIGENT_NO_HLSL`                      |    No       | Disable HLSL support in non-Direct3D backends                |
| `DILIGENT_NO_FORMAT_VALIDATION`         |    No       | Disable source code formatting validation                    |
| `DILIGENT_LOAD_PIX_EVENT_RUNTIME`       |    No       | Enable PIX event support                                     |
| `DILIGENT_NVAPI_PATH`                   |             | Path to NVAPI SDK                                            |
| `DILIGENT_INSTALL_CORE`                 |    Yes      | Install core module                                          |
| `DILIGENT_INSTALL_TOOLS`                |    Yes      | Install tools module                                         |
| `DILIGENT_INSTALL_FX`                   |    Yes      | Install FX module                                            |
| `DILIGENT_INSTALL_SAMPLES`              |    Yes      | Install Samples module                                       |
| `DILIGENT_INSTALL_PDB`                  |    No       | Install program debug database                               |
| `DILIGENT_DEAR_IMGUI_PATH`              |             | Optional path to a user-provided dear imgui project          |
| `DILIGENT_ARGS_DIR`                     |             | Optional path to a user-provided args project                |
| `DILIGENT_NUKLEAR_DIR`                  |             | Optional path to a user-provided nuklear project             |
| `DILIGENT_MSVC_COMPILE_OPTIONS`         |     /WX     | Additional MSVC compile options for all configurations       |
| `DILIGENT_MSVC_DEBUG_COMPILE_OPTIONS`   |             | Additional MSVC compile options for debug configuration      |
| `DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS` |  /arch:AVX2 | Additional MSVC compile options for release configurations   |
| `DILIGENT_CLANG_COMPILE_OPTIONS`        |   -Werror   | Additional Clang compile options for all configurations      |
| `DILIGENT_CLANG_DEBUG_COMPILE_OPTIONS`  |             | Additional Clang compile options for debug configuration     |
| `DILIGENT_CLANG_RELEASE_COMPILE_OPTIONS`|    -mavx2   | Additional Clang compile options for release configurations  |
| `DILIGENT_USD_PATH`                     |             | Path to USD installation folder                              |

By default, all back-ends available on the current platform are built. To disable specific back-ends,
use the following options: `DILIGENT_NO_DIRECT3D11`, `DILIGENT_NO_DIRECT3D12`, `DILIGENT_NO_OPENGL`,
`DILIGENT_NO_VULKAN`, `DILIGENT_NO_METAL`.
The options can be set through cmake UI or from the command line as in the example below:

```
cmake -D DILIGENT_NO_DIRECT3D11=TRUE -S . -B ./build/Win64 -G "Visual Studio 17 2022" -A x64
```

Additionally, individual engine components can be enabled or disabled using the following options:
`DILIGENT_BUILD_TOOLS`, `DILIGENT_BUILD_FX`, `DILIGENT_BUILD_SAMPLES`.
If you only want to build `SampleBase` project, you can use `DILIGENT_BUILD_SAMPLE_BASE_ONLY` option.

By default Vulkan back-end is linked with glslang that enables compiling HLSL and GLSL shaders to SPIRV at run time.
If run-time compilation is not required, glslang can be disabled with `DILIGENT_NO_GLSLANG` cmake option. 
Additionally, HLSL support in non-Direct3D backends can be disabled with `DILIGENT_NO_HLSL` option.
Enabling the options significantly reduces the size of Vulkan and OpenGL back-end binaries, which may be
especially important for mobile applications.

Diligent Engine uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to ensure consistent
formatting throughout the code base. The validation can be disabled using `DILIGENT_NO_FORMAT_VALIDATION`
CMake option. Note that any pull request will fail if formatting issues are found.

Diligent Engine uses extensive validation that is always enabled in Debug build. Some of the checks may be
enabled in release configurations by setting `DILIGENT_DEVELOPMENT` CMake option.

To enable PIX events support, set `DILIGENT_LOAD_PIX_EVENT_RUNTIME` CMake flag.

To enable some advanced features on NVidia GPUs (such as native multi draw indirect support in Direct3D11),
download [NVAPI](https://developer.nvidia.com/nvapi) and set the `DILIGENT_NVAPI_PATH` CMake variable. 

Diligent Engine uses multiple third-party libraries. If an application's CMake file defines any of
those libraries, Diligent will use existing targets. The application will need to make sure that
build settings are compatible with Diligent.

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
cmake -D BUILD_CONFIGURATION_FILE=BuildConfig.cmake -S . -B ./build/Win64 -G "Visual Studio 17 2022" -A x64
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


<a name="render_state_notation"></a>
# Render State Notation

Diligent Render State Notation is a JSON-based language that describes shaders, pipeline states,
resource signatures and other objects in a convenient form, e.g.:

```json
{
    "Shaders": [
        {
            "Desc": {
                "Name": "My Vertex shader",
                "ShaderType": "VERTEX"
            },
            "SourceLanguage": "HLSL",
            "FilePath": "cube.vsh"
        },
        {
            "Desc": {
                "Name": "My Pixel shader",
                "ShaderType": "PIXEL"
            },
            "SourceLanguage": "HLSL",
            "FilePath": "cube.psh",
        }
    ],
    "Pipeleines": [
        {
            "GraphicsPipeline": {
                "DepthStencilDesc": {
                    "DepthEnable": true
                },
                "RTVFormats": {
                    "0": "RGBA8_UNORM_SRGB"
                },
                "RasterizerDesc": {
                    "CullMode": "FRONT"
                },
                "BlendDesc": {
                    "RenderTargets": {
                        "0": {
                            "BlendEnable": true
                        }
                    }
                }
            },
            "PSODesc": {
                "Name": "My Pipeline State",
                "PipelineType": "GRAPHICS"
            },
            "pVS": "My Vertex shader",
            "pPS": "My Pixel shader"
        }
    ]
}
```

JSON files can be [parsed dynamically at run time](https://github.com/DiligentGraphics/DiligentTools/tree/master/RenderStateNotation/interface).
Alternatively, an application can use the [packager tool](https://github.com/DiligentGraphics/DiligentTools/tree/master/RenderStatePackager) to preprocess pipeline
descriptions (compile shaders for target platforms, define internal resource layouts, etc.) into a binary archive optimized for run-time loading performance.


<a name="tutorials"></a>
# [Tutorials](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials)

| Tutorial   | Screenshot  | Description          |
|------------|-------------|----------------------|
| [01 - Hello Triangle](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial01_HelloTriangle) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial01_HelloTriangle/Screenshot.png) | This tutorial shows how to render simple triangle using Diligent Engine API. |
| [02 - Cube](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial02_Cube) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial02_Cube/Animation_Small.gif) | This tutorial demonstrates how to render an actual 3D object, a cube. It shows how to load shaders from files, create and use vertex, index and uniform buffers. |
| [03 - Texturing](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial03_Texturing) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial03_Texturing/Animation_Small.gif) | This tutorial demonstrates how to apply a texture to a 3D object. It shows how to load a texture from file, create shader resource binding object and how to sample a texture in the shader. |
| [03 - Texturing-C](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial03_Texturing-C) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial03_Texturing/Animation_Small.gif) | This tutorial is identical to Tutorial03, but is implemented using C API. |
| [03 - Texturing-DotNet](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial03_Texturing-DotNet) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial03_Texturing/Animation_Small.gif) | This tutorial demonstrates how to use the Diligent Engine API in .NET applications. |
| [04 - Instancing](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial04_Instancing) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial04_Instancing/Animation_Small.gif) | This tutorial demonstrates how to use instancing to render multiple copies of one object using unique transformation matrix for every copy. |
| [05 - Texture Array](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial05_TextureArray) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial05_TextureArray/Animation_Small.gif) | This tutorial demonstrates how to combine instancing with texture arrays to use unique texture for every instance. |
| [06 - Multithreading](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial06_Multithreading) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial06_Multithreading/Animation_Small.gif) | This tutorial shows how to generate command lists in parallel from multiple threads. |
| [07 - Geometry Shader](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial07_GeometryShader) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial07_GeometryShader/Animation_Small.gif) | This tutorial shows how to use geometry shader to render smooth wireframe. |
| [08 - Tessellation](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial08_Tessellation) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial08_Tessellation/Animation_Small.gif) | This tutorial shows how to use hardware tessellation to implement simple adaptive terrain rendering algorithm. |
| [09 - Quads](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial09_Quads) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial09_Quads/Animation_Small.gif) | This tutorial shows how to render multiple 2D quads, frequently switching textures and blend modes. |
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
| [21 - Ray Tracing](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial21_RayTracing) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial21_RayTracing/Animation_Small.gif) | This tutorial demonstrates the basics of using ray tracing API in Diligent Engine. |
| [22 - Hybrid Rendering](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial22_HybridRendering) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial22_HybridRendering/Animation_Small.gif) | This tutorial demonstrates how to implement a simple hybrid renderer that combines rasterization with ray tracing. |
| [23 - Command Queues](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial23_CommandQueues) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial23_CommandQueues/Animation_Small.gif) | This tutorial demonstrates how to use multiple command queues to perform rendering in parallel with copy and compute operations. |
| [24 - Variable Rate Shading](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial24_VRS) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial24_VRS/Animation_Small.gif) | This tutorial demonstrates how to use variable rate shading to reduce the pixel shading load. |
| [25 - Render State Packager](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial25_StatePackager) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial25_StatePackager/Screenshot.jpg) | This tutorial shows how to create and archive pipeline states with the render state packager off-line tool on the example of a simple path tracer. |
| [26 - Render State Cache](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial26_StateCache) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial26_StateCache/Screenshot.jpg) | This tutorial expands the path tracing technique implemented in previous tutorial and demonstrates how to use the render state cache to save pipeline states created at run time and load them when the application starts. |
| [27 - Post-Processing](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial27_PostProcessing) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial27_PostProcessing/Screenshot.jpg) | This tutorial demonstrates how to use post-processing effects from the DiligentFX module. |

<a name="samples"></a>
# [Samples](https://github.com/DiligentGraphics/DiligentSamples)

| Sample     | Screenshot  | Description          |
|------------|-------------|----------------------|
| [Atmosphere Sample](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/Atmosphere) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/Atmosphere/Animation_Small.gif) | This sample demonstrates how to integrate [Epipolar Light Scattering](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/EpipolarLightScattering) post-processing effect into an application to render physically-based atmosphere. |
| [GLFW Demo](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/GLFWDemo) | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/GLFWDemo/Animation_Small.gif) | This maze mini-game demonstrates how to use GLFW to create window and handle keyboard and mouse input. |
| [GLTF Viewer](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/GLTFViewer) | <img src="https://github.com/DiligentGraphics/DiligentFX/blob/master/PBR/screenshots/flight_helmet.jpg" width=240> | This sample demonstrates how to use the [Asset Loader](https://github.com/DiligentGraphics/DiligentTools/tree/master/AssetLoader) and [PBR Renderer](https://github.com/DiligentGraphics/DiligentFX/tree/master/PBR) to load and render GLTF models. |
| [USD Viewer](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/USDViewer) | <img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/USDViewer/Screenshot.jpg" width=240> | This sample demonstrates how to render USD files using [Hydrogent](https://github.com/DiligentGraphics/DiligentFX/tree/master/Hydrogent), an implementation of the Hydra rendering API in Diligent Engine. |
| [Shadows](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/Shadows) | <img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/Shadows/Screenshot.jpg" width=240> | This sample demonstrates how to use the [Shadowing component](https://github.com/DiligentGraphics/DiligentFX/tree/master/Components#shadows) to render high-quality shadows. |
| [Dear ImGui Demo](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/ImguiDemo) | <img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/ImguiDemo/Screenshot.png" width=240> | This sample demonstrates the integration of the engine with [dear imgui](https://github.com/ocornut/imgui) UI library. |
| [Nuklear Demo](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/NuklearDemo) | <img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/NuklearDemo/Screenshot.png" width=240> | This sample demonstrates the integration of the engine with [nuklear](https://github.com/vurtun/nuklear) UI library. |
| [Hello AR](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Android/HelloAR) | <img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/Android/HelloAR/Screenshot.png" width=240> | This sample demonstrates how to use Diligent Engine in a basic Android AR application. |
| [Asteroids](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Samples/Asteroids) |  <img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/Asteroids/Screenshot.png" width=240> | This sampple is a performance benchmark that renders 50,000 unique textured asteroids and allows comparing performance of different rendering modes. |
| [Unity Integration Demo](https://github.com/DiligentGraphics/DiligentSamples/tree/master/UnityPlugin) | <img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/UnityPlugin/GhostCubePlugin/Screenshot.png" width=240> | This project demonstrates integration of Diligent Engine with Unity. |


<a name="high_level_components"></a>
# High-Level Rendering Components

High-level rendering functionality is implemented by [DiligentFX module](https://github.com/DiligentGraphics/DiligentFX).
The following components are now available:

<a name="gltf_loader_and_renderer"></a>
* [GLTF2.0 Loader](https://github.com/DiligentGraphics/DiligentTools/tree/master/AssetLoader)
  and [Physically-based renderer with image-based lighting](https://github.com/DiligentGraphics/DiligentFX/tree/master/PBR).
  
|||
|-----------------|-----------------|
| ![](https://github.com/DiligentGraphics/DiligentFX/blob/master/PBR/screenshots/damaged_helmet.jpg) | ![](https://github.com/DiligentGraphics/DiligentFX/blob/master/PBR/screenshots/flight_helmet.jpg) |
| ![](https://github.com/DiligentGraphics/DiligentFX/blob/master/PBR/screenshots/mr_spheres.jpg)     | ![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/GLTFViewer/screenshots/cesium_man_large.gif)  |


* [Hydrogent](https://github.com/DiligentGraphics/DiligentFX/tree/master/Hydrogent), an implementation of the Hydra rendering API in Diligent Engine.
<img src="https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/USDViewer/Screenshot.jpg" width=400>

* [Shadows](https://github.com/DiligentGraphics/DiligentFX/tree/master/Components#shadows)
<img src="https://github.com/DiligentGraphics/DiligentFX/blob/master/Components/media/Powerplant-Shadows.jpg" width=400>

### Post-processing effects

* [Screen-Space Reflections](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/ScreenSpaceReflection)
<img src="https://github.com/DiligentGraphics/DiligentFX/blob/master/PostProcess/ScreenSpaceReflection/media/ssr-logo.jpg" width=400>

* [Screen-Space Ambient Occlusion](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/ScreenSpaceAmbientOcclusion)
<img src="https://github.com/DiligentGraphics/DiligentFX/blob/master/PostProcess/ScreenSpaceAmbientOcclusion/media/ssao-kitchen.jpg" width=400>

* [Depth of Field](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/DepthOfField)
<img src="https://github.com/DiligentGraphics/DiligentFX/blob/master/PostProcess/DepthOfField/media/depth_of_field.jpg" width=400>

* [Bloom](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/Bloom)
<img src="https://github.com/DiligentGraphics/DiligentFX/blob/master/PostProcess/Bloom/media/bloom.jpg" width=400>

* [Epipolar light scattering](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/EpipolarLightScattering)
<img src="https://github.com/DiligentGraphics/DiligentFX/blob/master/PostProcess/EpipolarLightScattering/media/LightScattering.png" width=400>

* [Temporal Anti-Aliasing](https://github.com/DiligentGraphics/DiligentFX/tree/master/PostProcess/TemporalAntiAliasing)

* [Tone mapping shader utilities](https://github.com/DiligentGraphics/DiligentFX/tree/master/Shaders/PostProcess/ToneMapping/public)


<a name="products"></a>
# Products using Diligent Engine

We would appreciate it if you could send us a link in case your product uses Diligent Engine.

* Large-scale terrain visualization system for pilot training simulators by [Elbit Systems](https://elbitsystems.com/)
  <img src="https://github.com/DiligentGraphics/DiligentEngine/blob/master/Media/DiligentTerrain.jpg" width=600>

* [LumenRT](https://www.bentley.com/software/bentley-lumenrt/): A Visualization and Reality Modeling Software by [Bentley Systems](https://www.bentley.com/)

* [Godus](https://apps.apple.com/gb/app/godus/id815181808): An award-winning sandbox game by [22cans](http://22cans.com/)  
  <img src="http://22cans.com/wp-content/uploads/2016/11/godus_header1-01.jpg" width=600>

* [Vrmac Graphics](https://github.com/Const-me/Vrmac): A cross-platform graphics library for .NET  
  <img src="https://github.com/Const-me/Vrmac/blob/master/screenshots/Linux/TigerFullHD-1.png" width=600>


<a name="disclaimer"></a>
## Disclaimer

Diligent Engine is an open project that may be freely used by everyone. We started it to empower the community
and help people achieve their goals. Sadly enough, not everyone's goals are worthy. Please don't associate us with
suspicious projects you may find on the Web that appear to be using Diligent Engine. We neither can possibly track
all such uses nor can we really do anything about them because our permissive license does not give us a lot of leverage. 


<a name="license"></a>
# License

See [Apache 2.0 license](License.txt).

Each module has some third-party dependencies, each of which may have independent licensing:

* [Core module](https://github.com/DiligentGraphics/DiligentCore#license)
* [Tools module](https://github.com/DiligentGraphics/DiligentTools#license)
* [Samples module](https://github.com/DiligentGraphics/DiligentSamples#license)

<a name="contributing"></a>
# Contributing

To contribute your code, submit a [Pull Request](https://github.com/DiligentGraphics/DiligentEngine/pulls) 
to this repository. **Diligent Engine** is licensed under the [Apache 2.0 license](License.txt) that guarantees 
that content in the **DiligentEngine** repository is free of Intellectual Property encumbrances.
In submitting any content to this repository,
[you license that content under the same terms](https://docs.github.com/en/free-pro-team@latest/github/site-policy/github-terms-of-service#6-contributions-under-repository-license),
and you agree that the content is free of any Intellectual Property claims and you have the right to license it under those terms. 

Diligent Engine uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to ensure
consistent source code style throughout the code base. The format is validated by CI
for each commit and pull request, and the build will fail if any code formatting issue is found. Please refer
to [this page](https://github.com/DiligentGraphics/DiligentCore/blob/master/doc/code_formatting.md) for instructions
on how to set up clang-format and automatic code formatting.

<a name="references"></a>
# References

[Coding Guidelines](https://github.com/DiligentGraphics/DiligentCore/blob/master/doc/CodingGuidelines.md)

[Performance Best Practices](https://github.com/DiligentGraphics/DiligentCore/blob/master/doc/PerformanceGuide.md)

[Code Formatting](https://github.com/DiligentGraphics/DiligentCore/blob/master/doc/code_formatting.md)


<a name="release_history"></a>
# Release History

See [Release History](ReleaseHistory.md)

------------------------------

[diligentgraphics.com](http://diligentgraphics.com)

[![Diligent Engine on Twitter](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/twitter.png)](https://twitter.com/diligentengine)
[![Diligent Engine on Facebook](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/facebook.png)](https://www.facebook.com/DiligentGraphics/)
