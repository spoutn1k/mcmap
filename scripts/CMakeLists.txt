LINK_LIBRARIES(format ZLIB::ZLIB)

ADD_EXECUTABLE(json2bson json2bson.cpp)

IF(NOT WIN32)
    ADD_EXECUTABLE(nbt2json nbt2json.cpp)
    ADD_EXECUTABLE(regionReader regionReader.cpp)
    ADD_EXECUTABLE(extractChunk extractChunk.cpp)
    ADD_EXECUTABLE(regionMover regionMove.cpp)
    TARGET_LINK_LIBRARIES(regionMover mcmap_core)
ENDIF()
