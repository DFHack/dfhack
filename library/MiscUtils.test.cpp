
#include "MiscUtils.h"
#include <gtest/gtest.h>
#include <string>

TEST(MiscUtils, wordwrap) {
    std::vector<std::string> result;

    word_wrap(&result, "123", 3);
    ASSERT_EQ(result.size(), 1);

    result.clear();
    word_wrap(&result, "12345", 3);
    ASSERT_EQ(result.size(), 2);

    result.clear();
    word_wrap(&result, "1234567", 3);
    ASSERT_EQ(result.size(), 3);
}
