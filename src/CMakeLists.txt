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
    png.cpp
    section.cpp
    settings.cpp
    worldloader.cpp
    VERSION)

ADD_LIBRARY(mcmap_core STATIC ${SOURCES})

LINK_LIBRARIES(format mcmap_core ZLIB::ZLIB PNG::PNG)

ADD_EXECUTABLE(mcmap main.cpp)

IF (OpenMP_FOUND)
    TARGET_LINK_LIBRARIES(mcmap OpenMP::OpenMP_CXX)
ELSE()
    ADD_DEFINITIONS(-DDISABLE_OMP)
ENDIF()

IF (DEBUG_BUILD)
	ADD_DEFINITIONS(-O0 -g3)
ELSE()
	ADD_DEFINITIONS(-O3)
	SET_TARGET_PROPERTIES(mcmap PROPERTIES LINK_FLAGS -s)
ENDIF()

ADD_DEFINITIONS(-Wall -pedantic -D_FILE_OFFSET_BITS=64)
ADD_LINK_OPTIONS(-lstdc++fs)
