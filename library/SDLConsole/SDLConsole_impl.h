#pragma once

#include <SDL.h>
#include <memory>
#include <source_location>
#include <string>

namespace DFHack::SDLConsoleLib {

enum class Error {
    Internal,
    SDL,
    FreeType
};

template <Error S, typename T = int>
inline void log_error(std::string_view ctx, T err = {}, const std::source_location& loc = std::source_location::current());

template <Error S, typename T = int>
[[noreturn]] inline void fatal(std::string_view ctx, T err = {}, const std::source_location& loc = std::source_location::current());

using String = std::u32string;
using StringView = std::u32string_view;
using Char = char32_t;

using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;

namespace geometry {
    struct Size {
        int w { 0 };
        int h { 0 };
    };
}

}
