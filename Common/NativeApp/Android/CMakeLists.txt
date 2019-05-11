cmake_minimum_required (VERSION 3.6)

include(AndroidNdkModules)
android_ndk_import_module_native_app_glue()
target_include_directories(native_app_glue INTERFACE ${ANDROID_NDK}/sources/android/native_app_glue)
set_common_target_properties(native_app_glue)

add_subdirectory(ndk_helper)
