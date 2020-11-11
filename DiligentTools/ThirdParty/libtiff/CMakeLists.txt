cmake_minimum_required (VERSION 3.6)

project(LibTiff C)

set(SOURCE 
#	mkg3states.c
    tif_aux.c
    tif_close.c
    tif_codec.c
    tif_color.c
    tif_compress.c
    tif_dir.c
    tif_dirinfo.c
    tif_dirread.c
    tif_dirwrite.c
    tif_dumpmode.c
    tif_error.c
    tif_extension.c
    tif_fax3.c
    tif_fax3sm.c
    tif_flush.c
    tif_getimage.c
    tif_jbig.c
    tif_jpeg.c
    tif_jpeg_12.c
    tif_luv.c
    tif_lzma.c
    tif_lzw.c
    tif_next.c
    tif_ojpeg.c
    tif_open.c
    tif_packbits.c
    tif_pixarlog.c
    tif_predict.c
    tif_print.c
    tif_read.c
    tif_stream.cxx
    tif_strip.c
    tif_swab.c
    tif_thunder.c
    tif_tile.c
    tif_version.c
    tif_warning.c
    tif_write.c
    tif_zip.c
)

if(PLATFORM_WIN32)
    list(APPEND SOURCE tif_win32.c)
elseif(PLATFORM_ANDROID OR PLATFORM_UNIVERSAL_WINDOWS OR PLATFORM_LINUX OR PLATFORM_MACOS OR PLATFORM_IOS)
    list(APPEND SOURCE tif_unix.c)
else()
    message(FATAL_ERROR "Unknown platform")
endif()

set(INCLUDE 
    t4.h
    tif_config.android.h
    tif_config.linux.h
    tif_config.h
    tif_config.h-vms
    tif_config.vc.h
    tif_config.wince.h
    tif_dir.h
    tif_fax3.h
    tif_predict.h
    tiff.h
    tiffconf.linux.h
    tiffconf.h
    tiffconf.vc.h
    tiffconf.wince.h
    tiffio.h
    tiffio.hxx
    tiffiop.h
    tiffvers.h
    uvcode.h
)

add_library(LibTiff STATIC ${SOURCE} ${INCLUDE})
set_common_target_properties(LibTiff)

if(MSVC)
    # Disable MSVC-specific warnings
    target_compile_definitions(LibTiff PRIVATE -D_CRT_SECURE_NO_WARNINGS)
    target_compile_options(LibTiff PRIVATE /wd4244 /wd4456 /wd4457 /wd4706)

    if(PLATFORM_UNIVERSAL_WINDOWS)
        target_compile_definitions(LibTiff PRIVATE -D_CRT_NONSTDC_NO_WARNINGS)
        target_compile_options(LibTiff PRIVATE /wd4311 /wd4312)
    endif()
endif(MSVC)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
    CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Suppress the following warnings in tif_unix.c
    #   cast from pointer to integer of different size [-Wpointer-to-int-cast]
    # and
    #   cast to pointer from integer of different size [-Wint-to-pointer-cast]
    set_source_files_properties(
        tif_unix.c
        PROPERTIES 
        COMPILE_FLAGS "-Wno-pointer-to-int-cast -Wno-int-to-pointer-cast"
    )
endif()

target_link_libraries(LibTiff PRIVATE Diligent-BuildSettings)

target_include_directories(LibTiff 
    PUBLIC .
)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})

set_target_properties(LibTiff PROPERTIES
    FOLDER DiligentTools/ThirdParty
)
