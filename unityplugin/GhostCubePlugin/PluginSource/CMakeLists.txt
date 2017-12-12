cmake_minimum_required (VERSION 3.3)

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

if(PLATFORM_WIN32 OR PLATFORM_UNVIRSAL_WINDOWS)
	if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
	   set(ARCH 64)
	else("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
	   set(ARCH 32)
	endif("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
endif()

set(DLL_REL_SUFFIX _${ARCH}r)
set(DLL_DBG_SUFFIX _${ARCH}d)

set_target_properties(GhostCubePlugin-shared PROPERTIES
	OUTPUT_NAME_DEBUG GhostCubePlugin${DLL_DBG_SUFFIX}
	OUTPUT_NAME_RELEASE GhostCubePlugin${DLL_REL_SUFFIX}
	OUTPUT_NAME_RELWITHDEBINFO GhostCubePlugin${DLL_REL_SUFFIX}
	OUTPUT_NAME_MINSIZEREL GhostCubePlugin${DLL_REL_SUFFIX}
)

if(PLATFORM_WIN32)
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

if(MSVC)
	# Disable MSVC-specific warnings
	# - w4201: nonstandard extension used: nameless struct/unio
	target_compile_options(GhostCubePlugin-shared PRIVATE /wd4201)
	# Enable link-time code generation for release builds (I was not able to 
	# find any way to set these settings through interface library BuildSettings)
	set_target_properties(GhostCubePlugin-shared PROPERTIES
		LINK_FLAGS_RELEASE /LTCG
		LINK_FLAGS_MINSIZEREL /LTCG
		LINK_FLAGS_RELWITHDEBINFO /LTCG
	)
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("include\\Unity" FILES ${UNITY_INTERFACES})

set_target_properties(GhostCubePlugin-shared PROPERTIES
	FOLDER Unity
)
