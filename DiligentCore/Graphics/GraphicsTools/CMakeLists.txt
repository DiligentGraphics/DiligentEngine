cmake_minimum_required (VERSION 3.6)

project(Diligent-GraphicsTools CXX)

set(INTERFACE
    interface/CommonlyUsedStates.h
    interface/DurationQueryHelper.hpp
    interface/GraphicsUtilities.h
    interface/MapHelper.hpp
    interface/pch.h
    interface/ScopedQueryHelper.hpp
    interface/ScreenCapture.hpp
    interface/ShaderMacroHelper.hpp
    interface/StreamingBuffer.hpp
    interface/TextureUploader.hpp
    interface/TextureUploaderBase.hpp
)

set(SOURCE 
    src/DurationQueryHelper.cpp
    src/GraphicsUtilities.cpp
    src/ScopedQueryHelper.cpp
    src/ScreenCapture.cpp
    src/pch.cpp
    src/TextureUploader.cpp
)

set(DEPENDENCIES)

if(D3D11_SUPPORTED)
    list(APPEND SOURCE src/TextureUploaderD3D11.cpp)
    list(APPEND INTERFACE interface/TextureUploaderD3D11.hpp)
    list(APPEND DEPENDENCIES Diligent-GraphicsEngineD3D11Interface)
endif()

if(D3D12_SUPPORTED OR VULKAN_SUPPORTED)
    list(APPEND SOURCE src/TextureUploaderD3D12_Vk.cpp)
    list(APPEND INTERFACE interface/TextureUploaderD3D12_Vk.hpp)
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    list(APPEND SOURCE src/TextureUploaderGL.cpp)
    list(APPEND INTERFACE interface/TextureUploaderGL.hpp)
    list(APPEND DEPENDENCIES Diligent-GraphicsEngineOpenGLInterface)
endif()

add_library(Diligent-GraphicsTools STATIC ${SOURCE} ${INTERFACE})

target_include_directories(Diligent-GraphicsTools 
PUBLIC
    interface
PRIVATE
    ../GraphicsEngineD3DBase/include
)

target_link_libraries(Diligent-GraphicsTools 
PRIVATE 
    Diligent-Common 
    Diligent-BuildSettings
    Diligent-PlatformInterface
    Diligent-GraphicsAccessories
    ${DEPENDENCIES}
PUBLIC
    Diligent-GraphicsEngineInterface
)

if(D3D11_SUPPORTED OR D3D12_SUPPORTED)
    target_link_libraries(Diligent-GraphicsTools 
    PRIVATE 
        Diligent-GraphicsEngineD3DBase
    )
endif()

set_common_target_properties(Diligent-GraphicsTools)

source_group("src" FILES ${SOURCE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-GraphicsTools PROPERTIES
    FOLDER DiligentCore/Graphics
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsTools)
endif()
