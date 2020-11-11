cmake_minimum_required (VERSION 3.6)

project(Diligent-AndroidPlatform CXX)

set(INTERFACE 
    interface/AndroidDebug.hpp
    interface/AndroidFileSystem.hpp
    interface/AndroidPlatformDefinitions.h
    interface/AndroidPlatformMisc.hpp
    interface/AndroidNativeWindow.h
)

set(SOURCE 
    src/AndroidDebug.cpp
    src/AndroidFileSystem.cpp
)

add_library(Diligent-AndroidPlatform ${SOURCE} ${INTERFACE} ${PLATFORM_INTERFACE_HEADERS})
set_common_target_properties(Diligent-AndroidPlatform)

target_include_directories(Diligent-AndroidPlatform 
PUBLIC
    interface
)

target_link_libraries(Diligent-AndroidPlatform 
PRIVATE
    log
    android
    Diligent-BuildSettings 
PUBLIC
    Diligent-PlatformInterface
    Diligent-BasicPlatform
)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${PLATFORM_INTERFACE_HEADERS})

set_target_properties(Diligent-AndroidPlatform PROPERTIES
    FOLDER DiligentCore/Platforms
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-AndroidPlatform)
endif()