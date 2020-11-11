cmake_minimum_required (VERSION 3.6)

project(Tutorial18_Queries CXX)

set(SOURCE
    src/Tutorial18_Queries.cpp
    ../Common/src/TexturedCube.cpp
)

set(INCLUDE
    src/Tutorial18_Queries.hpp
    ../Common/src/TexturedCube.hpp
)

set(SHADERS
    assets/cube.vsh
    assets/cube.psh
    assets/DGLogo.png
)

add_sample_app("Tutorial18_Queries" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
