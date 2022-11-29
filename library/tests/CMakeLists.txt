file(GLOB_RECURSE TEST_SOURCES
    LIST_DIRECTORIES false
    *test.cpp)
dfhack_test(test-library "${TEST_SOURCES}")
