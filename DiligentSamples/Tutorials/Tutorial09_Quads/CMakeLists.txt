cmake_minimum_required (VERSION 3.6)

project(Tutorial09_Quads CXX)

set(SOURCE
    src/Tutorial09_Quads.cpp
)

set(INCLUDE
    src/Tutorial09_Quads.hpp
)

set(SHADERS
    assets/quad.vsh
    assets/quad.psh
    assets/quad_batch.vsh
    assets/quad_batch.psh
)

set(ASSETS
    assets/DGLogo0.png
    assets/DGLogo1.png
    assets/DGLogo2.png
    assets/DGLogo3.png
)

add_sample_app("Tutorial09_Quads" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
target_link_libraries(Tutorial09_Quads PRIVATE Diligent-TextureLoader)
if(PLATFORM_LINUX)
    target_link_libraries(Tutorial09_Quads PRIVATE pthread)
endif()