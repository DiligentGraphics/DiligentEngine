cmake_minimum_required (VERSION 3.6)

project(TownRunner CXX)

set(SOURCE
    src/Main.cpp
    src/Win32.cpp
    src/Cube.cpp
)

set(INCLUDE
    src/Win32.h
    src/Cube.h
    src/Sample.h
)

set(SHADERS
    assets/cube_inst.vsh
    assets/cube_inst.psh
)

add_executable(TownRunner WIN32 ${SOURCE} ${INCLUDE} ${SHADERS})
target_include_directories(TownRunner PRIVATE "../../DiligentCore")

target_compile_definitions(TownRunner 
PRIVATE 
    UNICODE
    D3D11_SUPPORTED=$<BOOL:${D3D11_SUPPORTED}>
    D3D12_SUPPORTED=$<BOOL:${D3D12_SUPPORTED}>
    GL_SUPPORTED=$<BOOL:${GL_SUPPORTED}>
    VULKAN_SUPPORTED=$<BOOL:${VULKAN_SUPPORTED}>
)

set_target_properties(TownRunner PROPERTIES
    FOLDER Projects
)

target_compile_definitions(TownRunner PRIVATE ENGINE_DLL=1)
target_link_libraries(TownRunner PRIVATE Diligent-BuildSettings)
target_link_libraries(TownRunner PRIVATE Diligent-TextureLoader)

if(D3D11_SUPPORTED)
    target_link_libraries(TownRunner PRIVATE Diligent-GraphicsEngineD3D11-shared)
endif()
if(D3D12_SUPPORTED)
    target_link_libraries(TownRunner PRIVATE Diligent-GraphicsEngineD3D12-shared)
endif()
if(GL_SUPPORTED)
    target_link_libraries(TownRunner PRIVATE Diligent-GraphicsEngineOpenGL-shared)
endif()
if(VULKAN_SUPPORTED)
    target_link_libraries(TownRunner PRIVATE Diligent-GraphicsEngineVk-shared)
endif()
copy_required_dlls(TownRunner)
