cmake_minimum_required (VERSION 3.6)

project(Tutorial06_Multithreading CXX)

set(SOURCE
    src/Tutorial06_Multithreading.cpp
    ../Common/src/TexturedCube.cpp
)

set(INCLUDE
    src/Tutorial06_Multithreading.hpp
    ../Common/src/TexturedCube.hpp
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

add_sample_app("Tutorial06_Multithreading" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
target_link_libraries(Tutorial06_Multithreading PRIVATE Diligent-TextureLoader)
if(PLATFORM_LINUX)
    target_link_libraries(Tutorial06_Multithreading PRIVATE pthread)
endif()