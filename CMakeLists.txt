cmake_minimum_required (VERSION 3.3)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(DiligentEngine)

add_subdirectory(diligentcore)
add_subdirectory(diligenttools)
add_subdirectory(diligentsamples)
add_subdirectory(projects)
add_subdirectory(unityplugin)

if(MSVC)
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Asteroids)
endif()
