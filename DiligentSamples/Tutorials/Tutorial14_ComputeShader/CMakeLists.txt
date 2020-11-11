cmake_minimum_required (VERSION 3.6)

project(Tutorial14_ComputeShader CXX)

set(SOURCE
    src/Tutorial14_ComputeShader.cpp
)

set(INCLUDE
    src/Tutorial14_ComputeShader.hpp
)

set(SHADERS
    assets/particle.psh
    assets/particle.vsh
    assets/structures.fxh
    assets/reset_particle_lists.csh
    assets/collide_particles.csh
    assets/move_particles.csh
    assets/particles.fxh
)

set(ASSETS)

add_sample_app("Tutorial14_ComputeShader" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
