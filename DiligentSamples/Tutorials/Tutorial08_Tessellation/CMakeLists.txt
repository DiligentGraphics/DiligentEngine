cmake_minimum_required (VERSION 3.6)

project(Tutorial08_Tessellation CXX)

set(SOURCE
    src/Tutorial08_Tessellation.cpp
)

set(INCLUDE
    src/Tutorial08_Tessellation.hpp
)

set(SHADERS
    assets/terrain.vsh
    assets/terrain.psh
    assets/terrain_wire.psh
    assets/terrain.gsh
    assets/terrain.hsh
    assets/terrain.dsh
    assets/structures.fxh
)

set(ASSETS
    assets/ps_height_1k.png
    assets/ps_texture_2k.png
)

add_sample_app("Tutorial08_Tessellation" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
target_link_libraries(Tutorial08_Tessellation PRIVATE Diligent-TextureLoader)
