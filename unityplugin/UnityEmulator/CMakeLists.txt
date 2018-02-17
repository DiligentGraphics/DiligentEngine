cmake_minimum_required (VERSION 3.6)

project(UnityEmulator CXX)

set(SOURCE 
    src/UnityGraphicsEmulator.cpp
)

set(INCLUDE
    include/DiligentGraphicsAdapter.h
    include/ResourceStateTransitionHandler.h
    include/UnityGraphicsEmulator.h
    include/UnitySceneBase.h
)

if(D3D11_SUPPORTED)
    list(APPEND SOURCE src/DiligentGraphicsAdapterD3D11.cpp)
    list(APPEND SOURCE src/UnityGraphicsD3D11Emulator.cpp)
    list(APPEND SOURCE src/UnityGraphicsD3D11Impl.h)
    list(APPEND INCLUDE include/DiligentGraphicsAdapterD3D11.h)
    list(APPEND INCLUDE include/UnityGraphicsD3D11Emulator.h)
endif()

if(D3D12_SUPPORTED)
    list(APPEND SOURCE src/DiligentGraphicsAdapterD3D12.cpp)
    list(APPEND SOURCE src/UnityGraphicsD3D12Emulator.cpp)
    list(APPEND SOURCE src/UnityGraphicsD3D12Impl.h)
    
    list(APPEND INCLUDE include/DiligentGraphicsAdapterD3D12.h)
    list(APPEND INCLUDE include/UnityGraphicsD3D12Emulator.h)
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    list(APPEND SOURCE src/UnityGraphicsGLCoreES_Emulator.cpp)
    list(APPEND SOURCE src/DiligentGraphicsAdapterGL.cpp)

    list(APPEND INCLUDE include/UnityGraphicsGLCoreES_Emulator.h)
    list(APPEND INCLUDE include/DiligentGraphicsAdapterGL.h)
endif()

if(GL_SUPPORTED)
    list(APPEND SOURCE src/UnityGraphicsGLCore_Impl.cpp)
    list(APPEND INCLUDE src/UnityGraphicsGLCore_Impl.h)
endif()

if(GLES_SUPPORTED)
    if(PLATFORM_ANDROID)
        list(APPEND SOURCE src/Android/UnityGraphicsGLESAndroid_Impl.cpp)
        list(APPEND INCLUDE src/Android/UnityGraphicsGLESAndroid_Impl.h)
    elseif(PLATFORM_IOS)
        list(APPEND SOURCE src/IOS/UnityGraphicsGLES_IOS_Impl.mm)
        list(APPEND INCLUDE src/IOS/UnityGraphicsGLES_IOS_Impl.h)
    else()
        message(FATAL_ERROR Unknown platform)
    endif()
endif()

if(PLATFORM_WIN32)
    list(APPEND SOURCE src/Windows/UnityAppWin32.cpp src/UnityAppBase.cpp)
    list(APPEND INCLUDE include/UnityAppBase.h)
    
elseif(PLATFORM_UNIVERSAL_WINDOWS)

    # Windows Runtime types cannot be included into static libraries
    # https://social.msdn.microsoft.com/Forums/en-US/269db513-64ef-4817-a025-43954f614eb3/lnk4264-why-are-static-libraries-not-recommended-when-authoring-windows-runtime-types?forum=winappswithnativecode
    # So as a workaround, we will include all source files into the target app project
    function(append_emulator_uwp_source TARGET_NAME)
        
        get_target_property(EMULATOR_SOURCE_DIR UnityEmulator SOURCE_DIR)
        set(EMULATOR_UWP_SOURCE
            ${EMULATOR_SOURCE_DIR}/src/UWP/UnityAppUWP.cpp
            ${EMULATOR_SOURCE_DIR}/src/UnityAppBase.cpp
        )
        set(EMULATOR_UWP_INCLUDE
            ${EMULATOR_SOURCE_DIR}/include/UnityAppBase.h
        )

        target_sources(${TARGET_NAME} PRIVATE ${EMULATOR_UWP_SOURCE} ${EMULATOR_UWP_INCLUDE})
        source_group("src\\UnityEmulator" FILES ${EMULATOR_UWP_SOURCE})
        source_group("include\\UnityEmulator" FILES ${EMULATOR_UWP_INCLUDE})
    endfunction()

elseif(PLATFORM_ANDROID)
    list(APPEND SOURCE 
        src/Android/UnityAppAndroid.cpp
        src/UnityAppBase.cpp
    )
    list(APPEND INCLUDE include/UnityAppBase.h)
elseif(PLATFORM_LINUX)
    list(APPEND SOURCE 
        src/Linux/UnityAppLinux.cpp 
        src/UnityAppBase.cpp
    )
    list(APPEND INCLUDE include/UnityAppBase.h)
elseif(PLATFORM_MACOS)
    list(APPEND SOURCE
        src/MacOS/UnityAppMacOS.cpp
    src/UnityAppBase.cpp
    )
    list(APPEND INCLUDE
    include/UnityAppBase.h
    )
    set_source_files_properties(src/UnityGraphicsGLCore_Impl.cpp
        PROPERTIES COMPILE_FLAGS "-x objective-c++"
    )

elseif(PLATFORM_IOS)
    list(APPEND SOURCE
        src/IOS/UnityAppIOS.cpp
        src/UnityAppBase.cpp
    )
    list(APPEND INCLUDE
         include/UnityAppBase.h
    )
else()
    message(FATAL_ERROR "Unknown platform")
endif()


add_library(UnityEmulator STATIC ${SOURCE} ${INCLUDE})
set_common_target_properties(UnityEmulator)

target_include_directories(UnityEmulator
PRIVATE
    ../GhostCubePlugin/PluginSource/src/Unity
PUBLIC
    include
)

if(MSVC)
    target_compile_options(UnityEmulator PRIVATE -DUNICODE)
endif()

target_link_libraries(UnityEmulator 
PRIVATE 
    BuildSettings
PUBLIC
    NativeAppBase
    Common 
    GraphicsEngine
    GraphicsTools
    TargetPlatform
)

if(GL_SUPPORTED)
    target_link_libraries(UnityEmulator PRIVATE glew-static) 
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    target_link_libraries(UnityEmulator PUBLIC GraphicsEngineOpenGL-static)
endif()

if(D3D11_SUPPORTED)
    target_link_libraries(UnityEmulator PUBLIC GraphicsEngineD3DBase GraphicsEngineD3D11-static)
endif()

if(D3D12_SUPPORTED)
    target_link_libraries(UnityEmulator PUBLIC GraphicsEngineD3DBase GraphicsEngineD3D12-static)
endif()

if(PLATFORM_UNIVERSAL_WINDOWS)
    target_link_libraries(UnityEmulator PRIVATE dxguid.lib)
elseif(PLATFORM_ANDROID)
    target_link_libraries(UnityEmulator PRIVATE NDKHelper GLESv3 android PUBLIC NativeAppGlue)
    target_include_directories(UnityEmulator PRIVATE
        ${ANDROID_NDK}/sources/android/cpufeatures
    )
elseif(PLATFORM_MACOS)
    target_include_directories(UnityEmulator PUBLIC
        src/MacOS
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Disable the following clang warning
    #    '<function name>' hides overloaded virtual function
    # as hiding is intended
    target_compile_options(UnityEmulator PRIVATE -Wno-overloaded-virtual)
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})

set_target_properties(UnityEmulator PROPERTIES
    FOLDER Unity
)
