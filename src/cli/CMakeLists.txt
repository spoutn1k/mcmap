ADD_EXECUTABLE(mcmap-cli cli.cpp parse.cpp)
TARGET_LINK_LIBRARIES(mcmap-cli mcmap_core)

IF (NOT DEBUG_BUILD)
    SET_TARGET_PROPERTIES(mcmap-cli PROPERTIES LINK_FLAGS -s)
ENDIF()
