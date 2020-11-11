cmake_minimum_required (VERSION 3.6)

project(DiligentCore-IncludeTest)

file(GLOB_RECURSE SOURCE LIST_DIRECTORIES false *.cpp *.c)

if(NOT D3D11_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_D3D11_INC_TEST LIST_DIRECTORIES false GraphicsEngineD3D11/*.cpp GraphicsEngineD3D11/*.c)
    list(REMOVE_ITEM SOURCE ${GRAPHICS_ENGINE_D3D11_INC_TEST})
endif()

if(NOT D3D12_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_D3D12_INC_TEST LIST_DIRECTORIES false GraphicsEngineD3D12/*.cpp GraphicsEngineD3D12/*.c)
    list(REMOVE_ITEM SOURCE ${GRAPHICS_ENGINE_D3D12_INC_TEST})
endif()

if(NOT D3D11_SUPPORTED AND NOT D3D12_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_D3DBASE_INC_TEST LIST_DIRECTORIES false GraphicsEngineD3DBase/*.cpp GraphicsEngineD3DBase/*.c)
    list(REMOVE_ITEM SOURCE ${GRAPHICS_ENGINE_D3DBASE_INC_TEST})
endif()

if(NOT GL_SUPPORTED AND NOT GLES_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_GL_INC_TEST LIST_DIRECTORIES false GraphicsEngineOpenGL/*.cpp GraphicsEngineOpenGL/*.c)
    list(REMOVE_ITEM SOURCE ${GRAPHICS_ENGINE_GL_INC_TEST})
endif()

if(NOT VULKAN_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_VK_INC_TEST LIST_DIRECTORIES false GraphicsEngineVk/*.cpp GraphicsEngineVk/*.c)
    list(REMOVE_ITEM SOURCE ${GRAPHICS_ENGINE_VK_INC_TEST})
endif()

add_library(DiligentCore-IncludeTest ${SOURCE})

target_include_directories(DiligentCore-IncludeTest PRIVATE ../../..)
target_link_libraries(DiligentCore-IncludeTest PRIVATE Diligent-BuildSettings)
set_common_target_properties(DiligentCore-IncludeTest)

if(MSVC)
    set_target_properties(DiligentCore-IncludeTest PROPERTIES 
        STATIC_LIBRARY_FLAGS "/IGNORE:4221"
    )

    # Treat warnings as errors to catch c compilation issues
    target_compile_options(DiligentCore-IncludeTest PRIVATE "/WX") 
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE})

set_target_properties(DiligentCore-IncludeTest PROPERTIES
    FOLDER "DiligentCore/Tests"
)
