/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf, doomchild

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

#ifndef GUI_C_API
#define GUI_C_API

#include "DFHack_C.h"
#include "dfhack/DFTypes.h"
#include "dfhack/modules/Gui.h"

#ifdef __cplusplus
extern "C" {
#endif

DFHACK_EXPORT int Gui_Start(DFHackObject* gui);
DFHACK_EXPORT int Gui_Finish(DFHackObject* gui);

DFHACK_EXPORT int Gui_getViewCoords(DFHackObject* gui, int32_t* x, int32_t* y, int32_t* z);
DFHACK_EXPORT int Gui_setViewCoords(DFHackObject* gui, int32_t x, int32_t y, int32_t z);

DFHACK_EXPORT int Gui_getCursorCoords(DFHackObject* gui, int32_t* x, int32_t* y, int32_t* z);
DFHACK_EXPORT int Gui_setCursorCoords(DFHackObject* gui, int32_t x, int32_t y, int32_t z);

DFHACK_EXPORT t_hotkey* Gui_ReadHotkeys(DFHackObject* gui);

DFHACK_EXPORT int Gui_getWindowSize(DFHackObject* gui, int32_t* width, int32_t* height);

DFHACK_EXPORT t_screen* Gui_getScreenTiles(DFHackObject* gui, int32_t width, int32_t height);

DFHACK_EXPORT int Gui_ReadViewScreen(DFHackObject* gui, t_viewscreen* viewscreen);
DFHACK_EXPORT int Gui_ReadMenuState(DFHackObject* gui, uint32_t* menuState);

#ifdef __cplusplus
}
#endif

#endif
