cmake_minimum_required (VERSION 3.6)

project(Diligent-TextureLoader CXX)

set(INCLUDE 
    include/DDSLoader.h
    include/dxgiformat.h
    include/JPEGCodec.h
    include/pch.h
    include/PNGCodec.h
)

set(INTERFACE
    interface/Image.h
    interface/TextureLoader.h
    interface/TextureUtilities.h
)

set(SOURCE 
    src/DDSLoader.cpp
    src/JPEGCodec.c
    src/Image.cpp
    src/KTXLoader.cpp
    src/PNGCodec.c
    src/TextureLoader.cpp
    src/TextureUtilities.cpp
)

add_library(Diligent-TextureLoader STATIC ${SOURCE} ${INCLUDE} ${INTERFACE})
set_common_target_properties(Diligent-TextureLoader)

target_include_directories(Diligent-TextureLoader
PUBLIC
    interface
PRIVATE
    include
)

source_group("source" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})

target_link_libraries(Diligent-TextureLoader 
PRIVATE 
    Diligent-BuildSettings 
    Diligent-Common 
    Diligent-PlatformInterface 
    Diligent-GraphicsEngineInterface 
    Diligent-GraphicsAccessories
    LibJpeg 
    LibPng 
    LibTiff 
    ZLib
)

set_target_properties(Diligent-TextureLoader PROPERTIES
    FOLDER DiligentTools
)

if(DILIGENT_INSTALL_TOOLS)
    install_tools_lib(Diligent-TextureLoader)
endif()
