cmake_minimum_required (VERSION 3.3)

project(Diligent-GraphicsEngineOpenGL CXX)

set(INCLUDE 
    include/AsyncWritableResource.hpp
    include/BufferGLImpl.hpp
    include/BufferViewGLImpl.hpp
    include/DeviceContextGLImpl.hpp 
    include/FBOCache.hpp
    include/FenceGLImpl.hpp
    include/FramebufferGLImpl.hpp
    include/GLContext.hpp
    include/GLContextState.hpp
    include/GLObjectWrapper.hpp
    include/GLProgramResourceCache.hpp
    include/GLPipelineResourceLayout.hpp
    include/GLProgramResources.hpp
    include/GLTypeConversions.hpp
    include/pch.h
    include/PipelineStateGLImpl.hpp
    include/QueryGLImpl.hpp
    include/RenderDeviceGLImpl.hpp
    include/RenderPassGLImpl.hpp
    include/SamplerGLImpl.hpp
    include/ShaderGLImpl.hpp
    include/ShaderResourceBindingGLImpl.hpp
    include/SwapChainGLBase.hpp
    include/TexRegionRender.hpp
    include/Texture1D_OGL.hpp
    include/Texture1DArray_OGL.hpp
    include/Texture2D_OGL.hpp
    include/Texture2DArray_OGL.hpp
    include/Texture3D_OGL.hpp
    include/TextureBaseGL.hpp
    include/TextureCube_OGL.hpp
    include/TextureCubeArray_OGL.hpp
    include/TextureViewGLImpl.hpp
    include/VAOCache.hpp
)

set(INTERFACE 
    interface/BaseInterfacesGL.h
    interface/BufferGL.h
    interface/BufferViewGL.h
    interface/DeviceContextGL.h
    interface/EngineFactoryOpenGL.h
    interface/FenceGL.h
    interface/PipelineStateGL.h
    interface/QueryGL.h
    interface/RenderDeviceGL.h
    interface/SamplerGL.h
    interface/ShaderGL.h
    interface/ShaderResourceBindingGL.h
    interface/SwapChainGL.h
    interface/TextureGL.h
    interface/TextureViewGL.h
)

set(SOURCE 
    src/BufferGLImpl.cpp
    src/BufferViewGLImpl.cpp
    src/DeviceContextGLImpl.cpp
    src/EngineFactoryOpenGL.cpp
    src/FBOCache.cpp
    src/FenceGLImpl.cpp
    src/FramebufferGLImpl.cpp
    src/GLContextState.cpp
    src/GLObjectWrapper.cpp
    src/GLProgramResourceCache.cpp
    src/GLPipelineResourceLayout.cpp
    src/GLProgramResources.cpp
    src/GLTypeConversions.cpp
    src/PipelineStateGLImpl.cpp
    src/QueryGLImpl.cpp
    src/RenderDeviceGLImpl.cpp
    src/RenderPassGLImpl.cpp
    src/SamplerGLImpl.cpp
    src/ShaderGLImpl.cpp
    src/ShaderResourceBindingGLImpl.cpp
    src/TexRegionRender.cpp
    src/Texture1D_OGL.cpp
    src/Texture1DArray_OGL.cpp
    src/Texture2D_OGL.cpp
    src/Texture2DArray_OGL.cpp
    src/Texture3D_OGL.cpp
    src/TextureBaseGL.cpp
    src/TextureCube_OGL.cpp
    src/TextureCubeArray_OGL.cpp
    src/TextureViewGLImpl.cpp
    src/VAOCache.cpp
)

if(PLATFORM_WIN32)
    list(APPEND SOURCE src/GLContextWindows.cpp)
    list(APPEND SOURCE src/SwapChainGLImpl.cpp)
    list(APPEND INCLUDE include/GLContextWindows.hpp)
    list(APPEND INCLUDE include/SwapChainGLImpl.hpp)
elseif(PLATFORM_ANDROID)
    list(APPEND SOURCE src/GLContextAndroid.cpp)
    list(APPEND SOURCE src/RenderDeviceGLESImpl.cpp)
    list(APPEND SOURCE src/GLStubsAndroid.cpp)
    list(APPEND SOURCE src/SwapChainGLImpl.cpp)

    list(APPEND INCLUDE include/GLContextAndroid.hpp)
    list(APPEND INCLUDE include/GLStubsAndroid.h)
    list(APPEND INCLUDE include/RenderDeviceGLESImpl.hpp)
    list(APPEND INCLUDE include/SwapChainGLImpl.hpp)

    list(APPEND INTERFACE interface/RenderDeviceGLES.h)
elseif(PLATFORM_LINUX)
    list(APPEND SOURCE src/GLContextLinux.cpp)
    list(APPEND SOURCE src/SwapChainGLImpl.cpp)
    list(APPEND INCLUDE include/GLContextLinux.hpp)
    list(APPEND INCLUDE include/SwapChainGLImpl.hpp)
elseif(PLATFORM_MACOS)
    list(APPEND SOURCE src/GLContextMacOS.mm)
    list(APPEND INCLUDE include/GLContextMacOS.hpp)
    list(APPEND SOURCE src/SwapChainGLImpl.cpp)
    list(APPEND INCLUDE include/SwapChainGLImpl.hpp)
elseif(PLATFORM_IOS)
    list(APPEND SOURCE src/GLContextIOS.mm)
    list(APPEND INCLUDE include/GLContextIOS.hpp)
    list(APPEND INCLUDE include/GLStubsIOS.h)
    list(APPEND SOURCE src/SwapChainGLIOS.mm)
    list(APPEND INCLUDE include/SwapChainGLIOS.hpp)
else()
    message(FATAL_ERROR "Unknown platform")
endif()

add_library(Diligent-GraphicsEngineOpenGLInterface INTERFACE)
target_include_directories(Diligent-GraphicsEngineOpenGLInterface
INTERFACE
    interface
)
target_link_libraries(Diligent-GraphicsEngineOpenGLInterface 
INTERFACE 
    Diligent-GraphicsEngineInterface
)


add_library(Diligent-GraphicsEngineOpenGL-static STATIC 
    ${SOURCE} ${INTERFACE} ${INCLUDE}
    readme.md
)

add_library(Diligent-GraphicsEngineOpenGL-shared SHARED 
    readme.md
)
if(MSVC)
    target_sources(Diligent-GraphicsEngineOpenGL-shared 
    PRIVATE	
        src/DLLMain.cpp
        src/GraphicsEngineOpenGL.def
    )
endif()

target_include_directories(Diligent-GraphicsEngineOpenGL-static
PRIVATE
    include
)

set(PRIVATE_DEPENDENCIES 
    Diligent-BuildSettings
    Diligent-Common
    Diligent-TargetPlatform
    Diligent-GraphicsEngine
    Diligent-ShaderTools
)

if(${DILIGENT_NO_HLSL})
    message("HLSL support is disabled. OpenGL backend will not be able to consume HLSL source.")
else()
    target_include_directories(Diligent-GraphicsEngineOpenGL-static PRIVATE ../HLSL2GLSLConverterLib/include)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} Diligent-HLSL2GLSLConverterLib)
endif()

set(PUBLIC_DEPENDENCIES 
    Diligent-GraphicsEngineOpenGLInterface
)

if(PLATFORM_WIN32)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} glew-static opengl32.lib)
elseif(PLATFORM_ANDROID)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} GLESv3 EGL)
elseif(PLATFORM_LINUX)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} glew-static GL X11)
elseif(PLATFORM_MACOS)
    find_package(OpenGL REQUIRED)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} glew-static ${OPENGL_LIBRARY})
elseif(PLATFORM_IOS)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} ${OPENGLES})
else()
    message(FATAL_ERROR "Unknown platform")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Disable the following clang warning
    #    '<function name>' hides overloaded virtual function
    # as hiding is intended
    target_compile_options(Diligent-GraphicsEngineOpenGL-static PRIVATE -Wno-overloaded-virtual)
    target_compile_options(Diligent-GraphicsEngineOpenGL-shared PRIVATE -Wno-overloaded-virtual)

    # Disable warings like this one:
    #   comparison of function 'glPolygonMode' not equal to a null pointer is always true [-Wtautological-pointer-compare]
    set_source_files_properties(src/RenderDeviceGLImpl.cpp src/GLContextState.cpp
    PROPERTIES
        COMPILE_FLAGS -Wno-tautological-pointer-compare
    )

    if (PLATFORM_IOS)
        # Disable warings like this one:
        #   unused variable 'BottomLeftY' [-Wunused-variable]
        set_source_files_properties(
            src/DeviceContextGLImpl.cpp
            src/GLContextState.cpp
            src/RenderDeviceGLImpl.cpp
            src/Texture1D_OGL.cpp
        PROPERTIES
            COMPILE_FLAGS -Wno-unused-variable
        )
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set_target_properties(Diligent-GraphicsEngineOpenGL-shared PROPERTIES
        # Disallow missing direct and indirect dependencies to enssure that .so is self-contained
        LINK_FLAGS "-Wl,--no-undefined -Wl,--no-allow-shlib-undefined"
    )
    if(PLATFORM_WIN32)
        # MinGW
        # Restrict export to GetEngineFactoryOpenGL
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/export.map
            "{ global: *GetEngineFactoryOpenGL*; local: *; };"
        )
        # set_target_properties does not append link flags, but overwrites them
        set_property(TARGET Diligent-GraphicsEngineOpenGL-shared APPEND_STRING PROPERTY
            LINK_FLAGS " -Wl,--version-script=export.map"
        )
    endif()
endif()

target_link_libraries(Diligent-GraphicsEngineOpenGL-static
PRIVATE
    ${PRIVATE_DEPENDENCIES}
PUBLIC
    ${PUBLIC_DEPENDENCIES}
)
target_link_libraries(Diligent-GraphicsEngineOpenGL-shared
PRIVATE
    Diligent-BuildSettings
    ${WHOLE_ARCHIVE_FLAG} Diligent-GraphicsEngineOpenGL-static ${NO_WHOLE_ARCHIVE_FLAG}
PUBLIC
    ${PUBLIC_DEPENDENCIES}
)

target_compile_definitions(Diligent-GraphicsEngineOpenGL-static PRIVATE DILIGENT_NO_HLSL=$<BOOL:${DILIGENT_NO_HLSL}>)
target_compile_definitions(Diligent-GraphicsEngineOpenGL-shared PUBLIC ENGINE_DLL=1)
if(PLATFORM_MACOS)
    # Silence OpenGL deprecation warnings
    target_compile_definitions(Diligent-GraphicsEngineOpenGL-static PUBLIC GL_SILENCE_DEPRECATION)
endif()

if(PLATFORM_WIN32)

    # Do not add 'lib' prefix when building with MinGW
    set_target_properties(Diligent-GraphicsEngineOpenGL-shared PROPERTIES PREFIX "")

    # Set output name to GraphicsEngineOpenGL_{32|64}{r|d}
    set_dll_output_name(Diligent-GraphicsEngineOpenGL-shared GraphicsEngineOpenGL)

else()
    set_target_properties(Diligent-GraphicsEngineOpenGL-shared PROPERTIES
        OUTPUT_NAME GraphicsEngineOpenGL
    )
endif()

set_common_target_properties(Diligent-GraphicsEngineOpenGL-shared)
set_common_target_properties(Diligent-GraphicsEngineOpenGL-static)

source_group("src" FILES ${SOURCE})
if(PLATFORM_WIN32)
    source_group("dll" FILES 
        src/DLLMain.cpp
        src/GraphicsEngineOpenGL.def
    )
endif()

source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-GraphicsEngineOpenGL-static PROPERTIES
    FOLDER DiligentCore/Graphics
)
set_target_properties(Diligent-GraphicsEngineOpenGL-shared PROPERTIES
    FOLDER DiligentCore/Graphics
)

set_source_files_properties(
    readme.md PROPERTIES HEADER_FILE_ONLY TRUE
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsEngineOpenGL-shared)
    install_core_lib(Diligent-GraphicsEngineOpenGL-static)
endif()
