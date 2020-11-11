cmake_minimum_required (VERSION 3.6)

project(Tutorial15_MultipleWindows CXX)

set(SOURCE
    src/Tutorial15_MultipleWindows.cpp
)

add_executable(Tutorial15_MultipleWindows WIN32 ${SOURCE})
target_include_directories(Tutorial15_MultipleWindows PRIVATE "../../../DiligentCore")

target_compile_definitions(Tutorial15_MultipleWindows 
PRIVATE 
    UNICODE
    D3D11_SUPPORTED=$<BOOL:${D3D11_SUPPORTED}>
    D3D12_SUPPORTED=$<BOOL:${D3D12_SUPPORTED}>
    GL_SUPPORTED=$<BOOL:${GL_SUPPORTED}>
    VULKAN_SUPPORTED=$<BOOL:${VULKAN_SUPPORTED}>
)

set_target_properties(Tutorial15_MultipleWindows PROPERTIES
    FOLDER DiligentSamples/Tutorials
)

target_compile_definitions(Tutorial15_MultipleWindows PRIVATE ENGINE_DLL=1)
target_link_libraries(Tutorial15_MultipleWindows PRIVATE Diligent-BuildSettings)

if(D3D11_SUPPORTED)
    target_link_libraries(Tutorial15_MultipleWindows PRIVATE Diligent-GraphicsEngineD3D11-shared)
endif()
if(D3D12_SUPPORTED)
    target_link_libraries(Tutorial15_MultipleWindows PRIVATE Diligent-GraphicsEngineD3D12-shared)
endif()
if(GL_SUPPORTED)
    target_link_libraries(Tutorial15_MultipleWindows PRIVATE Diligent-GraphicsEngineOpenGL-shared)
endif()
if(VULKAN_SUPPORTED)
    target_link_libraries(Tutorial15_MultipleWindows PRIVATE Diligent-GraphicsEngineVk-shared)
endif()
copy_required_dlls(Tutorial15_MultipleWindows)
