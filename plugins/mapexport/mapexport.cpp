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
#include "modules/Constructions.h"

#include "proto/Map.pb.h"
#include "proto/Block.pb.h"

using namespace DFHack;
using df::global::world;

typedef std::vector<df::plant *> PlantList;

command_result mapexport (Core * c, std::vector <std::string> & parameters);

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

command_result mapexport (Core * c, std::vector <std::string> & parameters)
{
    bool showHidden = false;

    int filenameParameter = 1;

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            c->con.print("Exports the currently visible map to a file.\n"
                         "Usage: mapexport [options] <filename>\n"
                         "Example: mapexport all embark.dfmap\n"
                         "Options:\n"
                         "   all   - Export the entire map, not just what's revealed."
            );
            return CR_OK;
        }
        if (parameters[i] == "all")
        {
            showHidden = true;
            filenameParameter++;
        }
    }


    uint32_t x_max=0, y_max=0, z_max=0;
    c->Suspend();
    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        c->Resume();
        return CR_FAILURE;
    }

    if (parameters.size() < filenameParameter)
    {
        c->con.printerr("Please supply a filename.\n");
        c->Resume();
        return CR_FAILURE;
    }

    std::string filename = parameters[filenameParameter-1];
    if (filename.rfind(".dfmap") == std::string::npos) filename += ".dfmap";
    c->con << "Writing to " << filename << "..." << std::endl;

    std::ofstream output_file(filename, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!output_file.is_open())
    {
        c->con.printerr("Couldn't open the output file.\n");
        c->Resume();
        return CR_FAILURE;
    }
    ZeroCopyOutputStream *raw_output = new OstreamOutputStream(&output_file);
    GzipOutputStream *zip_output = new GzipOutputStream(raw_output);
    CodedOutputStream *coded_output = new CodedOutputStream(zip_output);

    coded_output->WriteLittleEndian32(0x50414DDF); //Write our file header

    Maps::getSize(x_max, y_max, z_max);
    MapExtras::MapCache map;
    DFHack::Materials *mats = c->getMaterials();

    c->con << "Writing  map info..." << std::endl;

    dfproto::Map protomap;
    protomap.set_x_size(x_max);
    protomap.set_y_size(y_max);
    protomap.set_z_size(z_max);

    c->con << "Writing material dictionary..." << std::endl;
    
    for (size_t i = 0; i < world->raws.inorganics.size(); i++)
    {
        dfproto::Material *protomaterial = protomap.add_inorganic_material();
        protomaterial->set_type(i);
        protomaterial->set_name(world->raws.inorganics[i]->id);
    }

    for (size_t i = 0; i < world->raws.plants.all.size(); i++)
    {
        dfproto::Material *protomaterial = protomap.add_organic_material();
        protomaterial->set_type(i);
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

    c->con.print("Writing map block information");

    for(uint32_t z = 0; z < z_max; z++)
    {
        for(uint32_t b_y = 0; b_y < y_max; b_y++)
        {
            for(uint32_t b_x = 0; b_x < x_max; b_x++)
            {
                if (b_x == 0 && b_y == 0 && z % 10 == 0) c->con.print(".");
                // Get the map block
                df::coord2d blockCoord(b_x, b_y);
                MapExtras::Block *b = map.BlockAt(DFHack::DFCoord(b_x, b_y, z));
                if (!b || !b->valid)
                {
                    continue;
                }

                dfproto::Block protoblock;
                protoblock.set_x(b_x);
                protoblock.set_y(b_y);
                protoblock.set_z(z);

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

                        dfproto::Tile *prototile = protoblock.add_tile();
                        prototile->set_x(x);
                        prototile->set_y(y);

                        // Check for liquid
                        if (des.bits.flow_size)
                        {
                            prototile->set_liquid_type((dfproto::Tile::LiquidType)des.bits.liquid_type);
                            prototile->set_flow_size(des.bits.flow_size);
                        }

                        df::tiletype type = b->TileTypeAt(coord);
                        prototile->set_type((dfproto::Tile::TileType)tileShape(type));

                        prototile->set_material_type((dfproto::Tile::MaterialType)tileMaterial(type));

                        df::coord map_pos = df::coord(b_x*16+x,b_y*16+y,z);
                        
                        switch (tileMaterial(type))
                        {
                        case tiletype_material::SOIL:
                        case tiletype_material::STONE:
                            prototile->set_material_index(0);
                            prototile->set_material(b->baseMaterialAt(coord));
                            break;
                        case tiletype_material::MINERAL:
                            prototile->set_material_index(0);
                            prototile->set_material(b->veinMaterialAt(coord));
                            break;
                        case tiletype_material::FEATURE:
                            if (blockFeatureLocal.type != -1 && des.bits.feature_local)
                            {
                                if (blockFeatureLocal.type == feature_type::deep_special_tube
                                        && blockFeatureLocal.main_material == 0) // stone
                                {
                                    prototile->set_material_index(0);
                                    prototile->set_material(blockFeatureLocal.sub_material);
                                }
                                if (blockFeatureGlobal.type != -1 && des.bits.feature_global
                                        && blockFeatureGlobal.type == feature_type::feature_underworld_from_layer
                                        && blockFeatureGlobal.main_material == 0) // stone
                                {
                                    prototile->set_material_index(0);
                                    prototile->set_material(blockFeatureGlobal.sub_material);
                                }
                            }
                            break;
                        case tiletype_material::CONSTRUCTION:
                            if (constructionMaterials.find(map_pos) != constructionMaterials.end())
                            {
                                prototile->set_material_index(constructionMaterials[map_pos].first);
                                prototile->set_material(constructionMaterials[map_pos].second);
                            }
                            break;
                        }
                    }
                }

                PlantList *plants;
                if (Maps::ReadVegetation(b_x, b_y, z, plants))
                {
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
    c->con.print("\nMap succesfully exported!\n");
    c->Resume();
    return CR_OK;
}
