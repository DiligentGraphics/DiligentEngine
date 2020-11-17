cmake_minimum_required (VERSION 3.6)

project(ImportGLTFViewer CXX)

set(SOURCE
    src/TestScene.cpp
    src/Actor.cpp
    src/Component.cpp
    src/Camera.cpp
    src/Helmet.cpp
    src/Sphere.cpp
    src/GLTFObject.cpp
    src/TexturedCube.cpp
)

set(INCLUDE
    src/TestScene.hpp
    src/Actor.h
    src/Component.h
    src/Camera.h
    src/Helmet.h
    src/Sphere.h
    src/GLTFObject.h
    src/EnvMap.h
    src/TexturedCube.hpp
)

set(SHADERS
    assets/shaders/BoundBox.vsh
    assets/shaders/BoundBox.psh
    assets/shaders/env_map.vsh
    assets/shaders/env_map.psh
)

set(EXTERNAL_SHADERS
    ../../DiligentFX/Shaders/PostProcess/ToneMapping/public/ToneMappingStructures.fxh
    ../../DiligentFX/Shaders/PostProcess/ToneMapping/public/ToneMapping.fxh
    ../../DiligentFX/Shaders/Common/public/BasicStructures.fxh
    ../../DiligentFX/Shaders/Common/public/ShaderUtilities.fxh
)

set(ALL_SHADERS ${SHADERS} ${EXTERNAL_SHADERS})

file(GLOB BOOM_BOX_MODEL LIST_DIRECTORIES false assets/models/BoomBoxWithAxes/*.*)
file(GLOB CESIUM_MAN_MODEL LIST_DIRECTORIES false assets/models/CesiumMan/*.*)
file(GLOB DAMAGED_HELMENT_MODEL LIST_DIRECTORIES false assets/models/DamagedHelmet/*.*)
file(GLOB FLIGHT_HELMENT_MODEL LIST_DIRECTORIES false assets/models/FlightHelmet/*.*)
file(GLOB METAL_ROUGH_SPHERES_MODEL LIST_DIRECTORIES false assets/models/MetalRoughSpheres/*.*)
file(GLOB NORMAL_TANGENT_TEST_MODEL LIST_DIRECTORIES false assets/models/NormalTangentTest/*.*)

set(MODELS ${BOOM_BOX_MODEL} ${CESIUM_MAN_MODEL} ${DAMAGED_HELMENT_MODEL}
           ${FLIGHT_HELMENT_MODEL} ${METAL_ROUGH_SPHERES_MODEL} ${NORMAL_TANGENT_TEST_MODEL})

set(TEXTURES assets/textures/papermill.ktx)

set(ASSETS
    ${MODELS}
    ${TEXTURES}
)

function(add_sample_app APP_NAME IDE_FOLDER SOURCE INCLUDE SHADERS ASSETS)

    set_source_files_properties(${SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")
    set(ALL_ASSETS ${ASSETS} ${SHADERS})
    add_target_platform_app(${APP_NAME} "${SOURCE}" "${INCLUDE}" "${ALL_ASSETS}")

    set_source_files_properties(${ALL_ASSETS} PROPERTIES 
        VS_DEPLOYMENT_LOCATION "."
        MACOSX_PACKAGE_LOCATION "Resources" 
    )

    if(PLATFORM_WIN32)
        set_target_properties(${APP_NAME} PROPERTIES 
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        )
        copy_required_dlls(${APP_NAME})
        append_sample_base_win32_source(${APP_NAME})
    elseif(PLATFORM_UNIVERSAL_WINDOWS)
        append_sample_base_uwp_source(${APP_NAME})
        package_required_dlls(${APP_NAME})
    endif()

    target_include_directories(${APP_NAME}
    PRIVATE	
        src
    )

    target_link_libraries(${APP_NAME}
    PRIVATE
        # On Linux we must have Diligent-NativeAppBase go first, otherwise the linker 
        # will fail to resolve Diligent::CreateApplication() function.
        Diligent-NativeAppBase
        Diligent-BuildSettings
        Diligent-SampleBase
	Diligent-TextureLoader
    )
    set_common_target_properties(${APP_NAME})

    if(MSVC)
        # Disable MSVC-specific warnings
        # - w4201: nonstandard extension used: nameless struct/union
        target_compile_options(${APP_NAME} PRIVATE /wd4201)
    endif()

    set_target_properties(${APP_NAME} PROPERTIES
        FOLDER ${IDE_FOLDER}
    )

    source_group("src" FILES ${SOURCE} ${INCLUDE})
    source_group("assets" FILES ${ALL_ASSETS})	

    target_sources(${APP_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/readme.md")
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/readme.md" PROPERTIES HEADER_FILE_ONLY TRUE
    )

    if(PLATFORM_WIN32 OR PLATFORM_LINUX)
        # Copy assets to target folder
        add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "\"$<TARGET_FILE_DIR:${APP_NAME}>\"")
    endif()

    if(DILIGENT_INSTALL_SAMPLES)
        # Install instructions
        file(RELATIVE_PATH TUTORIAL_REL_PATH "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
        install(TARGETS     ${APP_NAME}
                DESTINATION "${CMAKE_INSTALL_BINDIR}/${TUTORIAL_REL_PATH}/$<CONFIG>")

        if(PLATFORM_LINUX OR PLATFORM_WIN32)
            install(DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/assets/"
                    DESTINATION "${CMAKE_INSTALL_BINDIR}/${TUTORIAL_REL_PATH}/$<CONFIG>")
        endif()

        if(PLATFORM_WIN32)
            get_supported_backends(BACKEND_LIBRARIES)
            install(TARGETS  ${BACKEND_LIBRARIES}
                    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/${TUTORIAL_REL_PATH}/$<CONFIG>"
                    LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}/${TUTORIAL_REL_PATH}/$<CONFIG>"
                    ARCHIVE DESTINATION "${CMAKE_INSTALL_BINDIR}/${TUTORIAL_REL_PATH}/$<CONFIG>")
        endif()

        if(PLATFORM_LINUX)
            set_target_properties(${APP_NAME} PROPERTIES
                INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${DILIGENT_CORE_DIR}/${CMAKE_BUILD_TYPE}"
            )
        endif()
    endif()

endfunction()


add_sample_app("ImportGLTFViewer" "Projects" "${SOURCE}" "${INCLUDE}" "${ALL_SHADERS}" "${ASSETS}")

target_link_libraries(ImportGLTFViewer PRIVATE Diligent-AssetLoader DiligentFX)

target_include_directories(ImportGLTFViewer PRIVATE
    ../../../DiligentFX/Shaders/PostProcess/ToneMapping/public/
)

foreach(FILE ${EXTERNAL_SHADERS})
    # Copy external shaders
    add_custom_command(TARGET ImportGLTFViewer PRE_BUILD 
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
    )
endforeach(FILE)

set_source_files_properties(${ALL_SHADERS} PROPERTIES
    VS_DEPLOYMENT_LOCATION "shaders"
    MACOSX_PACKAGE_LOCATION Resources/shaders
)

set_source_files_properties(${TEXTURES} PROPERTIES
    VS_DEPLOYMENT_LOCATION "textures"
    MACOSX_PACKAGE_LOCATION "Resources/textures"
)

set_source_files_properties(${BOOM_BOX_MODEL} PROPERTIES
    VS_DEPLOYMENT_LOCATION "models/BoomBoxWithAxes"
    MACOSX_PACKAGE_LOCATION "Resources/models/BoomBoxWithAxes"
)

set_source_files_properties(${CESIUM_MAN_MODEL} PROPERTIES
    VS_DEPLOYMENT_LOCATION "models/CesiumMan"
    MACOSX_PACKAGE_LOCATION "Resources/models/CesiumMan"
)

set_source_files_properties(${DAMAGED_HELMENT_MODEL} PROPERTIES
    VS_DEPLOYMENT_LOCATION "models/DamagedHelmet"
    MACOSX_PACKAGE_LOCATION "Resources/models/DamagedHelmet"
)

set_source_files_properties(${FLIGHT_HELMENT_MODEL} PROPERTIES
    VS_DEPLOYMENT_LOCATION "models/FlightHelmet"
    MACOSX_PACKAGE_LOCATION "Resources/models/FlightHelmet"
)

set_source_files_properties(${METAL_ROUGH_SPHERES_MODEL} PROPERTIES
    VS_DEPLOYMENT_LOCATION "models/MetalRoughSpheres"
    MACOSX_PACKAGE_LOCATION "Resources/models/MetalRoughSpheres"
)

set_source_files_properties(${NORMAL_TANGENT_TEST_MODEL} PROPERTIES
    VS_DEPLOYMENT_LOCATION "models/NormalTangentTest"
    MACOSX_PACKAGE_LOCATION "Resources/models/NormalTangentTest"
)

source_group("assets" FILES ${ASSETS})
source_group("assets\\shaders" FILES ${SHADERS})
source_group("assets\\shaders\\external" FILES ${EXTERNAL_SHADERS})
source_group("assets\\textures" FILES ${TEXTURES})
source_group("assets\\models\\BoomBoxWithAxes" FILES ${BOOM_BOX_MODEL})
source_group("assets\\models\\CesiumMan" FILES ${CESIUM_MAN_MODEL})
source_group("assets\\models\\DamagedHelmet" FILES ${DAMAGED_HELMENT_MODEL})
source_group("assets\\models\\FlightHelmet" FILES ${FLIGHT_HELMENT_MODEL})
source_group("assets\\models\\MetalRoughSpheres" FILES ${METAL_ROUGH_SPHERES_MODEL})
source_group("assets\\models\\NormalTangentTest" FILES ${NORMAL_TANGENT_TEST_MODEL})