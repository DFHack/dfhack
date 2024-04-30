
#include "PluginManager.h"

#include "modules/Maps.h"

#include "df/map_block.h"
#include "df/world.h"

using namespace DFHack;
using std::string;
using std::vector;

DFHACK_PLUGIN("lair");
REQUIRE_GLOBAL(world);

enum state
{
    LAIR_RESET,
    LAIR_SET,
};

command_result lair(color_ostream &out, vector<string> & params) {
    state do_what = LAIR_SET;
    for (auto & param : params) {
        if (param == "reset")
            do_what = LAIR_RESET;
    }

    CoreSuspender lock;
    if (!Maps::IsValid()) {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    for (auto block : world->map.map_blocks) {
        auto & occupancies = block->occupancy;
        // for each tile in block
        for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++) {
            // set to revealed
            occupancies[x][y].bits.monster_lair = (do_what == LAIR_SET);
        }
    }
    if (do_what == LAIR_SET)
        out.print("Map marked as lair.\n");
    else
        out.print("Map no longer marked as lair.\n");

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "lair",
        "Prevent item scatter when a site is reclaimed or visited.",
        lair));
    return CR_OK;
}
