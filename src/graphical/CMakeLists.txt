SET(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(CMAKE_AUTOUIC ON)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)

SET(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    logwindow.cpp
    logwindow.h
    mainwindow.ui)

IF(Qt5_FOUND)
    IF(NOT WIN32)
        ADD_EXECUTABLE(mcmap-gui ${PROJECT_SOURCES})
    ELSE()
        ADD_EXECUTABLE(mcmap-gui WIN32 ${PROJECT_SOURCES})
    ENDIF()

    TARGET_LINK_LIBRARIES(mcmap-gui
        PRIVATE
        Qt5::Widgets
        format
        mcmap_core
        ZLIB::ZLIB
        PNG::PNG)
ENDIF()
