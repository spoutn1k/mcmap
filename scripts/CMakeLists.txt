LINK_LIBRARIES(
    ZLIB::ZLIB
    fmt::fmt-header-only
    spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json)

ADD_EXECUTABLE(json2bson json2bson.cpp)

IF(NOT WIN32)
    ADD_EXECUTABLE(nbt2json nbt2json.cpp)
    ADD_EXECUTABLE(regionReader regionReader.cpp)
    ADD_EXECUTABLE(extractChunk extractChunk.cpp)
    ADD_EXECUTABLE(chunkPos chunkPos.cpp)
ENDIF()
