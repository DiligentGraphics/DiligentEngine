cmake_minimum_required (VERSION 3.10)

project(Diligent-GraphicsEngineD3D11 CXX)


set(INCLUDE 
    include/BufferD3D11Impl.hpp
    include/BufferViewD3D11Impl.hpp
    include/CommandListD3D11Impl.hpp
    include/D3D11TypeConversions.hpp
    include/D3D11TypeDefinitions.h
    include/DeviceContextD3D11Impl.hpp
    include/DisjointQueryPool.hpp
    include/EngineD3D11Defines.h
    include/pch.h
	include/FenceD3D11Impl.hpp
    include/FramebufferD3D11Impl.hpp
    include/PipelineStateD3D11Impl.hpp
    include/QueryD3D11Impl.hpp
    include/RenderDeviceD3D11Impl.hpp
    include/RenderPassD3D11Impl.hpp
    include/SamplerD3D11Impl.hpp
    include/ShaderD3D11Impl.hpp
    include/ShaderResourceBindingD3D11Impl.hpp
    include/ShaderResourceCacheD3D11.hpp
    include/ShaderResourceLayoutD3D11.hpp
    include/ShaderResourcesD3D11.hpp
    include/SwapChainD3D11Impl.hpp
    include/targetver.h
    include/Texture1D_D3D11.hpp
    include/Texture2D_D3D11.hpp
    include/Texture3D_D3D11.hpp
    include/TextureBaseD3D11.hpp
    include/TextureViewD3D11Impl.hpp
)

set(INTERFACE 
    interface/BufferD3D11.h
    interface/BufferViewD3D11.h
    interface/DeviceContextD3D11.h
    interface/EngineFactoryD3D11.h
    interface/FenceD3D11.h
    interface/PipelineStateD3D11.h
    interface/QueryD3D11.h
    interface/RenderDeviceD3D11.h
    interface/SamplerD3D11.h
    interface/ShaderD3D11.h
    interface/ShaderResourceBindingD3D11.h
    interface/SwapChainD3D11.h
    interface/TextureD3D11.h
    interface/TextureViewD3D11.h
)


set(SRC 
    src/BufferD3D11Impl.cpp
    src/BufferViewD3D11Impl.cpp
    src/CommandListD3D11Impl.cpp
    src/D3D11TypeConversions.cpp
    src/DeviceContextD3D11Impl.cpp
    src/EngineFactoryD3D11.cpp
    src/FenceD3D11Impl.cpp
    src/FramebufferD3D11Impl.cpp
    src/GUIDDef.cpp
    src/PipelineStateD3D11Impl.cpp
    src/QueryD3D11Impl.cpp
    src/RenderDeviceD3D11Impl.cpp
    src/RenderPassD3D11Impl.cpp
    src/SamplerD3D11Impl.cpp
    src/ShaderD3D11Impl.cpp
    src/ShaderResourceBindingD3D11Impl.cpp
    src/ShaderResourceCacheD3D11.cpp
    src/ShaderResourceLayoutD3D11.cpp
    src/ShaderResourcesD3D11.cpp
    src/SwapChainD3D11Impl.cpp
    src/Texture1D_D3D11.cpp
    src/Texture2D_D3D11.cpp
    src/Texture3D_D3D11.cpp
    src/TextureBaseD3D11.cpp
    src/TextureViewD3D11Impl.cpp
)

add_library(Diligent-GraphicsEngineD3D11Interface INTERFACE)
target_include_directories(Diligent-GraphicsEngineD3D11Interface
INTERFACE
    interface
)
target_link_libraries(Diligent-GraphicsEngineD3D11Interface 
INTERFACE 
    Diligent-GraphicsEngineD3DBaseInterface
)


add_library(Diligent-GraphicsEngineD3D11-static STATIC 
    ${SRC} ${INTERFACE} ${INCLUDE}
    readme.md
)

add_library(Diligent-GraphicsEngineD3D11-shared SHARED 
    src/DLLMain.cpp
    src/GraphicsEngineD3D11.def
    readme.md
)

# Set output name to GraphicsEngineD3D11_{32|64}{r|d}
set_dll_output_name(Diligent-GraphicsEngineD3D11-shared GraphicsEngineD3D11)

set_common_target_properties(Diligent-GraphicsEngineD3D11-shared)
set_common_target_properties(Diligent-GraphicsEngineD3D11-static)

target_include_directories(Diligent-GraphicsEngineD3D11-static 
PRIVATE
    include
)

target_link_libraries(Diligent-GraphicsEngineD3D11-static
PRIVATE
    Diligent-BuildSettings
    Diligent-GraphicsEngineD3DBase
    Diligent-TargetPlatform
    Diligent-Common
    Diligent-ShaderTools 
    dxgi.lib
    d3d11.lib
    d3dcompiler.lib
PUBLIC
    Diligent-GraphicsEngineD3D11Interface
)

target_link_libraries(Diligent-GraphicsEngineD3D11-shared
PRIVATE
    Diligent-BuildSettings 
    Diligent-GraphicsEngineD3D11-static
PUBLIC
    Diligent-GraphicsEngineD3D11Interface
)
target_compile_definitions(Diligent-GraphicsEngineD3D11-shared PUBLIC ENGINE_DLL=1)

source_group("src" FILES ${SRC} )

source_group("dll" FILES 
    src/DLLMain.cpp
    src/GraphicsEngineD3D11.def
)

source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-GraphicsEngineD3D11-static PROPERTIES
    FOLDER DiligentCore/Graphics
)
set_target_properties(Diligent-GraphicsEngineD3D11-shared PROPERTIES
    FOLDER DiligentCore/Graphics
)

set_source_files_properties(
    readme.md PROPERTIES HEADER_FILE_ONLY TRUE
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsEngineD3D11-shared)
    install_core_lib(Diligent-GraphicsEngineD3D11-static)
endif()