cmake_minimum_required (VERSION 3.6)

project(Tutorial02_Cube CXX)

set(SOURCE
    src/Tutorial02_Cube.cpp
)

set(INCLUDE
    src/Tutorial02_Cube.hpp
)

set(SHADERS
    assets/cube.vsh
    assets/cube.psh
)

set(ASSETS)

add_sample_app("Tutorial02_Cube" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
