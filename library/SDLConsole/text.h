#pragma once

#include "SDLConsole_impl.h"

namespace DFHack::SDLConsoleLib::text {

    /*
     * Converts a UTF-32 string to UTF-8.
     * Returns the UTF-8 string, or "?u8?" on failure.
     */
    std::string to_utf8(std::u32string_view u32_string);

    /*
     * Converts a UTF-8 string to UTF-32.
     * Returns the UTF-32 string, or U"?u8?" on failure.
     */
    std::u32string from_utf8(std::string_view u8_string);

    inline bool is_newline(Char ch) noexcept
    {
        return ch == U'\n' || ch == U'\r';
    }

    inline bool is_wspace(Char ch) noexcept
    {
        return ch == U' ' || ch == U'\t';
    }

    inline size_t skip_wspace(const StringView text, size_t pos) noexcept
    {
        if (text.empty())
            return 0;
        if (pos >= text.size())
            return text.size() - 1;

        const auto* it = std::ranges::find_if_not(
            text.begin() + pos,
            text.end(),
            is_wspace);

        if (it == text.end())
            return text.size() - 1;

        return std::distance(text.begin(), it);
    }

    inline size_t skip_wspace_reverse(const StringView text, size_t pos) noexcept
    {
        if (text.empty())
            return 0;
        if (pos >= text.size())
            pos = text.size() - 1;

        const auto* it = text.begin() + pos;
        while (it != text.begin() && is_wspace(*it)) {
            --it;
        }
        return std::distance(text.begin(), it);
    }

    inline size_t skip_graph(const StringView text, size_t pos) noexcept
    {
        if (text.empty())
            return 0;
        if (pos >= text.size())
            return text.size() - 1;

        const auto* it = std::ranges::find_if(
            text.begin() + pos,
            text.end(),
            is_wspace);

        if (it == text.end())
            return text.size() - 1;

        return std::distance(text.begin(), it);
    }

    inline size_t skip_graph_reverse(const StringView text, size_t pos) noexcept
    {
        if (text.empty())
            return 0;
        if (pos >= text.size())
            pos = text.size() - 1;

        const auto* it = text.begin() + pos;
        while (it != text.begin() && !is_wspace(*it)) {
            --it;
        }
        return std::distance(text.begin(), it);
    }

    /*
     * Finds the end of the previous word or non-space character in the text,
     * starting from `pos`.
     * - If `pos` points to spaces, skips backwards over them to find the previous word.
     * - If `pos` is already at a word, skips backwards over the current word and trailing spaces.
     * Returns the index of the end of that word or non-space character.
     * - Returns 0 if text is empty.
     * - Returns text.size() - 1 if `pos` is beyond the end of text.
     *
     * This function is a helper for callers expecting valid return indices.
     */
    inline size_t find_prev_word(const StringView text, size_t pos) noexcept
    {
        size_t start = pos;
        start = skip_wspace_reverse(text, start);
        if (start == pos) {
            pos = skip_graph_reverse(text, pos);
            pos = skip_wspace_reverse(text, pos);
        } else {
            pos = start;
        }
        return pos;
    }

    /*
     * Finds the start of the next word or non-space character in the text,
     * starting from `pos`.
     * - If `pos` points to spaces, skips forwards over them to find the next word.
     * - If `pos` is already at a word, skips forwards over the current word and trailing spaces.
     * Returns the index of the start of that word or non-space character.
     * - Returns 0 if text is empty.
     * - Returns text.size() - 1 if pos is beyond the end of text.
     *
     * This function is a helper for callers expecting valid return indices.
     */
    inline size_t find_next_word(const StringView text, size_t pos) noexcept
    {
        size_t start = pos;
        start = skip_wspace(text, start);
        if (start == pos) {
            pos = skip_graph(text, pos);
            pos = skip_wspace(text, pos);
        } else {
            pos = start;
        }
        return pos;
    }

    /*
     * Finds the continuous run (range) of characters in `text` at `pos`
     * for which the predicate `pred` returns true.
     * - Expands to the left and right from `pos` until `pred` is false or the text boundary is reached.
     *
     * Returns a pair of indices {start, end} representing the inclusive range.
     * - Returns {StringView::npos, StringView::npos} if `text` is empty or `pos` is beyond the end of text.
     */
    template <typename T>
    inline std::pair<size_t, size_t> find_run_with_pred(const StringView text, size_t pos, T&& pred)
    {
        if (text.empty()) return { StringView::npos, StringView::npos };

        if (pos >= text.size()) return { StringView::npos, StringView::npos };

        const auto* left = text.begin() + pos;
        const auto* right = left;

        while (left != text.begin() && pred(*(left - 1)))
            --left;

        const auto* tok = right;
        while (right != text.end() && pred(*right))
            ++right;

        if (tok != right)
            --right;

        return {
            std::distance(text.begin(), left),
            std::distance(text.begin(), right)
        };
    }

    inline std::pair<size_t, size_t> find_wspace_run(const StringView text, size_t pos) noexcept
    {
        return find_run_with_pred(text, pos, [](Char ch) { return is_wspace(ch); });
    }

    /*
     * Finds the continuous run (range) of characters at `pos`.
     * - If `pos` falls on a word character, returns the range of that word.
     * - If `pos` falls on whitespace, returns the range of consecutive whitespace.
     * Returns a pair of indices {start, end} representing the range.
     * - Returns {String::npos, String::npos} if text is empty or pos is beyond the end of text.
     */
    inline std::pair<size_t, size_t> find_run(const StringView text, size_t pos) noexcept
    {
        if (text.empty() || pos >= text.size()) {
            return { String::npos, String::npos };
        }

        if (is_wspace(text[pos])) {
            return find_wspace_run(text, pos);
        }
        return find_run_with_pred(text, pos, [](Char ch) { return !is_wspace(ch); });
    }

    inline size_t insert_at(String& text, size_t pos, const StringView str)
    {
        if (pos >= text.size()) {
            text += str;
        } else {
            text.insert(pos, str);
        }
        pos += str.length();
        return pos;
    }

    inline size_t backspace(String& text, size_t pos)
    {
        if (pos == 0 || text.empty()) {
            return pos;
        }

        if (text.length() == pos) {
            text.pop_back();
        } else {
            /* else shift the text from cursor left by one character */
            text.erase(pos-1, 1);
        }

        return --pos;
    }
}
