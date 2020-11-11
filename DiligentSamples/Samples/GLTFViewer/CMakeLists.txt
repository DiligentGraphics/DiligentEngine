cmake_minimum_required (VERSION 3.6)

project(GLTFViewer CXX)

set(SOURCE
    src/GLTFViewer.cpp
)

set(INCLUDE
    src/GLTFViewer.hpp
)

set(SHADERS
    assets/shaders/BoundBox.vsh
    assets/shaders/BoundBox.psh
    assets/shaders/env_map.vsh
    assets/shaders/env_map.psh
)

set(EXTERNAL_SHADERS
    ../../../DiligentFX/Shaders/PostProcess/ToneMapping/public/ToneMappingStructures.fxh
    ../../../DiligentFX/Shaders/PostProcess/ToneMapping/public/ToneMapping.fxh
    ../../../DiligentFX/Shaders/Common/public/BasicStructures.fxh
    ../../../DiligentFX/Shaders/Common/public/ShaderUtilities.fxh
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

add_sample_app("GLTFViewer" "DiligentSamples/Samples" "${SOURCE}" "${INCLUDE}" "${ALL_SHADERS}" "${ASSETS}")
target_link_libraries(GLTFViewer PRIVATE Diligent-AssetLoader DiligentFX)

target_include_directories(GLTFViewer PRIVATE
    ../../../DiligentFX/Shaders/PostProcess/ToneMapping/public/
)

foreach(FILE ${EXTERNAL_SHADERS})
    # Copy external shaders
    add_custom_command(TARGET GLTFViewer PRE_BUILD 
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
