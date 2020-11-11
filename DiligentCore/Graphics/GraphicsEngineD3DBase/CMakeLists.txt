cmake_minimum_required (VERSION 3.10)

project(Diligent-GraphicsEngineD3DBase CXX)

set(INCLUDE 
    include/D3DErrors.hpp
    include/D3DShaderResourceLoader.hpp
    include/D3DTypeConversionImpl.hpp
    include/D3DViewDescConversionImpl.hpp
    include/DXGITypeConversions.hpp
    include/EngineFactoryD3DBase.hpp
    include/HLSLDefinitions.fxh
    include/RenderDeviceD3DBase.hpp
    include/ShaderD3DBase.hpp
    include/ShaderResources.hpp
    include/ShaderVariableD3DBase.hpp
    include/SwapChainD3DBase.hpp
)

set(INTERFACE
    interface/ShaderD3D.h
)

set(SOURCE 
    src/DXGITypeConversions.cpp
    src/ShaderD3DBase.cpp
    src/ShaderResources.cpp
)

set(HLSL_DEFINITIONS include/HLSLDefinitions.fxh)

# We must use the full path, otherwise the build system will not be able to properly detect
# changes and shader conversion custom command will run every time
set(HLSL_DEFINITIONS_INC ${CMAKE_CURRENT_SOURCE_DIR}/include/HLSLDefinitions_inc.fxh)
set_source_files_properties(${HLSL_DEFINITIONS_INC} PROPERTIES GENERATED TRUE)

add_library(Diligent-GraphicsEngineD3DBase STATIC 
    ${SOURCE} ${INCLUDE} ${INTERFACE}

    # A target created in the same directory (CMakeLists.txt file) that specifies any output of the 
    # custom command as a source file is given a rule to generate the file using the command at build time. 
    ${HLSL_DEFINITIONS_INC}
)

add_custom_command(OUTPUT ${HLSL_DEFINITIONS_INC} # We must use full path here!
                   COMMAND ${FILE2STRING_PATH} ${HLSL_DEFINITIONS} include/HLSLDefinitions_inc.fxh
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                   MAIN_DEPENDENCY ${HLSL_DEFINITIONS}
                   COMMENT "Processing HLSLDefinitions.fxh"
                   VERBATIM
)

add_library(Diligent-GraphicsEngineD3DBaseInterface INTERFACE)
target_link_libraries(Diligent-GraphicsEngineD3DBaseInterface INTERFACE Diligent-GraphicsEngineInterface)
target_include_directories(Diligent-GraphicsEngineD3DBaseInterface INTERFACE interface)

target_include_directories(Diligent-GraphicsEngineD3DBase
PUBLIC
    include
)

target_link_libraries(Diligent-GraphicsEngineD3DBase
PRIVATE
    Diligent-BuildSettings
    Diligent-ShaderTools
PUBLIC
    Diligent-GraphicsEngine
    Diligent-GraphicsEngineD3DBaseInterface
)

set_common_target_properties(Diligent-GraphicsEngineD3DBase)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})
source_group("generated" FILES include/HLSLDefinitions_inc.fxh)

set_target_properties(Diligent-GraphicsEngineD3DBase PROPERTIES
    FOLDER DiligentCore/Graphics
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsEngineD3DBase)
endif()

# Set paths to D3Dcompiler_47.dll, dxcompiler.dll, and dxil.dll
if(MSVC)
    if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(ARCH_SUFFIX "x64")
    else()
        set(ARCH_SUFFIX "x86")
    endif()

    # Note that CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION is stated to be defined when targeting Windows 10
    # and above, however it is also defined when targeting 8.1 and Visual Studio 2019 (but not VS2017)
    if(CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "10.0")
        if (DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
            # Note that VS_WINDOWS_SDK_BIN_DIR as well as all derived paths can only be used in Visual Studio
            # commands and are not valid paths during CMake configuration
            set(VS_WINDOWS_SDK_BIN_DIR "$(WindowsSdkDir)\\bin\\${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}\\${ARCH_SUFFIX}")

            set(VS_D3D_COMPILER_PATH "\"${VS_WINDOWS_SDK_BIN_DIR}\\D3Dcompiler_47.dll\"" CACHE INTERNAL "D3Dcompiler_47.dll path")

            # DXC is only present in Windows SDK starting with version 10.0.17763.0
            if(${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} VERSION_GREATER_EQUAL "10.0.17763.0")
                set(VS_DXC_COMPILER_PATH "\"${VS_WINDOWS_SDK_BIN_DIR}\\dxcompiler.dll\"" CACHE INTERNAL "dxcompiler.dll path")
                set(VS_DXIL_SIGNER_PATH  "\"${VS_WINDOWS_SDK_BIN_DIR}\\dxil.dll\"" CACHE INTERNAL "dxil.dll path")
            endif()
        endif()
    elseif(CMAKE_SYSTEM_VERSION VERSION_EQUAL "8.1")
        # D3Dcompiler_47.dll from Win8.1 SDK is ancient (from 2013) and fails to
        # compile a number of test shaders. Use the compiler from Visual Studio 
        # executable path instead
        set(VS_D3D_COMPILER_PATH "\"$(VC_ExecutablePath_x64_${ARCH_SUFFIX})\\D3Dcompiler_47.dll\"" CACHE INTERNAL "D3Dcompiler_47.dll path")
    endif()
endif()
