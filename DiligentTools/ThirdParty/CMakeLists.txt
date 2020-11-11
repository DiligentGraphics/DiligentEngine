cmake_minimum_required (VERSION 3.6)

add_subdirectory(libjpeg-9a)
add_subdirectory(libtiff)
add_subdirectory(zlib-1.2.8)
add_subdirectory(lpng-1.6.17)

set(LICENSE_INSTALL_PATH "Licenses/ThirdParty/${DILIGENT_TOOLS_DIR}")

install(FILES libjpeg-9a/README DESTINATION ${LICENSE_INSTALL_PATH} RENAME libjpeg-readme.txt)
install(FILES libtiff/COPYRIGHT DESTINATION ${LICENSE_INSTALL_PATH} RENAME libtiff-copyright.txt)
install(FILES lpng-1.6.17/LICENSE DESTINATION ${LICENSE_INSTALL_PATH} RENAME lpng-license.txt)
install(FILES tinygltf/LICENSE DESTINATION ${LICENSE_INSTALL_PATH} RENAME tinygltf-license.txt)
install(FILES zlib-1.2.8/README DESTINATION ${LICENSE_INSTALL_PATH} RENAME zlib-readme.txt)
install(FILES imgui/LICENSE.txt DESTINATION ${LICENSE_INSTALL_PATH} RENAME imgui-license.txt)
install(FILES imGuIZMO.quat/license.txt DESTINATION ${LICENSE_INSTALL_PATH} RENAME imGuIZMO.quat-license.txt)

if(DILIGENT_INSTALL_TOOLS)
    set(THIRD_PARTY_TARGETS LibJpeg LibPng LibTiff ZLib)
    install(TARGETS ${THIRD_PARTY_TARGETS}
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/${DILIGENT_TOOLS_DIR}/$<CONFIG>"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/${DILIGENT_TOOLS_DIR}/$<CONFIG>"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/${DILIGENT_TOOLS_DIR}/$<CONFIG>"
    )

    list(APPEND DILIGENT_TOOLS_INSTALL_LIBS_LIST ${THIRD_PARTY_TARGETS})
    set(DILIGENT_TOOLS_INSTALL_LIBS_LIST ${DILIGENT_TOOLS_INSTALL_LIBS_LIST} CACHE INTERNAL "Diligent tools libraries installation list")

    install(
        FILES
            imgui/imconfig.h
            imgui/imgui.h
            imgui/imgui_internal.h
            imgui/imstb_rectpack.h
            imgui/imstb_textedit.h
            imgui/imstb_truetype.h
        DESTINATION
            "${CMAKE_INSTALL_INCLUDEDIR}/${DILIGENT_TOOLS_DIR}/ThirdParty/imgui"
    )
endif()
