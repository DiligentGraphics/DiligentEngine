cmake_minimum_required (VERSION 3.6)

project(IncludeTest)

file(GLOB GRAPHICS_ENGINE_INC_TEST LIST_DIRECTORIES false src/GraphicsEngine/*.cpp)
file(GLOB PLATFORMS_INC_TEST LIST_DIRECTORIES false src/Platforms/*.cpp)
file(GLOB COMMON_INC_TEST LIST_DIRECTORIES false src/Common/*.cpp)
file(GLOB GRAPHICS_TOOLS_INC_TEST LIST_DIRECTORIES false src/GraphicsTools/*.cpp)
file(GLOB TEXTURE_LOADER_INC_TEST LIST_DIRECTORIES false src/TextureLoader/*.cpp)
file(GLOB GRAPHICS_ACCESSORIES_INC_TEST LIST_DIRECTORIES false src/GraphicsAccessories/*.cpp)
add_library(IncludeTest ${GRAPHICS_ENGINE_INC_TEST} ${PLATFORMS_INC_TEST} ${COMMON_INC_TEST} ${GRAPHICS_TOOLS_INC_TEST} ${TEXTURE_LOADER_INC_TEST} ${GRAPHICS_ACCESSORIES_INC_TEST})

if(D3D11_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_D3D11_INC_TEST LIST_DIRECTORIES false src/GraphicsEngineD3D11/*.cpp)
    target_sources(IncludeTest PRIVATE ${GRAPHICS_ENGINE_D3D11_INC_TEST})
    source_group("src\\GraphicsEngineD3D11" FILES ${GRAPHICS_ENGINE_D3D11_INC_TEST})
endif()

if(D3D12_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_D3D12_INC_TEST LIST_DIRECTORIES false src/GraphicsEngineD3D12/*.cpp)
    target_sources(IncludeTest PRIVATE ${GRAPHICS_ENGINE_D3D12_INC_TEST})
    source_group("src\\GraphicsEngineD3D12" FILES ${GRAPHICS_ENGINE_D3D12_INC_TEST})
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_GL_INC_TEST LIST_DIRECTORIES false src/GraphicsEngineOpenGL/*.cpp)
    target_sources(IncludeTest PRIVATE ${GRAPHICS_ENGINE_GL_INC_TEST})
    source_group("src\\GraphicsEngineOpenGL" FILES ${GRAPHICS_ENGINE_GL_INC_TEST})
endif()

target_include_directories(IncludeTest PRIVATE ../..)
target_link_libraries(IncludeTest PRIVATE BuildSettings)
set_common_target_properties(IncludeTest)

if(MSVC)
    set_target_properties(IncludeTest PROPERTIES 
        STATIC_LIBRARY_FLAGS "/IGNORE:4221"
    )
endif()

source_group("src\\GraphicsEngine" FILES ${GRAPHICS_ENGINE_INC_TEST})
source_group("src\\Platforms" FILES ${PLATFORMS_INC_TEST})
source_group("src\\Common" FILES ${COMMON_INC_TEST})
source_group("src\\GraphicsTool" FILES ${GRAPHICS_TOOLS_INC_TEST})
source_group("src\\TextureLoader" FILES ${TEXTURE_LOADER_INC_TEST})
source_group("src\\GraphicsAccessories" FILES ${GRAPHICS_ACCESSORIES_INC_TEST})


set_target_properties(IncludeTest PROPERTIES
    FOLDER Tests
)
