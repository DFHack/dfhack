
#include "MiscUtils.h"
#include <gtest/gtest.h>
#include <iostream>
#include <string>

TEST(MiscUtils, wordwrap) {
    std::vector<std::string> result;

    std::cout << "MiscUtils.wordwrap: 0 wrap" << "... ";
    word_wrap(&result, "123", 3);
    ASSERT_EQ(result.size(), 1);
    std::cout << "ok\n";

    result.clear();
    std::cout << "MiscUtils.wordwrap: 1 wrap" << "... ";
    word_wrap(&result, "12345", 3);
    ASSERT_EQ(result.size(), 2);
    std::cout << "ok\n";

    result.clear();
    std::cout << "MiscUtils.wordwrap: 2 wrap" << "... ";
    word_wrap(&result, "1234567", 3);
    ASSERT_EQ(result.size(), 3);
    std::cout << "ok\n";
}
