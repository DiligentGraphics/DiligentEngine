cmake_minimum_required (VERSION 3.10)

project(Diligent-GraphicsEngineD3D12 CXX)

set(INCLUDE 
    include/BufferD3D12Impl.hpp
    include/BufferViewD3D12Impl.hpp
    include/CommandContext.hpp
    include/CommandListD3D12Impl.hpp
    include/CommandListManager.hpp
    include/CommandQueueD3D12Impl.hpp
    include/D3D12ResourceBase.hpp
    include/D3D12TypeConversions.hpp
    include/D3D12TypeDefinitions.h
    include/D3D12Utils.h
    include/d3dx12_win.h
    include/DescriptorHeap.hpp
    include/DeviceContextD3D12Impl.hpp
    include/D3D12DynamicHeap.hpp
    include/FenceD3D12Impl.hpp
    include/FramebufferD3D12Impl.hpp
    include/GenerateMips.hpp
    include/pch.h
    include/PipelineStateD3D12Impl.hpp
    include/QueryD3D12Impl.hpp
    include/QueryManagerD3D12.hpp
    include/RenderDeviceD3D12Impl.hpp
    include/RenderPassD3D12Impl.hpp
    include/RootSignature.hpp
    include/SamplerD3D12Impl.hpp
    include/ShaderD3D12Impl.hpp
    include/ShaderResourceBindingD3D12Impl.hpp
    include/ShaderResourceCacheD3D12.hpp
    include/ShaderResourceLayoutD3D12.hpp
    include/ShaderResourcesD3D12.hpp
    include/ShaderVariableD3D12.hpp
    include/SwapChainD3D12Impl.hpp
    include/TextureD3D12Impl.hpp
    include/TextureViewD3D12Impl.hpp
)

set(INTERFACE 
    interface/BufferD3D12.h
    interface/BufferViewD3D12.h
    interface/CommandQueueD3D12.h
    interface/DeviceContextD3D12.h
    interface/EngineFactoryD3D12.h
    interface/FenceD3D12.h
    interface/PipelineStateD3D12.h
    interface/QueryD3D12.h
    interface/RenderDeviceD3D12.h
    interface/SamplerD3D12.h
    interface/ShaderD3D12.h
    interface/ShaderResourceBindingD3D12.h
    interface/SwapChainD3D12.h
    interface/TextureD3D12.h
    interface/TextureViewD3D12.h
)


set(SRC 
    src/BufferD3D12Impl.cpp
    src/BufferViewD3D12Impl.cpp
    src/CommandContext.cpp
    src/CommandListManager.cpp
    src/CommandQueueD3D12Impl.cpp
    src/D3D12TypeConversions.cpp
    src/D3D12Utils.cpp
    src/DescriptorHeap.cpp
    src/DeviceContextD3D12Impl.cpp
    src/D3D12DynamicHeap.cpp
    src/EngineFactoryD3D12.cpp
    src/FenceD3D12Impl.cpp
    src/FramebufferD3D12Impl.cpp
    src/GenerateMips.cpp
    src/PipelineStateD3D12Impl.cpp
    src/QueryD3D12Impl.cpp
    src/QueryManagerD3D12.cpp
    src/RenderDeviceD3D12Impl.cpp
    src/RenderPassD3D12Impl.cpp
    src/RootSignature.cpp
    src/SamplerD3D12Impl.cpp
    src/ShaderD3D12Impl.cpp
    src/ShaderResourceBindingD3D12Impl.cpp
    src/ShaderResourceCacheD3D12.cpp
    src/ShaderResourceLayoutD3D12.cpp
    src/ShaderResourcesD3D12.cpp
    src/ShaderVariableD3D12.cpp
    src/SwapChainD3D12Impl.cpp
    src/TextureD3D12Impl.cpp
    src/TextureViewD3D12Impl.cpp
)

if(PLATFORM_WIN32)
    list(APPEND INCLUDE include/D3D12Loader.hpp)
    list(APPEND SRC src/D3D12Loader.cpp)
    set(USE_D3D12_LOADER 1)
else()
    set(USE_D3D12_LOADER 0)
endif()

set(SHADERS
    shaders/GenerateMips/GenerateMipsGammaCS.hlsl
    shaders/GenerateMips/GenerateMipsGammaOddCS.hlsl
    shaders/GenerateMips/GenerateMipsGammaOddXCS.hlsl
    shaders/GenerateMips/GenerateMipsGammaOddYCS.hlsl
    shaders/GenerateMips/GenerateMipsLinearCS.hlsl
    shaders/GenerateMips/GenerateMipsLinearOddCS.hlsl
    shaders/GenerateMips/GenerateMipsLinearOddXCS.hlsl
    shaders/GenerateMips/GenerateMipsLinearOddYCS.hlsl
)

set(COMPILED_SHADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/CompiledShaders/GenerateMips)
file(MAKE_DIRECTORY "${COMPILED_SHADERS_DIR}")

set_source_files_properties(${SHADERS} PROPERTIES 
    VS_SHADER_TYPE Compute
    VS_SHADER_ENTRYPOINT main
    VS_SHADER_MODEL 5.0
    VS_SHADER_VARIABLE_NAME "g_p%(Filename)"
    VS_SHADER_OUTPUT_HEADER_FILE "${COMPILED_SHADERS_DIR}/%(Filename).h"
    #VS_SHADER_FLAGS "/O3"
)

add_library(Diligent-GraphicsEngineD3D12Interface INTERFACE)
target_include_directories(Diligent-GraphicsEngineD3D12Interface
INTERFACE
    interface
)
target_link_libraries(Diligent-GraphicsEngineD3D12Interface 
INTERFACE 
    Diligent-GraphicsEngineD3DBaseInterface
)

add_library(Diligent-GraphicsEngineD3D12-static STATIC 
    ${SRC} ${INTERFACE} ${INCLUDE} ${SHADERS}
    readme.md
    shaders/GenerateMips/GenerateMipsCS.hlsli
)

add_library(Diligent-GraphicsEngineD3D12-shared SHARED 
    src/DLLMain.cpp
    src/GraphicsEngineD3D12.def
    readme.md
)

target_include_directories(Diligent-GraphicsEngineD3D12-static 
PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/CompiledShaders
    include
)

target_link_libraries(Diligent-GraphicsEngineD3D12-static
PRIVATE
    Diligent-BuildSettings 
    Diligent-Common 
    Diligent-GraphicsEngineD3DBase
    Diligent-GraphicsEngineNextGenBase
    Diligent-TargetPlatform 
    Diligent-ShaderTools
    dxgi.lib
    d3dcompiler.lib
PUBLIC
    Diligent-GraphicsEngineD3D12Interface
)

target_compile_definitions(Diligent-GraphicsEngineD3D12-static PRIVATE USE_D3D12_LOADER=${USE_D3D12_LOADER})
if(NOT ${USE_D3D12_LOADER})
    # Link with d3d12.lib if we don't use d3d12 loader
    target_link_libraries(Diligent-GraphicsEngineD3D12-static PRIVATE d3d12.lib)
endif()

target_link_libraries(Diligent-GraphicsEngineD3D12-shared
PRIVATE
    Diligent-BuildSettings
    Diligent-GraphicsEngineD3D12-static
PUBLIC
    Diligent-GraphicsEngineD3D12Interface
)
target_compile_definitions(Diligent-GraphicsEngineD3D12-shared PUBLIC ENGINE_DLL=1)

if(${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} STRGREATER_EQUAL "10.0.19041.0")
    set(D3D12_H_HAS_MESH_SHADER ON CACHE INTERNAL "D3D12 headers support mesh shaders" FORCE)
    target_compile_definitions(Diligent-GraphicsEngineD3D12-static PRIVATE D3D12_H_HAS_MESH_SHADER=1)
endif()

# Set output name to GraphicsEngineD3D12_{32|64}{r|d}
set_dll_output_name(Diligent-GraphicsEngineD3D12-shared GraphicsEngineD3D12)

set_common_target_properties(Diligent-GraphicsEngineD3D12-shared)
set_common_target_properties(Diligent-GraphicsEngineD3D12-static)

source_group("src" FILES ${SRC})

source_group("dll" FILES 
    src/DLLMain.cpp
    src/GraphicsEngineD3D12.def
)

source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})
source_group("shaders" FILES 
    ${SHADERS}
    shaders/GenerateMips/GenerateMipsCS.hlsli
)

set_target_properties(Diligent-GraphicsEngineD3D12-static PROPERTIES
    FOLDER DiligentCore/Graphics
)
set_target_properties(Diligent-GraphicsEngineD3D12-shared PROPERTIES
    FOLDER DiligentCore/Graphics
)

set_source_files_properties(
    readme.md PROPERTIES HEADER_FILE_ONLY TRUE
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsEngineD3D12-shared)
    install_core_lib(Diligent-GraphicsEngineD3D12-static)
endif()