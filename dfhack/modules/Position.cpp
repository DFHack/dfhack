/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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

#include "DFCommonInternal.h"
#include "../private/APIPrivate.h"
using namespace DFHack;

bool API::InitViewAndCursor()
{
    try
    {
        d->window_x_offset = d->offset_descriptor->getAddress ("window_x");
        d->window_y_offset = d->offset_descriptor->getAddress ("window_y");
        d->window_z_offset = d->offset_descriptor->getAddress ("window_z");
        d->cursor_xyz_offset = d->offset_descriptor->getAddress ("cursor_xyz");
        d->current_cursor_creature_offset = d->offset_descriptor->getAddress ("current_cursor_creature");
        d->window_dims_offset = d->offset_descriptor->getAddress ("window_dims");
        
        d->current_menu_state_offset = d->offset_descriptor->getAddress("current_menu_state");
        d->pause_state_offset = d->offset_descriptor->getAddress ("pause_state");
        d->view_screen_offset = d->offset_descriptor->getAddress ("view_screen");

        d->cursorWindowInited = true;
        return true;
    }
    catch (Error::MissingMemoryDefinition&)
    {
        d->cursorWindowInited = false;
        throw;
    }
}

bool API::getViewCoords (int32_t &x, int32_t &y, int32_t &z)
{
    if (!d->cursorWindowInited) return false;
    g_pProcess->readDWord (d->window_x_offset, (uint32_t &) x);
    g_pProcess->readDWord (d->window_y_offset, (uint32_t &) y);
    g_pProcess->readDWord (d->window_z_offset, (uint32_t &) z);
    return true;
}

//FIXME: confine writing of coords to map bounds?
bool API::setViewCoords (const int32_t x, const int32_t y, const int32_t z)
{
    if (!d->cursorWindowInited) return false;
    g_pProcess->writeDWord (d->window_x_offset, (uint32_t) x);
    g_pProcess->writeDWord (d->window_y_offset, (uint32_t) y);
    g_pProcess->writeDWord (d->window_z_offset, (uint32_t) z);
    return true;
}

bool API::getCursorCoords (int32_t &x, int32_t &y, int32_t &z)
{
    if(!d->cursorWindowInited) return false;
    int32_t coords[3];
    g_pProcess->read (d->cursor_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    x = coords[0];
    y = coords[1];
    z = coords[2];
    if (x == -30000) return false;
    return true;
}

//FIXME: confine writing of coords to map bounds?
bool API::setCursorCoords (const int32_t x, const int32_t y, const int32_t z)
{
    if (!d->cursorWindowInited) return false;
    int32_t coords[3] = {x, y, z};
    g_pProcess->write (d->cursor_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    return true;
}

bool API::getWindowSize (int32_t &width, int32_t &height)
{
    if(! d->cursorWindowInited) return false;
    
    int32_t coords[2];
    g_pProcess->read (d->window_dims_offset, 2*sizeof (int32_t), (uint8_t *) coords);
    width = coords[0];
    height = coords[1];
    return true;
}