cmake_minimum_required (VERSION 3.6)

project(TestApp)

file(GLOB SOURCE LIST_DIRECTORIES false Src/*.cpp)
#set(SOURCE 
#    src/TestApp.cpp
#)

set(INCLUDE 
    include/TestApp.h
)

set(SHADERS)

set(ASSETS)

set(ALL_ASSETS ${ASSETS} ${SHADERS})
add_target_platform_app(TestApp "${SOURCE}" "${INCLUDE}" "${ALL_ASSETS}")

if(PLATFORM_WIN32)
    set(WIN32_SOURCE src/Win32/TestAppWin32.cpp)
    target_sources(TestApp PRIVATE ${WIN32_SOURCE})
    source_group("src\\Win32" FILES ${WIN32_SOURCE})

    set_target_properties(TestApp PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    )
    copy_required_dlls(TestApp)

elseif(PLATFORM_UNIVERSAL_WINDOWS)

        set(UWP_SOURCE
            src/UWP/TestAppUWP.cpp
        )
        target_sources(TestApp PRIVATE ${UWP_SOURCE})
        source_group("src\\UWP" FILES ${UWP_SOURCE})
        target_include_directories(${TestApp}  PRIVATE src/UWP)

elseif(PLATFORM_ANDROID)
    set(ANDROID_SOURCE
        src/Android/TestAppAndroid.cpp
    )
    target_sources(TestApp PRIVATE ${ANDROID_SOURCE})
    source_group("src\\Android" FILES ${ANDROID_SOURCE})
elseif(PLATFORM_LINUX)
    set(LINUX_SOURCE 
        src/Linux/TestAppLinux.cpp
    )
    target_sources(TestApp PRIVATE ${LINUX_SOURCE})
    source_group("src\\Linux" FILES ${LINUX_SOURCE})
elseif(PLATFORM_MACOS)

    set(MAC_SOURCE
        src/MacOS/TestAppMacOS.cpp
    )
    target_sources(TestApp PRIVATE ${MAC_SOURCE})
    source_group("src\\McOS" FILES ${MAC_SOURCE})

elseif(PLATFORM_IOS)
    set(IOS_SOURCE
        src/IOS/TestAppIOS.cpp
    )
    target_sources(TestApp PRIVATE ${IOS_SOURCE})
    source_group("src\\McOS" FILES ${IOS_SOURCE})
endif()

set_common_target_properties(TestApp)
target_include_directories(TestApp
PUBLIC
    include
)


if(MSVC)
    target_compile_options(TestApp PRIVATE -DUNICODE)
    
    if(PLATFORM_UNIVERSAL_WINDOWS)
        # Disable w4189: local variable is initialized but not referenced
        # Disable w4063: case is not a valid value for switch of enum
        # Consume the windows runtime extensions (/ZW)
        target_compile_options(TestApp INTERFACE /wd4189 /wd4063 /ZW)
    endif()
endif()

if(PLATFORM_WIN32)
    SET(ENGINE_LIBRARIES
        Diligent-GraphicsEngineD3D11-shared
        Diligent-GraphicsEngineD3D12-shared 
        Diligent-GraphicsEngineOpenGL-shared
    )
elseif(PLATFORM_ANDROID)
    SET(ENGINE_LIBRARIES
        Diligent-GraphicsEngineOpenGL-shared
    )
elseif(PLATFORM_UNIVERSAL_WINDOWS)
    SET(ENGINE_LIBRARIES
        Diligent-GraphicsEngineD3D11-static
        Diligent-GraphicsEngineD3D12-static
    )
elseif(PLATFORM_LINUX)
    SET(ENGINE_LIBRARIES
        Diligent-GraphicsEngineOpenGL-shared
    )
elseif(PLATFORM_MACOS)
    SET(ENGINE_LIBRARIES
        Diligent-GraphicsEngineOpenGL-shared
    )
elseif(PLATFORM_IOS)
    SET(ENGINE_LIBRARIES
        Diligent-GraphicsEngineOpenGL-static
    )
else()
    message(FATAL_ERROR "Undefined platform")
endif()

target_link_libraries(TestApp 
PRIVATE 
    Diligent-BuildSettings 
PUBLIC
    NativeAppBase
    Diligent-Common 
    Diligent-GraphicsTools
    Diligent-TargetPlatform
    ${ENGINE_LIBRARIES}
)

if(PLATFORM_UNIVERSAL_WINDOWS)
    target_link_libraries(TestApp PRIVATE dxguid.lib)
elseif(PLATFORM_ANDROID)
    target_link_libraries(TestApp PRIVATE GLESv3 PUBLIC native_app_glue)
elseif(PLATFORM_LINUX)
    target_link_libraries(TestApp PRIVATE GL X11)
elseif(PLATFORM_MACOS OR PLATFORM_IOS)

endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})

set_target_properties(TestApp PROPERTIES
    FOLDER Tests
)
