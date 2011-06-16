/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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
#ifndef CL_MOD_GUI
#define CL_MOD_GUI

#include "dfhack/Export.h"
#include "dfhack/Module.h"

/**
 * \defgroup grp_gui query DF's GUI state
 * @ingroup grp_modules
 */

namespace DFHack
{
    class DFContextShared;
    /**
     * \ingroup grp_gui
     */
    struct t_viewscreen 
    {
        int32_t type;
        //There is more info in these objects, but I don't know what it is yet
    };
    #define NUM_HOTKEYS 16
    /**
     * \ingroup grp_gui
     */
    struct t_hotkey
    {
        std::string name;
        int16_t mode;
        int32_t x;
        int32_t y;
        int32_t z;
    };
    typedef t_hotkey hotkey_array[16];
    /**
     * \ingroup grp_gui
     */
    struct t_screen
    {
        uint8_t symbol;
        uint8_t foreground;
        uint8_t background;
        uint8_t bright;
        uint8_t gtile;
        uint8_t grayscale;
    };
    /**
     * The Gui module
     * \ingroup grp_modules
     * \ingroup grp_gui
     */
    class DFHACK_EXPORT Gui: public Module
    {
        public:

        Gui(DFHack::DFContextShared * d);
        ~Gui();
        bool Start();
        bool Finish();
        /*
         * Cursor and window coords
         */
        bool getViewCoords (int32_t &x, int32_t &y, int32_t &z);
        bool setViewCoords (const int32_t x, const int32_t y, const int32_t z);
        
        bool getCursorCoords (int32_t &x, int32_t &y, int32_t &z);
        bool setCursorCoords (const int32_t x, const int32_t y, const int32_t z);
        /*
         * Hotkeys (DF's zoom locations)
         */
        hotkey_array * hotkeys;
        
        /*
         * Window size in tiles
         */
        bool getWindowSize(int32_t & width, int32_t & height);
        
        /*
         * Screen tiles
         */
        bool getScreenTiles(int32_t width, int32_t height, t_screen screen[]);
        
        /// read the DF menu view state (stock screen, unit screen, other screens
        bool ReadViewScreen(t_viewscreen &);
        /// read the DF menu state (designation menu ect)
        uint32_t ReadMenuState();

        private:
        struct Private;
        Private *d;
    };
}
#endif

