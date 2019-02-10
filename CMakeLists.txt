cmake_minimum_required (VERSION 3.6)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(DiligentEngine)

add_subdirectory(DiligentCore)
add_subdirectory(DiligentTools)
add_subdirectory(Common)
add_subdirectory(DiligentFX)
add_subdirectory(DiligentSamples)
add_subdirectory(Projects)

if(D3D11_SUPPORTED OR D3D12_SUPPORTED OR GL_SUPPORTED OR GLES_SUPPORTED)
    add_subdirectory(unityplugin)
endif()

if(ENABLE_TESTS)
    add_subdirectory(Tests)
endif()

if(TARGET Asteroids)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Asteroids)
elseif(TARGET AtmosphereSample)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT AtmosphereSample)
endif()
