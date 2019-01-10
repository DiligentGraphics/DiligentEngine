#pragma once

// Standard base includes, defines that indicate our current platform, etc.

#include <stddef.h>


// Which platform we are on?
// UNITY_WIN - Windows (regular win32)
// UNITY_OSX - Mac OS X
// UNITY_LINUX - Linux
// UNITY_IPHONE - iOS
// UNITY_ANDROID - Android
// UNITY_METRO - WSA or UWP
// UNITY_WEBGL - WebGL
#if defined(UNITY_WIN) || defined(UNITY_METRO) || defined(UNITY_ANDROID) || defined(UNITY_LINUX) || defined(UNITY_WEBGL) || defined(UNITY_OSX) || defined(UNITY_IPHONE)
	// these are defined externally
#elif _MSC_VER
    #define UNITY_WIN 1
#elif defined(__APPLE__)
	#if defined(__arm__) || defined(__arm64__)
		#define UNITY_IPHONE 1
	#else
		#define UNITY_OSX 1
	#endif
#elif defined(__EMSCRIPTEN__)
	// this is already defined in Unity 5.6
	#define UNITY_WEBGL 1
#else
	#error "Unknown platform!"
#endif



// Which graphics device APIs we possibly support?
#if UNITY_METRO
	#define SUPPORT_D3D11 D3D11_SUPPORTED
	#if WINDOWS_UWP
		#define SUPPORT_D3D12 D3D12_SUPPORTED
	#endif
#elif UNITY_WIN
	#define SUPPORT_D3D11 D3D11_SUPPORTED
	#define SUPPORT_D3D12 D3D12_SUPPORTED
	#define SUPPORT_OPENGL_UNIFIED GL_SUPPORTED
	#define SUPPORT_OPENGL_CORE GL_SUPPORTED
#elif UNITY_IPHONE || UNITY_ANDROID || UNITY_WEBGL
	#define SUPPORT_OPENGL_UNIFIED GLES_SUPPORTED
	#define SUPPORT_OPENGL_ES GLES_SUPPORTED
#elif UNITY_OSX || UNITY_LINUX
	#define SUPPORT_OPENGL_LEGACY 1
	#define SUPPORT_OPENGL_UNIFIED GL_SUPPORTED
	#define SUPPORT_OPENGL_CORE GL_SUPPORTED
#endif

#if UNITY_IPHONE || UNITY_OSX
	#define SUPPORT_METAL METAL_SUPPORTED
#endif
