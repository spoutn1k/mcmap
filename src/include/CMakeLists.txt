GET_FILE (
    https://github.com/nlohmann/json/releases/download/v3.9.1/json.hpp
    ${CMAKE_SOURCE_DIR}/src/include/json.hpp
    MD5=5eabadfb8cf8fe1bf0811535c65f027f
)

ADD_SUBDIRECTORY(fmt)
