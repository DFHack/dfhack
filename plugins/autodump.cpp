// Quick Dumper : Moves items marked as "dump" to cursor
// FIXME: local item cache in map blocks needs to be fixed after teleporting items
#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
using namespace std;

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Materials.h"
#include "modules/MapCache.h"

#include "DataDefs.h"
#include "df/item.h"
#include "df/world.h"
#include "df/general_ref.h"

using namespace DFHack;
using namespace df::enums;

using MapExtras::Block;
using MapExtras::MapCache;
using df::global::world;

DFHACK_PLUGIN("autodump");

command_result df_autodump(color_ostream &out, vector <string> & parameters);
command_result df_autodump_destroy_here(color_ostream &out, vector <string> & parameters);
command_result df_autodump_destroy_item(color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "autodump", "Teleport items marked for dumping to the cursor.",
        df_autodump, false,
        "  This utility lets you quickly move all items designated to be dumped.\n"
        "  Items are instantly moved to the cursor position, the dump flag is unset,\n"
        "  and the forbid flag is set, as if it had been dumped normally.\n"
        "  Be aware that any active dump item tasks still point at the item.\n"
        "Options:\n"
        "  destroy       - instead of dumping, destroy the items instantly.\n"
        "  destroy-here  - only affect the tile under cursor.\n"
        "  visible       - only process items that are not hidden.\n"
        "  hidden        - only process hidden items.\n"
        "  forbidden     - only process forbidden items (default: only unforbidden).\n"
    ));
    commands.push_back(PluginCommand(
        "autodump-destroy-here", "Destroy items marked for dumping under cursor.",
        df_autodump_destroy_here, Gui::cursor_hotkey,
        "  Identical to autodump destroy-here, but intended for use as keybinding.\n"
    ));
    commands.push_back(PluginCommand(
        "autodump-destroy-item", "Destroy the selected item.",
        df_autodump_destroy_item, Gui::any_item_hotkey,
        "  Destroy the selected item. The item may be selected\n"
        "  in the 'k' list, or inside a container. If called\n"
        "  again before the game is resumed, cancels destroy.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

typedef map <DFCoord, uint32_t> coordmap;

static command_result autodump_main(color_ostream &out, vector <string> & parameters)
{
    // Command line options
    bool destroy = false;
    bool here = false;
    bool need_visible = false;
    bool need_hidden = false;
    bool need_forbidden = false;
    for (size_t i = 0; i < parameters.size(); i++)
    {
        string & p = parameters[i];
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

    if (need_visible && need_hidden)
    {
        out.printerr("An item can't be both hidden and visible.\n");
        return CR_WRONG_USAGE;
    }

    //DFHack::VersionInfo *mem = Core::getInstance().vinfo;
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    size_t numItems = world->items.all.size();

    MapCache MC;
    int i = 0;
    int dumped_total = 0;

    int cx, cy, cz;
    DFCoord pos_cursor;
    if(!destroy || here)
    {
        if (!Gui::getCursorCoords(cx,cy,cz))
        {
            out.printerr("Cursor position not found. Please enabled the cursor.\n");
            return CR_FAILURE;
        }
        pos_cursor = DFCoord(cx,cy,cz);
    }
    if (!destroy)
    {
        {
            Block * b = MC.BlockAt(pos_cursor / 16);
            if(!b)
            {
                out.printerr("Cursor is in an invalid/uninitialized area. Place it over a floor.\n");
                return CR_FAILURE;
            }
            df::tiletype ttype = MC.tiletypeAt(pos_cursor);
            if(!DFHack::isFloorTerrain(ttype))
            {
                out.printerr("Cursor should be placed over a floor.\n");
                return CR_FAILURE;
            }
        }
    }
    coordmap counts;
    // proceed with the dumpification operation
    for(size_t i=0; i< numItems; i++)
    {
        df::item * itm = world->items.all[i];
        DFCoord pos_item(itm->pos.x, itm->pos.y, itm->pos.z);

        // keep track how many items are at places. all items.
        coordmap::iterator it = counts.find(pos_item);
        if(it == counts.end())
        {
            pair< coordmap::iterator, bool > inserted = counts.insert(make_pair(pos_item,1));
            it = inserted.first;
        }
        else
        {
            it->second ++;
        }
        // iterator is valid here, we use it later to decrement the pile counter if the item is moved

        // only dump the stuff marked for dumping and laying on the ground
        if (   !itm->flags.bits.dump
            || !itm->flags.bits.on_ground
            ||  itm->flags.bits.construction
            ||  itm->flags.bits.in_building
            ||  itm->flags.bits.in_chest
            ||  itm->flags.bits.in_inventory
            ||  itm->flags.bits.artifact1
        )
            continue;

        if (need_visible && itm->flags.bits.hidden)
            continue;
        if (need_hidden && !itm->flags.bits.hidden)
            continue;
        if (need_forbidden && !itm->flags.bits.forbid)
            continue;
        if (!need_forbidden && itm->flags.bits.forbid)
            continue;

        if(!destroy) // move to cursor
        {
            // Change flags to indicate the dump was completed, as if by super-dwarfs
            itm->flags.bits.dump = false;
            itm->flags.bits.forbid = true;

            // Don't move items if they're already at the cursor
            if (pos_cursor == pos_item)
                continue;

            // Do we need to fix block-local item ID vector?
            if(pos_item/16 != pos_cursor/16)
            {
                // yes...
                cerr << "Moving from block to block!" << endl;
                df::map_block * bl_src = Maps::getBlockAbs(itm->pos.x, itm->pos.y, itm->pos.z);
                df::map_block * bl_tgt = Maps::getBlockAbs(cx, cy, cz);
                if(bl_src)
                {
                    remove(bl_src->items.begin(), bl_src->items.end(),itm->id);
                }
                else
                {
                    cerr << "No source block" << endl;
                }
                if(bl_tgt)
                {
                    bl_tgt->items.push_back(itm->id);
                }
                else
                {
                    cerr << "No target block" << endl;
                }
            }

            // Move the item
            itm->pos.x = pos_cursor.x;
            itm->pos.y = pos_cursor.y;
            itm->pos.z = pos_cursor.z;
        }
        else // destroy
        {
            if (here && pos_item != pos_cursor)
                continue;

            itm->flags.bits.garbage_colect = true;

            // Cosmetic changes: make them disappear from view instantly
            itm->flags.bits.forbid = true;
            itm->flags.bits.hidden = true;
        }
        // keeping track of item pile sizes ;)
        it->second --;
        dumped_total++;
    }
    if(!destroy) // TODO: do we have to do any of this when destroying items?
    {
        // for each item pile, see if it reached zero. if so, unset item flag on the tile it's on
        coordmap::iterator it = counts.begin();
        coordmap::iterator end = counts.end();
        while(it != end)
        {
            if(it->second == 0)
            {
                df::tile_occupancy occ = MC.occupancyAt(it->first);
                occ.bits.item = false;
                MC.setOccupancyAt(it->first, occ);
            }
            it++;
        }
        // Set "item here" flag on target tile, if we moved any items to the target tile.
        if (dumped_total > 0)
        {
            // assume there is a possibility the cursor points at some weird location with missing block data
            Block * b = MC.BlockAt(pos_cursor / 16);
            if(b)
            {
                df::tile_occupancy occ = MC.occupancyAt(pos_cursor);
                occ.bits.item = 1;
                MC.setOccupancyAt(pos_cursor,occ);
            }
        }
        // write map changes back to DF.
        MC.WriteAll();
        // Is this necessary?  Is "forbid" a dirtyable attribute like "dig" is?
        Maps::WriteDirtyBit(cx/16, cy/16, cz, true);
    }
    out.print("Done. %d items %s.\n", dumped_total, destroy ? "marked for destruction" : "quickdumped");
    return CR_OK;
}

command_result df_autodump(color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    return autodump_main(out, parameters);
}

command_result df_autodump_destroy_here(color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND; CORE ALREADY SUSPENDED
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    vector<string> args;
    args.push_back("destroy-here");

    return autodump_main(out, args);
}

static map<int, df::item_flags> pending_destroy;
static int last_frame = 0;

command_result df_autodump_destroy_item(color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND; CORE ALREADY SUSPENDED
    if (!parameters.empty())
        return CR_WRONG_USAGE;

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

        item->flags.bits.garbage_colect = false;
        item->flags.bits.hidden = old_flags.bits.hidden;
        item->flags.bits.dump = old_flags.bits.dump;
        item->flags.bits.forbid = old_flags.bits.forbid;
        return CR_OK;
    }

    // Check the item is good to destroy
    if (item->flags.bits.garbage_colect)
    {
        out.printerr("Item is already marked for destroy.\n");
        return CR_FAILURE;
    }

    if (item->flags.bits.construction ||
        item->flags.bits.in_building ||
        item->flags.bits.artifact1)
    {
        out.printerr("Choosing not to destroy buildings, constructions and artifacts.\n");
        return CR_FAILURE;
    }

    for (size_t i = 0; i < item->itemrefs.size(); i++)
    {
        df::general_ref *ref = item->itemrefs[i];
        if (ref->getType() == general_ref_type::UNIT_HOLDER)
        {
            out.printerr("Choosing not to destroy items in unit inventory.\n");
            return CR_FAILURE;
        }
    }

    // Set the flags
    pending_destroy[item->id] = item->flags;

    item->flags.bits.garbage_colect = true;
    item->flags.bits.hidden = true;
    item->flags.bits.dump = true;
    item->flags.bits.forbid = true;
    return CR_OK;
}
