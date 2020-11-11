cmake_minimum_required (VERSION 3.17)

project(DiligentCoreAPITest)

file(GLOB COMMON_SOURCE LIST_DIRECTORIES false src/*)
file(GLOB COMMON_INCLUDE LIST_DIRECTORIES false include/*)
file(GLOB INLINE_SHADERS LIST_DIRECTORIES false include/InlineShaders/*)
file(GLOB_RECURSE SHADERS LIST_DIRECTORIES false assets/shaders/*.*)
file(GLOB C_INTERFACE_SOURCE LIST_DIRECTORIES false src/c_interface/*)

set_source_files_properties(${SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")

set(SOURCE ${COMMON_SOURCE} ${C_INTERFACE_SOURCE})
set(INCLUDE ${COMMON_INCLUDE})

if(NOT TARGET Diligent-HLSL2GLSLConverterLib)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/HLSL2GLSLConverterTest.cpp)
endif()

if(NOT GL_SUPPORTED AND NOT GLES_SUPPORTED)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/CreateObjFromNativeResGL.cpp)
    list(REMOVE_ITEM INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include/CreateObjFromNativeResGL.hpp)
endif()

if(NOT D3D11_SUPPORTED)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/CreateObjFromNativeResD3D11.cpp)
    list(REMOVE_ITEM INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include/CreateObjFromNativeResD3D11.hpp)
endif()

if(NOT D3D12_SUPPORTED)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/CreateObjFromNativeResD3D12.cpp)
    list(REMOVE_ITEM INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include/CreateObjFromNativeResD3D12.hpp)
endif()

if(NOT VULKAN_SUPPORTED)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/CreateObjFromNativeResVK.cpp)
    list(REMOVE_ITEM INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include/CreateObjFromNativeResVK.hpp)
endif()

if(PLATFORM_WIN32)
    file(GLOB SOURCE_WIN32 LIST_DIRECTORIES false src/Win32/*)
    list(APPEND SOURCE ${SOURCE_WIN32})
elseif(PLATFORM_MACOS)
    file(GLOB SOURCE_MACOS LIST_DIRECTORIES false src/MacOS/*)
    list(APPEND SOURCE ${SOURCE_MACOS})
elseif(PLATFORM_LINUX)
    file(GLOB SOURCE_LINUX LIST_DIRECTORIES false src/Linux/*)
    list(APPEND SOURCE ${SOURCE_LINUX})
endif()

if(D3D11_SUPPORTED)
    file(GLOB D3D11_SOURCE LIST_DIRECTORIES false src/D3D11/*)
    file(GLOB D3D11_INCLUDE LIST_DIRECTORIES false include/D3D11/*)
    list(APPEND INCLUDE ${D3D11_INCLUDE})
    list(APPEND SOURCE ${D3D11_SOURCE})
endif()

if(D3D12_SUPPORTED)
    file(GLOB D3D12_SOURCE LIST_DIRECTORIES false src/D3D12/*)
    file(GLOB D3D12_INCLUDE LIST_DIRECTORIES false include/D3D12/*)
    list(APPEND INCLUDE ${D3D12_INCLUDE})
    list(APPEND SOURCE ${D3D12_SOURCE})
endif()

if(VULKAN_SUPPORTED)
    file(GLOB VK_SOURCE LIST_DIRECTORIES false src/Vulkan/*)
    file(GLOB VK_INCLUDE LIST_DIRECTORIES false include/Vulkan/*)
    list(APPEND INCLUDE ${VK_INCLUDE})
    list(APPEND SOURCE ${VK_SOURCE})
endif()

if(METAL_SUPPORTED)
    file(GLOB MTL_SOURCE LIST_DIRECTORIES false src/Metal/*)
    file(GLOB MTL_INCLUDE LIST_DIRECTORIES false include/Metal/*)
    list(APPEND INCLUDE ${MTL_INCLUDE})
    list(APPEND SOURCE ${MTL_SOURCE})
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    file(GLOB GL_SOURCE LIST_DIRECTORIES false src/GL/*)
    file(GLOB GL_INCLUDE LIST_DIRECTORIES false include/GL/*)
    list(APPEND INCLUDE ${GL_INCLUDE})
    list(APPEND SOURCE ${GL_SOURCE})
endif()

set(ALL_SOURCE ${SOURCE} ${INCLUDE} ${SHADERS} ${INLINE_SHADERS})
add_executable(DiligentCoreAPITest ${ALL_SOURCE})
set_common_target_properties(DiligentCoreAPITest)

get_supported_backends(ENGINE_LIBRARIES)

target_link_libraries(DiligentCoreAPITest 
PRIVATE 
    gtest
    Diligent-BuildSettings 
    Diligent-TargetPlatform
    Diligent-GraphicsAccessories
    Diligent-Common
    Diligent-GraphicsTools
    Diligent-ShaderTools
    ${ENGINE_LIBRARIES}
)

if(TARGET Diligent-HLSL2GLSLConverterLib)
    target_link_libraries(DiligentCoreAPITest PRIVATE Diligent-HLSL2GLSLConverterLib)
endif()

if(D3D11_SUPPORTED OR D3D12_SUPPORTED)
    target_link_libraries(DiligentCoreAPITest PRIVATE d3dcompiler.lib)
endif()

if(D3D12_SUPPORTED)
    target_link_libraries(DiligentCoreAPITest PRIVATE d3d12.lib)
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    if(PLATFORM_WIN32)
        target_link_libraries(DiligentCoreAPITest PRIVATE glew-static opengl32.lib)
    elseif(PLATFORM_LINUX)
        target_link_libraries(DiligentCoreAPITest PRIVATE glew-static GL X11)
    elseif(PLATFORM_MACOS)
        find_package(OpenGL REQUIRED)
        target_link_libraries(DiligentCoreAPITest PRIVATE glew-static ${OPENGL_LIBRARY})
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
endif()

if(VULKAN_SUPPORTED)
    target_include_directories(DiligentCoreAPITest PRIVATE ../../ThirdParty)
    if(PLATFORM_LINUX)
        target_link_libraries(DiligentCoreAPITest
        PRIVATE
            dl # Required by Volk
            xcb
        )
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Disable warings like this one:
    #   comparison of function 'glPolygonMode' not equal to a null pointer is always true [-Wtautological-pointer-compare]
    set_source_files_properties(
        src/GL/DrawCommandReferenceGL.cpp
        src/GL/GeometryShaderRefenceGL.cpp
        src/GL/TessellationRefenceGL.cpp
        src/GL/TestingEnvironmentGL.cpp
    PROPERTIES
        COMPILE_FLAGS -Wno-tautological-pointer-compare
    )
endif()

target_include_directories(DiligentCoreAPITest
PRIVATE
    include
)


set_target_properties(DiligentCoreAPITest
PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

if(PLATFORM_WIN32)
    copy_required_dlls(DiligentCoreAPITest)
endif()

target_compile_definitions(DiligentCoreAPITest PRIVATE DILIGENT_NO_GLSLANG=$<BOOL:${DILIGENT_NO_GLSLANG}>)
if(${D3D12_H_HAS_MESH_SHADER})
    target_compile_definitions(DiligentCoreAPITest PRIVATE D3D12_H_HAS_MESH_SHADER)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ALL_SOURCE})

set_target_properties(DiligentCoreAPITest PROPERTIES
    FOLDER "DiligentCore/Tests"
)
