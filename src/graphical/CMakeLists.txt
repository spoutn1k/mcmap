SET(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(CMAKE_AUTOUIC ON)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)

SET(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui)

IF(QT_FOUND)
    IF(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
        QT_ADD_EXECUTABLE(mcmap-gui ${PROJECT_SOURCES})
    ELSE()
        ADD_EXECUTABLE(mcmap-gui ${PROJECT_SOURCES})
    ENDIF()

    TARGET_LINK_LIBRARIES(mcmap-gui
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        format
        mcmap_core
        ZLIB::ZLIB
        PNG::PNG)
ENDIF()
