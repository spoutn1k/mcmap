SET(FMT_VERSION 8.0.0)

GET_FILE(
    https://raw.githubusercontent.com/fmtlib/fmt/${FMT_VERSION}/include/fmt/core.h 
    ${CMAKE_SOURCE_DIR}/src/include/fmt/core.h 
    MD5=8eff5dd4e380f63ebdb50936b407d741
)

GET_FILE(
    https://raw.githubusercontent.com/fmtlib/fmt/${FMT_VERSION}/include/fmt/format.h 
    ${CMAKE_SOURCE_DIR}/src/include/fmt/format.h
    MD5=dbc0dc8bd99db82f2b054f7cda23f848
)

GET_FILE(
    https://raw.githubusercontent.com/fmtlib/fmt/${FMT_VERSION}/include/fmt/format-inl.h 
    ${CMAKE_SOURCE_DIR}/src/include/fmt/format-inl.h
    MD5=fc80debc11fb314f5f83f1bfa793a729
)

GET_FILE(
    https://raw.githubusercontent.com/fmtlib/fmt/${FMT_VERSION}/include/fmt/color.h
    ${CMAKE_SOURCE_DIR}/src/include/fmt/color.h
    MD5=d799fef29da8736f4383eee2a3b3b34a
)

GET_FILE(
    https://raw.githubusercontent.com/fmtlib/fmt/${FMT_VERSION}/src/format.cc
    ${CMAKE_SOURCE_DIR}/src/include/fmt/format.cc
    MD5=a33a36ac55b22249a26eeb5594547b60
)

ADD_LIBRARY(format STATIC format.cc)
