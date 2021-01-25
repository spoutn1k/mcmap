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
    colors.cpp
    helper.cpp
    mcmap.cpp
    png.cpp
    savefile.cpp
    section.cpp
    settings.cpp
    worldloader.cpp
    VERSION)

ADD_LIBRARY(mcmap_core STATIC ${SOURCES})
TARGET_LINK_LIBRARIES(mcmap_core format ZLIB::ZLIB PNG::PNG)

IF (OpenMP_FOUND)
    TARGET_LINK_LIBRARIES(mcmap_core OpenMP::OpenMP_CXX)
ENDIF()
