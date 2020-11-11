# Include the volk target through add_subdirectory, use the static lib target.
# We must set platform defines.
# By default, Vulkan is pulled in as transitive dependency if found.

cmake_minimum_required(VERSION 3.0)
project(volk_test LANGUAGES C)

# Set a suitable platform define to compile volk with.
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
endif()

# Include volk as part of the build tree to make the target known.
# The two-argument version of add_subdirectory allows adding non-subdirs.
add_subdirectory(../.. volk)

add_executable(volk_test main.c)
target_link_libraries(volk_test PRIVATE volk)
