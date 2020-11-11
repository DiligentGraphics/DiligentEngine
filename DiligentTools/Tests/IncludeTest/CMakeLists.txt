cmake_minimum_required (VERSION 3.6)

project(DiligentTools-IncludeTest)

file(GLOB_RECURSE SOURCE LIST_DIRECTORIES false *.cpp *.c)

add_library(DiligentTools-IncludeTest ${SOURCE})

target_include_directories(DiligentTools-IncludeTest PRIVATE ../..)
target_link_libraries(DiligentTools-IncludeTest PRIVATE Diligent-BuildSettings)
set_common_target_properties(DiligentTools-IncludeTest)

if(MSVC)
    set_target_properties(DiligentTools-IncludeTest PROPERTIES 
        STATIC_LIBRARY_FLAGS "/IGNORE:4221"
    )
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE})

set_target_properties(DiligentTools-IncludeTest PROPERTIES
    FOLDER "DiligentTools/Tests"
)
