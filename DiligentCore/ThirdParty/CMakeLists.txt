cmake_minimum_required (VERSION 3.6)

if(PLATFORM_WIN32 OR PLATFORM_LINUX OR PLATFORM_MACOS)
	add_subdirectory(glew)
    if(DILIGENT_INSTALL_CORE)
        install(TARGETS glew-static
                ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/${DILIGENT_CORE_DIR}/$<CONFIG>"
                LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/${DILIGENT_CORE_DIR}/$<CONFIG>"
                RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/${DILIGENT_CORE_DIR}/$<CONFIG>"
        )
    endif()
    install(FILES glew/LICENSE.txt DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}" RENAME GLEW-License.txt)
endif()

if(VULKAN_SUPPORTED)
    # We only need to have SPIRV-Headers folder, but we
    # do not really need any target from the project.
    #add_subdirectory(SPIRV-Headers)
    #set_directory_root_folder("SPIRV-Headers" "ThirdParty/DiligentCore/SPIRV-Headers")
    install(FILES SPIRV-Headers/LICENSE DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}" RENAME SPIRV-Headers-License.txt)

    install(FILES Vulkan-Headers/LICENSE.txt DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}" RENAME Vulkan-Headers-License.txt)

    option(DILIGENT_NO_GLSLANG "Do NOT build glslang compiler" OFF)
    option(DILIGENT_IMPROVE_SPIRV_TOOLS_DEBUG_PERF "Enable some optimizations for SPIRV-Tools, glslang, SPIRV-Cross and related projects in debug build to improve performance" ON)

    if(NOT (${DILIGENT_NO_GLSLANG} AND ${DILIGENT_NO_HLSL}))
        set(SPIRV-Headers_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Headers")
        set(SPIRV_SKIP_TESTS ON CACHE BOOL "Skip SPIRV tests")
        set(SPIRV_BUILD_COMPRESSION OFF CACHE BOOL "Do not build SPIRV compressing codec")
        set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "Skip building SPIRV executable and tests along with the library")
        set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "Skip SPIRV tools installation")
        set(SPIRV_WERROR OFF CACHE BOOL "Do not enable error on warning")
        if(${DILIGENT_IMPROVE_SPIRV_TOOLS_DEBUG_PERF})
            set(SPIRV_CHECK_CONTEXT OFF CACHE BOOL "Do not check if the IR context is in a valid state")
        endif()
        set(CMAKE_MACOSX_RPATH TRUE)
        add_subdirectory(SPIRV-Tools)
        set_directory_root_folder("SPIRV-Tools" "DiligentCore/ThirdParty/SPIRV-Tools")
        # We only need SPIRV-Tools-opt for glslang that also depends on SPIRV-Tools-static.
        # Exclude all other targets from the build to save build time
        set(SPIRV_TOOLS_EXCLUDE_TARGETS SPIRV-Tools-link SPIRV-Tools-reduce SPIRV-Tools-shared)
        set_target_properties(${SPIRV_TOOLS_EXCLUDE_TARGETS} PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

        set(INSTALL_TARGETS SPIRV-Tools-static SPIRV-Tools-opt)
        install(FILES SPIRV-Tools/LICENSE DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}" RENAME SPIRV-Tools-License.txt)
    endif()

    if(NOT ${DILIGENT_NO_GLSLANG})    
        set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "Do not build glslang binaries")
        set(ENABLE_SPVREMAPPER OFF CACHE BOOL "Do not build SPIRV remapper")
        set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "Skip glslang installation")
        set(ENABLE_CTEST OFF CACHE BOOL "Disable testing")
        add_subdirectory(glslang)
        set_directory_root_folder("glslang" "DiligentCore/ThirdParty/glslang")

        # glslang "kindly" sets global CMAKE_DEBUG_POSTFIX to "d", which we have to unset now
        unset(CMAKE_DEBUG_POSTFIX CACHE)

        list(APPEND INSTALL_TARGETS GenericCodeGen glslang OGLCompiler OSDependent SPIRV HLSL)
        install(FILES glslang/LICENSE.txt DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}" RENAME GLSLang-License.txt)
    endif()

    set(SPIRV_CROSS_CLI OFF CACHE BOOL "Build the CLI binary. Requires SPIRV_CROSS_STATIC.")
    set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "Enable SPIRV-Cross tests.")
    set(SPIRV_CROSS_ENABLE_MSL ${METAL_SUPPORTED} CACHE BOOL "Enable MSL target support.")
    # MSL support requires GLSL
    set(SPIRV_CROSS_ENABLE_GLSL ${SPIRV_CROSS_ENABLE_MSL} CACHE BOOL "Enable GLSL support.")
    set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "Enable HLSL target support.")
    set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "Enable C++ target support.")
    set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "Enable JSON reflection target support.")
    set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "Enable C API wrapper support in static library.")
    set(SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "Enable util module support.")
    set(SPIRV_CROSS_FORCE_PIC ON CACHE BOOL "Force position-independent code.")
    set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "Skip SPIRV-Cross installation.")
    set(SPIRV_CROSS_NAMESPACE_OVERRIDE diligent_spirv_cross CACHE STRING "Override the namespace used in the C++ API.")
    add_subdirectory(SPIRV-Cross)
    set_directory_root_folder("SPIRV-Cross" "DiligentCore/ThirdParty/SPIRV-Cross")

    list(APPEND INSTALL_TARGETS spirv-cross-core)
    install(FILES SPIRV-Cross/LICENSE DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}" RENAME SPIRV-Cross-License.txt)

    if(DILIGENT_INSTALL_CORE)
        install(TARGETS ${INSTALL_TARGETS}
                ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/${DILIGENT_CORE_DIR}/$<CONFIG>"
                LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/${DILIGENT_CORE_DIR}/$<CONFIG>"
                RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/${DILIGENT_CORE_DIR}/$<CONFIG>"
        )
    endif()

    if(${DILIGENT_IMPROVE_SPIRV_TOOLS_DEBUG_PERF} AND MSVC)
        # Enable function inlining (/Ob1) in Debug build to improve shader compilation performance
        foreach(TARGET ${INSTALL_TARGETS}) 
            target_compile_options(${TARGET} PRIVATE "$<$<CONFIG:DEBUG>:/Ob1>")
        endforeach()
    endif()

endif()

if(${DILIGENT_BUILD_TESTS})
    set(INSTALL_GTEST OFF CACHE BOOL "Do not install googletest")
    set(BUILD_GMOCK OFF CACHE BOOL "Do not build gmock")
    set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
    add_subdirectory(googletest)
    set_directory_root_folder("googletest" "DiligentCore/ThirdParty/googletest")
    set_target_properties(gtest gtest_main PROPERTIES
        CXX_VISIBILITY_PRESET hidden # -fvisibility=hidden
        C_VISIBILITY_PRESET hidden # -fvisibility=hidden
        VISIBILITY_INLINES_HIDDEN TRUE
     )
endif()

install(FILES googletest/LICENSE DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}" RENAME googletest-License.txt)
install(FILES stb/stb_image_write_license.txt DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}")
install(FILES volk/LICENSE.md DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}" RENAME Volk-License.md)
install(FILES DirectXShaderCompiler/LICENSE.TXT DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}" RENAME DXC-License.txt)
install(FILES DirectXShaderCompiler/ThirdPartyNotices.txt DESTINATION "Licenses/ThirdParty/${DILIGENT_CORE_DIR}" RENAME DXC-ThirdPartyNotices.txt)
