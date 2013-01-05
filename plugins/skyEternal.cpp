
#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/World.h"

#include "df/construction.h"
#include "df/game_mode.h"
#include "df/map_block.h"
#include "df/world.h"
#include "df/z_level_flags.h"

#include <cstring>
#include <string>
#include <vector>

using namespace std;

using namespace DFHack;
using namespace df::enums;

command_result skyEternal (color_ostream &out, std::vector <std::string> & parameters);

DFHACK_PLUGIN("skyEternal");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "skyEternal", 
        "Creates new sky levels on request, or as you construct up.",
        skyEternal, false, 
        "Usage:\n"
        "  skyEternal\n"
        "    creates one more z-level\n"
        "  skyEternal [n]\n"
        "    creates n more z-level(s)\n"
        "  skyEternal enable\n"
        "    enables monitoring of constructions\n"
        "  skyEternal disable\n"
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
static bool enabled = false;

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    if ( !enabled )
        return CR_OK;
    if ( !Core::getInstance().isMapLoaded() )
        return CR_OK;
    {
        t_gamemodes mode;
        if ( !Core::getInstance().getWorld()->ReadGameMode(mode) )
            return CR_FAILURE;
        if ( mode.g_mode != df::enums::game_mode::DWARF )
            return CR_OK;
    }
    
    if ( df::global::world->constructions.size() == constructionSize )
        return CR_OK;
    int32_t zNow = df::global::world->map.z_count_block;
    vector<string> vec;
    for ( size_t a = constructionSize; a < df::global::world->constructions.size(); a++ ) {
        df::construction* construct = df::global::world->constructions[a];
        if ( construct->pos.z+2 < zNow )
            continue;
        skyEternal(out, vec);
        zNow = df::global::world->map.z_count_block;
        ///break;
    }
    constructionSize = df::global::world->constructions.size();
    
    return CR_OK;
}

command_result skyEternal (color_ostream &out, std::vector <std::string> & parameters)
{
    if ( parameters.size() > 1 )
        return CR_WRONG_USAGE;
    if ( parameters.size() == 0 ) {
        vector<string> vec;
        vec.push_back("1");
        return skyEternal(out, vec);
    }
    if (parameters[0] == "enable") {
        enabled = true;
        return CR_OK;
    }
    if (parameters[0] == "disable") {
        enabled = false;
        constructionSize = 0;
        return CR_OK;
    }
    int32_t howMany = 0;
    howMany = atoi(parameters[0].c_str());
    df::world* world = df::global::world;
    CoreSuspender suspend;
    int32_t x_count_block = world->map.x_count_block;
    int32_t y_count_block = world->map.y_count_block;
    for ( int32_t count = 0; count < howMany; count++ ) {
        //change the size of the pointer stuff
        int32_t z_count_block = world->map.z_count_block;
        df::map_block**** block_index = world->map.block_index;
        for ( int32_t a = 0; a < x_count_block; a++ ) {
            for ( int32_t b = 0; b < y_count_block; b++ ) {
                df::map_block** column = new df::map_block*[z_count_block+1];
                memcpy(column, block_index[a][b], z_count_block*sizeof(df::map_block*));
                column[z_count_block] = NULL;
                delete[] block_index[a][b];
                block_index[a][b] = column;
            }
        }
        df::z_level_flags* flags = new df::z_level_flags[z_count_block+1];
        memcpy(flags, world->map.z_level_flags, z_count_block*sizeof(df::z_level_flags));
        flags[z_count_block].whole = 0;
        flags[z_count_block].bits.update = 1;
        world->map.z_count_block++;
        world->map.z_count++;
    }
    
    return CR_OK;
}
