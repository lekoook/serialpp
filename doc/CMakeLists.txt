find_package(Doxygen REQUIRED COMPONENTS dot)
if (Doxygen_FOUND)
    set(FILEPATH_README_SRC ${PROJECT_SOURCE_DIR}/README.md)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/v${PROJECT_VERSION})
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)

    doxygen_add_docs(doc
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/examples
        ${FILEPATH_README_SRC}
        ALL
        COMMENT "Generating API documentation with Doxygen"
    )
else()
    message(STATUS "Doxygen not found, documentation will not be generated.")
endif()
