#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/MapCache.h"
using namespace DFHack;

#include <fstream>

#include "DataDefs.h"
#include "df/world.h"

#include "proto/Map.pb.h"

DFhackCExport command_result mapexport (Core * c, std::vector <std::string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "mapexport";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    commands.clear();
    commands.push_back(PluginCommand("mapexport", "Exports the current map to a file.", mapexport, true));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    google::protobuf::ShutdownProtobufLibrary();
    return CR_OK;
}

DFhackCExport command_result mapexport (Core * c, std::vector <std::string> & parameters)
{
    for(int i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            c->con.print("Exports the currently visible map to a file.\n"
                         "Usage: mapexport <filename>\n"
			);
            return CR_OK;
        }
    }
    std::string filename;
    if (parameters.size() < 1)
    {
            c->con.printerr("Please supply a filename.\n");
            return CR_OK;
    }
    filename = parameters[0];

    bool showHidden = true;

    uint32_t x_max=0, y_max=0, z_max=0;
    c->Suspend();
    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        c->Resume();
        return CR_FAILURE;
    }
    Maps::getSize(x_max, y_max, z_max);
    MapExtras::MapCache map;
    DFHack::Materials *mats = c->getMaterials();

    dfproto::Map protomap;
    protomap.set_x_size(x_max);
    protomap.set_y_size(y_max);
    protomap.set_z_size(z_max);

    DFHack::t_feature blockFeatureGlobal;
    DFHack::t_feature blockFeatureLocal;

    for(uint32_t z = 0; z < z_max; z++)
    {
        for(uint32_t b_y = 0; b_y < y_max; b_y++)
        {
            for(uint32_t b_x = 0; b_x < x_max; b_x++)
            {
                // Get the map block
                df::coord2d blockCoord(b_x, b_y);
                MapExtras::Block *b = map.BlockAt(DFHack::DFCoord(b_x, b_y, z));
                if (!b || !b->valid)
                {
                    continue;
                }

                dfproto::Block *protoblock = protomap.add_block();
                protoblock->set_x(b_x);
                protoblock->set_y(b_y);
                protoblock->set_z(z);

                { // Find features
                    uint32_t index = b->raw.global_feature;
                    if (index != -1)
                        Maps::GetGlobalFeature(blockFeatureGlobal, index);

                    index = b->raw.local_feature;
                    if (index != -1)
                        Maps::GetLocalFeature(blockFeatureLocal, blockCoord, index);
                }

                int global_z = df::global::world->map.region_z + z;

                // Iterate over all the tiles in the block
                for(uint32_t y = 0; y < 16; y++)
                {
                    for(uint32_t x = 0; x < 16; x++)
                    {
                        df::coord2d coord(x, y);
                        df::tile_designation des = b->DesignationAt(coord);
                        df::tile_occupancy occ = b->OccupancyAt(coord);

                        // Skip hidden tiles
                        if (!showHidden && des.bits.hidden)
                        {
                            continue;
                        }

                        dfproto::Tile *prototile = protoblock->add_tile();
                        prototile->set_x(x);
                        prototile->set_y(y);

                        // Check for liquid
                        if (des.bits.flow_size)
                        {
                            //if (des.bits.liquid_type == df::tile_liquid::Magma)

                            //else
                        }

                        uint16_t type = b->TileTypeAt(coord);
                        const DFHack::TileRow *info = DFHack::getTileRow(type);
                        prototile->set_type((dfproto::Tile::TileType)info->shape);
                        /*switch (info->shape)
                        {
                        case DFHack::WALL:
                            prototile->set_type(dfproto::Tile::WALL);
                            break;
                        default:
                            break;
                        }*/
                    }
                }
            } // block x

            // Clean uneeded memory
            map.trash();
        } // block y
    } // z

    std::ofstream output(filename, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!output.is_open())
    {
            c->con.printerr("Couldn't open the output file.\n");
            c->Resume();
            return CR_FAILURE;
    }
    if (!protomap.SerializeToOstream(&output))
    {
            c->con.printerr("Failed to save map file.\n");
            c->Resume();
            return CR_FAILURE;
    }
    c->con.print("Map succesfully exported.\n");
    c->Resume();
    return CR_OK;
}
