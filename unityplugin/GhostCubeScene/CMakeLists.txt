cmake_minimum_required (VERSION 3.8)

project(GhostCubeScene CXX)

set(SOURCE
	src/GhostCubeScene.cpp
)

set(INCLUDE
	src/GhostCubeScene.h
)

if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS)
	list(APPEND SOURCE src/GhostCubeSceneResTrsnHelper.cpp)
	list(APPEND INCLUDE src/GhostCubeSceneResTrsnHelper.h)
elseif(PLATFORM_ANDROID)
	list(APPEND SOURCE src/Android/AndroidMain.cpp)
endif()

if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS)

	add_executable(GhostCubeScene WIN32 ${SOURCE} ${INCLUDE})
	
	if(PLATFORM_WIN32)
		set_target_properties(GhostCubeScene PROPERTIES 
			VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/assets"
		)
	
		add_custom_command(TARGET GhostCubeScene POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:GhostCubePlugin-shared>\""
				"\"$<TARGET_FILE_DIR:GhostCubeScene>\"")
	endif()

#elseif(APPLE)
	#add_executable(GhostCubeScene MACOSX_BUNDLE Main.cpp)
#elseif(UNIX)
	#add_executable(GhostCubeScene Main.cpp)
else()
	message(FATAL_ERROR "Unknown platform")
endif()

if(PLATFORM_UNIVERSAL_WINDOWS)
	get_emulator_uwp_source(UWP_SOURCE UWP_INCLUDE UWP_INCLUDE_DIR)
	source_group("UWP\\src" FILES ${UWP_SOURCE})
	source_group("UWP\\include" FILES ${UWP_INCLUDE})
	target_include_directories(GhostCubeScene PRIVATE ${UWP_INCLUDE_DIR})
	target_sources(GhostCubeScene PRIVATE ${UWP_SOURCE} ${UWP_INCLUDE})
endif()


target_include_directories(GhostCubeScene
PRIVATE
	../GhostCubePlugin/PluginSource/src/Unity
)

target_link_libraries(GhostCubeScene
PRIVATE
	BuildSettings
	UnityEmulator
	TargetPlatform
)
set_common_target_properties(GhostCubeScene)

if(MSVC)
	# Disable MSVC-specific warnings
	# - w4201: nonstandard extension used: nameless struct/union
	target_compile_options(GhostCubeScene PRIVATE /wd4201)
	
	if(PLATFORM_UNIVERSAL_WINDOWS)
		# Consume the windows runtime extensions
		target_compile_options(GhostCubeScene PRIVATE /ZW)
	endif()
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})

set_target_properties(GhostCubeScene PROPERTIES
	FOLDER Unity
)
