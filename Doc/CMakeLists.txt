cmake_minimum_required (VERSION 3.10)

set(DOXYGEN_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/doxygen.cfg")
set(MD_PAGES_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/md_pages.py")
set(PAGES_DOX "${CMAKE_CURRENT_SOURCE_DIR}/pages.dox")

add_custom_target(DiligentDocs
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.." # Need to run in the root of the repo
    COMMAND python "${MD_PAGES_SCRIPT}" "${PAGES_DOX}"
    COMMAND doxygen "${DOXYGEN_CONFIG_FILE}"
    COMMENT "Building documentation..."
)
