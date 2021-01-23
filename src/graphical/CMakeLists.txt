SET(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(CMAKE_AUTOUIC ON)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)

FIND_PACKAGE(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools)
FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools)

SET(TS_FILES mcmap-gui_en_US.ts)

SET(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    ${TS_FILES}
    )

IF(QT_FOUND)
    IF(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
        qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

        qt_add_executable(mcmap-gui ${PROJECT_SOURCES})
    ELSE()
        qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

        ADD_EXECUTABLE(mcmap-gui ${PROJECT_SOURCES})
    ENDIF()

    TARGET_LINK_LIBRARIES(mcmap-gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets format mcmap_core ZLIB::ZLIB PNG::PNG)
ENDIF()
