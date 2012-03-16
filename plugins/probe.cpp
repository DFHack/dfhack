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

command_result df_probe (color_ostream &out, vector <string> & parameters);
command_result df_cprobe (color_ostream &out, vector <string> & parameters);
command_result df_bprobe (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("probe");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
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
                break;
            }
        }
    }
    return CR_OK;
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
    if(!b && !b->valid)
    {
        out.printerr("No data.\n");
        return CR_OK;
    }
    mapblock40d & block = b->raw;
    out.print("block addr: 0x%x\n\n", block.origin);
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
    out.print("tiletype: %d", tiletype);
    if(tileName(tiletype))
        out.print(" = %s",tileName(tiletype));
    out.print("\n");

    df::tiletype_shape shape = tileShape(tiletype);
    df::tiletype_material material = tileMaterial(tiletype);
    df::tiletype_special special = tileSpecial(tiletype);
    df::tiletype_variant variant = tileVariant(tiletype);
    out.print("%-10s: %4d %s\n","Class"    ,shape,
            ENUM_KEY_STR(tiletype_shape, shape));
    out.print("%-10s: %4d %s\n","Material" ,
            material, ENUM_KEY_STR(tiletype_material, material));
    out.print("%-10s: %4d %s\n","Special"  ,
            special, ENUM_KEY_STR(tiletype_special, special));
    out.print("%-10s: %4d %s\n"   ,"Variant"  ,
            variant, ENUM_KEY_STR(tiletype_variant, variant));
    out.print("%-10s: %s\n"    ,"Direction",
            tileDirection(tiletype).getStr());
    out.print("\n");

    out.print("temperature1: %d U\n",mc.temperature1At(cursor));
    out.print("temperature2: %d U\n",mc.temperature2At(cursor));

    // biome, geolayer
    out << "biome: " << des.bits.biome << std::endl;
    out << "geolayer: " << des.bits.geolayer_index
        << std::endl;
    int16_t base_rock = mc.baseMaterialAt(cursor);
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
    Maps::ReadFeatures(&(b->raw),&local,&global);
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
    out << "mystery: " << block.mystery << endl;
    out << std::endl;
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
        out.print("Building %i - \"%s\" - type %s", building.origin->id, name.c_str(), ENUM_KEY_STR(building_type, building.type));

        switch (building.type)
        {
        case building_type::Civzone:
            out.print(", subtype %s", ENUM_KEY_STR(civzone_type, building.civzone_type));
            break;
        case building_type::Furnace:
            out.print(", subtype %s", ENUM_KEY_STR(furnace_type, building.furnace_type));
            if (building.furnace_type == furnace_type::Custom)
                out.print(", custom type %i (%s)", building.custom_type, world->raws.buildings.all[building.custom_type]->code.c_str());
            break;
        case building_type::Workshop:
            out.print(", subtype %s", ENUM_KEY_STR(workshop_type, building.workshop_type));
            if (building.workshop_type == workshop_type::Custom)
                out.print(", custom type %i (%s)", building.custom_type, world->raws.buildings.all[building.custom_type]->code.c_str());
            break;
        case building_type::Construction:
            out.print(", subtype %s", ENUM_KEY_STR(construction_type, building.construction_type));
            break;
        case building_type::Shop:
            out.print(", subtype %s", ENUM_KEY_STR(shop_type, building.shop_type));
            break;
        case building_type::SiegeEngine:
            out.print(", subtype %s", ENUM_KEY_STR(siegeengine_type, building.siegeengine_type));
            break;
        case building_type::Trap:
            out.print(", subtype %s", ENUM_KEY_STR(trap_type, building.trap_type));
            break;
        default:
            if (building.subtype != -1)
                out.print(", subtype %i", building.subtype);
            break;
        }
        out.print("\n");

    }
    return CR_OK;
}
