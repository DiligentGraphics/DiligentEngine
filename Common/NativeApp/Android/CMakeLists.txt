cmake_minimum_required (VERSION 3.6)

add_library(native_app_glue ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_link_libraries(native_app_glue log)
target_include_directories(native_app_glue INTERFACE ${ANDROID_NDK}/sources/android/native_app_glue)
set_common_target_properties(native_app_glue)

add_subdirectory(ndk_helper)
