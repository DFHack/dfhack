#include "SDLConsole_impl.h"
#include <gtest/gtest.h>
#include <string>

using namespace sdl_console;

TEST(SDLConsole, skip_wspace) {
    std::u32string tstr;
    size_t ret;

    tstr = U"foo  bar";
    ret = text::skip_wspace(tstr, 3);
    ASSERT_EQ(ret, 5);

    tstr = U"foo ";
    ret = text::skip_wspace(tstr, 3);
    ASSERT_EQ(ret, tstr.size()-1);

    tstr = U"foo";
    ret = text::skip_wspace(tstr, 9);
    ASSERT_EQ(ret, tstr.size()-1);

    tstr = U"";
    ret = text::skip_wspace(tstr, 1);
    ASSERT_EQ(ret, 0);

    tstr = U"foo";
    ret = text::skip_wspace(tstr, -1);
    ASSERT_EQ(ret, tstr.size()-1);
}

TEST(SDLConsole, skip_wspace_reverse) {
    std::u32string tstr;
    size_t ret;

    tstr = U"foo  bar";
    ret = text::skip_wspace_reverse(tstr, 3);
    ASSERT_EQ(ret, 2);

    tstr = U"foo  bar";
    ret = text::skip_wspace_reverse(tstr, 4);
    ASSERT_EQ(ret, 2);

    tstr = U"foo ";
    ret = text::skip_wspace_reverse(tstr, 3);
    ASSERT_EQ(ret, 2);

    tstr = U"foo";
    ret = text::skip_wspace_reverse(tstr, 9);
    ASSERT_EQ(ret, tstr.size()-1);

    tstr = U"";
    ret = text::skip_wspace_reverse(tstr, 1);
    ASSERT_EQ(ret, 0);

    tstr = U"foo";
    ret = text::skip_wspace_reverse(tstr, -1);
    ASSERT_EQ(ret, tstr.size()-1);
}

TEST(SDLConsole, skip_graph) {
    std::u32string tstr;
    size_t ret;

    tstr = U"foo  bar";
    ret = text::skip_graph(tstr, 0);
    ASSERT_EQ(ret, 3);

    tstr = U"foo  bar";
    ret = text::skip_graph(tstr, 3);
    ASSERT_EQ(ret, 3);

    tstr = U"foo  bar";
    ret = text::skip_graph(tstr, 5);
    ASSERT_EQ(ret, tstr.size()-1);

    tstr = U"foo";
    ret = text::skip_graph(tstr, 9);
    ASSERT_EQ(ret, tstr.size()-1);

    tstr = U"foo";
    ret = text::skip_graph(tstr, -1);
    ASSERT_EQ(ret, tstr.size()-1);
}

TEST(SDLConsole, skip_graph_reverse) {
    std::u32string tstr;
    size_t ret;

    tstr = U"foo  bar";
    ret = text::skip_graph_reverse(tstr, 5);
    ASSERT_EQ(ret, 4);

    tstr = U"foo  bar";
    ret = text::skip_graph_reverse(tstr, 2);
    ASSERT_EQ(ret, 0);

    tstr = U"foo";
    ret = text::skip_graph_reverse(tstr, 5);
    ASSERT_EQ(ret, 0);

    tstr = U"foo";
    ret = text::skip_graph_reverse(tstr, -1);
    ASSERT_EQ(ret, 0);
}

TEST(SDLConsole, find_prev_word) {
    std::u32string tstr;
    size_t ret;

    tstr = U"foo bar baz";
    ret = text::find_prev_word(tstr, 7);
    ASSERT_EQ(ret, 6);

    tstr = U"foo bar baz";
    ret = text::find_prev_word(tstr, 2);
    ASSERT_EQ(ret, 0);
}

TEST(SDLConsole, find_next_word) {
    std::u32string tstr;
    size_t ret;

    tstr = U"foo bar baz";
    ret = text::find_next_word(tstr, 7);
    ASSERT_EQ(ret, 8);

    tstr = U"foo bar baz";
    ret = text::find_next_word(tstr, 8);
    ASSERT_EQ(ret, tstr.size()-1);
}

TEST(SDLConsole, find_wspace_range) {
    std::u32string tstr;
    std::pair<size_t, size_t> ret;
    std::pair<size_t, size_t> exp;

    tstr = U"   ";
    ret = text::find_wspace_range(tstr, 1);
    exp.first = 0;
    exp.second = 2;
    ASSERT_EQ(ret.first, exp.first);
    ASSERT_EQ(ret.second, exp.second);

    tstr = U"foo   bar";
    ret = text::find_wspace_range(tstr, 3);
    exp.first = 3;
    exp.second = 5;
    ASSERT_EQ(ret.first, exp.first);
    ASSERT_EQ(ret.second, exp.second);

    tstr = U"foobar  ";
    ret = text::find_wspace_range(tstr, 6);
    exp.first = 6;
    exp.second = 7;
    ASSERT_EQ(ret.first, exp.first);
    ASSERT_EQ(ret.second, exp.second);

    tstr = U"foobar ";
    ret = text::find_wspace_range(tstr, 6);
    exp.first = 6;
    exp.second = 6;
    ASSERT_EQ(ret.first, exp.first);
    ASSERT_EQ(ret.second, exp.second);

    tstr = U"";
    ret = text::find_wspace_range(tstr, 0);
    exp.first = 0;
    exp.second = 0;
    ASSERT_EQ(ret.first, exp.first);
    ASSERT_EQ(ret.second, exp.second);

}

TEST(SDLConsole, find_text_range) {
    std::u32string tstr;
    std::pair<size_t, size_t> ret;
    std::pair<size_t, size_t> exp;

    tstr = U"foo";
    ret = text::find_text_range(tstr, 0);
    exp.first = 0;
    exp.second = 2;
    ASSERT_EQ(ret.first, exp.first);
    ASSERT_EQ(ret.second, exp.second);

    tstr = U"foo bar";
    ret = text::find_text_range(tstr, 5);
    exp.first = 4;
    exp.second = 6;
    ASSERT_EQ(ret.first, exp.first);
    ASSERT_EQ(ret.second, exp.second);

    tstr = U"foo bar ";
    ret = text::find_text_range(tstr, 5);
    exp.first = 4;
    exp.second = 6;
    ASSERT_EQ(ret.first, exp.first);
    ASSERT_EQ(ret.second, exp.second);
}
