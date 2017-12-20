cmake_minimum_required (VERSION 3.6)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(DiligentEngine)

add_subdirectory(DiligentCore)
add_subdirectory(DiligentTools)
		if(NOT PLATFORM_LINUX)  # !!!! TEMPORARY  !!!!
add_subdirectory(DiligentSamples)
add_subdirectory(Projects)
add_subdirectory(unityplugin)
		endif()					# !!!! TEMPORARY  !!!!

if(PLATFORM_WIN32)
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Asteroids)
elseif(PLATFORM_UNIVERSAL_WINDOWS)
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT AtmosphereSample)
endif()
