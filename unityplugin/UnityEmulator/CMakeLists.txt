cmake_minimum_required (VERSION 3.6)

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

	# Windows Runtime types cannot be included into static libraries
	# https://social.msdn.microsoft.com/Forums/en-US/269db513-64ef-4817-a025-43954f614eb3/lnk4264-why-are-static-libraries-not-recommended-when-authoring-windows-runtime-types?forum=winappswithnativecode
	# So as a workaround, we will include all source files into the target app project
	function(get_emulator_uwp_source UWP_SOURCE UWP_INCLUDE UWP_INCLUDE_DIR)
		get_target_property(EMULATOR_SOURCE_DIR UnityEmulator SOURCE_DIR)
		
		set(${UWP_SOURCE}
			${EMULATOR_SOURCE_DIR}/src/UWP/App.cpp
			${EMULATOR_SOURCE_DIR}/src/UWP/DeviceResources.cpp
			${EMULATOR_SOURCE_DIR}/src/UWP/UnityEmulatorAppMain.cpp
			PARENT_SCOPE
		)

		set(${UWP_INCLUDE}
			${EMULATOR_SOURCE_DIR}/src/UWP/App.h
			${EMULATOR_SOURCE_DIR}/src/UWP/DeviceResources.h
			${EMULATOR_SOURCE_DIR}/src/UWP/DirectXHelper.h
			${EMULATOR_SOURCE_DIR}/src/UWP/pch2.h
			${EMULATOR_SOURCE_DIR}/src/UWP/StepTimer.h
			${EMULATOR_SOURCE_DIR}/src/UWP/UnityEmulatorAppMain.h
			PARENT_SCOPE
		)

		set(${UWP_INCLUDE_DIR}
			${EMULATOR_SOURCE_DIR}/Src/UWP
			PARENT_SCOPE
		)
	endfunction(get_emulator_uwp_source)

elseif(PLATFORM_ANDROID)
	list(APPEND SOURCE src/Android/AndroidMainImpl.cpp)
elseif(PLATFORM_LINUX)
	list(APPEND SOURCE src/Linux/LinuxMain.cpp)
else()
	message(FATAL_ERROR "Unknown platform")
endif()


add_library(UnityEmulator STATIC ${SOURCE} ${INCLUDE})
set_common_target_properties(UnityEmulator)

target_include_directories(UnityEmulator
PRIVATE
	../GhostCubePlugin/PluginSource/src/Unity
PUBLIC
	include
)

if(MSVC)
	target_compile_options(UnityEmulator PRIVATE -DUNICODE)
endif()

target_link_libraries(UnityEmulator 
PRIVATE 
	BuildSettings 
PUBLIC
	Common 
	GraphicsEngine
	GraphicsTools
	TargetPlatform
)

if(GL_SUPPORTED)
	target_link_libraries(UnityEmulator PRIVATE glew-static) 
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
	target_link_libraries(UnityEmulator PUBLIC GraphicsEngineOpenGL-static)
endif()

if(D3D11_SUPPORTED)
	target_link_libraries(UnityEmulator PUBLIC GraphicsEngineD3DBase GraphicsEngineD3D11-static)
endif()

if(D3D12_SUPPORTED)
	target_link_libraries(UnityEmulator PUBLIC GraphicsEngineD3DBase GraphicsEngineD3D12-static)
endif()

if(PLATFORM_UNIVERSAL_WINDOWS)
	target_link_libraries(UnityEmulator PRIVATE dxguid.lib)
elseif(PLATFORM_ANDROID)
	target_link_libraries(UnityEmulator PRIVATE NDKHelper GLESv3 android PUBLIC NativeAppGlue)
	target_include_directories(UnityEmulator PRIVATE
		${ANDROID_NDK}/sources/android/cpufeatures
	)
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})

set_target_properties(UnityEmulator PROPERTIES
	FOLDER Unity
)
