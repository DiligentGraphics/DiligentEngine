cmake_minimum_required (VERSION 3.6)

project(Tutorial12_RenderTarget CXX)

set(SOURCE
    src/Tutorial12_RenderTarget.cpp
    ../Common/src/TexturedCube.cpp
)

set(INCLUDE
    src/Tutorial12_RenderTarget.hpp
    ../Common/src/TexturedCube.hpp
)

set(SHADERS
    assets/cube.vsh
    assets/cube.psh
    assets/DGLogo.png
    assets/rendertarget.vsh
    assets/rendertarget.psh
)

add_sample_app("Tutorial12_RenderTarget" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
