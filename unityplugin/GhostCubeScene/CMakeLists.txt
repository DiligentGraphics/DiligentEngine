cmake_minimum_required (VERSION 3.6)

project(GhostCubeScene CXX)

set(SOURCE
	src/GhostCubeScene.cpp
)

set(INCLUDE
	src/GhostCubeScene.h
)

set(SHADERS
	build/assets/shaders/Mirror.psh
	build/assets/shaders/Mirror.vsh
)
set(ASSETS ${SHADERS})
set_source_files_properties(${SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")


if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS)
	list(APPEND SOURCE src/GhostCubeSceneResTrsnHelper.cpp)
	list(APPEND INCLUDE src/GhostCubeSceneResTrsnHelper.h)
elseif(PLATFORM_ANDROID)
	list(APPEND SOURCE src/Android/AndroidMain.cpp)
endif()

if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS)

	add_executable(GhostCubeScene WIN32 ${SOURCE} ${INCLUDE} ${ASSETS})
	
	if(PLATFORM_WIN32)
		set_target_properties(GhostCubeScene PROPERTIES 
			VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/assets"
		)
	
		add_custom_command(TARGET GhostCubeScene POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:GhostCubePlugin-shared>\""
				"\"$<TARGET_FILE_DIR:GhostCubeScene>\"")

	elseif(PLATFORM_UNIVERSAL_WINDOWS)
		
		# We cannot use genreator expression to add a file because the file name
		# must not depend on the configuration
		set(PLUGIN_REL_DLL ${CMAKE_CURRENT_BINARY_DIR}/GhostCubePlugin_${ARCH}r.dll)
		set(PLUGIN_DBG_DLL ${CMAKE_CURRENT_BINARY_DIR}/GhostCubePlugin_${ARCH}d.dll)

		set(PLUGIN_DLLS ${PLUGIN_REL_DLL} ${PLUGIN_DBG_DLL})
		set_source_files_properties(${PLUGIN_DLLS} PROPERTIES GENERATED TRUE)
		set_source_files_properties(${PLUGIN_DBG_DLL} PROPERTIES 
			VS_DEPLOYMENT_CONTENT $<CONFIG:Debug>
			VS_DEPLOYMENT_LOCATION "assets"
		)
		set_source_files_properties(${PLUGIN_REL_DLL} PROPERTIES 
			VS_DEPLOYMENT_CONTENT $<NOT:$<CONFIG:Debug>>
			VS_DEPLOYMENT_LOCATION "assets"
		)

		add_custom_command(TARGET GhostCubeScene PRE_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"\"$<TARGET_FILE:GhostCubePlugin-shared>\""
				"${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:GhostCubePlugin-shared>"
		)

		target_sources(GhostCubeScene PRIVATE ${PLUGIN_DLLS})
		source_group("assets" FILES ${PLUGIN_DLLS})
		
		set_source_files_properties(${SHADERS} PROPERTIES 
			VS_DEPLOYMENT_CONTENT 1
			VS_DEPLOYMENT_LOCATION "assets/shaders"
		)

	endif()

elseif(PLATFORM_ANDROID)
	
	list(APPEND SOURCE src/Android/AndroidMain.cpp src/LoadPluginFunctions.cpp)
	add_library(GhostCubeScene SHARED ${SOURCE} ${INCLUDE})

elseif(PLATFORM_LINUX)

	list(APPEND SOURCE src/LoadPluginFunctions.cpp)
	add_executable(GhostCubeScene ${SOURCE} ${INCLUDE} ${ASSETS})
	
#elseif(APPLE)
	#add_executable(GhostCubeScene MACOSX_BUNDLE Main.cpp)
else()
	message(FATAL_ERROR "Unknown platform")
endif()

add_dependencies(GhostCubeScene GhostCubePlugin-shared)

if(PLATFORM_UNIVERSAL_WINDOWS)
	get_emulator_uwp_source(UWP_SOURCE UWP_INCLUDE UWP_INCLUDE_DIR)
	source_group("UWP\\src" FILES ${UWP_SOURCE})
	source_group("UWP\\include" FILES ${UWP_INCLUDE})
	target_include_directories(GhostCubeScene PRIVATE ${UWP_INCLUDE_DIR})
	target_sources(GhostCubeScene PRIVATE ${UWP_SOURCE} ${UWP_INCLUDE})
endif()


target_include_directories(GhostCubeScene
PRIVATE
	../GhostCubePlugin/PluginSource/src/Unity
)

target_link_libraries(GhostCubeScene
PRIVATE
	BuildSettings
	UnityEmulator
	TargetPlatform
)
if(PLATFORM_ANDROID)
	target_link_libraries(GhostCubeScene PRIVATE GhostCubePlugin-shared)
elseif (PLATFORM_LINUX)
	target_link_libraries(GhostCubeScene PRIVATE GhostCubePlugin-shared GL X11)
endif()

set_common_target_properties(GhostCubeScene)

if(MSVC)
	# Disable MSVC-specific warnings
	# - w4201: nonstandard extension used: nameless struct/union
	target_compile_options(GhostCubeScene PRIVATE /wd4201)
	
	if(PLATFORM_UNIVERSAL_WINDOWS)
		# Consume the windows runtime extensions
		target_compile_options(GhostCubeScene PRIVATE /ZW)
	endif()
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("assets\\shaders" FILES ${SHADERS})

set_target_properties(GhostCubeScene PROPERTIES
	FOLDER Unity
)
