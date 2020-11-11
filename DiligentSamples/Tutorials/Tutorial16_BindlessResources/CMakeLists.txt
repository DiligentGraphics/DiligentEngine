cmake_minimum_required (VERSION 3.6)

project(Tutorial16_BindlessResources CXX)

set(SOURCE
    src/Tutorial16_BindlessResources.cpp
)

set(INCLUDE
    src/Tutorial16_BindlessResources.hpp
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

add_sample_app("Tutorial16_BindlessResources" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
target_link_libraries(Tutorial16_BindlessResources PRIVATE Diligent-TextureLoader)