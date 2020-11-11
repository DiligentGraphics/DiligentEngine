cmake_minimum_required (VERSION 3.6)

project(Diligent-GraphicsAccessories CXX)

set(INTERFACE 
    interface/ColorConversion.h
    interface/GraphicsAccessories.hpp
    interface/GraphicsTypesOutputInserters.hpp
    interface/ResourceReleaseQueue.hpp
    interface/RingBuffer.hpp
    interface/SRBMemoryAllocator.hpp
    interface/VariableSizeAllocationsManager.hpp
    interface/VariableSizeGPUAllocationsManager.hpp
)

set(SOURCE
    src/ColorConversion.cpp
    src/SRBMemoryAllocator.cpp
    src/GraphicsAccessories.cpp
)

add_library(Diligent-GraphicsAccessories STATIC ${SOURCE} ${INTERFACE})

target_include_directories(Diligent-GraphicsAccessories 
PUBLIC
    interface
)

target_link_libraries(Diligent-GraphicsAccessories 
PRIVATE
    Diligent-BuildSettings
PUBLIC
    Diligent-Common 
    Diligent-GraphicsEngineInterface
)

set_common_target_properties(Diligent-GraphicsAccessories)

source_group("src" FILES ${SOURCE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-GraphicsAccessories PROPERTIES
    FOLDER DiligentCore/Graphics
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsAccessories)
endif()