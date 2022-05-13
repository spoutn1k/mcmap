SET(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(Qt5_FOUND)
    SET(CMAKE_AUTOUIC ON)
    SET(CMAKE_AUTOMOC ON)
    SET(CMAKE_AUTORCC ON)

    SET(ICON_INDEX_FILE icons.qrc)

    FUNCTION(INDEX_RESOURCES OUTPUT PATH)
        FILE(GLOB RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${PATH}/*)
        FILE(WRITE ${OUTPUT} "<!DOCTYPE RCC><RCC version=\"1.0\"><qresource>")
        FOREACH(RESOURCE ${RESOURCES})
            FILE(APPEND ${OUTPUT} "<file>${RESOURCE}</file>")
        ENDFOREACH()
        FILE(APPEND ${OUTPUT} "</qresource></RCC>")
    ENDFUNCTION()

    INDEX_RESOURCES(${ICON_INDEX_FILE} icons)

    SET(GUI_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        ${ICON_INDEX_FILE})

    IF(NOT WIN32)
        ADD_EXECUTABLE(mcmap-gui ${GUI_SOURCES})
    ELSE()
        ADD_EXECUTABLE(mcmap-gui WIN32 ${GUI_SOURCES})
    ENDIF()

    TARGET_LINK_LIBRARIES(mcmap-gui
        PRIVATE
        Qt5::Widgets
        ZLIB::ZLIB
        PNG::PNG
        fmt::fmt-header-only
        mcmap_core)
ENDIF()
