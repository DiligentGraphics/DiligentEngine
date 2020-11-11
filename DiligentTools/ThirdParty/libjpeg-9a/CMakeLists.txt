cmake_minimum_required (VERSION 3.6)

project(LibJpeg C)

set(SOURCE 
    jaricom.c
    jcapimin.c
    jcapistd.c
    jcarith.c
    jccoefct.c
    jccolor.c
    jcdctmgr.c
    jchuff.c
    jcinit.c
    jcmainct.c
    jcmarker.c
    jcmaster.c
    jcomapi.c
    jcparam.c
    jcprepct.c
    jcsample.c
    jctrans.c
    jdapimin.c
    jdapistd.c
    jdarith.c
    jdatadst.c
    jdatasrc.c
    jdcoefct.c
    jdcolor.c
    jddctmgr.c
    jdhuff.c
    jdinput.c
    jdmainct.c
    jdmarker.c
    jdmaster.c
    jdmerge.c
    jdpostct.c
    jdsample.c
    jdtrans.c
    jerror.c
    jfdctflt.c
    jfdctfst.c
    jfdctint.c
    jidctflt.c
    jidctfst.c
    jidctint.c
    jmemmgr.c
    jmemnobs.c
    jquant1.c
    jquant2.c
    jutils.c
)

set(INCLUDE 
    jconfig.h
    jdct.h
    jerror.h
    jinclude.h
    jmemsys.h
    jmorecfg.h
    jpegint.h
    jpeglib.h
    jversion.h
)

add_library(LibJpeg STATIC ${SOURCE} ${INCLUDE})
set_common_target_properties(LibJpeg)

target_include_directories(LibJpeg PUBLIC .)

target_link_libraries(LibJpeg PRIVATE Diligent-BuildSettings)

if(MSVC)
    # Disable MSVC-specific warnings
    # - w4244: '+=': conversion from 'int' to 'JCOEF', possible loss of data
    # - w4127: conditional expression is constant
    target_compile_options(LibJpeg PRIVATE /wd4244 /wd4127)
    target_compile_definitions(LibJpeg PRIVATE -D_CRT_SECURE_NO_WARNINGS)
    if(PLATFORM_UNIVERSAL_WINDOWS)
        target_compile_definitions(LibJpeg PRIVATE -DNO_GETENV)
    endif()
endif(MSVC)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})

set_target_properties(LibJpeg PROPERTIES
    FOLDER DiligentTools/ThirdParty
)