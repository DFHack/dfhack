#ifndef SDL_CONSOLE_INTERNAL
#define SDL_CONSOLE_INTERNAL

#include <string>
#include <functional>

namespace sdl_console {
namespace text {
size_t skip_wspace(const std::u32string& text, size_t pos);

size_t skip_wspace_reverse(const std::u32string& text, size_t pos);

size_t skip_graph(const std::u32string& text, size_t pos);

size_t skip_graph_reverse(const std::u32string& text, size_t pos);

size_t find_prev_word(const std::u32string& text, size_t pos);

size_t find_next_word(const std::u32string& text, size_t pos);

std::pair<size_t, size_t> find_wspace_run(const std::u32string& text, size_t pos);

template <typename T>
std::pair<size_t, size_t> find_run_with_pred(const std::u32string& text, size_t pos, T&& pred);

/*
 * If pos falls on a word, returns range of word.
 * If pos falls on whitespace, returns range of whitespace.
 */
std::pair<size_t, size_t> find_run(const std::u32string& text, size_t pos);
}
}
#endif
