cmake_minimum_required (VERSION 3.6)

project(Tutorial20_MeshShader CXX)

set(SOURCE
    src/Tutorial20_MeshShader.cpp
)

set(INCLUDE
    src/Tutorial20_MeshShader.hpp
)

set(SHADERS
    assets/dx_cube.ash
    assets/dx_cube.msh
    assets/dx_cube.psh
    assets/vk_cube.ash
    assets/vk_cube.msh
    assets/vk_cube.psh
    assets/structures.fxh
)

set(ASSETS
    assets/DGLogo.png
)

add_sample_app("Tutorial20_MeshShader" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
