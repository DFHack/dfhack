// Just show some position data

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Units.h"
#include "df/unit_inventory_item.h"
#include "df/building_nest_boxst.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "modules/Materials.h"
#include "modules/MapCache.h"
#include "modules/Buildings.h"
#include "MiscUtils.h"

#include "df/world.h"
#include "df/world_raws.h"
#include "df/building_def.h"
#include "df/region_map_entry.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::cursor;

command_result df_probe (color_ostream &out, vector <string> & parameters);
command_result df_cprobe (color_ostream &out, vector <string> & parameters);
command_result df_bprobe (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("probe");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("probe",
                                     "A tile probe",
                                     df_probe));
    commands.push_back(PluginCommand("cprobe",
                                     "A creature probe",
                                     df_cprobe));
    commands.push_back(PluginCommand("bprobe",
                                     "A simple building probe",
                                     df_bprobe));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

command_result df_cprobe (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;
    int32_t cursorX, cursorY, cursorZ;
    Gui::getCursorCoords(cursorX,cursorY,cursorZ);
    if(cursorX == -30000)
    {
        out.printerr("No cursor; place cursor over creature to probe.\n");
    }
    else
    {
        for(size_t i = 0; i < world->units.all.size(); i++)
        {
            df::unit * unit = world->units.all[i];
            if(unit->pos.x == cursorX && unit->pos.y == cursorY && unit->pos.z == cursorZ)
            {
                out.print("Creature %d, race %d (%x), civ %d (%x)\n", unit->id, unit->race, unit->race, unit->civ_id, unit->civ_id);
                
                for(size_t j=0; j<unit->inventory.size(); j++)
                {
                    df::unit_inventory_item* inv_item = unit->inventory[j];
                    df::item* item = inv_item->item;
                    if(inv_item->mode == df::unit_inventory_item::T_mode::Worn)
                    {
                        out << "   wears item: #" << item->id;
                        if(item->flags.bits.owned)
                            out << " (owned)";
                        else
                            out << " (not owned)";
                        if(item->getEffectiveArmorLevel() != 0)
                            out << ", armor";
                        out << endl;
                    }
                }
                
                // don't leave loop, there may be more than 1 creature at the cursor position
                //break;
            }
        }
    }
    return CR_OK;
}

void describeTile(color_ostream &out, df::tiletype tiletype)
{
    out.print("%d", tiletype);
    if(tileName(tiletype))
        out.print(" = %s",tileName(tiletype));
    out.print("\n");

    df::tiletype_shape shape = tileShape(tiletype);
    df::tiletype_material material = tileMaterial(tiletype);
    df::tiletype_special special = tileSpecial(tiletype);
    df::tiletype_variant variant = tileVariant(tiletype);
    out.print("%-10s: %4d %s\n","Class"    ,shape,
              ENUM_KEY_STR(tiletype_shape, shape).c_str());
    out.print("%-10s: %4d %s\n","Material" ,
              material, ENUM_KEY_STR(tiletype_material, material).c_str());
    out.print("%-10s: %4d %s\n","Special"  ,
              special, ENUM_KEY_STR(tiletype_special, special).c_str());
    out.print("%-10s: %4d %s\n"   ,"Variant"  ,
              variant, ENUM_KEY_STR(tiletype_variant, variant).c_str());
    out.print("%-10s: %s\n"    ,"Direction",
              tileDirection(tiletype).getStr());
    out.print("\n");
}

command_result df_probe (color_ostream &out, vector <string> & parameters)
{
    //bool showBlock, showDesig, showOccup, showTile, showMisc;

    /*
    if (!parseOptions(parameters, showBlock, showDesig, showOccup,
                      showTile, showMisc))
    {
        out.printerr("Unknown parameters!\n");
        return CR_FAILURE;
    }
    */

    CoreSuspender suspend;

    DFHack::Materials *Materials = Core::getInstance().getMaterials();

    std::vector<t_matglossInorganic> inorganic;
    bool hasmats = Materials->CopyInorganicMaterials(inorganic);

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    MapExtras::MapCache mc;

    int32_t regionX, regionY, regionZ;
    Maps::getPosition(regionX,regionY,regionZ);

    int32_t cursorX, cursorY, cursorZ;
    Gui::getCursorCoords(cursorX,cursorY,cursorZ);
    if(cursorX == -30000)
    {
        out.printerr("No cursor; place cursor over tile to probe.\n");
        return CR_FAILURE;
    }
    DFCoord cursor (cursorX,cursorY,cursorZ);

    uint32_t blockX = cursorX / 16;
    uint32_t tileX = cursorX % 16;
    uint32_t blockY = cursorY / 16;
    uint32_t tileY = cursorY % 16;

    MapExtras::Block * b = mc.BlockAt(cursor/16);
    if(!b || !b->is_valid())
    {
        out.printerr("No data.\n");
        return CR_OK;
    }

    auto &block = *b->getRaw();
    out.print("block addr: 0x%x\n\n", &block);
/*
    if (showBlock)
    {
        out.print("block flags:\n");
        print_bits<uint32_t>(block.blockflags.whole,out);
        out.print("\n\n");
    }
*/
    df::tiletype tiletype = mc.tiletypeAt(cursor);
    df::tile_designation &des = block.designation[tileX][tileY];
    df::tile_occupancy &occ = block.occupancy[tileX][tileY];
/*
    if(showDesig)
    {
        out.print("designation\n");
        print_bits<uint32_t>(block.designation[tileX][tileY].whole,
                                out);
        out.print("\n\n");
    }

    if(showOccup)
    {
        out.print("occupancy\n");
        print_bits<uint32_t>(block.occupancy[tileX][tileY].whole,
                                out);
        out.print("\n\n");
    }
*/

    // tiletype
    out.print("tiletype: ");
    describeTile(out, tiletype);
    out.print("static: ");
    describeTile(out, mc.staticTiletypeAt(cursor));
    out.print("base: ");
    describeTile(out, mc.baseTiletypeAt(cursor));

    out.print("temperature1: %d U\n",mc.temperature1At(cursor));
    out.print("temperature2: %d U\n",mc.temperature2At(cursor));

    int offset = block.region_offset[des.bits.biome];
    int bx = clip_range(block.region_pos.x + (offset % 3) - 1, 0, world->world_data->world_width-1);
    int by = clip_range(block.region_pos.y + (offset / 3) - 1, 0, world->world_data->world_height-1);

    auto biome = &world->world_data->region_map[bx][by];

    int sav = biome->savagery;
    int evi = biome->evilness;
    int sindex = sav > 65 ? 2 : sav < 33 ? 0 : 1;
    int eindex = evi > 65 ? 2 : evi < 33 ? 0 : 1;
    int surr = sindex + eindex * 3;

    const char* surroundings[] = { "Serene", "Mirthful", "Joyous Wilds", "Calm", "Wilderness", "Untamed Wilds", "Sinister", "Haunted", "Terrifying" };

    // biome, geolayer
    out << "biome: " << des.bits.biome << " (" << 
        "region id=" << biome->region_id << ", " <<
        surroundings[surr] << ", " <<
        "savagery " << biome->savagery << ", " <<
        "evilness " << biome->evilness << ")" << std::endl;
    out << "geolayer: " << des.bits.geolayer_index
        << std::endl;
    int16_t base_rock = mc.layerMaterialAt(cursor);
    if(base_rock != -1)
    {
        out << "Layer material: " << dec << base_rock;
        if(hasmats)
            out << " / " << inorganic[base_rock].id
                << " / "
                << inorganic[base_rock].name
                << endl;
        else
            out << endl;
    }
    int16_t vein_rock = mc.veinMaterialAt(cursor);
    if(vein_rock != -1)
    {
        out << "Vein material (final): " << dec << vein_rock;
        if(hasmats)
            out << " / " << inorganic[vein_rock].id
                << " / "
                << inorganic[vein_rock].name
                << endl;
        else
            out << endl;
    }
    MaterialInfo minfo(mc.baseMaterialAt(cursor));
    if (minfo.isValid())
        out << "Base material: " << minfo.getToken() << " / " << minfo.toString() << endl;
    minfo.decode(mc.staticMaterialAt(cursor));
    if (minfo.isValid())
        out << "Static material: " << minfo.getToken() << " / " << minfo.toString() << endl;
    // liquids
    if(des.bits.flow_size)
    {
        if(des.bits.liquid_type == tile_liquid::Magma)
            out <<"magma: ";
        else out <<"water: ";
        out << des.bits.flow_size << std::endl;
    }
    if(des.bits.flow_forbid)
        out << "flow forbid" << std::endl;
    if(des.bits.pile)
        out << "stockpile?" << std::endl;
    if(des.bits.rained)
        out << "rained?" << std::endl;
    if(des.bits.smooth)
        out << "smooth?" << std::endl;
    if(des.bits.water_salt)
        out << "salty" << endl;
    if(des.bits.water_stagnant)
        out << "stagnant" << endl;

    #define PRINT_FLAG( FIELD, BIT )  out.print("%-16s= %c\n", #BIT , ( FIELD.bits.BIT ? 'Y' : ' ' ) )
    PRINT_FLAG( des, hidden );
    PRINT_FLAG( des, light );
    PRINT_FLAG( des, outside );
    PRINT_FLAG( des, subterranean );
    PRINT_FLAG( des, water_table );
    PRINT_FLAG( des, rained );
    PRINT_FLAG( occ, monster_lair);

    df::coord2d pc(blockX, blockY);

    t_feature local;
    t_feature global;
    Maps::ReadFeatures(&block,&local,&global);
    PRINT_FLAG( des, feature_local );
    if(local.type != -1)
    {
        out.print("%-16s", "");
        out.print("  %4d", block.local_feature);
        out.print(" (%2d)", local.type);
        out.print(" addr 0x%X ", local.origin);
        out.print(" %s\n", sa_feature(local.type));
    }
    PRINT_FLAG( des, feature_global );
    if(global.type != -1)
    {
        out.print("%-16s", "");
        out.print("  %4d", block.global_feature);
        out.print(" (%2d)", global.type);
        out.print(" %s\n", sa_feature(global.type));
    }
    #undef PRINT_FLAG
    out << "local feature idx: " << block.local_feature
        << endl;
    out << "global feature idx: " << block.global_feature
        << endl;
    out << std::endl;

    if(block.occupancy[tileX][tileY].bits.no_grow)
        out << "no grow" << endl;

    for(size_t e=0; e<block.block_events.size(); e++)
    {            
        df::block_square_event * blev = block.block_events[e];
        df::block_square_event_type blevtype = blev->getType();
        switch(blevtype)
        {
        case df::block_square_event_type::grass:
            {
                df::block_square_event_grassst * gr_ev = (df::block_square_event_grassst *)blev;
                if(gr_ev->amount[tileX][tileY] > 0)
                {
                    out << "amount of grass: " << (int)gr_ev->amount[tileX][tileY] << endl;
                }
                break;
            }
        case df::block_square_event_type::world_construction:
            {
                df::block_square_event_world_constructionst * co_ev = (df::block_square_event_world_constructionst*)blev;
                uint16_t bits = co_ev->tile_bitmask[tileY];
                out << "construction bits: " << bits << endl;
                break;
            }
        default:
            //out << "unhandled block event type!" << endl;
            break;
        }
    }


    return CR_OK;
}

command_result df_bprobe (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    if(cursor->x == -30000)
    {
        out.printerr("No cursor; place cursor over tile to probe.\n");
        return CR_FAILURE;
    }

    for (size_t i = 0; i < world->buildings.all.size(); i++)
    {
        Buildings::t_building building;
        if (!Buildings::Read(i, building))
            continue;
        if (!(building.x1 <= cursor->x && cursor->x <= building.x2 &&
            building.y1 <= cursor->y && cursor->y <= building.y2 &&
            building.z == cursor->z))
            continue;
        string name;
        building.origin->getName(&name);
        out.print("Building %i - \"%s\" - type %s (%i)",
                  building.origin->id,
                  name.c_str(),
                  ENUM_KEY_STR(building_type, building.type).c_str(),
                  building.type);

        switch (building.type)
        {
        case building_type::Civzone:
            out.print(", subtype %s (%i)",
                      ENUM_KEY_STR(civzone_type, building.civzone_type).c_str(),
                      building.civzone_type);
            break;
        case building_type::Furnace:
            out.print(", subtype %s (%i)",
                      ENUM_KEY_STR(furnace_type, building.furnace_type).c_str(),
                      building.furnace_type);
            if (building.furnace_type == furnace_type::Custom)
                out.print(", custom type %s (%i)",
                          world->raws.buildings.all[building.custom_type]->code.c_str(),
                          building.custom_type);
            break;
        case building_type::Workshop:
            out.print(", subtype %s (%i)",
                      ENUM_KEY_STR(workshop_type, building.workshop_type).c_str(),
                      building.workshop_type);
            if (building.workshop_type == workshop_type::Custom)
                out.print(", custom type %s (%i)",
                          world->raws.buildings.all[building.custom_type]->code.c_str(),
                          building.custom_type);
            break;
        case building_type::Construction:
            out.print(", subtype %s (%i)",
                      ENUM_KEY_STR(construction_type, building.construction_type).c_str(),
                      building.construction_type);
            break;
        case building_type::Shop:
            out.print(", subtype %s (%i)",
                      ENUM_KEY_STR(shop_type, building.shop_type).c_str(),
                      building.shop_type);
            break;
        case building_type::SiegeEngine:
            out.print(", subtype %s (%i)",
                      ENUM_KEY_STR(siegeengine_type, building.siegeengine_type).c_str(),
                      building.siegeengine_type);
            break;
        case building_type::Trap:
            out.print(", subtype %s (%i)",
                      ENUM_KEY_STR(trap_type, building.trap_type).c_str(),
                      building.trap_type);
            break;
        case building_type::NestBox:
            {
                df::building_nest_boxst* nestbox = (df::building_nest_boxst*) building.origin;
                out.print(", claimed:(%i), items:%i", nestbox->claimed_by, nestbox->contained_items.size());
                break;
            }
        default:
            if (building.subtype != -1)
                out.print(", subtype %i", building.subtype);
            break;
        }
        if(building.origin->is_room)  //isRoom())
            out << ", room";
        if(building.origin->getBuildStage()!=building.origin->getMaxBuildStage())
            out << ", in construction";
        out.print("\n");
    }
    return CR_OK;
}
