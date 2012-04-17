#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Maps.h"

#include "DataDefs.h"
#include "df/item_actual.h"
#include "df/unit.h"
#include "df/unit_spatter.h"
#include "df/matter_state.h"
#include "df/global_objects.h"
#include "df/builtin_mats.h"
#include "df/contaminant.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::cursor;

DFHACK_PLUGIN("cleaners");

command_result cleanmap (color_ostream &out, bool snow, bool mud)
{
    // Invoked from clean(), already suspended
    int num_blocks = 0, blocks_total = world->map.map_blocks.size();
    for (int i = 0; i < blocks_total; i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        bool cleaned = false;
        for(int x = 0; x < 16; x++)
        {
            for(int y = 0; y < 16; y++)
            {
                block->occupancy[x][y].bits.arrow_color = 0;
                block->occupancy[x][y].bits.arrow_variant = 0;
            }
        }
        for (size_t j = 0; j < block->block_events.size(); j++)
        {
            df::block_square_event *evt = block->block_events[j];
            if (evt->getType() != block_square_event_type::material_spatter)
                continue;
            // type verified - recast to subclass
            df::block_square_event_material_spatterst *spatter = (df::block_square_event_material_spatterst *)evt;

            // filter snow
            if(!snow
                && spatter->mat_type == builtin_mats::WATER
                && spatter->mat_state == matter_state::Powder)
                continue;
            // filter mud
            if(!mud
                && spatter->mat_type == builtin_mats::MUD
                && spatter->mat_state == matter_state::Solid)
                continue;

            delete evt;
            block->block_events.erase(block->block_events.begin() + j);
            j--;
            cleaned = true;
        }
        num_blocks += cleaned;
    }

    if(num_blocks)
        out.print("Cleaned %d of %d map blocks.\n", num_blocks, blocks_total);
    return CR_OK;
}

command_result cleanitems (color_ostream &out)
{
    // Invoked from clean(), already suspended
    int cleaned_items = 0, cleaned_total = 0;
    for (size_t i = 0; i < world->items.all.size(); i++)
    {
        // currently, all item classes extend item_actual, so this should be safe
        df::item_actual *item = (df::item_actual *)world->items.all[i];
        if (item->contaminants && item->contaminants->size())
        {
            for (size_t j = 0; j < item->contaminants->size(); j++)
                delete item->contaminants->at(j);
            cleaned_items++;
            cleaned_total += item->contaminants->size();
            item->contaminants->clear();
        }
    }
    if (cleaned_total)
        out.print("Removed %d contaminants from %d items.\n", cleaned_total, cleaned_items);
    return CR_OK;
}

command_result cleanunits (color_ostream &out)
{
    // Invoked from clean(), already suspended
    int cleaned_units = 0, cleaned_total = 0;
    for (size_t i = 0; i < world->units.all.size(); i++)
    {
        df::unit *unit = world->units.all[i];
        if (unit->body.spatters.size())
        {
            for (size_t j = 0; j < unit->body.spatters.size(); j++)
                delete unit->body.spatters[j];
            cleaned_units++;
            cleaned_total += unit->body.spatters.size();
            unit->body.spatters.clear();
        }
    }
    if (cleaned_total)
        out.print("Removed %d contaminants from %d creatures.\n", cleaned_total, cleaned_units);
    return CR_OK;
}

command_result spotclean (color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    if (cursor->x == -30000)
    {
        out.printerr("The cursor is not active.\n");
        return CR_WRONG_USAGE;
    }
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available.\n");
        return CR_FAILURE;
    }
    df::map_block *block = Maps::getTileBlock(cursor->x, cursor->y, cursor->z);
    if (block == NULL)
    {
        out.printerr("Invalid map block selected!\n");
        return CR_FAILURE;
    }

    for (size_t i = 0; i < block->block_events.size(); i++)
    {
        df::block_square_event *evt = block->block_events[i];
        if (evt->getType() != block_square_event_type::material_spatter)
            continue;
        // type verified - recast to subclass
        df::block_square_event_material_spatterst *spatter = (df::block_square_event_material_spatterst *)evt;
        spatter->amount[cursor->x % 16][cursor->y % 16] = 0;
    }
    return CR_OK;
}

command_result clean (color_ostream &out, vector <string> & parameters)
{
    bool map = false;
    bool snow = false;
    bool mud = false;
    bool units = false;
    bool items = false;
    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "map")
            map = true;
        else if(parameters[i] == "units")
            units = true;
        else if(parameters[i] == "items")
            items = true;
        else if(parameters[i] == "all")
        {
            map = true;
            items = true;
            units = true;
        }
        else if(parameters[i] == "snow")
            snow = true;
        else if(parameters[i] == "mud")
            mud = true;
        else
            return CR_WRONG_USAGE;
    }
    if(!map && !units && !items)
        return CR_WRONG_USAGE;

    CoreSuspender suspend;

    if(map)
        cleanmap(out,snow,mud);
    if(units)
        cleanunits(out);
    if(items)
        cleanitems(out);
    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "clean","Removes contaminants from map tiles, items and creatures.",
        clean, false,
        "  Removes contaminants from map tiles, items and creatures.\n"
        "Options:\n"
        "  map        - clean the map tiles\n"
        "  items      - clean all items\n"
        "  units      - clean all creatures\n"
        "  all        - clean everything.\n"
        "More options for 'map':\n"
        "  snow       - also remove snow\n"
        "  mud        - also remove mud\n"
        "Example:\n"
        "  clean all mud snow\n"
        "    Removes all spatter, including mud and snow from map tiles.\n"
    ));
    commands.push_back(PluginCommand(
        "spotclean","Cleans map tile under cursor.",
        spotclean,Gui::cursor_hotkey
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
