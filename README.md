# DiligentEngine

Diligent Engine is a light-weight cross-platform abstraction layer between the application and the platform-specific graphics API. 
Its main goal is to take advantages of the next-generation APIs such as Direct3D12 and Vulkan, but at the same time provide support 
for older platforms via Direct3D11, OpenGL and OpenGLES. Diligent Engine exposes common front-end for all supported platforms and 
provides [interoperability with underlying native API](http://diligentgraphics.com/diligent-engine/native-api-interoperability/). 
It also supports [integration with Unity](http://diligentgraphics.com/diligent-engine/unity-integration/) and is designed to be used 
as a graphics subsystem in a standalone game engine, Unity native plugin or any other 3D application. It is distributed under 
[Apache 2.0 license](License.txt) and is free to use. The engine contains 
[shader source code converter](http://diligentgraphics.com/diligent-engine/shader-converter/) that allows shaders authored in HLSL to 
be translated to GLSL.

# Clonning the Repository

This is the master repository that contains three [submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules). To get the repository and all submodules, use the following command:

 git clone --recursive https://github.com/DiligentGraphics/DiligentEngine.git 
 
 Alternatively, you can get master repository fisrt, and then individually clone all submodules into the engine's root folder.

## Repository Structure

Master repository includes the following submodules:

* [Core](https://github.com/DiligentGraphics/DiligentCore) submodule provides basic engine functionality. It implements the engine API using 
  [Direct3D11](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineD3D11), [Direct3D12](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineD3D12), and
  [OpenGL/GLES](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineOpenGL). It also implements 
  [HLSL to GLSL source code converter](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/HLSL2GLSLConverterLib)
* [Samples](https://github.com/DiligentGraphics/DiligentSamples) submodule contains several simple graphics applications intended to demonstrate the usage of 
  the Diligent Engine API.
* [Tools](https://github.com/DiligentGraphics/DiligentTools) submodule contains [texture loading library](https://github.com/DiligentGraphics/DiligentTools/tree/master/TextureLoader) and 
  [Render Script](https://github.com/DiligentGraphics/DiligentTools/tree/master/RenderScript), a Lua-based run-time graphics resource managing system.

# Build Instructions

## Win32

To build the engine for Win32 platform, [Microsoft Visual Studio 2015](https://www.visualstudio.com/vs/community) or later is required. 
Any edition including Visual Studio Community is sufficient. Open [EngineAll.sln](build/Win32/EngineAll.sln) solution file located in 
[build/Win32](build/Win32) folder, choose the desired configuration and build it. Note that ARM platform is intended 
to build the engine for Android (see below).

### Build Details

Diligent engine is self-contained and does not have any external dependencies. Installing Visual Studio is all that is 
required to build the engine.

Tools module references core module and samples module references both core and tools modules. The modules must share 
the same parent directory, otherwise links in the project files will be broken.

Core module contains several property pages that define common build settings. The pages are located in 
diligentcore\Shared\Build subdirectory and are referenced by every project.

There is a build subdirectory in each project’s directory that contains Visual Studio project files.

GraphicsEngineD3D11, GraphicsEngineD3D12 and GraphicsEngineOpenGL projects can be built as either static or dynamic link library.

## Universal Windows Platform

As with the Windows Desktop, Microsoft Visual Studio 2015 or later is required to build the engine for Universal Windows 
Platform platform. Again, any edition including Visual Studio Community is sufficient. Navigate to [build/UWP](build/UWP) 
directory, open [EngineAll.sln](build/UWP/EngineAll.sln) solution file and build the solution for the desired configuration.

## Android

To build the engine for Android, you first need to set up your machine for Android development: 
download Android SDK (Android Studio is not required), Android NDK, Apache Ant and other required tools. 

There are two ways to build the engine for Android.
The first way is to download and install [Visual GDB](http://visualgdb.com/) plugin for Visual Studio, open Windows Desktop solution 
(located in [build/Win32](build/Win32]) folder), select ARM platform and build the solution as usual. Note that despite 
the name, you can build the application for both ARM and x86 Android platforms. You can then deploy and run your Android 
application directly from the IDE

It is important to rebuild the app project for the first time

To run the app from Visual Studio, go to Project Settings->Debugging and select Local Windows Debugger option in the 
Debugger to launch drop-down list. Then start the application

The second way to build for Android is to navigate to the build/Win32 folder of the project you want to build in command line and 
use ndk-build. For executable projects, there is also **android_build.bat** file that builds and runs the application on the device

Note that when building for Android, the engine root path must not contain white spaces.

# Building with CMake (Work in Progress)

Reworking Diligent Engine build system with CMake is in progress.

[CMake](https://cmake.org/) is a cross-platform build system generator. To start using cmake, download the [latest release](https://cmake.org/download/) (3.10 or later is recommended).

## Win32

To generate build files for Windows desktop platform, use either CMake GUI or command line tool. The build system uses standalone 
effect compiler, path to which needs to be specified via FXC variable. For example, to generate Visual Studio 2017 64-bit
solution and project files in cmk_build folder, navigate to the engine's root folder and run the following command:

cmake -D FXC="C:/Program Files (x86)/Windows Kits/10/bin/x86/fxc.exe" -H. -B./cmk_build -G "Visual Studio 15 2017 Win64"

**WARNING!** In current implementation, full path to cmake build folder **must not contain white spaces**. (If anybody knows a way
to add quotes to CMake's custom commands, please let me know!)

Open DiligentEngine.sln file in cmk_build folder, select the desired configuration and build the engine. By default, Asteroids
demo will be set up as startup project.

## Universal Windows Platform

Not yet supported

## Android

Not yet supported

# Samples

[Sample source code](https://github.com/DiligentGraphics/DiligentSamples)

## AntTweakBar Sample

This sample demonstrates how to use [AntTweakBar library](http://anttweakbar.sourceforge.net/doc) to create simple user interface. 
It can also be thought of as Diligent Engine’s “Hello World” example. 

![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/AntTweakBar/Screenshot.png)

## Atmosphere Sample

The sample demonstrates how Diligent Engine can be used to implement various rendering tasks: 
loading textures from files, using complex shaders, rendering to textures, using compute shaders 
and unordered access views, etc.

![](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Samples/Atmosphere/Screenshot.png)

# Projects

## Asteroids Performance Benchmark

This sample is designed to be a performance benchmark and is based on 
[this demo](https://software.intel.com/en-us/articles/asteroids-and-directx-12-performance-and-power-savings) developed by Intel. 
It renders 50,000 unique textured asteroids. Every asteroid is a combination of one of 1000 unique 
meshes and one of 10 unique textures. The sample uses original D3D11 and D3D12 native implementations, 
and adds implementation using Diligent Engine API to allow comparing performance of different rendering modes.

![](Projects/Asteroids/Screenshot.png)

## Unity Integration Demo

[This project](unityplugin) demonstrates integration of Diligent Engine with Unity

![](unityplugin/GhostCubePlugin/Screenshot.png)


# Version History

## v2.1

### New Features

#### Core

* Interoperability with native API
** Accessing internal objects and handles
** Createing diligent engine buffers/textures from native resources
** Attaching to existing D3D11/D3D12 device or GL context
** Resource state and command queue synchronization for D3D12
* Integraion with Unity
* Geometry shader support
* Tessellation support
* Performance optimizations

#### HLSL->GLSL converter
* Support for structured buffers
* HLSL->GLSL conversion is now a two-stage process:
** Creating conversion stream
** Creating GLSL source from the stream
* Geometry shader support
* Tessellation control and tessellation evaluation shader support
* Support for non-void shader functions
* Allowing structs as input parameters for shader functions


## v2.0 (alpha)

Alpha release of Diligent Engine 2.0. The engine has been updated to take advantages of Direct3D12:

* Pipeline State Object encompasses all coarse-grain state objects like Depth-Stencil State, Blend State, Rasterizer State, shader states etc.
* New shader resource binding model implemented to leverage Direct3D12

* OpenGL and Direct3D11 backends
* Alpha release is only available on Windows platform
* Direct3D11 backend is very thoroughly optimized and has very low overhead compared to native D3D11 implementation
* Direct3D12 implementation is preliminary and not yet optimized

### v1.0.0

Initial release

# License

Licensed under the [Apache License, Version 2.0](License.txt)

**Copyright 2015-2017 Egor Yusov**

[Diligent Graphics](http://diligentgraphics.com)
