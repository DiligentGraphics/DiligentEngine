cmake_minimum_required (VERSION 3.8)

project(Diligent-UniversalWindowsPlatform CXX)

set(INTERFACE 
    interface/UWPDebug.hpp
    interface/UWPFileSystem.hpp
    interface/UWPDefinitions.h
    interface/UWPNativeWindow.h
    ../Win32/interface/Win32Atomics.hpp
)

set(SOURCE 
    src/UWPDebug.cpp
    src/UWPFileSystem.cpp
    ../Win32/src/Win32Atomics.cpp
)

add_library(Diligent-UniversalWindowsPlatform ${SOURCE} ${INTERFACE} ${PLATFORM_INTERFACE_HEADERS})
set_common_target_properties(Diligent-UniversalWindowsPlatform)

target_include_directories(Diligent-UniversalWindowsPlatform 
PUBLIC
    ../Win32/interface
    interface
)

target_link_libraries(Diligent-UniversalWindowsPlatform 
PRIVATE
    Diligent-BuildSettings 
PUBLIC
    Diligent-BasicPlatform
    Diligent-PlatformInterface
)

source_group("src" FILES ${SOURCE})
source_group("interface\\UWP" FILES ${INTERFACE})
source_group("interface\\common" FILES ${PLATFORM_INTERFACE_HEADERS})

set_target_properties(Diligent-UniversalWindowsPlatform PROPERTIES
    FOLDER DiligentCore/Platforms
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-UniversalWindowsPlatform)
endif()
