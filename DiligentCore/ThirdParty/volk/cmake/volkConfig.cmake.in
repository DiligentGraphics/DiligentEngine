get_filename_component(volk_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

if(NOT TARGET volk::volk)
  include("${volk_CMAKE_DIR}/volkTargets.cmake")
endif()

# Mirror the default behaviour of the respective option.
if(NOT DEFINED VOLK_PULL_IN_VULKAN)
    set(VOLK_PULL_IN_VULKAN ON)
endif()

if(VOLK_PULL_IN_VULKAN)
  find_package(Vulkan QUIET)
  if(TARGET Vulkan::Vulkan) 
    add_dependencies(volk::volk Vulkan::Vulkan)
    add_dependencies(volk::volk_headers Vulkan::Vulkan)
  elseif(DEFINED ENV{VULKAN_SDK})
    target_include_directories(volk::volk INTERFACE "$ENV{VULKAN_SDK}/include")
    target_include_directories(volk::volk_headers INTERFACE "$ENV{VULKAN_SDK}/include")
  endif()
endif()
