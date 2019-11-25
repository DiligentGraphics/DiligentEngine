cmake_minimum_required (VERSION 3.6)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(DiligentEngine)

option(DILIGENT_BUILD_FX "Build DiligentFX module" ON)
option(DILIGENT_BUILD_SAMPLES "Build DiligentSamples module" ON)
option(DILIGENT_BUILD_DEMOS "Build demo projects" ON)
option(DILIGENT_BUILD_UNITY_PLUGIN "Build Unity plugin" ON)

# Create a custom target to run source code formatting validation command
# Need to do this before including Core
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    add_custom_target(Diligent-ValidateFormatting ALL
        COMMAND validate_format_win.bat
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/DiligentCore/Utilities/FormatValidation"
        COMMENT "Validating Core module's source code formatting..."
        VERBATIM
    )
    add_custom_command(TARGET Diligent-ValidateFormatting
        COMMAND validate_format_win.bat
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Utilities/FormatValidation"
        COMMENT "Validating Samples, Tools, and Tutorials modules' source code formatting..."
        VERBATIM
    )
endif()


add_subdirectory(DiligentCore)
add_subdirectory(DiligentTools)

if(${DILIGENT_BUILD_FX})
    add_subdirectory(DiligentFX)
endif()

if(${DILIGENT_BUILD_SAMPLES})
    add_subdirectory(DiligentSamples)
endif()

if(${DILIGENT_BUILD_DEMOS})
    add_subdirectory(Projects)
endif()

if(${DILIGENT_BUILD_UNITY_PLUGIN} AND (D3D11_SUPPORTED OR D3D12_SUPPORTED OR GL_SUPPORTED OR GLES_SUPPORTED))
    add_subdirectory(unityplugin)
endif()

if(DILIGENT_ENABLE_TESTS)
    add_subdirectory(Tests)
endif()

if(TARGET GLTFViewer)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT GLTFViewer)
elseif(TARGET Asteroids)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Asteroids)
elseif(TARGET AtmosphereSample)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT AtmosphereSample)
endif()
