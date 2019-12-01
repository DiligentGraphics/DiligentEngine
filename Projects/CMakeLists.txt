cmake_minimum_required (VERSION 3.3)

if(PLATFORM_WIN32 AND D3D11_SUPPORTED AND D3D12_SUPPORTED)
    if(TARGET Diligent-TextureLoader)
	    add_subdirectory(Asteroids)
    else()
        message("Unable to find Diligent-TextureLoader target: Asteroids demo will be disabled")
    endif()
endif()
