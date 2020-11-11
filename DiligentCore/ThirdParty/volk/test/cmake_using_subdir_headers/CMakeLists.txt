# Include the volk target through add_subdirectory.

cmake_minimum_required(VERSION 3.0)
project(volk_test LANGUAGES C)

# Include volk as part of the build tree to make the target known.
# The two-argument version of add_subdirectory allows adding non-subdirs.
add_subdirectory(../.. volk)

add_executable(volk_test main.c)
target_link_libraries(volk_test PRIVATE volk_headers)
