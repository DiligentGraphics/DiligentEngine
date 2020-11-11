cmake_minimum_required (VERSION 3.6)

project(DiligentToolsTest)

file(GLOB SOURCE src/*.*)

set(INCLUDE)

add_executable(DiligentToolsTest ${SOURCE} ${INCLUDE})
set_common_target_properties(DiligentToolsTest)

target_link_libraries(DiligentToolsTest 
PRIVATE 
    gtest_main
    Diligent-BuildSettings 
    Diligent-TargetPlatform
    Diligent-TextureLoader
    Diligent-Common
    LibPng
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE} ${INCLUDE})

set_target_properties(DiligentToolsTest PROPERTIES
    FOLDER "DiligentTools/Tests"
)
