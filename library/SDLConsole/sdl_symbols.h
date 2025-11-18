#pragma once

#include <SDL.h>
#include <memory>

namespace DFHack::SDLConsoleLib {

    using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
    using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;

#define SDLCONSOLE_SDL_SYMBOL(name) extern decltype(name)* name;
#include "sdl_symbols.inl"
#undef SDLCONSOLE_SDL_SYMBOL

    // Wrap a SDL_Surface with a unique_ptr.
    // It is safe to call with nullptr
    static inline Surface make_surface(SDL_Surface* s)
    {
        return Surface(s, SDLConsoleLib::SDL_FreeSurface);
    }

    // Wrap a SDL_Texture with a unique_ptr.
    // It is safe to call it with nullptr
    static inline Texture make_texture(SDL_Texture* t)
    {
        return Texture(t, SDLConsoleLib::SDL_DestroyTexture);
    }

}
