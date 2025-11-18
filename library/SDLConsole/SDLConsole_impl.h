#pragma once

//#include <SDL.h>
#include <memory>
#include <source_location>
#include <string>
#include <filesystem>
#include <iostream>
//#include "sdl_symbols.h"

namespace DFHack::SDLConsoleLib {
extern const char* (*SDL_GetError)();

enum class Error {
    Internal,
    SDL,
    FreeType
};

template <Error S, typename T = int>
inline void log_error(std::string_view ctx, T err = {}, const std::source_location& loc = std::source_location::current())
{
    std::filesystem::path p(loc.file_name());

    std::cerr << "SDLConsole [ERROR] " << ctx
    << " at " << p.filename().string()  << ":" << loc.line();

    if constexpr (S == Error::FreeType) {
        if (err) std::cerr << " (FT_Error: " << err << ")";
        std::cerr << " (FreeType)";
    }

    if constexpr (S == Error::SDL) {
        const char* sdl_err = SDL_GetError();
        if (sdl_err && *sdl_err) std::cerr << " (SDL_Error: " << sdl_err << ")";
        std::cerr << " (SDL)";
    }

    if constexpr (S == Error::Internal) {
        if (err) std::cerr << " (Error: " << err << ")";
        std::cerr << " (Internal)";
    }

    std::cerr << std::endl;
}

template <Error S, typename T = int>
[[noreturn]] inline void fatal(std::string_view ctx, T err = {}, const std::source_location& loc = std::source_location::current())

{
    log_error<S>(ctx, err, loc);
    throw std::runtime_error(std::string(ctx));
}

using String = std::u32string;
using StringView = std::u32string_view;
using Char = char32_t;


namespace geometry {
    struct Size {
        int w { 0 };
        int h { 0 };
    };
}

}
