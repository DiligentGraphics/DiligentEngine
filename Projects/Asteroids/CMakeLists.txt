cmake_minimum_required (VERSION 3.8)

project(Asteroids CXX)

set(SOURCE
	src/asteroids_d3d11.cpp
	src/asteroids_d3d12.cpp
	src/asteroids_DE.cpp
	src/camera.cpp
	src/DDSTextureLoader.cpp
	src/mesh.cpp
	src/simplexnoise1234.c
	src/simulation.cpp
	src/texture.cpp
	src/WinWrapper.cpp
)

set(INCLUDE
	src/asteroids_d3d11.h
	src/asteroids_d3d12.h
	src/asteroids_DE.h
	src/camera.h
	src/dds.h
	src/DDSTextureLoader.h
	src/descriptor.h
	src/mesh.h
	src/noise.h
	src/settings.h
	src/simplexnoise1234.h
	src/simulation.h
	src/subset_d3d12.h
	src/texture.h
	src/upload_heap.h
	src/util.h
)

set(SHADERS
	src/asteroid_ps.hlsl
	src/asteroid_ps_d3d11.hlsl
	src/asteroid_vs.hlsl
	src/font_ps.hlsl
	src/skybox_ps.hlsl
	src/skybox_vs.hlsl
	src/sprite_ps.hlsl
	src/sprite_vs.hlsl
)

set(GUI
	src/font.h
	src/gui.h
	src/intel_clear_bd_50_usascii.inl
	src/sprite.h
	src/stb_font_consolas_bold_50_usascii.inl
)

set(MEDIA
	DiligentD3D11.dds
	DiligentD3D12.dds
	DiligentGL.dds
	directx11.dds
	directx12.dds
	starbox_1024.dds
)

if(WIN32)
	add_executable(Asteroids WIN32 
		${SOURCE} 
		${INCLUDE} 
		${SHADERS}
		${GUI}
		${MEDIA}
		SDK/Include/d3dx12.h
		src/common_defines.h
		readme.md
	)
	set_target_properties(Asteroids PROPERTIES 
		LINK_FLAGS "/SUBSYSTEM:CONSOLE"
		VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	)
	copy_required_dlls(Asteroids)

else()
	message(FATAL_ERROR "Unsupported platform")
endif()


add_custom_target(AsteroidsShaders)

set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/Shaders)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

macro(COMPILE_SHADER FILE SHADER_MODEL)
	get_filename_component(FILE_NAME_WE ${FILE} NAME_WE)
	get_filename_component(FILE_NAME ${FILE} NAME)
	set(OUT_FILE ${SHADER_OUTPUT_DIR}/${FILE_NAME_WE}.h)

	add_custom_command(TARGET AsteroidsShaders 
						# Unfortunately it is not possible to set TARGET directly to GraphicsEngineD3D12-static/shared
						# because PRE_BUILD is only supported on Visual Studio 8 or later. For all other generators 
						# PRE_BUILD is treated as PRE_LINK.
						COMMAND ${FXC} /T ${SHADER_MODEL} ${FILE_NAME} /E ${FILE_NAME_WE} /O3 /Vn g_${FILE_NAME_WE} /Fh ${OUT_FILE} /nologo
						MAIN_DEPENDENCY ${FILE} # the primary input source file to the command
						WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
						COMMENT "Compiling shader ${FILE}"
						VERBATIM
	)
endmacro()

COMPILE_SHADER(src/asteroid_ps.hlsl ps_5_1)
COMPILE_SHADER(src/asteroid_ps_d3d11.hlsl ps_5_0)
COMPILE_SHADER(src/asteroid_vs.hlsl vs_5_0)
COMPILE_SHADER(src/font_ps.hlsl ps_5_0)
COMPILE_SHADER(src/skybox_ps.hlsl ps_5_0)
COMPILE_SHADER(src/skybox_vs.hlsl vs_5_0)
COMPILE_SHADER(src/sprite_ps.hlsl ps_5_0)
COMPILE_SHADER(src/sprite_vs.hlsl vs_5_0)

add_dependencies(Asteroids AsteroidsShaders)

target_compile_definitions(Asteroids PRIVATE NOMINMAX)

target_include_directories(Asteroids
PRIVATE
	Src
	SDK/Include
	${CMAKE_CURRENT_BINARY_DIR}/Shaders
)

target_link_libraries(Asteroids
PRIVATE
	BuildSettings
	GraphicsEngine
	GraphicsEngineD3D11-shared 
	GraphicsEngineD3D12-shared
	GraphicsEngineOpenGL-shared
	TargetPlatform
	TextureLoader
	GraphicsTools
	d3dcompiler.lib
	d3d12.lib
	d3d11.lib
	ninput.lib
	winmm.lib
	dxgi.lib
	shcore.lib
)

set_common_target_properties(Asteroids)

if(MSVC)
	set_header_file_only_property("${SHADERS}")

	# Disable MSVC-specific warnings
	# - w4201: nonstandard extension used: nameless struct/union
	# - w4324: structure was padded due to alignment specifier
	# - w4238: nonstandard extension used: class rvalue used as lvalue
	target_compile_options(Asteroids PRIVATE /wd4201 /wd4324 /wd4238)
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("shaders" FILES 
	${SHADERS}
	src/common_defines.h
)
source_group("SDK" FILES SDK/Include/d3dx12.h)
source_group("GUI" FILES ${GUI})
source_group("media" FILES ${MEDIA})


