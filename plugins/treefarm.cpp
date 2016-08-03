#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/EventManager.h"
#include "modules/Once.h"

#include "df/block_burrow.h"
#include "df/block_burrow_link.h"
#include "df/burrow.h"
#include "df/map_block.h"
#include "df/tile_bitmask.h"
#include "df/tile_dig_designation.h"
#include "df/tiletype.h"
#include "df/tiletype_shape.h"
#include "df/world.h"

//#include "df/world.h"

using namespace DFHack;
/*
treefarm
========
Automatically manages special burrows and regularly schedules tree chopping
and digging when appropriate.

Every time the plugin runs, it checks for burrows with a name containing the
string ``"treefarm"``. For each such burrow, it checks every tile in it for
fully-grown trees and for diggable walls. For each fully-grown tree it finds,
it designates the tree to be chopped, and for each natural wall it finds, it
designates the wall to be dug.

Usage:

:treefarm:      Enables treefarm monitoring, starting next frame
:treefarm n:    Enables treefarm monitoring, starting next frame, and sets
                interval to n frames.  If n is less than one, disables monitoring.
*/

DFHACK_PLUGIN("treefarm");
DFHACK_PLUGIN_IS_ENABLED(enabled);

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);

void checkFarms(color_ostream& out, void* ptr);
command_result treefarm (color_ostream &out, std::vector <std::string> & parameters);

EventManager::EventHandler handler(&checkFarms, -1);
int32_t frequency = 1200*30;

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "treefarm",
        "automatically manages special burrows and regularly schedules tree chopping and digging when appropriate",
        treefarm,
        false, //allow non-interactive use
        "treefarm\n"
        "  enables treefarm monitoring, starting next frame\n"
        "treefarm n\n"
        "  enables treefarm monitoring, starting next frame\n"
        "  sets monitoring interval to n frames\n"
        "  if n is less than one, disables monitoring\n"
        "\n"
        "Every time the plugin runs, it checks for burrows with a name containing the string \"treefarm\". For each such burrow, it checks every tile in it for fully-grown trees and for diggable walls. For each fully-grown tree it finds, it designates the tree to be chopped, and for each natural wall it finds, it designates the wall to be dug.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream& out, bool enable) {
    enabled = enable;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

void checkFarms(color_ostream& out, void* ptr) {
    EventManager::unregisterAll(plugin_self);
    if ( !enabled )
        return;
    EventManager::registerTick(handler, frequency, plugin_self);
    CoreSuspender suspend;

    int32_t xOffset = world->map.region_x*3;
    int32_t yOffset = world->map.region_y*3;
    int32_t zOffset = world->map.region_z;
    //for each burrow named treefarm or obsidianfarm, check if you can dig/chop any obsidian/trees
    for ( size_t a = 0; a < df::burrow::get_vector().size(); a++ ) {
        df::burrow* burrow = df::burrow::get_vector()[a];
        if ( !burrow || burrow->name.find("treefarm") == std::string::npos )
            continue;

        if ( burrow->block_x.size() != burrow->block_y.size() || burrow->block_x.size() != burrow->block_z.size() )
            continue;

        for ( size_t b = 0; b < burrow->block_x.size(); b++ ) {
            int32_t x=burrow->block_x[b] - xOffset;
            int32_t y=burrow->block_y[b] - yOffset;
            int32_t z=burrow->block_z[b] - zOffset;

            df::map_block* block = world->map.block_index[x][y][z];
            if ( !block )
                continue;

            df::block_burrow_link* link = &block->block_burrows;
            df::tile_bitmask mask;
            for ( ; link != NULL; link = link->next ) {
                if ( link->item == NULL )
                    continue;
                if ( link->item->id == burrow->id ) {
                    mask = link->item->tile_bitmask;
                    break;
                }
            }
            if ( link == NULL )
                continue;

            for ( int32_t x = 0; x < 16; x++ ) {
                for ( int32_t y = 0; y < 16; y++ ) {
                    if ( !mask.getassignment(x,y) )
                        continue;
                    df::tiletype type = block->tiletype[x][y];
                    df::tiletype_shape shape = ENUM_ATTR(tiletype, shape, type);
                    if ( !block->designation[x][y].bits.hidden &&
                         shape != df::enums::tiletype_shape::WALL &&
                         shape != df::enums::tiletype_shape::TREE )
                        continue;
                    if ( shape != df::enums::tiletype_shape::TREE ) {
                        if ( x == 0 && (block->map_pos.x/16) == 0 )
                            continue;
                        if ( y == 0 && (block->map_pos.y/16) == 0 )
                            continue;
                        if ( x == 15 && (block->map_pos.x/16) == world->map.x_count_block-1 )
                            continue;
                        if ( y == 15 && (block->map_pos.y/16) == world->map.y_count_block-1 )
                            continue;
                    }

                    block->designation[x][y].bits.dig = df::enums::tile_dig_designation::Default;
                }
            }
        }
    }
}

command_result treefarm (color_ostream &out, std::vector <std::string> & parameters)
{
    EventManager::unregisterAll(plugin_self);

    if ( parameters.size() > 1 )
        return CR_WRONG_USAGE;
    if ( parameters.size() == 1 ) {
        int32_t i = atoi(parameters[0].c_str());
        if ( i < 1 ) {
            plugin_enable(out, false);
            out.print("treefarm disabled\n");
            return CR_OK;
        }
        plugin_enable(out, true);
        frequency = i;
    }

    if ( enabled ) {
        EventManager::registerTick(handler, 1, plugin_self);
        out.print("treefarm enabled with update frequency %d ticks\n", frequency);
    }
    return CR_OK;
}

