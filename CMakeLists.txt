CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(mcmap LANGUAGES CXX)

OPTION(HOST_DEBUG "Debug build" OFF)

FIND_PACKAGE(OpenMP)
FIND_PACKAGE(PNG REQUIRED)

ADD_DEFINITIONS(-std=c++17 -Wall -pedantic -D_FILE_OFFSET_BITS=64 -Isrc/include)

ADD_EXECUTABLE(json2bson scripts/json2bson.cpp src/include/fmt/format.default.o)

ADD_CUSTOM_COMMAND(OUTPUT src/colors.bson
    PRE_BUILD
    COMMAND json2bson src/colors.json > src/colors.bson
    MAIN_DEPENDENCY src/colors.json
    DEPENDS src/colors.json)

FILE(GLOB SOURCES src/*.cpp src/include/fmt/format.cpp)

ADD_EXECUTABLE(mcmap ${SOURCES} src/colors.bson)

IF (OpenMP_FOUND)
    TARGET_LINK_LIBRARIES(mcmap OpenMP::OpenMP_CXX)
ELSE()
    ADD_DEFINITIONS(-DDISABLE_OMP)
ENDIF()

IF(HOST_DEBUG)
	ADD_DEFINITIONS(-O0 -g3)
ELSE()
	ADD_DEFINITIONS(-O3 -s -fomit-frame-pointer)
	SET_TARGET_PROPERTIES(mcmap PROPERTIES LINK_FLAGS -s)
ENDIF()

TARGET_LINK_LIBRARIES(mcmap PNG::PNG)

INSTALL(TARGETS mcmap)
