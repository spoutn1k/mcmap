LINK_LIBRARIES(
    ZLIB::ZLIB
    fmt::fmt-header-only
    spdlog::spdlog_header_only)

ADD_EXECUTABLE(json2bson json2bson.cpp)

IF(NOT WIN32)
    ADD_EXECUTABLE(nbt2json nbt2json.cpp)
    ADD_EXECUTABLE(regionReader regionReader.cpp)
    ADD_EXECUTABLE(extractChunk extractChunk.cpp)
    ADD_EXECUTABLE(regionMover regionMove.cpp)
    TARGET_LINK_LIBRARIES(regionMover mcmap_core)
    ADD_EXECUTABLE(chunkPos chunkPos.cpp)
ENDIF()
