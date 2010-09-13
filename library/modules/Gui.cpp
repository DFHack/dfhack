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

#include "Internal.h"
#include "ContextShared.h"
#include "dfhack/modules/Gui.h"
#include "dfhack/DFProcess.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFTypes.h"

using namespace DFHack;

struct Gui::Private
{
    Private()
    {
        Started = PauseInited = ViewScreeInited = MenuStateInited = false;
    }
    bool Started;
    uint32_t pause_state_offset;
    bool PauseInited;
    uint32_t view_screen_offset;
    bool ViewScreeInited;
    uint32_t current_menu_state_offset;
    bool MenuStateInited;
    uint32_t tileset_filename_offset;
    bool TilesetFilenameInited;
    uint32_t colors_filename_offset;
    bool ColorsFilenameInited;
    DFContextShared *d;
    Process * owner;
};

Gui::Gui(DFContextShared * _d)
{

    d = new Private;
    d->d = _d;
    d->owner = _d->p;
    OffsetGroup * OG_Gui = d->d->offset_descriptor->getGroup("GUI");
    try
    {
        d->current_menu_state_offset = OG_Gui->getAddress("current_menu_state");
        d->MenuStateInited = true;
    }
    catch(exception &){};
    try
    {
        d->pause_state_offset = OG_Gui->getAddress ("pause_state");
        d->PauseInited = true;
    }
    catch(exception &){};
    try
    {
        d->view_screen_offset = OG_Gui->getAddress ("view_screen");
        d->ViewScreeInited = true;
    }
    catch(exception &){};
    try
    {
        d->tileset_filename_offset = OG_Gui->getAddress ("tileset_filename");
        d->TilesetFilenameInited = true;
    }
    catch(exception &){};
    try
    {
        d->colors_filename_offset = OG_Gui->getAddress ("colors_filename");
        d->ColorsFilenameInited = true;
    }
    catch(exception &){};
    d->Started = true;
}

Gui::~Gui()
{
    delete d;
}

bool Gui::Start()
{
    return true;
}

bool Gui::Finish()
{
    return true;
}

bool Gui::ReadPauseState()
{
    if(!d->PauseInited) return false;

    uint32_t pauseState = d->owner->readDWord (d->pause_state_offset);
    return pauseState & 1;
}

void Gui::SetPauseState(bool paused)
{
    if(!d->PauseInited) return;
    cout << "pause set" << endl;
    d->owner->writeDWord (d->pause_state_offset, paused);
}

uint32_t Gui::ReadMenuState()
{
    if(d->MenuStateInited)
        return(d->owner->readDWord(d->current_menu_state_offset));
    return false;
}

bool Gui::ReadViewScreen (t_viewscreen &screen)
{
    if (!d->ViewScreeInited) return false;
    Process * p = d->owner;

    uint32_t last = p->readDWord (d->view_screen_offset);
    uint32_t screenAddr = p->readDWord (last);
    uint32_t nextScreenPtr = p->readDWord (last + 4);
    while (nextScreenPtr != 0)
    {
        last = nextScreenPtr;
        screenAddr = p->readDWord (nextScreenPtr);
        nextScreenPtr = p->readDWord (nextScreenPtr + 4);
    }
    return d->d->offset_descriptor->resolveObjectToClassID (last, screen.type);
}
string Gui::ReadCurrentTileSetFilename()
{
    if(d->TilesetFilenameInited)
        return(d->owner->readCString(d->tileset_filename_offset));
    return "";
}
string Gui::ReadCurrentColorsFilename()
{
    if(d->ColorsFilenameInited)
        return(d->owner->readCString(d->colors_filename_offset));
    return "";
}