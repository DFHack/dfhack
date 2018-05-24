/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#pragma once

/*
 *  Some much needed SDL fakery.
 */

#include "Pragma.h"
#include "Export.h"
#include <string>
#include <stdint.h>

#include "modules/Graphic.h"

// function and variable pointer... we don't try to understand what SDL does here
typedef void * fPtr;
typedef void * vPtr;
struct WINDOW;
namespace SDL
{
    union Event;
}

// these functions are here because they call into DFHack::Core and therefore need to
// be declared as friend functions/known
#ifdef _DARWIN
#include "modules/Graphic.h"
DFhackCExport int DFH_SDL_NumJoysticks(void);
DFhackCExport void DFH_SDL_Quit(void);
DFhackCExport int DFH_SDL_PollEvent(SDL::Event* event);
DFhackCExport int DFH_SDL_Init(uint32_t flags);
DFhackCExport int DFH_wgetch(WINDOW * win);
#endif
DFhackCExport int SDL_NumJoysticks(void);
DFhackCExport void SDL_Quit(void);
DFhackCExport int SDL_PollEvent(SDL::Event* event);
DFhackCExport int SDL_PushEvent(SDL::Event* event);
DFhackCExport int SDL_Init(uint32_t flags);
DFhackCExport int wgetch(WINDOW * win);

DFhackCExport int SDL_UpperBlit(DFHack::DFSDL_Surface* src, DFHack::DFSDL_Rect* srcrect, DFHack::DFSDL_Surface* dst, DFHack::DFSDL_Rect* dstrect);
DFhackCExport vPtr SDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth,
                                     uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask);
DFhackCExport vPtr SDL_CreateRGBSurfaceFrom(vPtr pixels, int width, int height, int depth, int pitch,
                                         uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask);
DFhackCExport void SDL_FreeSurface(vPtr surface);
DFhackCExport vPtr SDL_ConvertSurface(vPtr surface, vPtr format, uint32_t flags);
DFhackCExport int SDL_LockSurface(vPtr surface);
DFhackCExport void SDL_UnlockSurface(vPtr surface);
DFhackCExport uint8_t SDL_GetMouseState(int *x, int *y);
DFhackCExport void * SDL_GetVideoSurface(void);

DFhackCExport int SDL_SemWait(vPtr sem);
DFhackCExport int SDL_SemPost(vPtr sem);

// hook - called early from DF's main()
DFhackCExport int egg_init(void);

// hook - called before rendering
DFhackCExport int egg_shutdown(void);

// hook - called for each game tick (or more often)
DFhackCExport int egg_tick(void);

// hook - called before rendering
DFhackCExport int egg_prerender(void);

// hook - called for each SDL event, can filter both the event and the return value
DFhackCExport int egg_sdl_event(SDL::Event* event);

// hook - ncurses event. return -1 to consume
DFhackCExport int egg_curses_event(int orig_return);

