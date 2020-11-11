cmake_minimum_required (VERSION 3.6)

project(Diligent-GraphicsEngine CXX)

set(INCLUDE 
    include/BufferBase.hpp
    include/BufferViewBase.hpp
    include/CommandListBase.hpp
    include/DefaultShaderSourceStreamFactory.h
    include/Defines.h
    include/DeviceContextBase.hpp
    include/DeviceObjectBase.hpp
    include/EngineFactoryBase.hpp
    include/EngineMemory.h
    include/FenceBase.hpp
    include/FramebufferBase.hpp
    include/pch.h
    include/PipelineStateBase.hpp
    include/QueryBase.hpp
    include/RenderDeviceBase.hpp
    include/RenderPassBase.hpp
    include/ResourceMappingImpl.hpp
    include/SamplerBase.hpp
    include/ShaderBase.hpp
    include/ShaderResourceBindingBase.hpp
    include/ShaderResourceVariableBase.hpp
    include/StateObjectsRegistry.hpp
    include/SwapChainBase.hpp
    include/TextureBase.hpp
    include/TextureViewBase.hpp
)

set(INTERFACE 
    interface/APIInfo.h
    interface/BlendState.h
    interface/Buffer.h
    interface/BufferView.h
    interface/CommandList.h
    interface/Constants.h
    interface/DepthStencilState.h
    interface/DeviceContext.h
    interface/DeviceObject.h
    interface/EngineFactory.h
    interface/Fence.h
    interface/Framebuffer.h
    interface/GraphicsTypes.h
    interface/InputLayout.h
    interface/PipelineState.h
    interface/Query.h
    interface/RasterizerState.h
    interface/RenderDevice.h
    interface/RenderPass.h
    interface/ResourceMapping.h
    interface/Sampler.h
    interface/Shader.h
    interface/ShaderResourceBinding.h
    interface/ShaderResourceVariable.h
    interface/SwapChain.h
    interface/Texture.h
    interface/TextureView.h
)

set(SOURCE
    src/APIInfo.cpp
    src/BufferBase.cpp
    src/DefaultShaderSourceStreamFactory.cpp
    src/EngineMemory.cpp
    src/FramebufferBase.cpp
    src/PipelineStateBase.cpp
    src/ResourceMappingBase.cpp
    src/RenderPassBase.cpp
    src/TextureBase.cpp
)

add_library(Diligent-GraphicsEngine STATIC ${SOURCE} ${INTERFACE} ${INCLUDE})
set_common_target_properties(Diligent-GraphicsEngine)

target_include_directories(Diligent-GraphicsEngine 
PUBLIC
    include
    interface
)

target_link_libraries(Diligent-GraphicsEngine 
PRIVATE
    Diligent-BuildSettings
PUBLIC
    Diligent-PlatformInterface
    Diligent-Common
    Diligent-GraphicsAccessories
)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-GraphicsEngine PROPERTIES
    FOLDER DiligentCore/Graphics
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsEngine)
endif()