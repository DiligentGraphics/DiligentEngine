cmake_minimum_required (VERSION 3.6)

set(DLL_REL_SUFFIX _${ARCH}r)
set(DLL_DBG_SUFFIX _${ARCH}d)

add_library(Diligent-GraphicsEngineInterface INTERFACE)
target_include_directories(Diligent-GraphicsEngineInterface
INTERFACE
    GraphicsEngine/interface
)
target_link_libraries(Diligent-GraphicsEngineInterface
INTERFACE
    Diligent-Primitives
)

add_subdirectory(GraphicsAccessories)
add_subdirectory(GraphicsEngine)
add_subdirectory(ShaderTools)

if(D3D12_SUPPORTED OR VULKAN_SUPPORTED)
    add_subdirectory(GraphicsEngineNextGenBase)
endif()

if(D3D11_SUPPORTED OR D3D12_SUPPORTED)
    add_subdirectory(GraphicsEngineD3DBase)
endif()

if(D3D11_SUPPORTED)
    add_subdirectory(GraphicsEngineD3D11)
endif()

if(D3D12_SUPPORTED)
    add_subdirectory(GraphicsEngineD3D12)
endif()

if(VULKAN_SUPPORTED)
    add_subdirectory(GraphicsEngineVulkan)
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED OR VULKAN_SUPPORTED)
    add_subdirectory(HLSL2GLSLConverterLib)
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    add_subdirectory(GraphicsEngineOpenGL)
endif()

add_subdirectory(GraphicsTools)