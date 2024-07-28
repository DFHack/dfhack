// Quick Dumper : Moves items marked as "dump" to cursor

#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Buildings.h"
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Maps.h"
#include "modules/Materials.h"

#include "df/building_stockpilest.h"
#include "df/general_ref.h"
#include "df/item.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"

#include <algorithm>
#include <climits>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::vector;

using namespace DFHack;
using namespace df::enums;

using df::building_stockpilest;

DFHACK_PLUGIN("autodump");
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(world);

command_result df_autodump(color_ostream &out, vector <string> & parameters);
command_result df_autodump_destroy_here(color_ostream &out, vector <string> & parameters);
command_result df_autodump_destroy_item(color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "autodump",
        "Teleport items marked for dumping to the keyboard cursor.",
        df_autodump));
    commands.push_back(PluginCommand(
        "autodump-destroy-here",
        "Destroy items marked for dumping under the keyboard cursor.",
        df_autodump_destroy_here,
        Gui::cursor_hotkey));
    commands.push_back(PluginCommand(
        "autodump-destroy-item",
        "Destroy the selected item.",
        df_autodump_destroy_item,
        Gui::any_item_hotkey));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    return CR_OK;
}

typedef map<DFCoord, uint32_t> coordmap;

static command_result autodump_main(color_ostream &out, vector<string> &parameters)
{   // Command line options
    bool destroy = false;
    bool here = false;
    bool need_visible = false;
    bool need_hidden = false;
    bool need_forbidden = false;
    for (size_t i = 0; i < parameters.size(); i++)
    {
        string &p = parameters[i];
        if(p == "destroy")
            destroy = true;
        else if (p == "destroy-here")
            destroy = here = true;
        else if (p == "visible")
            need_visible = true;
        else if (p == "hidden")
            need_hidden = true;
        else if (p == "forbidden")
            need_forbidden = true;
        else
            return CR_WRONG_USAGE;
    }

    if (need_visible && need_hidden) {
        out.printerr("An item can't be both hidden and visible.\n");
        return CR_WRONG_USAGE;
    }
    else if (!Maps::IsValid()) {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int dumped_total = 0;

    df::coord pos_cursor;
    if(!destroy || here)
    {
        pos_cursor = Gui::getCursorPos();
        if (!pos_cursor.isValid()) {
            out.printerr("Keyboard cursor must be over a suitable map tile.\n");
            return CR_FAILURE;
        }
    }

    if (!destroy) {
        auto ttype = Maps::getTileType(pos_cursor);
        if(!ttype) {
            out.printerr("Cursor is in an invalid/uninitialized area. Place it over a floor.\n");
            return CR_FAILURE;
        }
        else if(!isWalkable(*ttype)) { // TODO: Improve this w/r/t impassible buildings
            auto b_occ = Maps::getTileOccupancy(pos_cursor)->bits.building;
            if (b_occ != tile_building_occ::Floored)
            {   // Some Dynamic act as floors
                auto bld = b_occ == tile_building_occ::Dynamic ? Buildings::findAtTile(pos_cursor) : NULL;
                if (bld &&
                    bld->getType() != building_type::Hatch &&
                    bld->getType() != building_type::GrateFloor &&
                    bld->getType() != building_type::BarsFloor)
                {
                    out.printerr("Cursor should be placed over a floor.\n");
                    return CR_FAILURE;
                }
            }
        }
    }

    // Proceed with the dumpification operation
    for (auto itm : world->items.other.IN_PLAY) {
        // Only dump valid stuff marked for dumping
        if (!itm->flags.bits.dump ||
            itm->flags.bits.construction ||
            itm->flags.bits.in_building ||
            itm->flags.bits.artifact
        )
            continue;
        else if ((need_visible && itm->flags.bits.hidden)
            || (need_hidden && !itm->flags.bits.hidden)
            || (need_forbidden && !itm->flags.bits.forbid)
            || (!need_forbidden && itm->flags.bits.forbid)
        )
            continue;

        if (!destroy) // Move to cursor
        {   // Don't move items if they're already at the cursor
            if (pos_cursor != itm->pos) {
                if (Items::moveToGround(itm, pos_cursor))
                {   // Change flags to indicate the dump was completed, as if by super-dwarfs
                    itm->flags.bits.dump = false;
                    itm->flags.bits.forbid = true;
                }
                else
                    out.print("Could not move item: %s\n", Items::getDescription(itm, 0, true).c_str());
            }
        }
        else // Destroy
        {
            if (here && itm->pos != pos_cursor)
                continue;

            itm->flags.bits.garbage_collect = true;

            // Cosmetic changes: make them disappear from view instantly
            itm->flags.bits.forbid = true;
            itm->flags.bits.hidden = true;
        }

        dumped_total++;
    }

    out.print("Done. %d items %s.\n", dumped_total, destroy ? "marked for destruction" : "quickdumped");
    return CR_OK;
}

command_result df_autodump(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;
    return autodump_main(out, parameters);
}

command_result df_autodump_destroy_here(color_ostream &out, vector<string> &parameters) {
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    vector<string> args;
    args.push_back("destroy-here");

    CoreSuspender suspend;
    return autodump_main(out, args);
}

static map<int, df::item_flags> pending_destroy;
static int last_frame = 0;

command_result df_autodump_destroy_item(color_ostream &out, vector<string> &parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend;

    df::item *item = Gui::getSelectedItem(out);
    if (!item)
        return CR_FAILURE;

    // Allow undoing the destroy
    if (world->frame_counter != last_frame)
    {
        last_frame = world->frame_counter;
        pending_destroy.clear();
    }

    if (pending_destroy.count(item->id))
    {
        df::item_flags old_flags = pending_destroy[item->id];
        pending_destroy.erase(item->id);

        item->flags.bits.garbage_collect = false;
        item->flags.bits.hidden = old_flags.bits.hidden;
        item->flags.bits.dump = old_flags.bits.dump;
        item->flags.bits.forbid = old_flags.bits.forbid;
        return CR_OK;
    }

    // Check the item is good to destroy
    if (item->flags.bits.garbage_collect) {
        out.printerr("Item is already marked for destroy.\n");
        return CR_FAILURE;
    }

    if (item->flags.bits.construction ||
        item->flags.bits.in_building ||
        item->flags.bits.artifact)
    {
        out.printerr("Choosing not to destroy buildings, constructions and artifacts.\n");
        return CR_FAILURE;
    }

    for (auto ref : item->general_refs) {
        if (ref->getType() == general_ref_type::UNIT_HOLDER) {
            out.printerr("Choosing not to destroy items in unit inventory.\n");
            return CR_FAILURE;
        }
    }

    // Set the flags
    pending_destroy[item->id] = item->flags;

    item->flags.bits.garbage_collect = true;
    item->flags.bits.hidden = true;
    item->flags.bits.dump = true;
    item->flags.bits.forbid = true;
    return CR_OK;
}
