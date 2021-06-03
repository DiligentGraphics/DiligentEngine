cmake_minimum_required (VERSION 3.6)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Generate XCode schema files
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
# Make malloc write 0xAA to newly allocated memory and 0x55 to deallocated memory
set(CMAKE_XCODE_SCHEME_MALLOC_SCRIBBLE YES)
# Place guard pages on each side of large (4096 bytes or more) buffers
set(CMAKE_XCODE_SCHEME_MALLOC_GUARD_EDGES YES)

project(DiligentEngine)

option(DILIGENT_BUILD_TOOLS "Build DiligentTools module" ON)
option(DILIGENT_BUILD_FX "Build DiligentFX module" ON)
option(DILIGENT_BUILD_SAMPLES "Build DiligentSamples module" ON)

add_subdirectory(DiligentCore)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/DiligentCorePro")
    add_subdirectory(DiligentCorePro)
endif()

if(${DILIGENT_BUILD_TOOLS})
    add_subdirectory(DiligentTools)
else()
    if(${DILIGENT_BUILD_FX})
        message("Disabling DiligentFX module as it requires DiligentTools module to be enabled")
        set(DILIGENT_BUILD_FX OFF CACHE BOOL "Build DiligentFX module" FORCE)
    endif()

    if (${DILIGENT_BUILD_SAMPLES})
        message("Disabling DiligentSamples module as it requires DiligentTools module to be enabled")
        set(DILIGENT_BUILD_SAMPLES OFF CACHE BOOL "Build DiligentSamples module" FORCE)
    endif()

    if (${DILIGENT_BUILD_UNITY_PLUGIN})
        message("Disabling unity plugin as it requires DiligentTools module to be enabled")
        set(DILIGENT_BUILD_UNITY_PLUGIN OFF CACHE BOOL "Build Unity plugin" FORCE)
    endif()
endif()

if(${DILIGENT_BUILD_FX})
    add_subdirectory(DiligentFX)
endif()

if(${DILIGENT_BUILD_SAMPLES})
    add_subdirectory(DiligentSamples)
endif()

if(TARGET GLTFViewer)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT GLTFViewer)
elseif(TARGET Asteroids)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Asteroids)
elseif(TARGET Atmosphere)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Atmosphere)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/DiligentCommunity")
    add_subdirectory(DiligentCommunity)
endif()
