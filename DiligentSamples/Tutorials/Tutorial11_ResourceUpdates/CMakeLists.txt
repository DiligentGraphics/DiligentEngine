cmake_minimum_required (VERSION 3.6)

project(Tutorial11_ResourceUpdates CXX)

set(SOURCE
    src/Tutorial11_ResourceUpdates.cpp
)

set(INCLUDE
    src/Tutorial11_ResourceUpdates.hpp
)

set(SHADERS
    assets/cube.vsh
    assets/cube.psh
)

set(ASSETS
    assets/DGLogo0.png
    assets/DGLogo1.png
    assets/DGLogo2.png
    assets/DGLogo3.png
)

add_sample_app("Tutorial11_ResourceUpdates" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
target_link_libraries(Tutorial11_ResourceUpdates PRIVATE Diligent-TextureLoader)