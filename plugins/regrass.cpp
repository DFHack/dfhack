#include "PluginManager.h"
#include "TileTypes.h"

#include "df/block_square_event_grassst.h"
#include "df/map_block.h"
#include "df/world.h"

using std::string;
using std::vector;
using namespace DFHack;

DFHACK_PLUGIN("regrass");
REQUIRE_GLOBAL(world);

command_result df_regrass(color_ostream &out, vector<string> & parameters);

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "regrass",
        "Regrow surface grass and cavern moss.",
        df_regrass));
    return CR_OK;
}

command_result df_regrass(color_ostream &out, vector<string> & parameters) {
    bool max = false;
    if (!parameters.empty())
    {
        if(parameters[0] == "max")
            max = true;
        else
            return CR_WRONG_USAGE;
    }

    CoreSuspender suspend;

    int count = 0;
    for (auto block : world->map.map_blocks) {
        bool found = false;
        for (auto blev : block->block_events) {
            df::block_square_event_type blevtype = blev->getType();
            if (blevtype == df::block_square_event_type::grass) {
                found = true;
                break;
            }
        }

        if (!found) {
            // if there is no existing grass event for the block, we could:
            // - examine other blocks to see what plant types could exist
            // - create one or more grass events for the block with appropriate plant types
            continue;
        }

        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                if (   tileShape(block->tiletype[x][y]) != tiletype_shape::FLOOR
                    || block->occupancy[x][y].bits.building
                    || block->occupancy[x][y].bits.no_grow)
                    continue;

                // don't touch furrowed tiles (dirt roads made on soil)
                if (tileSpecial(block->tiletype[x][y]) == tiletype_special::FURROWED)
                    continue;

                int mat = tileMaterial(block->tiletype[x][y]);
                if (   mat != tiletype_material::SOIL
                    && mat != tiletype_material::GRASS_DARK  // refill existing grass, too
                    && mat != tiletype_material::GRASS_LIGHT // refill existing grass, too
                    )
                    continue;

                // max = set amounts of all grass events on that tile to 100
                if (max) {
                    for (auto blev : block->block_events) {
                        df::block_square_event_type blevtype = blev->getType();
                        if (blevtype == df::block_square_event_type::grass) {
                            auto gr_ev = (df::block_square_event_grassst *)blev;
                            gr_ev->amount[x][y] = 100;
                        }
                    }
                } else {
                    // try to find the 'original' event
                    bool regrew = false;
                    for (auto blev : block->block_events) {
                        df::block_square_event_type blevtype = blev->getType();
                        if (blevtype == df::block_square_event_type::grass) {
                            auto gr_ev = (df::block_square_event_grassst *)blev;
                            if (gr_ev->amount[x][y] > 0) {
                                gr_ev->amount[x][y] = 100;
                                regrew = true;
                                break;
                            }
                        }
                    }
                    // if original could not be found (meaning it was already depleted):
                    // refresh random grass event in the map block
                    if (!regrew) {
                        vector<df::block_square_event_grassst *> gr_evs;
                        for (auto blev : block->block_events) {
                            df::block_square_event_type blevtype = blev->getType();
                            if (blevtype == df::block_square_event_type::grass) {
                                auto gr_ev = (df::block_square_event_grassst *)blev;
                                gr_evs.push_back(gr_ev);
                            }
                        }
                        int r = rand() % gr_evs.size();
                        gr_evs[r]->amount[x][y] = 100;
                    }
                }
                block->tiletype[x][y] = findRandomVariant((rand() & 1) ? tiletype::GrassLightFloor1 : tiletype::GrassDarkFloor1);
                count++;
            }
        }
    }

    if (count)
        out.print("Regrew %d tiles of grass.\n", count);
    return CR_OK;
}
