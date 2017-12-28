cmake_minimum_required (VERSION 3.6)

project(GhostCubePlugin CXX)

set(SOURCE
	src/RenderAPI.cpp
	src/RenderingPlugin.cpp
	src/SamplePlugin.cpp
)

set(INCLUDE
	src/PlatformBase.h
	src/RenderAPI.h
	src/SamplePlugin.h
)

if(D3D11_SUPPORTED)
	list(APPEND SOURCE src/RenderAPI_D3D11.cpp)
endif()

if(D3D12_SUPPORTED)
	list(APPEND SOURCE src/RenderAPI_D3D12.cpp)
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
	list(APPEND SOURCE src/RenderAPI_OpenGLCoreES.cpp)
endif()

set(UNITY_INTERFACES
	src/Unity/IUnityGraphics.h
	src/Unity/IUnityGraphicsD3D9.h
	src/Unity/IUnityGraphicsD3D11.h
	src/Unity/IUnityGraphicsD3D12.h
	src/Unity/IUnityGraphicsMetal.h
	src/Unity/IUnityInterface.h
)

add_library(GhostCubePlugin-shared SHARED 
	${SOURCE} ${INCLUDE} ${UNITY_INTERFACES} 
)

set(DLL_REL_SUFFIX _${ARCH}r)
set(DLL_DBG_SUFFIX _${ARCH}d)

set_target_properties(GhostCubePlugin-shared PROPERTIES
	OUTPUT_NAME_DEBUG GhostCubePlugin${DLL_DBG_SUFFIX}
	OUTPUT_NAME_RELEASE GhostCubePlugin${DLL_REL_SUFFIX}
	OUTPUT_NAME_RELWITHDEBINFO GhostCubePlugin${DLL_REL_SUFFIX}
	OUTPUT_NAME_MINSIZEREL GhostCubePlugin${DLL_REL_SUFFIX}
)

if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS)
	target_sources(GhostCubePlugin-shared 
	PRIVATE	
		src/RenderingPlugin.def
	)
	source_group("src" FILES src/RenderingPlugin.def)
endif()

target_link_libraries(GhostCubePlugin-shared
PRIVATE
	BuildSettings
	TargetPlatform
	Common
	GraphicsAccessories
	GraphicsTools
)

if(D3D11_SUPPORTED)
	target_link_libraries(GhostCubePlugin-shared PRIVATE GraphicsEngineD3D11-static)
endif()

if(D3D12_SUPPORTED)
	target_link_libraries(GhostCubePlugin-shared PRIVATE GraphicsEngineD3D12-static)
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
	target_link_libraries(GhostCubePlugin-shared PRIVATE GraphicsEngineOpenGL-static)
endif()

set_common_target_properties(GhostCubePlugin-shared)

if(MSVC)
	# Disable MSVC-specific warnings
	# - w4201: nonstandard extension used: nameless struct/unio
	target_compile_options(GhostCubePlugin-shared PRIVATE /wd4201)
	if(PLATFORM_UNIVERSAL_WINDOWS)
		target_compile_definitions(GhostCubePlugin-shared PRIVATE WINDOWS_UWP UNITY_METRO)
	endif()
endif()

if(PLATFORM_ANDROID)
	target_compile_definitions(GhostCubePlugin-shared PRIVATE UNITY_ANDROID)
	set_target_properties(GhostCubePlugin-shared PROPERTIES CXX_VISIBILITY_PRESET hidden) # -fvisibility=hidden
elseif(PLATFORM_LINUX)
	target_compile_definitions(GhostCubePlugin-shared PRIVATE UNITY_LINUX)
	set_target_properties(GhostCubePlugin-shared PROPERTIES CXX_VISIBILITY_PRESET hidden) # -fvisibility=hidden
endif()

set(UNITY_PLUGIN_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../UnityProject/Assets/Plugins)
if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS)
	if(PLATFORM_WIN32)
		if(ARCH EQUAL 64)
			set(UNITY_PLUGIN_PATH ${UNITY_PLUGIN_PATH}/x86_64)
		else()
			set(UNITY_PLUGIN_PATH ${UNITY_PLUGIN_PATH}/x86)
		endif()
	elseif(PLATFORM_UNIVERSAL_WINDOWS)
		if(ARCH EQUAL 64)
			set(UNITY_PLUGIN_PATH ${UNITY_PLUGIN_PATH}/Metro/UWP/x64)
		else()
			set(UNITY_PLUGIN_PATH ${UNITY_PLUGIN_PATH}/Metro/UWP/x86)
		endif()
	endif()
	set(UNITY_PLUGIN_PATH ${UNITY_PLUGIN_PATH}/GhostCubePlugin.dll)
elseif(PLATFORM_ANDROID)
	set(UNITY_PLUGIN_PATH ${UNITY_PLUGIN_PATH}/Android/libGhostCubePlugin.so)
elseif(PLATFORM_LINUX)
	set(UNITY_PLUGIN_PATH ${UNITY_PLUGIN_PATH}/libGhostCubePlugin.so)
else()
	message(FATAL_ERROR "Unsupported platform")
endif()

add_custom_command(TARGET GhostCubePlugin-shared POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"\"$<TARGET_FILE:GhostCubePlugin-shared>\""
		"${UNITY_PLUGIN_PATH}"
)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("include\\Unity" FILES ${UNITY_INTERFACES})

set_target_properties(GhostCubePlugin-shared PROPERTIES
	FOLDER Unity
)
