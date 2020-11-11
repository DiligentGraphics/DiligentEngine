cmake_minimum_required (VERSION 3.3)

project(WindowsPlatform CXX)

set(INTERFACE
    interface/Win32Atomics.hpp
    interface/Win32Debug.hpp
    interface/Win32FileSystem.hpp
    interface/Win32PlatformDefinitions.h
    interface/Win32PlatformMisc.hpp
    interface/Win32NativeWindow.h
)

set(SOURCE 
    src/Win32Atomics.cpp
    src/Win32Debug.cpp
    src/Win32FileSystem.cpp
)

add_library(Diligent-Win32Platform ${SOURCE} ${INTERFACE} ${PLATFORM_INTERFACE_HEADERS})
set_common_target_properties(Diligent-Win32Platform)

target_include_directories(Diligent-Win32Platform 
PUBLIC
    interface
)

target_link_libraries(Diligent-Win32Platform 
PUBLIC
    Diligent-BasicPlatform
    Diligent-PlatformInterface
PRIVATE
    Diligent-BuildSettings
    Shlwapi.lib
)

source_group("src" FILES ${SOURCE})
source_group("interface\\win32" FILES ${INTERFACE})
source_group("interface\\common" FILES ${PLATFORM_INTERFACE_HEADERS})

set_target_properties(Diligent-Win32Platform PROPERTIES
    FOLDER DiligentCore/Platforms
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-Win32Platform)
endif()
