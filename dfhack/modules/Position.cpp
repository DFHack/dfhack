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
#include "modules/Position.h"
#include "DFMemInfo.h"
#include "DFProcess.h"
using namespace DFHack;




struct Position::Private
{
    uint32_t window_x_offset;
    uint32_t window_y_offset;
    uint32_t window_z_offset;
    uint32_t cursor_xyz_offset;
    uint32_t window_dims_offset;
    
    uint32_t hotkey_start;
    uint32_t hotkey_mode_offset;
    uint32_t hotkey_xyz_offset;
    uint32_t hotkey_size;
    
    DFContextPrivate *d;
    Process * owner;
    bool Inited;
    bool Started;
    bool StartedHotkeys;
    //uint32_t biome_stuffs;
    //vector<uint16_t> v_geology[eBiomeCount];
};

Position::Position(DFContextPrivate * d_)
{
    d = new Private;
    d->d = d_;
    d->owner = d_->p;
    d->Inited = true;
    d->StartedHotkeys = d->Started = false;
    memory_info * mem;
    try
    {
        mem = d->d->offset_descriptor;
        d->window_x_offset = mem->getAddress ("window_x");
        d->window_y_offset = mem->getAddress ("window_y");
        d->window_z_offset = mem->getAddress ("window_z");
        d->cursor_xyz_offset = mem->getAddress ("cursor_xyz");
        d->window_dims_offset = mem->getAddress ("window_dims");
        d->Started = true;
    
        d->hotkey_start = mem->getAddress("hotkey_start");
        d->hotkey_mode_offset = mem->getOffset ("hotkey_mode");
        d->hotkey_xyz_offset = mem->getOffset("hotkey_xyz");
        d->hotkey_size = mem->getHexValue("hotkey_size");
        d->StartedHotkeys = true;
    }
    catch(exception &){};
}

Position::~Position()
{
    delete d;
}

bool Position::ReadHotkeys(t_hotkey hotkeys[])
{
    if (!d->StartedHotkeys) return false;
    uint32_t currHotkey = d->hotkey_start;
    Process * p = d->owner;
    
    for(uint32_t i = 0 ; i < NUM_HOTKEYS ;i++)
    {
        p->readSTLString(currHotkey,hotkeys[i].name,10);
        hotkeys[i].mode = p->readWord(currHotkey+d->hotkey_mode_offset);
        p->read (currHotkey + d->hotkey_xyz_offset, 3*sizeof (int32_t), (uint8_t *) &hotkeys[i].x);
        currHotkey+=d->hotkey_size;
    }
    return true;
}

bool Position::getViewCoords (int32_t &x, int32_t &y, int32_t &z)
{
    if (!d->Inited) return false;
    Process * p = d->owner;
    
    p->readDWord (d->window_x_offset, (uint32_t &) x);
    p->readDWord (d->window_y_offset, (uint32_t &) y);
    p->readDWord (d->window_z_offset, (uint32_t &) z);
    return true;
}

//FIXME: confine writing of coords to map bounds?
bool Position::setViewCoords (const int32_t x, const int32_t y, const int32_t z)
{
    if (!d->Inited) return false;
    Process * p = d->owner;
    
    p->writeDWord (d->window_x_offset, (uint32_t) x);
    p->writeDWord (d->window_y_offset, (uint32_t) y);
    p->writeDWord (d->window_z_offset, (uint32_t) z);
    return true;
}

bool Position::getCursorCoords (int32_t &x, int32_t &y, int32_t &z)
{
    if(!d->Inited) return false;
    int32_t coords[3];
    d->owner->read (d->cursor_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    x = coords[0];
    y = coords[1];
    z = coords[2];
    if (x == -30000) return false;
    return true;
}

//FIXME: confine writing of coords to map bounds?
bool Position::setCursorCoords (const int32_t x, const int32_t y, const int32_t z)
{
    if (!d->Inited) return false;
    int32_t coords[3] = {x, y, z};
    d->owner->write (d->cursor_xyz_offset, 3*sizeof (int32_t), (uint8_t *) coords);
    return true;
}

bool Position::getWindowSize (int32_t &width, int32_t &height)
{
    if(!d->Inited) return false;
    
    int32_t coords[2];
    d->owner->read (d->window_dims_offset, 2*sizeof (int32_t), (uint8_t *) coords);
    width = coords[0];
    height = coords[1];
    return true;
}

