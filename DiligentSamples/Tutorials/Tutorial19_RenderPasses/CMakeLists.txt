cmake_minimum_required (VERSION 3.6)

project(Tutorial19_RenderPasses CXX)

set(SOURCE
    src/Tutorial19_RenderPasses.cpp
    ../Common/src/TexturedCube.cpp
)

set(INCLUDE
    src/Tutorial19_RenderPasses.hpp
    ../Common/src/TexturedCube.hpp
)

set(SHADERS
    assets/cube.vsh
    assets/cube.psh
    assets/light_volume.vsh
    assets/light_volume_glsl.psh
    assets/light_volume_hlsl.psh
    assets/ambient_light.vsh
    assets/ambient_light_glsl.psh
    assets/ambient_light_hlsl.psh
    assets/DGLogo.png
)

add_sample_app("Tutorial19_RenderPasses" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
