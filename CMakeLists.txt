CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(mcmap LANGUAGES CXX VERSION 3.0.1)

OPTION(HOST_DEBUG "Debug build" OFF)
OPTION(NBT_TOOLS "Build NBT tools" OFF)
OPTION(TESTS_ "Run tests" OFF)
OPTION(PROFILING "Run tests" OFF)

FIND_PACKAGE(OpenMP)
FIND_PACKAGE(PNG REQUIRED)
FIND_PACKAGE(ZLIB REQUIRED)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(SOURCES ${SOURCES}
    src/blocktypes.def
    src/block_drawers.cpp
    src/canvas.cpp
    src/colors.cpp
    src/colors.bson
    src/helper.cpp
    src/png.cpp
    src/section.cpp
    src/settings.cpp
    src/worldloader.cpp
    src/VERSION
    src/include/fmt/format.cpp)

INCLUDE_DIRECTORIES(src/include)

ADD_DEFINITIONS(-Wall -pedantic -D_FILE_OFFSET_BITS=64)
LINK_LIBRARIES(ZLIB::ZLIB PNG::PNG)
ADD_LINK_OPTIONS(-lstdc++fs)

ADD_EXECUTABLE(json2bson scripts/json2bson.cpp src/include/fmt/format.cpp)

ADD_CUSTOM_COMMAND(OUTPUT src/colors.bson
    PRE_BUILD
    COMMAND json2bson src/colors.json > src/colors.bson
    MAIN_DEPENDENCY src/colors.json
    DEPENDS src/colors.json)

IF (TESTS_)
    FIND_PACKAGE(GTest REQUIRED)
    FILE(GLOB TEST tests/*cpp)
    ADD_EXECUTABLE(run_tests ${TEST} ${SOURCES})
    TARGET_LINK_LIBRARIES(run_tests GTest::gtest GTest::gtest_main)
ENDIF ()

ADD_EXECUTABLE(mcmap src/main.cpp ${SOURCES})

IF (OpenMP_FOUND)
    TARGET_LINK_LIBRARIES(mcmap OpenMP::OpenMP_CXX)
ELSE()
    ADD_DEFINITIONS(-DDISABLE_OMP)
ENDIF()

IF (HOST_DEBUG)
	ADD_DEFINITIONS(-O0 -g3)
ELSE()
	ADD_DEFINITIONS(-O3)
	SET_TARGET_PROPERTIES(mcmap PROPERTIES LINK_FLAGS -s)
ENDIF()

IF (PROFILING)
	ADD_DEFINITIONS(-g3 -pg)
	SET_TARGET_PROPERTIES(mcmap PROPERTIES LINK_FLAGS -pg)
ELSE()
	ADD_DEFINITIONS(-fomit-frame-pointer)
ENDIF()

INSTALL(TARGETS mcmap)

IF (NBT_TOOLS)
    ADD_EXECUTABLE(nbt2json scripts/nbt2json.cpp src/include/fmt/format.cpp)
    ADD_EXECUTABLE(regionReader scripts/regionReader.cpp src/include/fmt/format.cpp)
    ADD_EXECUTABLE(extractChunk scripts/extractChunk.cpp src/include/fmt/format.cpp)
ENDIF ()
