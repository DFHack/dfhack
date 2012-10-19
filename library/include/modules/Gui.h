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
#include "Export.h"
#include "Module.h"
#include "BitArray.h"
#include "ColorText.h"
#include <string>

#include "DataDefs.h"
#include "df/init.h"
#include "df/ui.h"
#include "df/announcement_type.h"

namespace df {
    struct viewscreen;
    struct job;
    struct unit;
    struct item;
};

/**
 * \defgroup grp_gui utility code that helps dealing with DF's user interface
 * @ingroup grp_modules
 */

namespace DFHack
{
    class Core;
    /**
     * The Gui module
     * \ingroup grp_modules
     * \ingroup grp_gui
     */
    namespace Gui
    {
        DFHACK_EXPORT std::string getFocusString(df::viewscreen *top);

        // Full-screen item details view
        DFHACK_EXPORT bool item_details_hotkey(df::viewscreen *top);
        // 'u'nits or 'j'obs full-screen view
        DFHACK_EXPORT bool unitjobs_hotkey(df::viewscreen *top);

        // A job is selected in a workshop
        DFHACK_EXPORT bool workshop_job_hotkey(df::viewscreen *top);
        // Building material selection mode
        DFHACK_EXPORT bool build_selector_hotkey(df::viewscreen *top);
        // A unit is selected in the 'v' mode
        DFHACK_EXPORT bool view_unit_hotkey(df::viewscreen *top);
        // Above + the inventory page is selected.
        DFHACK_EXPORT bool unit_inventory_hotkey(df::viewscreen *top);

        // In workshop_job_hotkey, returns the job
        DFHACK_EXPORT df::job *getSelectedWorkshopJob(color_ostream &out, bool quiet = false);

        // A job is selected in a workshop, or unitjobs
        DFHACK_EXPORT bool any_job_hotkey(df::viewscreen *top);
        DFHACK_EXPORT df::job *getSelectedJob(color_ostream &out, bool quiet = false);

        // A unit is selected via 'v', 'k', unitjobs, or
        // a full-screen item view of a cage or suchlike
        DFHACK_EXPORT bool any_unit_hotkey(df::viewscreen *top);
        DFHACK_EXPORT df::unit *getSelectedUnit(color_ostream &out, bool quiet = false);

        // An item is selected via 'v'->inventory, 'k', 't', or
        // a full-screen item view of a container. Note that in the
        // last case, the highlighted contained item is returned, not
        // the container itself.
        DFHACK_EXPORT bool any_item_hotkey(df::viewscreen *top);
        DFHACK_EXPORT df::item *getSelectedItem(color_ostream &out, bool quiet = false);

        // A building is selected via 'q', 't' or 'i' (civzone)
        DFHACK_EXPORT bool any_building_hotkey(df::viewscreen *top);
        DFHACK_EXPORT df::building *getSelectedBuilding(color_ostream &out, bool quiet = false);

        // Show a plain announcement, or a titan-style popup message
        DFHACK_EXPORT void showAnnouncement(std::string message, int color = 7, bool bright = true);
        DFHACK_EXPORT void showZoomAnnouncement(df::announcement_type type, df::coord pos, std::string message, int color = 7, bool bright = true);
        DFHACK_EXPORT void showPopupAnnouncement(std::string message, int color = 7, bool bright = true);

        // Show an announcement with effects determined by announcements.txt
        DFHACK_EXPORT void showAutoAnnouncement(df::announcement_type type, df::coord pos, std::string message, int color = 7, bool bright = true);

        /*
         * Cursor and window coords
         */
        DFHACK_EXPORT df::coord getViewportPos();
        DFHACK_EXPORT df::coord getCursorPos();

        static const int AREA_MAP_WIDTH = 23;
        static const int MENU_WIDTH = 30;

        struct DwarfmodeDims {
            int map_x1, map_x2, menu_x1, menu_x2, area_x1, area_x2;
            int y1, y2;
            bool menu_on, area_on, menu_forced;
        };

        DFHACK_EXPORT DwarfmodeDims getDwarfmodeViewDims();

        DFHACK_EXPORT void resetDwarfmodeView(bool pause = false);
        DFHACK_EXPORT bool revealInDwarfmodeMap(df::coord pos, bool center = false);

        DFHACK_EXPORT bool getViewCoords (int32_t &x, int32_t &y, int32_t &z);
        DFHACK_EXPORT bool setViewCoords (const int32_t x, const int32_t y, const int32_t z);

        DFHACK_EXPORT bool getCursorCoords (int32_t &x, int32_t &y, int32_t &z);
        DFHACK_EXPORT bool setCursorCoords (const int32_t x, const int32_t y, const int32_t z);

        DFHACK_EXPORT bool getDesignationCoords (int32_t &x, int32_t &y, int32_t &z);
        DFHACK_EXPORT bool setDesignationCoords (const int32_t x, const int32_t y, const int32_t z);

        DFHACK_EXPORT bool getMousePos (int32_t & x, int32_t & y);
        /*
         * Gui screens
         */
        /// Get the current top-level view-screen
        DFHACK_EXPORT df::viewscreen *getCurViewscreen(bool skip_dismissed = false);

        inline std::string getCurFocus(bool skip_dismissed = false) {
            return getFocusString(getCurViewscreen(skip_dismissed));
        }

        /// get the size of the window buffer
        DFHACK_EXPORT bool getWindowSize(int32_t & width, int32_t & height);

        /**
         *Menu width:
         *3:3 - menu and area map closed
         *2:3 - menu open single width
         *1:3 - menu open double width
         *1:2 - menu and area map open
         *2:2 - area map open
         */

        DFHACK_EXPORT bool getMenuWidth(uint8_t & menu_width, uint8_t & area_map_width);
        DFHACK_EXPORT bool setMenuWidth(const uint8_t menu_width, const uint8_t area_map_width);
    }
}
