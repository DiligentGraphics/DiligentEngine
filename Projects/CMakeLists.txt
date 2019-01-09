cmake_minimum_required (VERSION 3.3)

if(PLATFORM_WIN32 AND D3D11_SUPPORTED AND D3D12_SUPPORTED)
	add_subdirectory(Asteroids)
endif()
