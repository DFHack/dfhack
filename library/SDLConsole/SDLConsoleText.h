#pragma once

#include "SDLConsole_impl.h"
#include "sdl_symbols.h"

namespace DFHack::SDLConsoleLib::text {

    inline std::string to_utf8(const std::u32string_view u32_string)
    {
        char* conv = SDLConsoleLib::SDL_iconv_string("UTF-8", "UTF-32LE",
                                                     reinterpret_cast<const char*>(u32_string.data()),
                                                     (u32_string.size()+1) * sizeof(char32_t));
        if (!conv)
            return "?u8?";

        std::string result(conv);
        SDLConsoleLib::SDL_free(conv);
        return result;
    }

    inline String from_utf8(const std::string_view& u8_string)
    {
        char* conv = SDLConsoleLib::SDL_iconv_string("UTF-32LE", "UTF-8",
                                                     u8_string.data(),
                                                     u8_string.size() + 1);
        if (!conv)
            return U"?u8?";

        std::u32string result(reinterpret_cast<char32_t*>(conv));
        SDLConsoleLib::SDL_free(conv);
        return result;
    }

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

        auto sub = std::ranges::subrange(text.begin() + pos, text.end() - 1);
        return std::distance(text.begin(),
                             std::ranges::find_if_not(sub, is_wspace));
    }

    inline size_t skip_wspace_reverse(const StringView text, size_t pos) noexcept
    {
        if (text.empty()) return 0;
        if (pos >= text.size()) pos = text.size() - 1;

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

        const auto sub = std::ranges::subrange(text.begin() + pos, text.end() - 1);
        return std::distance(text.begin(),
                             std::ranges::find_if(sub, is_wspace));
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
     * starting from `pos`. If `pos` points to a space, it skips consecutive
     * spaces to find the previous word. If `pos` is already at a word, it skips
     * the current word and trailing spaces to find the next one. Returns the
     * position of the end of the previous word or non-space character.
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
     * starting from `pos`. If `pos` points to a space, it skips consecutive
     * spaces to find the next word. If `pos` is already at a word, it skips
     * the current word and trailing spaces to find the next one. Returns the
     * position of the start of the next word or non-space character.
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
     * If pos falls on a word, returns range of word.
     * If pos falls on whitespace, returns range of whitespace.
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
