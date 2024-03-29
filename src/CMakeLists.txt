FILE(GLOB JSON colors.json)
SET(BSON ${CMAKE_CURRENT_SOURCE_DIR}/colors.bson CACHE FILEPATH "Embedded color file, generated from colors.json")

SET(ADDITIONAL_CLEAN_FILES ${ADDITIONAL_CLEAN_FILES} ${BSON})

ADD_CUSTOM_COMMAND(OUTPUT ${BSON}
    COMMAND json2bson ${JSON} > ${BSON}
    DEPENDS ${JSON})

SET(SOURCES ${SOURCES}
    ${BSON}
    blocktypes.def
    block_drawers.cpp
    canvas.cpp
    chunk.cpp
    colors.cpp
    helper.cpp
    mcmap.cpp
    png.cpp
    savefile.cpp
    section.cpp
    settings.cpp
    worldloader.cpp
    chunk_format_versions/assert.cpp
    chunk_format_versions/get_section.cpp
    chunk_format_versions/section_format.cpp
    VERSION)

ADD_LIBRARY(mcmap_core STATIC ${SOURCES})
TARGET_LINK_LIBRARIES(
    mcmap_core
    ZLIB::ZLIB
    PNG::PNG
    fmt::fmt-header-only
    spdlog::spdlog_header_only)

IF (Git_FOUND)
    SET_SOURCE_FILES_PROPERTIES(
        mcmap.cpp
        PROPERTIES COMPILE_DEFINITIONS
        SCM_COMMIT="${GIT_DESCRIBE}")
ENDIF()

IF (OpenMP_FOUND)
    TARGET_LINK_LIBRARIES(
        mcmap_core
        OpenMP::OpenMP_CXX)
ENDIF()

ADD_SUBDIRECTORY(cli)
ADD_SUBDIRECTORY(graphical)
ADD_SUBDIRECTORY(include)
