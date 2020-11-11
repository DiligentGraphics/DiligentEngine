cmake_minimum_required (VERSION 3.6)

project(ZLib C)

set(ZLIB_SOURCE 
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    infback.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    uncompr.c
    zutil.c
)

set(ZLIB_INCLUDE 
    crc32.h
    deflate.h
    gzguts.h
    inffast.h
    inffixed.h
    inflate.h
    inftrees.h
    trees.h
    zconf.h
    zlib.h
    zutil.h
)

add_library(ZLib STATIC ${ZLIB_SOURCE} ${ZLIB_INCLUDE})
set_common_target_properties(ZLib)

target_link_libraries(ZLib PRIVATE Diligent-BuildSettings)

if(MSVC)
    target_compile_definitions(ZLib PRIVATE -D_CRT_SECURE_NO_DEPRECATE)
    target_compile_options(ZLib PRIVATE /W3 /wd4131 /wd4127 /wd4244 /wd4996)
endif()

if(PLATFORM_LINUX OR PLATFORM_ANDROID OR PLATFORM_MACOS OR PLATFORM_IOS)
    target_compile_definitions(ZLib PRIVATE HAVE_UNISTD_H)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR 
    CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # For inflate.c, disable the following
    # warning: shifting a negative signed value is undefined [-Wshift-negative-value]
    set_source_files_properties(inflate.c PROPERTIES COMPILE_FLAGS -Wno-shift-negative-value)
endif()


target_include_directories(ZLib PUBLIC .)

source_group("src" FILES ${ZLIB_SOURCE})
source_group("include" FILES ${ZLIB_INCLUDE})

set_target_properties(ZLib PROPERTIES
    FOLDER DiligentTools/ThirdParty
)


option(ZLIB_BUILD_MINIZIP "Build minizip library" OFF)
if(${ZLIB_BUILD_MINIZIP})
    set(MINIZIP_SOURCE
       contrib/minizip/ioapi.c
       contrib/minizip/iowin32.c
       contrib/minizip/mztools.c
       contrib/minizip/unzip.c
       contrib/minizip/zip.c
    )
    set(MINIZIP_INCLUDE
       contrib/minizip/crypt.h
       contrib/minizip/ioapi.h
       contrib/minizip/iowin32.h
       contrib/minizip/mztools.h
       contrib/minizip/unzip.h
       contrib/minizip/zip.h
    )

    add_library(MiniZip STATIC ${MINIZIP_SOURCE} ${MINIZIP_INCLUDE})
    set_common_target_properties(MiniZip)

    target_link_libraries(MiniZip PRIVATE Diligent-BuildSettings PUBLIC ZLib)

    if(MSVC)
        target_compile_definitions(MiniZip PRIVATE -D_CRT_SECURE_NO_DEPRECATE)
        target_compile_options(MiniZip PRIVATE /W3 /wd4131 /wd4189 /wd4456 /wd4244 /wd4701 /wd4703)
    endif()

    target_include_directories(MiniZip PUBLIC contrib/minizip/)

    source_group("src" FILES ${MINIZIP_SOURCE})
    source_group("include" FILES ${MINIZIP_INCLUDE})

    set_target_properties(MiniZip PROPERTIES
        FOLDER DiligentTools/ThirdParty
    )
endif()
