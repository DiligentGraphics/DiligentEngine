cmake_minimum_required (VERSION 3.3)

project(UnityEmulator CXX)

set(SOURCE 
	src/UnityGraphicsEmulator.cpp
)

set(INCLUDE
	include/DiligentGraphicsAdapter.h
	include/ResourceStateTransitionHandler.h
	include/UnityGraphicsEmulator.h
	include/UnitySceneBase.h
)

if(D3D11_SUPPORTED)
	list(APPEND SOURCE src/DiligentGraphicsAdapterD3D11.cpp)
	list(APPEND SOURCE src/UnityGraphicsD3D11Emulator.cpp)
	list(APPEND SOURCE src/UnityGraphicsD3D11Impl.h)
	list(APPEND INCLUDE include/DiligentGraphicsAdapterD3D11.h)
	list(APPEND INCLUDE include/UnityGraphicsD3D11Emulator.h)
endif()

if(D3D12_SUPPORTED)
	list(APPEND SOURCE src/DiligentGraphicsAdapterD3D12.cpp)
	list(APPEND SOURCE src/UnityGraphicsD3D12Emulator.cpp)
	list(APPEND SOURCE src/UnityGraphicsD3D12Impl.h)
	
	list(APPEND INCLUDE include/DiligentGraphicsAdapterD3D12.h)
	list(APPEND INCLUDE include/UnityGraphicsD3D12Emulator.h)
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
	list(APPEND SOURCE src/UnityGraphicsGLCoreES_Emulator.cpp)
	list(APPEND SOURCE src/DiligentGraphicsAdapterGL.cpp)

	list(APPEND INCLUDE include/UnityGraphicsGLCoreES_Emulator.h)
	list(APPEND INCLUDE include/DiligentGraphicsAdapterGL.h)
endif()

if(GL_SUPPORTED)
	list(APPEND SOURCE src/UnityGraphicsGLCore_Impl.cpp)
	list(APPEND INCLUDE src/UnityGraphicsGLCore_Impl.h)
endif()

if(GLES_SUPPORTED)
	list(APPEND SOURCE src/Android/UnityGraphicsGLESAndroid_Impl.cpp)
	list(APPEND INCLUDE src/Android/UnityGraphicsGLESAndroid_Impl.h)
endif()

if(PLATFORM_WIN32)
	list(APPEND SOURCE src/Windows/WinMain.cpp)
elseif(PLATFORM_UNIVERSAL_WINDOWS)
	list(APPEND SOURCE src/UWP/App.cpp)
	list(APPEND INCLUDE src/UWP/DeviceResources.cpp)
	list(APPEND INCLUDE src/UWP/UnityEmulatorAppMain.cpp)

	list(APPEND INCLUDE src/UWP/App.h)
	list(APPEND INCLUDE src/UWP/DeviceResources.h)
	list(APPEND INCLUDE src/UWP/DirectXHelper.h)
	list(APPEND INCLUDE src/UWP/pch2.h)
	list(APPEND INCLUDE src/UWP/StepTimer.h)
	list(APPEND INCLUDE src/UWP/UnityEmulatorAppMain.h)
elseif(PLATFORM_ANDROID)
	list(APPEND SOURCE src/Android/AndroidMainImpl.cpp)
else()
	message(FATAL_ERROR "Unknown platform")
endif()


add_library(UnityEmulator STATIC ${SOURCE} ${INCLUDE})

target_include_directories(UnityEmulator
PRIVATE
	../GhostCubePlugin/PluginSource/src/Unity
PUBLIC
	include
)

if(MSVC)
	target_compile_options(UnityEmulator PRIVATE -DUNICODE)

	# Enable link-time code generation for release builds (I was not able to 
	# find any way to set these settings through interface library BuildSettings)
	set_target_properties(UnityEmulator PROPERTIES
		STATIC_LIBRARY_FLAGS_RELEASE /LTCG
		STATIC_LIBRARY_FLAGS_MINSIZEREL /LTCG
		STATIC_LIBRARY_FLAGS_RELWITHDEBINFO /LTCG
	)
endif()

target_link_libraries(UnityEmulator 
PRIVATE 
	BuildSettings 
	glew-static
PUBLIC
	Common 
	GraphicsEngine
	GraphicsEngineOpenGL-static
	GraphicsTools
	TargetPlatform
)

if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS)
	target_link_libraries(UnityEmulator 
	PUBLIC
		GraphicsEngineD3DBase
		GraphicsEngineD3D11-static
		GraphicsEngineD3D12-static
)		
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})

set_target_properties(UnityEmulator PROPERTIES
	FOLDER Unity
)
