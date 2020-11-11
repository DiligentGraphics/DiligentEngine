cmake_minimum_required (VERSION 3.6)

project(DiligentCore)

# Define GNU standard installation directories such as CMAKE_INSTALL_INCLUDEDIR, CMAKE_INSTALL_LIBDIR, etc.
include(GNUInstallDirs)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Generate XCode schema files
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
    message(STATUS "CMAKE_BUILD_TYPE is not specified, default to Debug. Note that this is only relevant for single-configuration generators (such as Makefile Generators and Ninja).")
endif()

set(DEBUG_CONFIGURATIONS DEBUG CACHE INTERNAL "Debug configurations")
set(RELEASE_CONFIGURATIONS RELEASE RELWITHDEBINFO MINSIZEREL CACHE INTERNAL "Release configurations")

if(BUILD_CONFIGURATION_FILE)
    message("Using build configuration file " ${CUSTOM_BUILD_SCRIPT})
    include(${CMAKE_SOURCE_DIR}/${BUILD_CONFIGURATION_FILE})

    if(COMMAND custom_configure_build)
        custom_configure_build()
    else()
        message("custom_configure_build() function not found in " ${CUSTOM_BUILD_SCRIPT})
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../DiligentCorePro")
    set(DILIGENT_CORE_PRO_EXISTS TRUE)
else()
    set(DILIGENT_CORE_PRO_EXISTS FALSE)
endif()


set(PLATFORM_WIN32 FALSE CACHE INTERNAL "")
set(PLATFORM_UNIVERSAL_WINDOWS FALSE CACHE INTERNAL "")
set(PLATFORM_ANDROID FALSE CACHE INTERNAL "")
set(PLATFORM_LINUX FALSE CACHE INTERNAL "")
set(PLATFORM_MACOS FALSE CACHE INTERNAL "")
set(PLATFORM_IOS FALSE CACHE INTERNAL "")
set(D3D11_SUPPORTED FALSE CACHE INTERNAL "D3D11 is not supported")
set(D3D12_SUPPORTED FALSE CACHE INTERNAL "D3D12 is not supported")
set(GL_SUPPORTED FALSE CACHE INTERNAL "GL is not supported")
set(GLES_SUPPORTED FALSE CACHE INTERNAL "GLES is not supported")
set(VULKAN_SUPPORTED FALSE CACHE INTERNAL "Vulkan is not supported")
set(METAL_SUPPORTED FALSE CACHE INTERNAL "Metal is not supported")

set(DILIGENT_CORE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "DiligentCore module source directory")

set(CMAKE_OBJECT_PATH_MAX 4096)

if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(ARCH 64 CACHE INTERNAL "64-bit architecture")
else()
    set(ARCH 32 CACHE INTERNAL "32-bit architecture")
endif()

if(WIN32)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
        set(PLATFORM_UNIVERSAL_WINDOWS TRUE CACHE INTERNAL "Target platform: Windows Store")
        message("Target platform: Universal Windows. SDK Version: " ${CMAKE_SYSTEM_VERSION})
    else()
        set(PLATFORM_WIN32 TRUE CACHE INTERNAL "Target platform: Win32") #WIN32 is a variable, so we cannot use string "WIN32"
        message("Target platform: Win32. SDK Version: " ${CMAKE_SYSTEM_VERSION})
    endif()
else()
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
        set(PLATFORM_ANDROID TRUE CACHE INTERNAL "Target platform: Android")
        message("Target platform: Android")
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(PLATFORM_LINUX TRUE CACHE INTERNAL "Target platform: Linux")
        message("Target Platform: Linux")
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        if(IOS)
            set(PLATFORM_IOS TRUE CACHE INTERNAL "Target platform: iOS")
            message("Target Platform: iOS")
        else()
            set(PLATFORM_MACOS TRUE CACHE INTERNAL "Target platform: MacOS")
            message("Target Platform: MacOS")
        endif()
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
        set(PLATFORM_IOS TRUE CACHE INTERNAL "Target platform: iOS")
        message("Target Platform: iOS")
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
endif(WIN32)

add_library(Diligent-BuildSettings INTERFACE)

if(PLATFORM_WIN32)
    if(MSVC)
        set(D3D11_SUPPORTED TRUE CACHE INTERNAL "D3D11 is supported on Win32 platform")
        if(CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "10.0")
            set(D3D12_SUPPORTED TRUE CACHE INTERNAL "D3D12 is supported on Win32 platform")
        endif()
    else()
        message("Building with MinGW")
        set(MINGW_BUILD TRUE CACHE INTERNAL "Building with MinGW")
        set(D3D11_SUPPORTED FALSE CACHE INTERNAL "D3D11 requires compiling with MSVC")
        set(D3D12_SUPPORTED FALSE CACHE INTERNAL "D3D12 requires compiling with MSVC")
    endif()

    set(GL_SUPPORTED TRUE CACHE INTERNAL "OpenGL is supported on Win32 platform")
	set(VULKAN_SUPPORTED TRUE CACHE INTERNAL "Vulkan is supported on Win32 platform")
    target_compile_definitions(Diligent-BuildSettings INTERFACE PLATFORM_WIN32=1)
elseif(PLATFORM_UNIVERSAL_WINDOWS)
    set(D3D11_SUPPORTED TRUE CACHE INTERNAL "D3D11 is supported on Universal Windows platform")
	if(CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "10.0")
		set(D3D12_SUPPORTED TRUE CACHE INTERNAL "D3D12 is supported on Universal Windows platform")
	endif()
    target_compile_definitions(Diligent-BuildSettings INTERFACE PLATFORM_UNIVERSAL_WINDOWS=1)
elseif(PLATFORM_ANDROID)
    set(GLES_SUPPORTED TRUE CACHE INTERNAL "OpenGLES is supported on Android platform")
    set(VULKAN_SUPPORTED TRUE CACHE INTERNAL "Vulkan is supported on Android platform")
    target_compile_definitions(Diligent-BuildSettings INTERFACE PLATFORM_ANDROID=1)
elseif(PLATFORM_LINUX)
    set(GL_SUPPORTED TRUE CACHE INTERNAL "OpenGL is supported on Linux platform")
    if(${ARCH} EQUAL 64)
        set(VULKAN_SUPPORTED TRUE CACHE INTERNAL "Vulkan is supported on Linux64 platform")
    endif()
    target_compile_definitions(Diligent-BuildSettings INTERFACE PLATFORM_LINUX=1)
elseif(PLATFORM_MACOS)
    set(GL_SUPPORTED TRUE CACHE INTERNAL "OpenGL is supported on MacOS platform")
    if(${DILIGENT_CORE_PRO_EXISTS})
        set(METAL_SUPPORTED TRUE CACHE INTERNAL "Metal is supported on MacOS platform")
    else()
        message("DiligentCorePro module is not found. Metal backend will be disabled")
    endif()
    set(VULKAN_SUPPORTED TRUE CACHE INTERNAL "Vulkan is enabled through MoltenVK on MacOS platform")
    target_compile_definitions(Diligent-BuildSettings INTERFACE PLATFORM_MACOS=1)
elseif(PLATFORM_IOS)
    set(GLES_SUPPORTED TRUE CACHE INTERNAL "OpenGLES is supported on iOS platform")
    if(${DILIGENT_CORE_PRO_EXISTS})
        set(METAL_SUPPORTED TRUE CACHE INTERNAL "Metal is supported on iOS platform")
    else()
        message("DiligentCorePro module is not found. Metal backend will be disabled")
    endif()
    if(VULKAN_SDK OR MoltenVK_LIBRARY)
        if(NOT MoltenVK_LIBRARY)
            # iOS.toolchain.cmake currently configures cmake to only search iOS SDK for libraries,
            # thus find_library will be unable to find MoltenVK in VulkanSDK
            # find_library(MoltenVK_LIBRARY MoltenVK PATHS "${VULKAN_SDK}/MoltenVK/iOS/static")
            set(MoltenVK_LIBRARY "${VULKAN_SDK}/MoltenVK/iOS/static/libMoltenVK.a" CACHE FILEPATH "MoltenVK library")
        endif()

        if(EXISTS ${MoltenVK_LIBRARY})
            set(VULKAN_SUPPORTED TRUE CACHE INTERNAL "Vulkan is enabled through MoltenVK on iOS platform")
        else()
            message(WARNING "${MoltenVK_LIBRARY} does not exist. Vulkan backend will be disabled.")
            unset(MoltenVK_LIBRARY CACHE)
        endif()
    else()
        message("VULKAN_SDK or MoltenVK_LIBRARY is not defined. Vulkan backend will be disabled.")
    endif()
    target_compile_definitions(Diligent-BuildSettings INTERFACE PLATFORM_IOS=1)
else()
    message(FATAL_ERROR "No PLATFORM_XXX variable defined. Make sure that 'DiligentCore' folder is processed first")
endif()

if(PLATFORM_WIN32 OR PLATFORM_LINUX OR PLATFORM_MACOS)
    option(DILIGENT_BUILD_TESTS "Build Diligent Engine tests" OFF)
else()
    if(DILIGENT_BUILD_TESTS)
        message("Unit tests are not supported on this platform and will be disabled")
    endif()
    set(DILIGENT_BUILD_TESTS FALSE CACHE INTERNAL "Tests are not available on this platform" FORCE)
endif()


option(DILIGENT_NO_HLSL "Disable HLSL support in non-Direct3D backends" OFF)
option(DILIGENT_NO_FORMAT_VALIDATION "Disable source code format validation" OFF)
option(DILIGENT_NO_DIRECT3D11 "Disable Direct3D11 backend" OFF)
option(DILIGENT_NO_DIRECT3D12 "Disable Direct3D12 backend" OFF)
option(DILIGENT_NO_OPENGL "Disable OpenGL/GLES backend" OFF)
option(DILIGENT_NO_VULKAN "Disable Vulkan backend" OFF)
option(DILIGENT_NO_METAL "Disable Metal backend" OFF)
if(${DILIGENT_NO_DIRECT3D11})
    set(D3D11_SUPPORTED FALSE CACHE INTERNAL "D3D11 backend is forcibly disabled")
endif()
if(${DILIGENT_NO_DIRECT3D12})
    set(D3D12_SUPPORTED FALSE CACHE INTERNAL "D3D12 backend is forcibly disabled")
endif()
if(${DILIGENT_NO_OPENGL})
    set(GL_SUPPORTED FALSE CACHE INTERNAL "OpenGL backend is forcibly disabled")
    set(GLES_SUPPORTED FALSE CACHE INTERNAL "OpenGLES backend is forcibly disabled")
endif()
if(${DILIGENT_NO_VULKAN})
    set(VULKAN_SUPPORTED FALSE CACHE INTERNAL "Vulkan backend is forcibly disabled")
endif()
if(${DILIGENT_NO_METAL})
    set(METAL_SUPPORTED FALSE CACHE INTERNAL "Metal backend is forcibly disabled")
endif()

if(NOT (${D3D11_SUPPORTED} OR ${D3D12_SUPPORTED} OR ${GL_SUPPORTED} OR ${GLES_SUPPORTED} OR ${VULKAN_SUPPORTED} OR ${METAL_SUPPORTED}))
    message(FATAL_ERROR "No rendering backends are select to build")
endif()


message("D3D11_SUPPORTED: " ${D3D11_SUPPORTED})
message("D3D12_SUPPORTED: " ${D3D12_SUPPORTED})
message("GL_SUPPORTED: " ${GL_SUPPORTED})
message("GLES_SUPPORTED: " ${GLES_SUPPORTED})
message("VULKAN_SUPPORTED: " ${VULKAN_SUPPORTED})
message("METAL_SUPPORTED: " ${METAL_SUPPORTED})

target_compile_definitions(Diligent-BuildSettings 
INTERFACE 
    D3D11_SUPPORTED=$<BOOL:${D3D11_SUPPORTED}>
    D3D12_SUPPORTED=$<BOOL:${D3D12_SUPPORTED}>
    GL_SUPPORTED=$<BOOL:${GL_SUPPORTED}>
    GLES_SUPPORTED=$<BOOL:${GLES_SUPPORTED}>
    VULKAN_SUPPORTED=$<BOOL:${VULKAN_SUPPORTED}>
    METAL_SUPPORTED=$<BOOL:${METAL_SUPPORTED}>
)


if(MSVC)
    # For msvc, enable level 4 warnings and treat warnings as errors, except for
    # - w4100 - unreferenced formal parameter
    # - w4505 - unreferenced local function has been removed
    # - w4201 - nonstandard extension used: nameless struct/union
    target_compile_options(Diligent-BuildSettings INTERFACE /W4 /WX /wd4100 /wd4505 /wd4201)
    # In all release modes also:
    # - disable w4189 - local variable is initialized but not referenced
    # - Disable RTTI (/GR-)
    # - Enable whole program optimization (/GL)
    # - Enable string pooling (/GF)
    set(MSVC_ALL_RELEASE_COMPILE_OPTIONS /wd4189 /GR- /GL /GF)
    #target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:RELEASE>:/wd4189 /Ot")
    # In RELEASE mode:
    # - Set favor fast code option (/Ot)
    # - Enable intrinsic functions (/Oi)
	# - Maximize Speed (/O2)
    # - Inline any suitable function (/Ob2)
    set(MSVC_RELEASE_COMPILE_OPTIONS ${MSVC_ALL_RELEASE_COMPILE_OPTIONS} /Ot /Oi /Ob2 /O2)
    set(MSVC_RELWITHDEBINFO_COMPILE_OPTIONS ${MSVC_RELEASE_COMPILE_OPTIONS})
    # In MINSIZEREL mode set favor small code option (/Os)
    set(MSVC_MINSIZEREL_COMPILE_OPTIONS ${MSVC_ALL_RELEASE_COMPILE_OPTIONS} /Os)
    target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:RELEASE>:${MSVC_RELEASE_COMPILE_OPTIONS}>")
    target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:MINSIZEREL>:${MSVC_MINSIZEREL_COMPILE_OPTIONS}>")
    target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:RELWITHDEBINFO>:${MSVC_RELWITHDEBINFO_COMPILE_OPTIONS}>")
    # !!!NOTE!!! For some reason above is the only form of generator expression that works
    # For instance, this way
    # target_compile_options(Diligent-BuildSettings INTERFACE "$<$<CONFIG:RELEASE>:/Ot>")
    # does not work as expected

    set(DEBUG_MACROS DILIGENT_DEVELOPMENT DILIGENT_DEBUG)
    target_compile_definitions(Diligent-BuildSettings INTERFACE "$<$<CONFIG:DEBUG>:${DEBUG_MACROS}>")
else()
    # Todo: use __attribute__((always_inline)), but it needs to be defined in a header file
    target_compile_definitions(Diligent-BuildSettings INTERFACE __forceinline=inline)

    set(DEBUG_MACROS _DEBUG DEBUG DILIGENT_DEVELOPMENT DILIGENT_DEBUG)
    set(RELEASE_MACROS NDEBUG)

    foreach(DBG_CONFIG ${DEBUG_CONFIGURATIONS})
        target_compile_definitions(Diligent-BuildSettings INTERFACE "$<$<CONFIG:${DBG_CONFIG}>:${DEBUG_MACROS}>")
    endforeach()

    foreach(REL_CONFIG ${RELEASE_CONFIGURATIONS})
        target_compile_definitions(Diligent-BuildSettings INTERFACE "$<$<CONFIG:${REL_CONFIG}>:${RELEASE_MACROS}>")
    endforeach()
endif(MSVC)


if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
    CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if(PLATFORM_MACOS OR PLATFORM_IOS)
        set(WHOLE_ARCHIVE_FLAG "-Wl,-all_load")
        set(NO_WHOLE_ARCHIVE_FLAG "-Wl,-noall_load")
    else()
        set(WHOLE_ARCHIVE_FLAG "-Wl,--whole-archive")
        set(NO_WHOLE_ARCHIVE_FLAG "-Wl,--no-whole-archive")
    endif()
else()
    set(WHOLE_ARCHIVE_FLAG "")
    set(NO_WHOLE_ARCHIVE_FLAG "")
endif()


if(PLATFORM_MACOS)
    find_library(APP_KIT AppKit)
    if (NOT APP_KIT)
            message(FATAL_ERROR "AppKit not found")
    endif()
elseif(PLATFORM_IOS)
    find_library(CORE_FOUNDATION CoreFoundation)
    if(NOT CORE_FOUNDATION)
        message(FATAL_ERROR "Cannot find CoreFoundation framework")
    endif()

    find_library(FOUNDATION Foundation)
    if(NOT FOUNDATION)
        message(FATAL_ERROR "Cannot find Foundation framework")
    endif()

    find_library(OPENGLES OpenGLES)
    if(NOT OPENGLES)
        message(FATAL_ERROR "Cannot find OpenGLES framework")
    endif()
endif()

if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS OR PLATFORM_LINUX OR PLATFORM_MACOS OR PLATFORM_IOS)
    option(DILIGENT_INSTALL_CORE "Install DiligentCore module headers and libraries" ON)
else()
    set(DILIGENT_INSTALL_CORE OFF)
endif()

if(MSVC)
    option(DILIGENT_INSTALL_PDB "Install PDB files" OFF)
else()
    set(DILIGENT_INSTALL_PDB OFF)
endif()

file(RELATIVE_PATH DILIGENT_CORE_DIR "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
SET(DILIGENT_CORE_DIR ${DILIGENT_CORE_DIR} CACHE INTERNAL "Diligent Core installation directory")

SET(DILIGENT_CORE_INSTALL_LIBS_LIST "" CACHE INTERNAL "Core libraries installation list")
# CMAKE_INSTALL_PREFIX must be absolute otherwise rpath won't work
if(NOT IS_ABSOLUTE ${CMAKE_INSTALL_PREFIX})
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_PREFIX}" CACHE FILEPATH "Installation path" FORCE)
    message("Transformed CMAKE_INSTALL_PREFIX into absolute path: " ${CMAKE_INSTALL_PREFIX})
endif()

include(BuildUtils.cmake)

add_subdirectory(ThirdParty)
add_subdirectory(BuildTools)
add_subdirectory(Primitives)
add_subdirectory(Platforms)
add_subdirectory(Common)
add_subdirectory(Graphics)

if(DILIGENT_BUILD_TESTS)
    add_subdirectory(Tests)
endif()


# Installation instructions
if(DILIGENT_INSTALL_CORE)

    install_combined_static_lib(
        "${CMAKE_STATIC_LIBRARY_PREFIX}DiligentCore${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${DILIGENT_CORE_INSTALL_LIBS_LIST}"
        DiligentCore-static                     # Custom target name
        DiligentCore                            # Folder
        "${CMAKE_INSTALL_LIBDIR}/${DILIGENT_CORE_DIR}/$<CONFIG>"    # Install destination
    )

    install(FILES License.txt DESTINATION "Licenses" RENAME DiligentEngine-License.txt)
endif(DILIGENT_INSTALL_CORE)

# Create a custom target to run source code formatting validation command
add_format_validation_target(DiligentCore "${CMAKE_CURRENT_SOURCE_DIR}" DiligentCore/BuildTools)
