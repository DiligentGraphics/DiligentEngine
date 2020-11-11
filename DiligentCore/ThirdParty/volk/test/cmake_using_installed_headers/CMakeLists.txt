cmake_minimum_required(VERSION 3.0)
project(volk_test LANGUAGES C)

# Include volk from a CMake package config.
# CMAKE_PREFIX_PATH or volk_DIR must be set properly.
find_package(volk CONFIG REQUIRED)

add_executable(volk_test main.c)
target_link_libraries(volk_test PRIVATE volk::volk_headers)
