FILE(GLOB TESTS *cpp)

IF(GTest_FOUND)
    ADD_EXECUTABLE(run_tests ${TESTS})

    TARGET_LINK_LIBRARIES(run_tests
        GTest::gtest
        GTest::gtest_main
        mcmap_core
        format
        ZLIB::ZLIB)
ENDIF()
