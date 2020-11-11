cmake_minimum_required (VERSION 3.6)

project(Tutorial03_Texturing-C CXX)

set(SOURCE
    src/Tutorial03_Texturing-C.cpp
    src/Tutorial03_Texturing.c
)

set(INCLUDE
    src/Tutorial03_Texturing-C.hpp
)

set(SHADERS
    assets/cube.vsh
    assets/cube.psh
)

set(ASSETS
    assets/DGLogo.png
)

add_sample_app("Tutorial03_Texturing-C" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
target_link_libraries(Tutorial03_Texturing-C PRIVATE Diligent-TextureLoader)
