cmake_minimum_required (VERSION 3.6)

if(TARGET gtest)
    add_subdirectory(DiligentCoreTest)
    add_subdirectory(DiligentCoreAPITest)
endif()
add_subdirectory(IncludeTest)
