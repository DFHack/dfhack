#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/MapCache.h"
using namespace DFHack;

#include <fstream>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/gzip_stream.h>
using namespace google::protobuf::io;

#include "DataDefs.h"
#include "df/world.h"
#include "df/plant.h"
#include "modules/Constructions.h"

#include "proto/Map.pb.h"
#include "proto/Block.pb.h"

using namespace DFHack;
using df::global::world;
/*
mapexport
=========
Export the current loaded map as a file. This was used by visualizers for
DF 0.34.11, but is now basically obsolete.
*/

typedef std::vector<df::plant *> PlantList;

command_result mapexport (color_ostream &out, std::vector <std::string> & parameters);

DFHACK_PLUGIN("mapexport");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    commands.push_back(PluginCommand("mapexport", "Exports the current map to a file.", mapexport, true));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

static dfproto::Tile::TileMaterialType toProto(df::tiletype_material mat)
{
    /*
     * This is surely ugly, but casting enums without officially
     * defined numerical values to protobuf enums is against the
     * way protobufs are supposed to be used, because it defeats
     * the backward compatible nature of the protocols.
     */
    switch (mat)
    {
#define CONVERT(name) case tiletype_material::name: return dfproto::Tile::name;
        case tiletype_material::NONE:
        CONVERT(AIR)
        case tiletype_material::PLANT:
        CONVERT(SOIL)
        CONVERT(STONE)
        CONVERT(FEATURE)
        CONVERT(LAVA_STONE)
        CONVERT(MINERAL)
        CONVERT(FROZEN_LIQUID)
        CONVERT(CONSTRUCTION)
        CONVERT(GRASS_LIGHT)
        CONVERT(GRASS_DARK)
        CONVERT(GRASS_DRY)
        CONVERT(GRASS_DEAD)
        CONVERT(HFS)
        CONVERT(CAMPFIRE)
        CONVERT(FIRE)
        CONVERT(ASHES)
        case tiletype_material::MAGMA:
            return dfproto::Tile::MAGMA_TYPE;
        CONVERT(DRIFTWOOD)
        CONVERT(POOL)
        CONVERT(BROOK)
        CONVERT(RIVER)
#undef CONVERT
    }
    return dfproto::Tile::AIR;
}

command_result mapexport (color_ostream &out, std::vector <std::string> & parameters)
{
    bool showHidden = false;

    int filenameParameter = 1;

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            out.print("Exports the currently visible map to a file.\n"
                         "Usage: mapexport [options] <filename>\n"
                         "Example: mapexport all embark.dfmap\n"
                         "Options:\n"
                         "   all   - Export the entire map, not just what's revealed.\n"
            );
            return CR_OK;
        }
        if (parameters[i] == "all")
        {
            showHidden = true;
            filenameParameter++;
        }
    }

    CoreSuspender suspend;

    uint32_t x_max=0, y_max=0, z_max=0;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    if (parameters.size() < filenameParameter)
    {
        out.printerr("Please supply a filename.\n");
        return CR_FAILURE;
    }

    std::string filename = parameters[filenameParameter-1];
    if (filename.rfind(".dfmap") == std::string::npos) filename += ".dfmap";
    out << "Writing to " << filename << "..." << std::endl;

    std::ofstream output_file(filename.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
    if (!output_file.is_open())
    {
        out.printerr("Couldn't open the output file.\n");
        return CR_FAILURE;
    }
    ZeroCopyOutputStream *raw_output = new OstreamOutputStream(&output_file);
    GzipOutputStream *zip_output = new GzipOutputStream(raw_output);
    CodedOutputStream *coded_output = new CodedOutputStream(zip_output);

    coded_output->WriteLittleEndian32(0x50414DDF); //Write our file header

    Maps::getSize(x_max, y_max, z_max);
    MapExtras::MapCache map;
    DFHack::Materials *mats = Core::getInstance().getMaterials();

    out << "Writing  map info..." << std::endl;

    dfproto::Map protomap;
    protomap.set_x_size(x_max);
    protomap.set_y_size(y_max);
    protomap.set_z_size(z_max);

    out << "Writing material dictionary..." << std::endl;

    for (size_t i = 0; i < world->raws.inorganics.size(); i++)
    {
        dfproto::Material *protomaterial = protomap.add_inorganic_material();
        protomaterial->set_index(i);
        protomaterial->set_name(world->raws.inorganics[i]->id);
    }

    for (size_t i = 0; i < world->raws.plants.all.size(); i++)
    {
        dfproto::Material *protomaterial = protomap.add_organic_material();
        protomaterial->set_index(i);
        protomaterial->set_name(world->raws.plants.all[i]->id);
    }

    std::map<df::coord,std::pair<uint32_t,uint16_t> > constructionMaterials;
    if (Constructions::isValid())
    {
        for (uint32_t i = 0; i < Constructions::getCount(); i++)
        {
            df::construction *construction = Constructions::getConstruction(i);
            constructionMaterials[construction->pos] = std::make_pair(construction->mat_index, construction->mat_type);
        }
    }

    coded_output->WriteVarint32(protomap.ByteSize());
    protomap.SerializeToCodedStream(coded_output);

    DFHack::t_feature blockFeatureGlobal;
    DFHack::t_feature blockFeatureLocal;

    out.print("Writing map block information");

    for(uint32_t z = 0; z < z_max; z++)
    {
        for(uint32_t b_y = 0; b_y < y_max; b_y++)
        {
            for(uint32_t b_x = 0; b_x < x_max; b_x++)
            {
                if (b_x == 0 && b_y == 0 && z % 10 == 0) out.print(".");
                // Get the map block
                df::coord2d blockCoord(b_x, b_y);
                MapExtras::Block *b = map.BlockAt(DFHack::DFCoord(b_x, b_y, z));
                if (!b || !b->is_valid())
                {
                    continue;
                }

                dfproto::Block protoblock;
                protoblock.set_x(b_x);
                protoblock.set_y(b_y);
                protoblock.set_z(z);

                // Find features
                b->GetGlobalFeature(&blockFeatureGlobal);
                b->GetLocalFeature(&blockFeatureLocal);

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

                        dfproto::Tile *prototile = protoblock.add_tile();
                        prototile->set_x(x);
                        prototile->set_y(y);

                        // Check for liquid
                        if (des.bits.flow_size)
                        {
                            prototile->set_liquid_type((dfproto::Tile::LiquidType)des.bits.liquid_type);
                            prototile->set_flow_size(des.bits.flow_size);
                        }

                        df::tiletype type = b->tiletypeAt(coord);
                        prototile->set_type((dfproto::Tile::TileType)tileShape(type));
                        prototile->set_tile_material(toProto(tileMaterial(type)));

                        df::coord map_pos = df::coord(b_x*16+x,b_y*16+y,z);

                        switch (tileMaterial(type))
                        {
                        case tiletype_material::SOIL:
                        case tiletype_material::STONE:
                            prototile->set_material_type(0);
                            prototile->set_material_index(b->layerMaterialAt(coord));
                            break;
                        case tiletype_material::MINERAL:
                            prototile->set_material_type(0);
                            prototile->set_material_index(b->veinMaterialAt(coord));
                            break;
                        case tiletype_material::FEATURE:
                            if (blockFeatureLocal.type != -1 && des.bits.feature_local)
                            {
                                if (blockFeatureLocal.type == feature_type::deep_special_tube
                                        && blockFeatureLocal.main_material == 0) // stone
                                {
                                    prototile->set_material_type(0);
                                    prototile->set_material_index(blockFeatureLocal.sub_material);
                                }
                                if (blockFeatureGlobal.type != -1 && des.bits.feature_global
                                        && blockFeatureGlobal.type == feature_type::feature_underworld_from_layer
                                        && blockFeatureGlobal.main_material == 0) // stone
                                {
                                    prototile->set_material_type(0);
                                    prototile->set_material_index(blockFeatureGlobal.sub_material);
                                }
                            }
                            break;
                        case tiletype_material::CONSTRUCTION:
                            if (constructionMaterials.find(map_pos) != constructionMaterials.end())
                            {
                                prototile->set_material_index(constructionMaterials[map_pos].first);
                                prototile->set_material_type(constructionMaterials[map_pos].second);
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }

                if (b->getRaw())
                {
                    PlantList *plants = &b->getRaw()->plants;
                    for (PlantList::const_iterator it = plants->begin(); it != plants->end(); it++)
                    {
                        const df::plant & plant = *(*it);
                        df::coord2d loc(plant.pos.x, plant.pos.y);
                        loc = loc % 16;
                        if (showHidden || !b->DesignationAt(loc).bits.hidden)
                        {
                            dfproto::Plant *protoplant = protoblock.add_plant();
                            protoplant->set_x(loc.x);
                            protoplant->set_y(loc.y);
                            protoplant->set_is_shrub(plant.flags.bits.is_shrub);
                            protoplant->set_material(plant.material);
                        }
                    }
                }

                coded_output->WriteVarint32(protoblock.ByteSize());
                protoblock.SerializeToCodedStream(coded_output);
            } // block x
            // Clean uneeded memory
            map.trash();
        } // block y
    } // z

    delete coded_output;
    delete zip_output;
    delete raw_output;

    mats->Finish();
    out.print("\nMap succesfully exported!\n");
    return CR_OK;
}
