file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *test.cpp)
dfhack_test(test-library "${TEST_SOURCES}")

# How to get `test` to ensure everything is up to date before running
# tests? This add_dependencies() fails with:
#   Cannot add target-level dependencies to non-existent target "test".
#add_dependencies(test MiscUtils.test)
