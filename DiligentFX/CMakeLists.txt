cmake_minimum_required (VERSION 3.6)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(DiligentFX CXX)

file(GLOB_RECURSE SHADERS LIST_DIRECTORIES false Shaders/*.*)

add_library(DiligentFX STATIC README.md ${SHADERS})

file(RELATIVE_PATH DILIGENT_FX_DIR "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")

if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS OR PLATFORM_LINUX OR PLATFORM_MACOS OR PLATFORM_IOS)
    option(DILIGENT_INSTALL_FX "Install DiligentFX module headers and libraries" ON)
else()
    set(DILIGENT_INSTALL_FX OFF)
endif()

target_link_libraries(DiligentFX 
PRIVATE
    Diligent-BuildSettings
PUBLIC
    Diligent-GraphicsEngine
    Diligent-GraphicsTools
    Diligent-AssetLoader
)
set_common_target_properties(DiligentFX)

target_include_directories(DiligentFX
PUBLIC
    .
)

add_subdirectory(Utilities)
add_subdirectory(Components)
add_subdirectory(PostProcess)
add_subdirectory(GLTF_PBR_Renderer)

if(DILIGENT_BUILD_TESTS)
    add_subdirectory(Tests)
endif()

get_target_property(SOURCE DiligentFX SOURCES)

foreach(FILE ${SOURCE}) 
    # Get the directory of the source file
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

    # Remove common directory prefix to make the group
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

    # Make sure we are using windows slashes
    string(REPLACE "/" "\\" GROUP "${GROUP}")

    source_group("${GROUP}" FILES "${FILE}")
endforeach()

# Convert shaders to headers and generate master header with the list of all files
if(NOT FILE2STRING_PATH STREQUAL "")
    set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders_inc)

    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

    set(SHADERS_LIST_FILE shaders_inc/shaders_list.h)
    file(WRITE ${SHADERS_LIST_FILE}
        "struct ShaderIncInfo\n"
        "{\n"
        "    const char* const FileName;\n"
        "    const char* const Source;\n"
        "};\n"
        "\n"
        "static const ShaderIncInfo g_Shaders[] =\n"
        "{"
        )

    foreach(FILE ${SHADERS})

        get_filename_component(FILE_NAME ${FILE} NAME)
        set(CONVERTED_FILE ${SHADER_OUTPUT_DIR}/${FILE_NAME}.h)
        add_custom_command(OUTPUT ${CONVERTED_FILE}
                           COMMAND ${FILE2STRING_PATH} ${FILE} ${CONVERTED_FILE}
                           MAIN_DEPENDENCY ${FILE} # the primary input source file to the command
                           COMMENT "Processing shader ${FILE}"
                           VERBATIM)

        string(REPLACE "." "_" VAR_NAME "${FILE_NAME}")
        file(APPEND ${SHADERS_LIST_FILE}
                "\n    {"
                "\n        \"${FILE_NAME}\","
                "\n        #include \"${FILE_NAME}.h\""
                "\n    },"
            )

            list(APPEND SHADERS_INC_LIST ${CONVERTED_FILE})

    endforeach()
    file(APPEND ${SHADERS_LIST_FILE}
        "\n};\n"
        )
else()
    message(WARNING "File2String utility is currently unavailable on this host system. This is not an issues unless you modify shaders")
endif()

set_source_files_properties(
    ${SHADERS_INC_LIST}
    PROPERTIES GENERATED TRUE
)
target_sources(DiligentFX PRIVATE
    # A target created in the same directory (CMakeLists.txt file) that specifies any output of the 
    # custom command as a source file is given a rule to generate the file using the command at build time. 
    ${SHADERS_INC_LIST}
)
source_group("generated" FILES
    ${SHADERS_INC_LIST}
)


if(DILIGENT_INSTALL_FX)
    install(TARGETS				 DiligentFX
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/${DILIGENT_FX_DIR}/$<CONFIG>"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/${DILIGENT_FX_DIR}/$<CONFIG>"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/${DILIGENT_FX_DIR}/$<CONFIG>"
    )
    install(DIRECTORY    PostProcess/EpipolarLightScattering/interface
            DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}/${DILIGENT_FX_DIR}/PostProcess/EpipolarLightScattering"
    )
    install(DIRECTORY    Components/interface
            DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}/${DILIGENT_FX_DIR}/Components"
    )
    install(DIRECTORY    GLTF_PBR_Renderer/interface
            DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}/${DILIGENT_FX_DIR}/GLTF_PBR_Renderer"
    )
    install(DIRECTORY    Shaders
            DESTINATION  "."
            FILES_MATCHING PATTERN "public/*.*"
            PATTERN "private" EXCLUDE
    )
endif()

set_target_properties(DiligentFX PROPERTIES
    FOLDER DiligentFX
)

# Create a custom target to run source code formatting validation command
add_format_validation_target(DiligentFX "${CMAKE_CURRENT_SOURCE_DIR}" DiligentFX)
