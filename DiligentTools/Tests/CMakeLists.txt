cmake_minimum_required (VERSION 3.6)

if(TARGET gtest)
    add_subdirectory(DiligentToolsTest)
endif()

add_subdirectory(IncludeTest)
