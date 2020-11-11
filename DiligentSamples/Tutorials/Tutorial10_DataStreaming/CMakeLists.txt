cmake_minimum_required (VERSION 3.6)

project(Tutorial10_DataStreaming CXX)

set(SOURCE
    src/Tutorial10_DataStreaming.cpp
)

set(INCLUDE
    src/Tutorial10_DataStreaming.hpp
)

set(SHADERS
    assets/polygon.vsh
    assets/polygon.psh
    assets/polygon_batch.vsh
    assets/polygon_batch.psh
)

set(ASSETS
    assets/DGLogo0.png
    assets/DGLogo1.png
    assets/DGLogo2.png
    assets/DGLogo3.png
)

add_sample_app("Tutorial10_DataStreaming" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
target_link_libraries(Tutorial10_DataStreaming PRIVATE Diligent-TextureLoader)
if(PLATFORM_LINUX)
    target_link_libraries(Tutorial10_DataStreaming PRIVATE pthread)
endif()