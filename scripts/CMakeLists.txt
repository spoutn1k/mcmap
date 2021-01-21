LINK_LIBRARIES(format ZLIB::ZLIB)

ADD_EXECUTABLE(json2bson json2bson.cpp)
ADD_EXECUTABLE(nbt2json nbt2json.cpp)
ADD_EXECUTABLE(regionReader regionReader.cpp)
ADD_EXECUTABLE(extractChunk extractChunk.cpp)
ADD_EXECUTABLE(levelInfo levelInfo.cpp)
