cmake_minimum_required (VERSION 3.6)

project(IncludeTest)

file(GLOB GRAPHICS_ENGINE_INC_TEST LIST_DIRECTORIES false src/DiligentCore/GraphicsEngine/*.cpp)
file(GLOB PLATFORMS_INC_TEST LIST_DIRECTORIES false src/DiligentCore/Platforms/*.cpp)
file(GLOB COMMON_INC_TEST LIST_DIRECTORIES false src/DiligentCore/Common/*.cpp)
file(GLOB GRAPHICS_TOOLS_INC_TEST LIST_DIRECTORIES false src/DiligentCore/GraphicsTools/*.cpp)
file(GLOB TEXTURE_LOADER_INC_TEST LIST_DIRECTORIES false src/DiligentCore/TextureLoader/*.cpp)
file(GLOB GRAPHICS_ACCESSORIES_INC_TEST LIST_DIRECTORIES false src/DiligentCore/GraphicsAccessories/*.cpp)
file(GLOB_RECURSE DILIGENT_FX_INC_TEST LIST_DIRECTORIES false src/DiligentFX/*.cpp)
add_library(IncludeTest ${GRAPHICS_ENGINE_INC_TEST} ${PLATFORMS_INC_TEST} ${COMMON_INC_TEST} ${GRAPHICS_TOOLS_INC_TEST} ${TEXTURE_LOADER_INC_TEST} ${GRAPHICS_ACCESSORIES_INC_TEST} ${DILIGENT_FX_INC_TEST})

if(D3D11_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_D3D11_INC_TEST LIST_DIRECTORIES false src/DiligentCore/GraphicsEngineD3D11/*.cpp)
    target_sources(IncludeTest PUBLIC ${GRAPHICS_ENGINE_D3D11_INC_TEST})
    source_group("src\\DiligentCore\\GraphicsEngineD3D11" FILES ${GRAPHICS_ENGINE_D3D11_INC_TEST})
endif()

if(D3D12_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_D3D12_INC_TEST LIST_DIRECTORIES false src/DiligentCore/GraphicsEngineD3D12/*.cpp)
    target_sources(IncludeTest PUBLIC ${GRAPHICS_ENGINE_D3D12_INC_TEST})
    source_group("src\\DiligentCore\\GraphicsEngineD3D12" FILES ${GRAPHICS_ENGINE_D3D12_INC_TEST})
endif()

if(D3D11_SUPPORTED OR D3D12_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_D3DBASE_INC_TEST LIST_DIRECTORIES false src/DiligentCore/GraphicsEngineD3DBase/*.cpp)
    target_sources(IncludeTest PUBLIC ${GRAPHICS_ENGINE_D3DBASE_INC_TEST})
    source_group("src\\DiligentCore\\GraphicsEngineD3DBase" FILES ${GRAPHICS_ENGINE_D3DBASE_INC_TEST})
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_GL_INC_TEST LIST_DIRECTORIES false src/DiligentCore/GraphicsEngineOpenGL/*.cpp)
    target_sources(IncludeTest PUBLIC ${GRAPHICS_ENGINE_GL_INC_TEST})
    source_group("src\\DiligentCore\\GraphicsEngineOpenGL" FILES ${GRAPHICS_ENGINE_GL_INC_TEST})
endif()

if(VULKAN_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_VK_INC_TEST LIST_DIRECTORIES false src/DiligentCore/GraphicsEngineVk/*.cpp)
    target_sources(IncludeTest PUBLIC ${GRAPHICS_ENGINE_VK_INC_TEST})
    source_group("src\\DiligentCore\\GraphicsEngineVk" FILES ${GRAPHICS_ENGINE_VK_INC_TEST})
endif()

target_include_directories(IncludeTest PRIVATE ../..)
target_link_libraries(IncludeTest PRIVATE Diligent-BuildSettings)
set_common_target_properties(IncludeTest)

if(MSVC)
    set_target_properties(IncludeTest PROPERTIES 
        STATIC_LIBRARY_FLAGS "/IGNORE:4221"
    )
endif()

source_group("src\\DiligentCore\\GraphicsEngine" FILES ${GRAPHICS_ENGINE_INC_TEST})
source_group("src\\DiligentCore\\Platforms" FILES ${PLATFORMS_INC_TEST})
source_group("src\\DiligentCore\\Common" FILES ${COMMON_INC_TEST})
source_group("src\\DiligentCore\\GraphicsTool" FILES ${GRAPHICS_TOOLS_INC_TEST})
source_group("src\\DiligentCore\\TextureLoader" FILES ${TEXTURE_LOADER_INC_TEST})
source_group("src\\DiligentCore\\GraphicsAccessories" FILES ${GRAPHICS_ACCESSORIES_INC_TEST})
source_group("src\\DiligentFX" FILES ${DILIGENT_FX_INC_TEST})

set_target_properties(IncludeTest PROPERTIES
    FOLDER Tests
)
