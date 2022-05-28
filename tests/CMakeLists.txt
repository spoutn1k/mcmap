FILE(GLOB TESTS *cpp)

IF(GTest_FOUND)
    ADD_EXECUTABLE(run_tests ${TESTS})

    TARGET_LINK_LIBRARIES(run_tests
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        fmt::fmt-header-only
        ZLIB::ZLIB
        mcmap_core)
ENDIF()
