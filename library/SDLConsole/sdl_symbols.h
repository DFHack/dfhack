#pragma once

#include <SDL.h>

namespace DFHack::SDLConsoleLib {

#define SDLCONSOLE_SDL_SYMBOL(name) extern decltype(name)* name;
#include "sdl_symbols.inl"
#undef SDLCONSOLE_SDL_SYMBOL

}
