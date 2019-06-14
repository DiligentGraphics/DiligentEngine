cmake_minimum_required (VERSION 3.3)

if(TARGET Diligent-RenderScript)
    add_subdirectory(TestApp)
endif()

add_subdirectory(IncludeTest)
