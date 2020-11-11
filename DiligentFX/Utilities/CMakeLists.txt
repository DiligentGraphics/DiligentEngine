cmake_minimum_required (VERSION 3.6)

target_sources(DiligentFX PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/DiligentFXShaderSourceStreamFactory.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/DiligentFXShaderSourceStreamFactory.cpp"
)
