cmake_minimum_required (VERSION 3.6)

project(Tutorial05_TextureArray CXX)

set(SOURCE
    src/Tutorial05_TextureArray.cpp
    ../Common/src/TexturedCube.cpp
)

set(INCLUDE
    src/Tutorial05_TextureArray.hpp
    ../Common/src/TexturedCube.hpp
)

set(SHADERS
    assets/cube_inst.vsh
    assets/cube_inst.psh
)

set(ASSETS
    assets/DGLogo0.png
    assets/DGLogo1.png
    assets/DGLogo2.png
    assets/DGLogo3.png
)

add_sample_app("Tutorial05_TextureArray" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
target_link_libraries(Tutorial05_TextureArray PRIVATE Diligent-TextureLoader)