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

#include "Types.h"

#include "DataDefs.h"
#include "df/announcement_flags.h"
#include "df/announcement_infost.h"
#include "df/announcement_type.h"
#include "df/building_stockpilest.h"
#include "df/init.h"
#include "df/markup_text_boxst.h"
#include "df/plotinfost.h"
#include "df/report.h"
#include "df/report_zoom_type.h"
#include "df/unit_report_type.h"
#include "df/world.h"

#include "modules/GuiHooks.h"

namespace df {
    struct viewscreen;
    struct job;
    struct unit;
    struct item;
    struct plant;
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
        DFHACK_EXPORT std::vector<std::string> getFocusStrings(df::viewscreen *top);
        DFHACK_EXPORT bool matchFocusString(std::string focus_string, df::viewscreen *top = NULL);
        void clearFocusStringCache();

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
        DFHACK_EXPORT df::job *getAnyWorkshopJob(df::viewscreen *top);
        DFHACK_EXPORT df::job *getSelectedWorkshopJob(color_ostream &out, bool quiet = false);

        // A job is selected in a workshop, or unitjobs
        DFHACK_EXPORT bool any_job_hotkey(df::viewscreen *top);
        DFHACK_EXPORT df::job *getAnyJob(df::viewscreen *top);
        DFHACK_EXPORT df::job *getSelectedJob(color_ostream &out, bool quiet = false);

        // A unit is selected via 'v', 'k', unitjobs, or
        // a full-screen item view of a cage or suchlike
        DFHACK_EXPORT bool any_unit_hotkey(df::viewscreen *top);
        DFHACK_EXPORT df::unit *getAnyUnit(df::viewscreen *top);
        DFHACK_EXPORT df::unit *getSelectedUnit(color_ostream &out, bool quiet = false);

        // An item is selected via 'v'->inventory, 'k', 't', or
        // a full-screen item view of a container. Note that in the
        // last case, the highlighted contained item is returned, not
        // the container itself.
        DFHACK_EXPORT bool any_item_hotkey(df::viewscreen *top);
        DFHACK_EXPORT df::item *getAnyItem(df::viewscreen *top);
        DFHACK_EXPORT df::item *getSelectedItem(color_ostream &out, bool quiet = false);

        // A building is selected via 'q', 't' or 'i' (?)
        DFHACK_EXPORT bool any_building_hotkey(df::viewscreen *top);
        DFHACK_EXPORT df::building *getAnyBuilding(df::viewscreen *top);
        DFHACK_EXPORT df::building *getSelectedBuilding(color_ostream &out, bool quiet = false);

        // A (civ)zone is selected via 'z'
        DFHACK_EXPORT bool any_civzone_hotkey(df::viewscreen* top);
        DFHACK_EXPORT df::building_civzonest *getAnyCivZone(df::viewscreen* top);
        DFHACK_EXPORT df::building_civzonest *getSelectedCivZone(color_ostream& out, bool quiet = false);

        // A stockpile is selected via 'p'
        DFHACK_EXPORT bool any_stockpile_hotkey(df::viewscreen* top);
        DFHACK_EXPORT df::building_stockpilest *getAnyStockpile(df::viewscreen* top);
        DFHACK_EXPORT df::building_stockpilest *getSelectedStockpile(color_ostream& out, bool quiet = false);

        // A plant is selected, e.g. via 'k'
        DFHACK_EXPORT bool any_plant_hotkey(df::viewscreen *top);
        DFHACK_EXPORT df::plant *getAnyPlant(df::viewscreen *top);
        DFHACK_EXPORT df::plant *getSelectedPlant(color_ostream &out, bool quiet = false);

        // Low-level API that gives full control over announcements and reports
        DFHACK_EXPORT void writeToGamelog(std::string message);

        DFHACK_EXPORT int makeAnnouncement(df::announcement_type type, df::announcement_flags mode, df::coord pos, std::string message, int color = 7, bool bright = true);

        DFHACK_EXPORT bool addCombatReport(df::unit *unit, df::unit_report_type slot, df::report *report, bool update_alert = false);
        DFHACK_EXPORT inline bool addCombatReport(df::unit *unit, df::unit_report_type slot, int report_index, bool update_alert = false)
        {
            return addCombatReport(unit, slot, vector_get(df::global::world->status.reports, report_index), update_alert);
        }

        DFHACK_EXPORT bool addCombatReportAuto(df::unit *unit, df::announcement_flags mode, df::report *report);
        DFHACK_EXPORT inline bool addCombatReportAuto(df::unit *unit, df::announcement_flags mode, int report_index)
        {
            return addCombatReportAuto(unit, mode, vector_get(df::global::world->status.reports, report_index));
        }

        // Show a plain announcement, or a titan-style popup message
        DFHACK_EXPORT void showAnnouncement(std::string message, int color = 7, bool bright = true);
        DFHACK_EXPORT void showZoomAnnouncement(df::announcement_type type, df::coord pos, std::string message, int color = 7, bool bright = true);
        DFHACK_EXPORT void showPopupAnnouncement(std::string message, int color = 7, bool bright = true);

        // Show an announcement with effects determined by announcements.txt
        DFHACK_EXPORT void showAutoAnnouncement(df::announcement_type type, df::coord pos, std::string message, int color = 7, bool bright = true,
                                                df::unit *unit_a = NULL, df::unit *unit_d = NULL);

        // Process an announcement exactly like DF would, which might result in no announcement
        DFHACK_EXPORT bool autoDFAnnouncement(df::announcement_infost info, std::string message);
        DFHACK_EXPORT bool autoDFAnnouncement(df::announcement_type type, df::coord pos, std::string message, int color = 7, bool bright = true,
                                              df::unit *unit_a = NULL, df::unit *unit_d = NULL, bool is_sparring = false);
        /*
         * Markup Text Box functions
         */
        // Clear MTB before use
        DFHACK_EXPORT void MTB_clean(df::markup_text_boxst *mtb);
        // Build MTB's word vector from string
        DFHACK_EXPORT void MTB_parse(df::markup_text_boxst *mtb, std::string parse_text);
        // Size MTB appropriately and place words at proper positions
        DFHACK_EXPORT void MTB_set_width(df::markup_text_boxst *mtb, int32_t width = 50);

        /*
         * Cursor and window map coords
         */
        DFHACK_EXPORT df::coord getViewportPos();
        DFHACK_EXPORT df::coord getCursorPos();
        DFHACK_EXPORT df::coord getMousePos(bool allow_out_of_bounds = false);

        static const int AREA_MAP_WIDTH = 23;
        static const int MENU_WIDTH = 30;

        struct DwarfmodeDims {
            int map_x1, map_x2;
            int map_y1, map_y2;

            rect2d map() { return mkrect_xy(map_x1, map_y1, map_x2, map_y2); }
        };

        DFHACK_EXPORT DwarfmodeDims getDwarfmodeViewDims();

        DFHACK_EXPORT void resetDwarfmodeView(bool pause = false);
        DFHACK_EXPORT bool revealInDwarfmodeMap(int32_t x, int32_t y, int32_t z, bool center = false, bool highlight = false);
        DFHACK_EXPORT inline bool revealInDwarfmodeMap(df::coord pos, bool center = false, bool highlight = false) { return revealInDwarfmodeMap(pos.x, pos.y, pos.z, center, highlight); };
        DFHACK_EXPORT bool pauseRecenter(int32_t x, int32_t y, int32_t z, bool pause = true);
        DFHACK_EXPORT inline bool pauseRecenter(df::coord pos, bool pause = true) { return pauseRecenter(pos.x, pos.y, pos.z, pause); };
        DFHACK_EXPORT bool refreshSidebar();

        DFHACK_EXPORT bool inRenameBuilding();

        DFHACK_EXPORT bool getViewCoords (int32_t &x, int32_t &y, int32_t &z);
        DFHACK_EXPORT bool setViewCoords (const int32_t x, const int32_t y, const int32_t z);

        DFHACK_EXPORT bool getCursorCoords (int32_t &x, int32_t &y, int32_t &z);
        DFHACK_EXPORT bool getCursorCoords (df::coord &pos);
        DFHACK_EXPORT bool setCursorCoords (const int32_t x, const int32_t y, const int32_t z);

        DFHACK_EXPORT bool getDesignationCoords (int32_t &x, int32_t &y, int32_t &z);
        DFHACK_EXPORT bool setDesignationCoords (const int32_t x, const int32_t y, const int32_t z);

        // The distance from the z-level of the tile at map coordinates (x, y) to the closest ground z-level below
        // Defaults to 0, unless overriden by plugins
        DFHACK_EXPORT int getDepthAt (int32_t x, int32_t y);
        /*
         * Gui screens
         */
        /// Get the current top-level view-screen
        DFHACK_EXPORT df::viewscreen *getCurViewscreen(bool skip_dismissed = false);

        DFHACK_EXPORT df::viewscreen *getViewscreenByIdentity(virtual_identity &id, int n = 1);

        /// Get the top-most underlying DF viewscreen (not owned by DFHack)
        DFHACK_EXPORT df::viewscreen *getDFViewscreen(bool skip_dismissed = false, df::viewscreen *top = NULL);

        /// Get the top-most viewscreen of the given type from the top `n` viewscreens (or all viewscreens if n < 1)
        /// returns NULL if none match
        template <typename T>
        inline T *getViewscreenByType (int n = 1) {
            return strict_virtual_cast<T>(getViewscreenByIdentity(T::_identity, n));
        }

        inline std::vector<std::string> getCurFocus(bool skip_dismissed = false) {
            return getFocusStrings(getCurViewscreen(skip_dismissed));
        }

        /// get the size of the window buffer
        DFHACK_EXPORT bool getWindowSize(int32_t & width, int32_t & height);

        namespace Hooks {
            GUI_HOOK_DECLARE(depth_at, int, (int32_t x, int32_t y));
            GUI_HOOK_DECLARE(dwarfmode_view_dims, DwarfmodeDims, ());
        }
    }
}
