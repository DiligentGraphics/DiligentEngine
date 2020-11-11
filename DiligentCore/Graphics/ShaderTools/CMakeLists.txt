cmake_minimum_required (VERSION 3.6)

project(Diligent-ShaderTools CXX)

set(INCLUDE 
    include/ShaderToolsCommon.hpp
)

set(SOURCE 
    src/ShaderToolsCommon.cpp
)

if(VULKAN_SUPPORTED OR GL_SUPPORTED OR GLES_SUPPORTED)
    list(APPEND SOURCE src/GLSLUtils.cpp)
    list(APPEND INCLUDE include/GLSLUtils.hpp)
endif()

if(D3D11_SUPPORTED OR D3D12_SUPPORTED OR VULKAN_SUPPORTED)
    list(APPEND SOURCE src/HLSLUtils.cpp)
    list(APPEND INCLUDE include/HLSLUtils.hpp)
endif()

if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS OR PLATFORM_LINUX)
    set(DXC_SUPPORTED TRUE)
endif()

if (DXC_SUPPORTED)
    list(APPEND INCLUDE include/DXCompiler.hpp)
    list(APPEND SOURCE src/DXCompiler.cpp)

    if(PLATFORM_WIN32)
        list(APPEND INCLUDE include/DXCompilerBaseWin32.hpp)
    elseif(PLATFORM_UNIVERSAL_WINDOWS)
        list(APPEND INCLUDE include/DXCompilerBaseUWP.hpp)
    elseif(PLATFORM_LINUX)
        list(APPEND INCLUDE include/DXCompilerBaseLiunx.hpp)
    else()
        message(FATAL_ERROR "Unexpected platform")
    endif()

    if(PLATFORM_LINUX)
        list(APPEND INCLUDE
            ../../ThirdParty/DirectXShaderCompiler/dxc/dxcapi.h
            ../../ThirdParty/DirectXShaderCompiler/dxc/Support/WinAdapter.h
            ../../ThirdParty/DirectXShaderCompiler/dxc/Support/WinFunctions.h)
        list(APPEND SOURCE
            ../../ThirdParty/DirectXShaderCompiler/dxc/Support/dxcapi.cpp
            ../../ThirdParty/DirectXShaderCompiler/dxc/Support/WinAdapter.cpp)
    endif()
else()
    list(APPEND SOURCE src/DXILUtilsStub.cpp)
endif()

if(VULKAN_SUPPORTED)
    list(APPEND SOURCE src/SPIRVShaderResources.cpp)
    list(APPEND INCLUDE include/SPIRVShaderResources.hpp)

    if (NOT ${DILIGENT_NO_GLSLANG})
        list(APPEND SOURCE src/GLSLangUtils.cpp)
        list(APPEND INCLUDE include/GLSLangUtils.hpp)

        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Disable the following warning:
            #   moving a local object in a return statement prevents copy elision [-Wpessimizing-move]
            set_source_files_properties(src/SPIRVUtils.cpp
            PROPERTIES
                COMPILE_FLAGS -Wno-pessimizing-move
            )
        endif()
    endif()
endif()

add_library(Diligent-ShaderTools STATIC ${SOURCE} ${INCLUDE})

target_include_directories(Diligent-ShaderTools 
PUBLIC
    include
PRIVATE
    ../GraphicsEngine/include
)

if (DXC_SUPPORTED)
    target_include_directories(Diligent-ShaderTools PUBLIC ../../ThirdParty/DirectXShaderCompiler)
endif()

target_link_libraries(Diligent-ShaderTools 
PRIVATE 
    Diligent-BuildSettings
    Diligent-GraphicsAccessories
    Diligent-Common
PUBLIC
    Diligent-GraphicsEngineInterface
)

if (VULKAN_SUPPORTED OR GL_SUPPORTED OR GLES_SUPPORTED)
    target_compile_definitions(Diligent-ShaderTools PRIVATE DILIGENT_NO_HLSL=$<BOOL:${DILIGENT_NO_HLSL}>)
    if (NOT ${DILIGENT_NO_HLSL})
        target_include_directories(Diligent-ShaderTools PRIVATE ../HLSL2GLSLConverterLib/include)
        target_link_libraries(Diligent-ShaderTools PRIVATE Diligent-HLSL2GLSLConverterLib)
    endif()
endif()

if(VULKAN_SUPPORTED)
    target_link_libraries(Diligent-ShaderTools 
    PRIVATE
        spirv-cross-core
    )

    if (NOT ${DILIGENT_NO_GLSLANG})
        target_link_libraries(Diligent-ShaderTools 
        PRIVATE
            glslang
            SPIRV
            SPIRV-Tools-opt
        )

        target_include_directories(Diligent-ShaderTools 
        PRIVATE
            ../../ThirdParty/glslang
        )
    endif()
endif()

set_common_target_properties(Diligent-ShaderTools)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-ShaderTools PROPERTIES
    FOLDER DiligentCore/Graphics
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-ShaderTools)
endif()
