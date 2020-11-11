cmake_minimum_required (VERSION 3.6)

project(DiligentFX-IncludeTest)

file(GLOB_RECURSE SOURCE LIST_DIRECTORIES false *.cpp)

add_library(DiligentFX-IncludeTest ${SOURCE})

target_include_directories(DiligentFX-IncludeTest PRIVATE ../..)
target_link_libraries(DiligentFX-IncludeTest PRIVATE Diligent-BuildSettings)
set_common_target_properties(DiligentFX-IncludeTest)

if(MSVC)
    set_target_properties(DiligentFX-IncludeTest PROPERTIES 
        STATIC_LIBRARY_FLAGS "/IGNORE:4221"
    )
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE})

set_target_properties(DiligentFX-IncludeTest PROPERTIES
    FOLDER "DiligentFX/Tests"
)
