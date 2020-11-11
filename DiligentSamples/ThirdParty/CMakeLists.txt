cmake_minimum_required (VERSION 3.6)

file(RELATIVE_PATH SAMPLES_DIR "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
install(FILES nuklear/Readme.md DESTINATION "Licenses/ThirdParty/${SAMPLES_DIR}" RENAME Nuklear-Readme.md)
