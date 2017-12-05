cmake_minimum_required (VERSION 3.3)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(DiligentEngine)

if(MSVC)
	#add_compile_options(/W4)
else()
	#add_compile_options(-W -Wall)
endif()

add_subdirectory(diligentcore)
add_subdirectory(diligenttools)
add_subdirectory(diligentsamples)