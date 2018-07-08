cmake_minimum_required (VERSION 3.10)

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

set(PIXEL_SHADERS
    src/asteroid_ps.hlsl
    src/asteroid_ps_d3d11.hlsl
    src/font_ps.hlsl
    src/skybox_ps.hlsl
    src/sprite_ps.hlsl
)

set(VERTEX_SHADERS
    src/asteroid_vs.hlsl
    src/skybox_vs.hlsl
    src/sprite_vs.hlsl
)

set(SHADERS ${PIXEL_SHADERS} ${VERTEX_SHADERS})

set_source_files_properties(${SHADERS} PROPERTIES
    VS_SHADER_ENTRYPOINT "%(Filename)"
    VS_SHADER_MODEL 5.0
    VS_SHADER_VARIABLE_NAME "g_%(Filename)"
    VS_SHADER_OUTPUT_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/CompiledShaders/%(Filename).h"
)
set_source_files_properties(${PIXEL_SHADERS} PROPERTIES VS_SHADER_TYPE Pixel)
set_source_files_properties(${VERTEX_SHADERS} PROPERTIES VS_SHADER_TYPE Vertex)
set_source_files_properties(src/asteroid_ps.hlsl PROPERTIES VS_SHADER_MODEL 5.1)

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

target_include_directories(Asteroids
PRIVATE
    Src
    SDK/Include
    ${CMAKE_CURRENT_BINARY_DIR}/CompiledShaders
)

target_link_libraries(Asteroids
PRIVATE
    BuildSettings
    GraphicsEngineD3D11-shared 
    GraphicsEngineD3D12-shared
    GraphicsEngineOpenGL-shared
    TargetPlatform
    TextureLoader
    Common
    GraphicsTools
    d3dcompiler.lib
    d3d12.lib
    d3d11.lib
    ninput.lib
    winmm.lib
    dxgi.lib
    shcore.lib
)

if(VULKAN_SUPPORTED)
    target_link_libraries(Asteroids
    PRIVATE
        GraphicsEngineVk-shared
    )
endif()

set_common_target_properties(Asteroids)

if(MSVC)
    target_compile_definitions(Asteroids PRIVATE NOMINMAX)
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
