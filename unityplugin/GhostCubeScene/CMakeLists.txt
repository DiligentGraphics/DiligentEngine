cmake_minimum_required (VERSION 3.3)

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

if(PLATFORM_WIN32)
	add_executable(GhostCubeScene WIN32 ${SOURCE} ${INCLUDE})
	set_target_properties(GhostCubeScene PROPERTIES 
		VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/assets"
	)
	
	add_custom_command(TARGET GhostCubeScene POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"\"$<TARGET_FILE:GhostCubePlugin-shared>\""
			"\"$<TARGET_FILE_DIR:GhostCubeScene>\"")

#elseif(APPLE)
	#add_executable(GhostCubeScene MACOSX_BUNDLE Main.cpp)
#elseif(UNIX)
	#add_executable(GhostCubeScene Main.cpp)
else()
	message(FATAL_ERROR "Unknown platform")
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

if(MSVC)
	# Disable MSVC-specific warnings
	# - w4201: nonstandard extension used: nameless struct/unio
	target_compile_options(GhostCubeScene PRIVATE /wd4201)
	# Enable link-time code generation for release builds (I was not able to 
	# find any way to set these settings through interface library BuildSettings)
	set_target_properties(GhostCubeScene PROPERTIES
		LINK_FLAGS_RELEASE /LTCG
		LINK_FLAGS_MINSIZEREL /LTCG
		LINK_FLAGS_RELWITHDEBINFO /LTCG
	)
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})

set_target_properties(GhostCubeScene PROPERTIES
	FOLDER Unity
)
