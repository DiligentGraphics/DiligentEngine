cmake_minimum_required (VERSION 3.6)

project(Tutorial13_ShadowMap CXX)

set(SOURCE
    src/Tutorial13_ShadowMap.cpp
    ../Common/src/TexturedCube.cpp
)

set(INCLUDE
    src/Tutorial13_ShadowMap.hpp
    ../Common/src/TexturedCube.hpp
)

set(SHADERS
    assets/cube.vsh
    assets/cube.psh
    assets/cube_shadow.vsh
    assets/plane.vsh
    assets/plane.psh
    assets/shadow_map_vis.vsh
    assets/shadow_map_vis.psh
    assets/structures.fxh
)

set(ASSETS
    assets/DGLogo.png
)

add_sample_app("Tutorial13_ShadowMap" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
target_link_libraries(Tutorial13_ShadowMap PRIVATE Diligent-TextureLoader)
