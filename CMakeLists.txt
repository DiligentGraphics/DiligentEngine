cmake_minimum_required (VERSION 3.6)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(DiligentEngine)

option(DILIGENT_BUILD_FX "Build DiligentFX module" ON)
option(DILIGENT_BUILD_SAMPLES "Build DiligentSamples module" ON)
option(DILIGENT_BUILD_DEMOS "Build demo projects" ON)
option(DILIGENT_BUILD_UNITY_PLUGIN "Build Unity plugin" ON)

add_subdirectory(DiligentCore)
add_subdirectory(DiligentTools)
add_subdirectory(Common)

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

if(ENABLE_TESTS)
    add_subdirectory(Tests)
endif()

if(TARGET GLTFViewer)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT GLTFViewer)
elseif(TARGET Asteroids)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Asteroids)
elseif(TARGET AtmosphereSample)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT AtmosphereSample)
endif()
