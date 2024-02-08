#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Gui.h"
#include "modules/Materials.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"

#include "df/block_square_event_grassst.h"
#include "df/block_square_event_world_constructionst.h"
#include "df/building.h"
#include "df/building_def.h"
#include "df/building_nest_boxst.h"
#include "df/civzone_type.h"
#include "df/construction_type.h"
#include "df/furnace_type.h"
#include "df/item.h"
#include "df/region_map_entry.h"
#include "df/shop_type.h"
#include "df/siegeengine_type.h"
#include "df/tiletype.h"
#include "df/trap_type.h"
#include "df/unit.h"
#include "df/unit_inventory_item.h"
#include "df/workshop_type.h"
#include "df/world.h"
#include "df/world_data.h"

using std::vector;
using std::string;
using namespace DFHack;

DFHACK_PLUGIN("probe");

REQUIRE_GLOBAL(world);

static command_result df_cprobe(color_ostream &out, vector<string> & parameters) {
    CoreSuspender suspend;

    auto unit = Gui::getSelectedUnit(out);
    if (!unit)
        return CR_FAILURE;

    out.print("Creature %d, race %d (%x), civ %d (%x)\n",
        unit->id, unit->race, unit->race, unit->civ_id, unit->civ_id);

    for (auto inv_item : unit->inventory) {
        df::item* item = inv_item->item;
        if (inv_item->mode == df::unit_inventory_item::T_mode::Worn) {
            out << "   wears item: #" << item->id;
            if (item->flags.bits.owned)
                out << " (owned)";
            else
                out << " (not owned)";
            if (item->getEffectiveArmorLevel() != 0)
                out << ", armor";
            out << std::endl;
        }
    }

    return CR_OK;
}

static void describeTile(color_ostream &out, df::tiletype tiletype) {
    out.print("%d", tiletype);
    if (tileName(tiletype))
        out.print(" = %s", tileName(tiletype));
    out.print(" (%s)", ENUM_KEY_STR(tiletype, tiletype).c_str());
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

static command_result df_probe(color_ostream &out, vector <string> & parameters) {
    CoreSuspender suspend;

    DFHack::Materials *Materials = Core::getInstance().getMaterials();

    vector<t_matglossInorganic> inorganic;
    bool hasmats = Materials->CopyInorganicMaterials(inorganic);

    if (!Maps::IsValid()) {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    MapExtras::MapCache mc;

    int32_t regionX, regionY, regionZ;
    Maps::getPosition(regionX,regionY,regionZ);

    df::coord cursor;
    if (!Gui::getCursorCoords(cursor)) {
        out.printerr("No cursor; place cursor over tile to probe.\n");
        return CR_FAILURE;
    }

    uint32_t blockX = cursor.x / 16;
    uint32_t tileX = cursor.x % 16;
    uint32_t blockY = cursor.y / 16;
    uint32_t tileY = cursor.y % 16;

    MapExtras::Block * b = mc.BlockAt(cursor/16);
    if (!b || !b->is_valid()) {
        out.printerr("No data.\n");
        return CR_OK;
    }

    auto &block = *b->getRaw();
    out.print("block addr: %p\n\n", &block);

    df::tiletype tiletype = mc.tiletypeAt(cursor);
    df::tile_designation &des = block.designation[tileX][tileY];
    df::tile_occupancy &occ = block.occupancy[tileX][tileY];
    uint8_t fog_of_war = block.fog_of_war[tileX][tileY];

    out.print("tiletype: ");
    describeTile(out, tiletype);
    out.print("static: ");
    describeTile(out, mc.staticTiletypeAt(cursor));
    out.print("base: ");
    describeTile(out, mc.baseTiletypeAt(cursor));

    out.print("temperature1: %d U\n", mc.temperature1At(cursor));
    out.print("temperature2: %d U\n", mc.temperature2At(cursor));

    int offset = block.region_offset[des.bits.biome];
    int bx = clip_range(block.region_pos.x + (offset % 3) - 1, 0, world->world_data->world_width-1);
    int by = clip_range(block.region_pos.y + (offset / 3) - 1, 0, world->world_data->world_height-1);

    auto biome = &world->world_data->region_map[bx][by];

    int sav = biome->savagery;
    int evi = biome->evilness;
    int sindex = sav > 65 ? 2 : sav < 33 ? 0 : 1;
    int eindex = evi > 65 ? 2 : evi < 33 ? 0 : 1;
    int surr = sindex + eindex * 3;

    const char* surroundings[] = {
        "Serene", "Mirthful", "Joyous Wilds",
        "Calm", "Wilderness", "Untamed Wilds",
        "Sinister", "Haunted", "Terrifying"
    };

    // biome, geolayer
    out << "biome: " << des.bits.biome << " (" <<
        "region id=" << biome->region_id << ", " <<
        surroundings[surr] << ", " <<
        "savagery " << biome->savagery << ", " <<
        "evilness " << biome->evilness << ")" << std::endl;
    out << "geolayer: " << des.bits.geolayer_index
        << std::endl;
    int16_t base_rock = mc.layerMaterialAt(cursor);
    if (base_rock != -1) {
        out << "Layer material: " << std::dec << base_rock;
        if(hasmats)
            out << " / " << inorganic[base_rock].id
                << " / "
                << inorganic[base_rock].name
                << std::endl;
        else
            out << std::endl;
    }
    int16_t vein_rock = mc.veinMaterialAt(cursor);
    if (vein_rock != -1) {
        out << "Vein material (final): " << std::dec << vein_rock;
        if(hasmats)
            out << " / " << inorganic[vein_rock].id
                << " / "
                << inorganic[vein_rock].name
                << " ("
                << ENUM_KEY_STR(inclusion_type,b->veinTypeAt(cursor))
                << ")"
                << std::endl;
        else
            out << std::endl;
    }
    MaterialInfo minfo(mc.baseMaterialAt(cursor));
    if (minfo.isValid())
        out << "Base material: " << minfo.getToken() << " / " << minfo.toString() << std::endl;
    minfo.decode(mc.staticMaterialAt(cursor));
    if (minfo.isValid())
        out << "Static material: " << minfo.getToken() << " / " << minfo.toString() << std::endl;
    // liquids
    if (des.bits.flow_size) {
        if(des.bits.liquid_type == tile_liquid::Magma)
            out <<"magma: ";
        else out <<"water: ";
        out << des.bits.flow_size << std::endl;
    }
    if (des.bits.flow_forbid)
        out << "flow forbid" << std::endl;
    if (des.bits.pile)
        out << "stockpile?" << std::endl;
    if (des.bits.rained)
        out << "rained?" << std::endl;
    if (des.bits.smooth)
        out << "smooth?" << std::endl;
    if (des.bits.water_salt)
        out << "salty" << std::endl;
    if (des.bits.water_stagnant)
        out << "stagnant" << std::endl;

    #define PRINT_FLAG( FIELD, BIT )  out.print("%-16s= %c\n", #BIT , ( FIELD.bits.BIT ? 'Y' : ' ' ) )

    out.print("%-16s= %s\n", "dig", enum_item_key(des.bits.dig).c_str());
    PRINT_FLAG(occ, dig_marked);
    PRINT_FLAG(occ, dig_auto);
    out.print("%-16s= %s\n", "traffic", enum_item_key(des.bits.traffic).c_str());
    PRINT_FLAG(occ, carve_track_north);
    PRINT_FLAG(occ, carve_track_south);
    PRINT_FLAG(occ, carve_track_east);
    PRINT_FLAG(occ, carve_track_west);

    PRINT_FLAG( des, hidden );
    PRINT_FLAG( des, light );
    PRINT_FLAG( des, outside );
    PRINT_FLAG( des, subterranean );
    PRINT_FLAG( des, water_table );
    PRINT_FLAG( des, rained );
    PRINT_FLAG( occ, monster_lair);
    out.print("%-16s= %d\n", "fog_of_war", fog_of_war);

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
        out.print(" addr %p ", local.origin);
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
    out << "local feature idx: " << block.local_feature
        << std::endl;
    out << "global feature idx: " << block.global_feature
        << std::endl;
    out << std::endl;

    out << "Occupancy:" << std::endl;
    out.print("%-16s= %s\n", "building", enum_item_key(occ.bits.building).c_str());
    PRINT_FLAG(occ, unit);
    PRINT_FLAG(occ, unit_grounded);
    PRINT_FLAG(occ, item);
    PRINT_FLAG(occ, moss);
    out << std::endl;

    if (block.occupancy[tileX][tileY].bits.no_grow)
        out << "no grow" << std::endl;

    for (auto blev : block.block_events) {
        df::block_square_event_type blevtype = blev->getType();
        switch(blevtype) {
        case df::block_square_event_type::grass:
        {
            auto gr_ev = (df::block_square_event_grassst *)blev;
            if (gr_ev->amount[tileX][tileY] > 0)
                out << "amount of grass: " << (int)gr_ev->amount[tileX][tileY] << std::endl;
            break;
        }
        case df::block_square_event_type::world_construction:
        {
            auto co_ev = (df::block_square_event_world_constructionst *)blev;
            uint16_t bits = co_ev->tile_bitmask[tileY];
            out << "construction bits: " << bits << std::endl;
            break;
        }
        default:
            break;
        }
    }

    #undef PRINT_FLAG
    return CR_OK;
}

union Subtype {
    int16_t subtype;
    df::civzone_type civzone_type;
    df::furnace_type furnace_type;
    df::workshop_type workshop_type;
    df::construction_type construction_type;
    df::shop_type shop_type;
    df::siegeengine_type siegeengine_type;
    df::trap_type trap_type;
};

static command_result df_bprobe(color_ostream &out, vector<string> & parameters) {
    CoreSuspender suspend;

    auto bld = Gui::getSelectedBuilding(out);
    if (!bld)
        return CR_FAILURE;

    string name;
    bld->getName(&name);

    auto bld_type = bld->getType();
    Subtype subtype{bld->getSubtype()};
    int32_t custom = bld->getCustomType();

    out.print("Building %i - \"%s\" - type %s (%i)",
                bld->id,
                name.c_str(),
                ENUM_KEY_STR(building_type, bld_type).c_str(),
                bld_type);


    switch (bld_type) {
    case df::building_type::Civzone:
        out.print(", subtype %s (%i)",
                    ENUM_KEY_STR(civzone_type, subtype.civzone_type).c_str(),
                    subtype.subtype);
        break;
    case df::building_type::Furnace:
        out.print(", subtype %s (%i)",
                    ENUM_KEY_STR(furnace_type, subtype.furnace_type).c_str(),
                    subtype.subtype);
        if (subtype.furnace_type == df::furnace_type::Custom)
            out.print(", custom type %s (%i)",
                        world->raws.buildings.all[custom]->code.c_str(),
                        custom);
        break;
    case df::building_type::Workshop:
        out.print(", subtype %s (%i)",
                    ENUM_KEY_STR(workshop_type, subtype.workshop_type).c_str(),
                    subtype.subtype);
        if (subtype.workshop_type == df::workshop_type::Custom)
            out.print(", custom type %s (%i)",
                        world->raws.buildings.all[custom]->code.c_str(),
                        custom);
        break;
    case df::building_type::Construction:
        out.print(", subtype %s (%i)",
                    ENUM_KEY_STR(construction_type, subtype.construction_type).c_str(),
                    subtype.subtype);
        break;
    case df::building_type::Shop:
        out.print(", subtype %s (%i)",
                    ENUM_KEY_STR(shop_type, subtype.shop_type).c_str(),
                    subtype.subtype);
        break;
    case df::building_type::SiegeEngine:
        out.print(", subtype %s (%i)",
                    ENUM_KEY_STR(siegeengine_type, subtype.siegeengine_type).c_str(),
                    subtype.subtype);
        break;
    case df::building_type::Trap:
        out.print(", subtype %s (%i)",
                    ENUM_KEY_STR(trap_type, subtype.trap_type).c_str(),
                    subtype.subtype);
        break;
    case df::building_type::NestBox:
    {
        df::building_nest_boxst* nestbox = virtual_cast<df::building_nest_boxst>(bld);
        if (nestbox)
            out.print(", claimed:(%i), items:%zu", nestbox->claimed_by, nestbox->contained_items.size());
        break;
    }
    default:
        if (subtype.subtype != -1)
            out.print(", subtype %i", subtype.subtype);
        break;
    }

    if (bld->relations.size())  //Connected to rooms.
        out << ", " << bld->relations.size() << " rooms";
    if (bld->getBuildStage() != bld->getMaxBuildStage())
        out << ", in construction";
    out.print("\n");

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands){
    commands.push_back(PluginCommand("cprobe",
                                     "Display low-level properties of the selected unit.",
                                     df_cprobe));
    commands.push_back(PluginCommand("probe",
                                     "Display low-level properties of the selected tile.",
                                     df_probe));
    commands.push_back(PluginCommand("bprobe",
                                     "Display low-level properties of the selected building.",
                                     df_bprobe));
    return CR_OK;
}
