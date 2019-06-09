cmake_minimum_required (VERSION 3.6)

project(TestApp)

file(GLOB SOURCE LIST_DIRECTORIES false src/*.cpp)
file(GLOB INCLUDE LIST_DIRECTORIES false include/*.h)

if(NOT GL_SUPPORTED AND NOT GLES_SUPPORTED)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/ShaderConverterTest.cpp)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/TestCreateObjFromNativeResGL.cpp)
    list(REMOVE_ITEM INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include/ShaderConverterTest.h)
    list(REMOVE_ITEM INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include/TestCreateObjFromNativeResGL.h)
endif()

if(NOT D3D11_SUPPORTED)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/TestCreateObjFromNativeResD3D11.cpp)
    list(REMOVE_ITEM INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include/TestCreateObjFromNativeResD3D11.h)
endif()

if(NOT D3D12_SUPPORTED)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/TestCreateObjFromNativeResD3D12.cpp)
    list(REMOVE_ITEM INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include/TestCreateObjFromNativeResD3D12.h)
endif()

if(NOT VULKAN_SUPPORTED)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/TestCreateObjFromNativeResVK.cpp)
    list(REMOVE_ITEM INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include/TestCreateObjFromNativeResVK.h)
endif()

file(GLOB ASSETS LIST_DIRECTORIES false assets/*.*)
file(GLOB SHADERS LIST_DIRECTORIES false assets/shaders/*.*)
file(GLOB CS_TEST_SHADERS LIST_DIRECTORIES false assets/shaders/CSTest/*.*)
file(GLOB RT_TEST_SHADERS LIST_DIRECTORIES false assets/shaders/RTTest/*.*)

set_source_files_properties(${ASSETS} PROPERTIES
    VS_DEPLOYMENT_LOCATION "."
    MACOSX_PACKAGE_LOCATION "Resources"
)

set_source_files_properties(${SHADERS} PROPERTIES
    VS_DEPLOYMENT_LOCATION "Shaders"
    MACOSX_PACKAGE_LOCATION "Resources/Shaders"
)

set_source_files_properties(${CS_TEST_SHADERS} PROPERTIES
    VS_DEPLOYMENT_LOCATION "Shaders/CSTest"
    MACOSX_PACKAGE_LOCATION "Resources/Shaders/CSTest"
)

set_source_files_properties(${RT_TEST_SHADERS} PROPERTIES
    VS_DEPLOYMENT_LOCATION "Shaders/RTTest"
    MACOSX_PACKAGE_LOCATION "Resources/Shaders/RTTest"
)

set(ALL_SHADERS ${SHADERS} ${CS_TEST_SHADERS} ${RT_TEST_SHADERS})
set_source_files_properties(${ALL_SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")

set(ALL_ASSETS ${ASSETS} ${ALL_SHADERS})
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
        target_include_directories(TestApp PRIVATE src/UWP)

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
    source_group("src\\MacOS" FILES ${MAC_SOURCE})

elseif(PLATFORM_IOS)
    set(IOS_SOURCE
        src/IOS/TestAppIOS.cpp
    )
    target_sources(TestApp PRIVATE ${IOS_SOURCE})
    source_group("src\\MacOS" FILES ${IOS_SOURCE})
endif()

set_common_target_properties(TestApp)
target_include_directories(TestApp
PRIVATE
    ../../DiligentCore/External/vulkan
PUBLIC
    include
)


if(MSVC)
    target_compile_options(TestApp PRIVATE -DUNICODE /wd4201 /wd4189)
    
    if(PLATFORM_UNIVERSAL_WINDOWS)
        # Disable w4189: local variable is initialized but not referenced
        # Disable w4063: case is not a valid value for switch of enum
        # Consume the windows runtime extensions (/ZW)
        target_compile_options(TestApp PRIVATE /wd4063 /ZW)
    endif()
endif()

get_supported_backends(ENGINE_LIBRARIES)

target_link_libraries(TestApp 
PRIVATE 
    Diligent-BuildSettings
PUBLIC
    NativeAppBase
    Diligent-Common 
    Diligent-GraphicsTools
    Diligent-TargetPlatform
    Diligent-RenderScript
    Diligent-GraphicsAccessories
    ${ENGINE_LIBRARIES}
)

if(GL_SUPPORTED OR GLES_SUPPORTED)
    target_link_libraries(TestApp PRIVATE Diligent-HLSL2GLSLConverterLib)
endif()

if(GL_SUPPORTED)
    target_link_libraries(TestApp PRIVATE glew-static)
endif()


if(PLATFORM_UNIVERSAL_WINDOWS)
    target_link_libraries(TestApp PRIVATE dxguid.lib)
elseif(PLATFORM_ANDROID)
    target_link_libraries(TestApp PRIVATE GLESv3 PUBLIC native_app_glue)
elseif(PLATFORM_LINUX)
    target_link_libraries(TestApp PRIVATE GL X11 pthread)
elseif(PLATFORM_MACOS OR PLATFORM_IOS)

endif()

if(PLATFORM_WIN32 OR PLATFORM_LINUX)
    # Copy assets to target folder
    add_custom_command(TARGET TestApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "\"$<TARGET_FILE_DIR:TestApp>\"")
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("assets" FILES ${ASSETS})
source_group("assets\\shaders" FILES ${SHADERS})
source_group("assets\\shaders\\CSTest" FILES ${CS_TEST_SHADERS})
source_group("assets\\shaders\\RTTest" FILES ${RT_TEST_SHADERS})

set_target_properties(TestApp PROPERTIES
    FOLDER Tests
)
