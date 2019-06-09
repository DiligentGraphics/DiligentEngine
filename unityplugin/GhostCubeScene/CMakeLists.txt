cmake_minimum_required (VERSION 3.6)

project(GhostCubeScene CXX)

set(SOURCE
    src/GhostCubeScene.cpp
)

set(INCLUDE
    src/GhostCubeScene.h
)

set(SHADERS
    assets/shaders/Mirror.psh
    assets/shaders/Mirror.vsh
)
set(ALL_ASSETS ${SHADERS})
set_source_files_properties(${SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")


if(D3D12_SUPPORTED)
    list(APPEND SOURCE src/GhostCubeSceneResTrsnHelper.cpp)
    list(APPEND INCLUDE src/GhostCubeSceneResTrsnHelper.h)
endif()

if(PLATFORM_ANDROID OR PLATFORM_LINUX OR PLATFORM_MACOS OR PLATFORM_IOS)
    list(APPEND SOURCE src/LoadPluginFunctions.cpp)
endif()

        
set_source_files_properties(${SHADERS} PROPERTIES 
    VS_DEPLOYMENT_CONTENT 1
    VS_DEPLOYMENT_LOCATION "shaders"
    MACOSX_PACKAGE_LOCATION "Resources/shaders"
)

add_target_platform_app(GhostCubeScene "${SOURCE}" "${INCLUDE}" "${ALL_ASSETS}")

if(PLATFORM_WIN32)
    set_target_properties(GhostCubeScene PROPERTIES 
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    )
    
    add_custom_command(TARGET GhostCubeScene POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "\"$<TARGET_FILE:GhostCubePlugin-shared>\""
            "\"$<TARGET_FILE_DIR:GhostCubeScene>\"")

elseif(PLATFORM_UNIVERSAL_WINDOWS)
    append_emulator_uwp_source(GhostCubeScene)

    # We cannot use genreator expression to add a file because the file name
    # must not depend on the configuration
    set(PLUGIN_REL_DLL ${CMAKE_CURRENT_BINARY_DIR}/GhostCubePlugin_${ARCH}r.dll)
    set(PLUGIN_DBG_DLL ${CMAKE_CURRENT_BINARY_DIR}/GhostCubePlugin_${ARCH}d.dll)

    set(PLUGIN_DLLS ${PLUGIN_REL_DLL} ${PLUGIN_DBG_DLL})
    set_source_files_properties(${PLUGIN_DLLS} PROPERTIES GENERATED TRUE)
    set_source_files_properties(${PLUGIN_DBG_DLL} PROPERTIES 
        VS_DEPLOYMENT_CONTENT $<CONFIG:Debug>
        VS_DEPLOYMENT_LOCATION "."
    )
    set_source_files_properties(${PLUGIN_REL_DLL} PROPERTIES 
        VS_DEPLOYMENT_CONTENT $<NOT:$<CONFIG:Debug>>
        VS_DEPLOYMENT_LOCATION "."
    )

    add_custom_command(TARGET GhostCubeScene PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "\"$<TARGET_FILE:GhostCubePlugin-shared>\""
            "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:GhostCubePlugin-shared>"
    )

    target_sources(GhostCubeScene PRIVATE ${PLUGIN_DLLS})
    source_group("assets" FILES ${PLUGIN_DLLS})

endif()

add_dependencies(GhostCubeScene GhostCubePlugin-shared)


target_include_directories(GhostCubeScene
PRIVATE
    ../GhostCubePlugin/PluginSource/src/Unity
)

target_link_libraries(GhostCubeScene
PRIVATE
    Diligent-BuildSettings
    NativeAppBase
    UnityEmulator
    Diligent-TargetPlatform
)
if(PLATFORM_ANDROID)
    target_link_libraries(GhostCubeScene PRIVATE GhostCubePlugin-shared)
elseif (PLATFORM_LINUX)
    target_link_libraries(GhostCubeScene PRIVATE GhostCubePlugin-shared GL X11)
elseif (PLATFORM_MACOS OR PLATFORM_IOS)
    target_link_libraries(GhostCubeScene PRIVATE GhostCubePlugin-shared)
endif()

set_common_target_properties(GhostCubeScene)

if(MSVC)
    # Disable MSVC-specific warnings
    # - w4201: nonstandard extension used: nameless struct/union
    target_compile_options(GhostCubeScene PRIVATE /wd4201)
    
    if(PLATFORM_UNIVERSAL_WINDOWS)
        # Consume the windows runtime extensions
        target_compile_options(GhostCubeScene PRIVATE /ZW)
    endif()
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("assets\\shaders" FILES ${SHADERS})

set_target_properties(GhostCubeScene PROPERTIES
    FOLDER Unity
)
