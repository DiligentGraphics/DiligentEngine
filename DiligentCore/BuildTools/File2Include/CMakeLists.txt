cmake_minimum_required (VERSION 3.6)

if(PLATFORM_WIN32 OR PLATFORM_LINUX OR PLATFORM_MACOS)
    project(File2Include)

    set(SOURCE 
        File2String.cpp
    )

    add_executable(File2String ${SOURCE})
    target_compile_features(File2String PRIVATE cxx_std_11)

    if(PLATFORM_WIN32)
        if(MSVC)
            set(MSVC_DBG_COMPILE_OPTIONS /MTd)
            set(MSVC_REL_COMPILE_OPTIONS /MT)
            target_compile_options(File2String PRIVATE /wd4996)
            foreach(DBG_CONFIG ${DEBUG_CONFIGURATIONS})
                target_compile_options(File2String PRIVATE "$<$<CONFIG:${DBG_CONFIG}>:${MSVC_DBG_COMPILE_OPTIONS}>")
            endforeach()

            foreach(REL_CONFIG ${RELEASE_CONFIGURATIONS})
                target_compile_options(File2String PRIVATE "$<$<CONFIG:${REL_CONFIG}>:${MSVC_REL_COMPILE_OPTIONS}>")
            endforeach()
        endif()
        set(DST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin/Win32/x${ARCH}/")
    elseif(PLATFORM_LINUX)
        set(DST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin/Linux/")
    elseif(PLATFORM_MACOS)
        set(DST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin/MacOS/")
    endif()
    source_group("source" FILES ${SOURCE})

    set_target_properties(File2String PROPERTIES
        FOLDER DiligentCore/BuildTools
    )

    # When setting RUNTIME_OUTPUT_DIRECTORY_RELEASE to DST_DIR,
    # the executable is deleted by the clean build command
    add_custom_command(TARGET File2String POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "\"$<TARGET_FILE:File2String>\""
        # In release build, copy to the destination folder
        # In all other configurations, do nothing by copying to the same target folder
        $<IF:$<CONFIG:RELEASE>,${DST_DIR},$<TARGET_FILE_DIR:File2String>>
    )
endif()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    # Use prebuilt 32-bit version
    set(FILE2STRING_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin/Win32/x32/File2String.exe" CACHE INTERNAL "File2String utility")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    # Use prebuilt version
    set(FILE2STRING_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin/Linux/File2String" CACHE INTERNAL "File2String utility")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    # Use prebuilt version
    set(FILE2STRING_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin/MacOS/File2String" CACHE INTERNAL "File2String utility")
else()
    set(FILE2STRING_PATH "" CACHE INTERNAL "File2String utility unavailable on this host system")
endif()
