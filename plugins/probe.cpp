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
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "modules/Materials.h"
#include "modules/MapCache.h"
#include "modules/Buildings.h"
#include "MiscUtils.h"

#include "df/world.h"
#include "df/world_raws.h"
#include "df/building_def.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::cursor;

command_result df_probe (Core * c, vector <string> & parameters);
command_result df_cprobe (Core * c, vector <string> & parameters);
command_result df_bprobe (Core * c, vector <string> & parameters);

DFHACK_PLUGIN("probe");

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
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

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

command_result df_cprobe (Core * c, vector <string> & parameters)
{
    Console & con = c->con;
    CoreSuspender suspend(c);
    int32_t cursorX, cursorY, cursorZ;
    Gui::getCursorCoords(cursorX,cursorY,cursorZ);
    if(cursorX == -30000)
    {
        con.printerr("No cursor; place cursor over creature to probe.\n");
    }
    else
    {
        for(size_t i = 0; i < world->units.all.size(); i++)
        {
            df::unit * unit = world->units.all[i];
            if(unit->pos.x == cursorX && unit->pos.y == cursorY && unit->pos.z == cursorZ)
            {
                con.print("Creature %d, race %d (%x), civ %d (%x)\n", unit->id, unit->race, unit->race, unit->civ_id, unit->civ_id);
                break;
            }
        }
    }
    return CR_OK;
}

command_result df_probe (Core * c, vector <string> & parameters)
{
    //bool showBlock, showDesig, showOccup, showTile, showMisc;
    Console & con = c->con;
    /*
    if (!parseOptions(parameters, showBlock, showDesig, showOccup,
                      showTile, showMisc))
    {
        con.printerr("Unknown parameters!\n");
        return CR_FAILURE;
    }
    */

    CoreSuspender suspend(c);

    DFHack::Materials *Materials = c->getMaterials();
    DFHack::VersionInfo* mem = c->vinfo;
    std::vector<t_matglossInorganic> inorganic;
    bool hasmats = Materials->CopyInorganicMaterials(inorganic);

    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    MapExtras::MapCache mc;

    int32_t regionX, regionY, regionZ;
    Maps::getPosition(regionX,regionY,regionZ);

    int32_t cursorX, cursorY, cursorZ;
    Gui::getCursorCoords(cursorX,cursorY,cursorZ);
    if(cursorX == -30000)
    {
        con.printerr("No cursor; place cursor over tile to probe.\n");
        return CR_FAILURE;
    }
    DFCoord cursor (cursorX,cursorY,cursorZ);

    uint32_t blockX = cursorX / 16;
    uint32_t tileX = cursorX % 16;
    uint32_t blockY = cursorY / 16;
    uint32_t tileY = cursorY % 16;

    MapExtras::Block * b = mc.BlockAt(cursor/16);
    if(!b && !b->valid)
    {
        con.printerr("No data.\n");
        return CR_OK;
    }
    mapblock40d & block = b->raw;
    con.print("block addr: 0x%x\n\n", block.origin);
/*
    if (showBlock)
    {
        con.print("block flags:\n");
        print_bits<uint32_t>(block.blockflags.whole,con);
        con.print("\n\n");
    }
*/
    df::tiletype tiletype = mc.tiletypeAt(cursor);
    df::tile_designation &des = block.designation[tileX][tileY];
/*
    if(showDesig)
    {
        con.print("designation\n");
        print_bits<uint32_t>(block.designation[tileX][tileY].whole,
                                con);
        con.print("\n\n");
    }

    if(showOccup)
    {
        con.print("occupancy\n");
        print_bits<uint32_t>(block.occupancy[tileX][tileY].whole,
                                con);
        con.print("\n\n");
    }
*/

    // tiletype
    con.print("tiletype: %d", tiletype);
    if(tileName(tiletype))
        con.print(" = %s",tileName(tiletype));
    con.print("\n");

    df::tiletype_shape shape = tileShape(tiletype);
    df::tiletype_material material = tileMaterial(tiletype);
    df::tiletype_special special = tileSpecial(tiletype);
    df::tiletype_variant variant = tileVariant(tiletype);
    con.print("%-10s: %4d %s\n","Class"    ,shape,
            ENUM_KEY_STR(tiletype_shape, shape));
    con.print("%-10s: %4d %s\n","Material" ,
            material, ENUM_KEY_STR(tiletype_material, material));
    con.print("%-10s: %4d %s\n","Special"  ,
            special, ENUM_KEY_STR(tiletype_special, special));
    con.print("%-10s: %4d %s\n"   ,"Variant"  ,
            variant, ENUM_KEY_STR(tiletype_variant, variant));
    con.print("%-10s: %s\n"    ,"Direction",
            tileDirection(tiletype).getStr());
    con.print("\n");

    con.print("temperature1: %d U\n",mc.temperature1At(cursor));
    con.print("temperature2: %d U\n",mc.temperature2At(cursor));

    // biome, geolayer
    con << "biome: " << des.bits.biome << std::endl;
    con << "geolayer: " << des.bits.geolayer_index
        << std::endl;
    int16_t base_rock = mc.baseMaterialAt(cursor);
    if(base_rock != -1)
    {
        con << "Layer material: " << dec << base_rock;
        if(hasmats)
            con << " / " << inorganic[base_rock].id
                << " / "
                << inorganic[base_rock].name
                << endl;
        else
            con << endl;
    }
    int16_t vein_rock = mc.veinMaterialAt(cursor);
    if(vein_rock != -1)
    {
        con << "Vein material (final): " << dec << vein_rock;
        if(hasmats)
            con << " / " << inorganic[vein_rock].id
                << " / "
                << inorganic[vein_rock].name
                << endl;
        else
            con << endl;
    }
    // liquids
    if(des.bits.flow_size)
    {
        if(des.bits.liquid_type == tile_liquid::Magma)
            con <<"magma: ";
        else con <<"water: ";
        con << des.bits.flow_size << std::endl;
    }
    if(des.bits.flow_forbid)
        con << "flow forbid" << std::endl;
    if(des.bits.pile)
        con << "stockpile?" << std::endl;
    if(des.bits.rained)
        con << "rained?" << std::endl;
    if(des.bits.smooth)
        con << "smooth?" << std::endl;
    if(des.bits.water_salt)
        con << "salty" << endl;
    if(des.bits.water_stagnant)
        con << "stagnant" << endl;

    #define PRINT_FLAG( X )  con.print("%-16s= %c\n", #X , ( des.X ? 'Y' : ' ' ) )
    PRINT_FLAG( bits.hidden );
    PRINT_FLAG( bits.light );
    PRINT_FLAG( bits.outside );
    PRINT_FLAG( bits.subterranean );
    PRINT_FLAG( bits.water_table );
    PRINT_FLAG( bits.rained );

    df::coord2d pc(blockX, blockY);

    t_feature local;
    t_feature global;
    Maps::ReadFeatures(&(b->raw),&local,&global);
    PRINT_FLAG( bits.feature_local );
    if(local.type != -1)
    {
        con.print("%-16s", "");
        con.print("  %4d", block.local_feature);
        con.print(" (%2d)", local.type);
        con.print(" addr 0x%X ", local.origin);
        con.print(" %s\n", sa_feature(local.type));
    }
    PRINT_FLAG( bits.feature_global );
    if(global.type != -1)
    {
        con.print("%-16s", "");
        con.print("  %4d", block.global_feature);
        con.print(" (%2d)", global.type);
        con.print(" %s\n", sa_feature(global.type));
    }
    #undef PRINT_FLAG
    con << "local feature idx: " << block.local_feature
        << endl;
    con << "global feature idx: " << block.global_feature
        << endl;
    con << "mystery: " << block.mystery << endl;
    con << std::endl;
    return CR_OK;
}

command_result df_bprobe (Core * c, vector <string> & parameters)
{
    CoreSuspender suspend(c);

    if(cursor->x == -30000)
    {
        c->con.printerr("No cursor; place cursor over tile to probe.\n");
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
        c->con.print("Building %i - \"%s\" - type %s", building.origin->id, name.c_str(), ENUM_KEY_STR(building_type, building.type));

        switch (building.type)
        {
        case building_type::Furnace:
            c->con.print(", subtype %s", ENUM_KEY_STR(furnace_type, building.furnace_type));
            if (building.furnace_type == furnace_type::Custom)
                c->con.print(", custom type %i (%s)", building.custom_type, world->raws.buildings.all[building.custom_type]->code.c_str());
            break;
        case building_type::Workshop:
            c->con.print(", subtype %s", ENUM_KEY_STR(workshop_type, building.workshop_type));
            if (building.workshop_type == workshop_type::Custom)
                c->con.print(", custom type %i (%s)", building.custom_type, world->raws.buildings.all[building.custom_type]->code.c_str());
            break;
        case building_type::Construction:
            c->con.print(", subtype %s", ENUM_KEY_STR(construction_type, building.construction_type));
            break;
        case building_type::Shop:
            c->con.print(", subtype %s", ENUM_KEY_STR(shop_type, building.shop_type));
            break;
        case building_type::SiegeEngine:
            c->con.print(", subtype %s", ENUM_KEY_STR(siegeengine_type, building.siegeengine_type));
            break;
        case building_type::Trap:
            c->con.print(", subtype %s", ENUM_KEY_STR(trap_type, building.trap_type));
            break;
        default:
            if (building.subtype != -1)
                c->con.print(", subtype %i", building.subtype);
            break;
        }
        c->con.print("\n");

    }
    return CR_OK;
}
