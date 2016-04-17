
#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/World.h"

#include "df/construction.h"
#include "df/game_mode.h"
#include "df/map_block.h"
#include "df/map_block_column.h"
#include "df/world.h"
#include "df/z_level_flags.h"

#include <cstring>
#include <string>
#include <vector>

using namespace std;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("infiniteSky");
REQUIRE_GLOBAL(world);

command_result infiniteSky (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "infiniteSky",
        "Creates new sky levels on request, or as needed.",
        infiniteSky, false,
        "Usage:\n"
        "  infiniteSky\n"
        "    creates one more z-level\n"
        "  infiniteSky [n]\n"
        "    creates n more z-level(s)\n"
        "  infiniteSky enable\n"
        "    enables monitoring of constructions\n"
        "  infiniteSky disable\n"
        "    disable monitoring of constructions\n"
        "\n"
        "If construction monitoring is enabled, then the plugin will automatically create new sky z-levels as you construct upward.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

/*
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_GAME_LOADED:
        // initialize from the world just loaded
        break;
    case SC_GAME_UNLOADED:
        // cleanup
        break;
    default:
        break;
    }
    return CR_OK;
}
*/

static size_t constructionSize = 0;
DFHACK_PLUGIN_IS_ENABLED(enabled);
void doInfiniteSky(color_ostream& out, int32_t howMany);

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    if ( !enabled )
        return CR_OK;
    if ( !Core::getInstance().isMapLoaded() )
        return CR_OK;
    {
        t_gamemodes mode;
        if ( !World::ReadGameMode(mode) )
            return CR_FAILURE;
        if ( mode.g_mode != df::enums::game_mode::DWARF )
            return CR_OK;
    }

    if ( world->constructions.size() == constructionSize )
        return CR_OK;
    int32_t zNow = world->map.z_count_block;
    for ( size_t a = constructionSize; a < world->constructions.size(); a++ ) {
        df::construction* construct = world->constructions[a];
        if ( construct->pos.z+2 < zNow )
            continue;
        doInfiniteSky(out, 1);
        zNow = world->map.z_count_block;
        ///break;
    }
    constructionSize = world->constructions.size();

    return CR_OK;
}

void doInfiniteSky(color_ostream& out, int32_t howMany) {
    CoreSuspender suspend;
    int32_t x_count_block = world->map.x_count_block;
    int32_t y_count_block = world->map.y_count_block;
    for ( int32_t count = 0; count < howMany; count++ ) {
        //change the size of the pointer stuff
        int32_t z_count_block = world->map.z_count_block;
        df::map_block**** block_index = world->map.block_index;
        for ( int32_t a = 0; a < x_count_block; a++ ) {
            for ( int32_t b = 0; b < y_count_block; b++ ) {
                df::map_block** blockColumn = new df::map_block*[z_count_block+1];
                memcpy(blockColumn, block_index[a][b], z_count_block*sizeof(df::map_block*));
                blockColumn[z_count_block] = NULL;
                delete[] block_index[a][b];
                block_index[a][b] = blockColumn;

                //deal with map_block_column stuff even though it'd probably be fine
                df::map_block_column* column = world->map.column_index[a][b];
                if ( !column ) {
                    out.print("%s, line %d: column is null (%d, %d).\n", __FILE__, __LINE__, a, b);
                    continue;
                }
                df::map_block_column::T_unmined_glyphs* glyphs = new df::map_block_column::T_unmined_glyphs;
                glyphs->x[0] = 0;
                glyphs->x[1] = 1;
                glyphs->x[2] = 2;
                glyphs->x[3] = 3;
                glyphs->y[0] = 0;
                glyphs->y[1] = 0;
                glyphs->y[2] = 0;
                glyphs->y[3] = 0;
                glyphs->tile[0] = 'e';
                glyphs->tile[1] = 'x';
                glyphs->tile[2] = 'p';
                glyphs->tile[3] = '^';
                column->unmined_glyphs.push_back(glyphs);
            }
        }
        df::z_level_flags* flags = new df::z_level_flags[z_count_block+1];
        memcpy(flags, world->map_extras.z_level_flags, z_count_block*sizeof(df::z_level_flags));
        flags[z_count_block].whole = 0;
        flags[z_count_block].bits.update = 1;
        world->map.z_count_block++;
        world->map.z_count++;
        delete[] world->map_extras.z_level_flags;
        world->map_extras.z_level_flags = flags;
    }

}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    enabled = enable;
    return CR_OK;
}

command_result infiniteSky (color_ostream &out, std::vector <std::string> & parameters)
{
    if ( parameters.size() > 1 )
        return CR_WRONG_USAGE;
    if ( parameters.size() == 0 ) {
        out.print("Construction monitoring is %s.\n", enabled ? "enabled" : "disabled");
        return CR_OK;
    }
    if (parameters[0] == "enable") {
        enabled = true;
        out.print("Construction monitoring enabled.\n");
        return CR_OK;
    }
    if (parameters[0] == "disable") {
        enabled = false;
        out.print("Construction monitoring disabled.\n");
        constructionSize = 0;
        return CR_OK;
    }
    int32_t howMany = 0;
    howMany = atoi(parameters[0].c_str());
    out.print("InfiniteSky: creating %d new z-level%s of sky.\n", howMany, howMany == 1 ? "" : "s" );
    doInfiniteSky(out, howMany);
    return CR_OK;
}
