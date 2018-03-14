#include "df_version_int.h"
#define RFR_VERSION "0.19.0"

#include <cstdio>
#include <time.h>
#include <vector>

#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "Hooks.h"
#include "MiscUtils.h"
#include "PluginManager.h"
#include "RemoteFortressReader.pb.h"
#include "RemoteServer.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "TileTypes.h"
#include "VersionInfo.h"
#if DF_VERSION_INT > 34011
#include "DFHackVersion.h"
#endif
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
#include "modules/Translation.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/unit_action.h"
#if DF_VERSION_INT > 34011
#include "df/army.h"
#include "df/army_flags.h"
#include "df/block_square_event_item_spatterst.h"
#include "df/block_square_event_grassst.h"
#endif
#include "df/art_image_element_shapest.h"
#include "df/block_square_event_material_spatterst.h"
#include "df/body_appearance_modifier.h"
#include "df/body_part_layer_raw.h"
#include "df/body_part_raw.h"
#include "df/bp_appearance_modifier.h"
#include "df/builtin_mats.h"
#include "df/building_wellst.h"

#include "df/caste_raw.h"
#include "df/caste_raw.h"
#include "df/color_modifier_raw.h"
#include "df/creature_raw.h"
#include "df/creature_raw.h"
#include "df/descriptor_color.h"
#include "df/descriptor_pattern.h"
#include "df/descriptor_shape.h"
#include "df/dfhack_material_category.h"
#include "df/enabler.h"
#include "df/engraving.h"
#include "df/flow_info.h"
#include "df/flow_guide.h"
#include "df/flow_guide_item_cloudst.h"
#include "df/graphic.h"
#include "df/historical_figure.h"

#include "df/job.h"
#include "df/job_type.h"
#include "df/job_item.h"
#include "df/job_material_category.h"
#include "df/map_block_column.h"
#include "df/material_vec_ref.h"
#include "df/matter_state.h"
#include "df/mental_attribute_type.h"
#include "df/ocean_wave.h"
#include "df/physical_attribute_type.h"
#include "df/plant.h"
#include "df/plant_raw_flags.h"
#include "df/projectile.h"
#include "df/proj_itemst.h"
#include "df/proj_unitst.h"
#include "df/region_map_entry.h"
#include "df/report.h"
#include "df/site_realization_building.h"
#include "df/site_realization_building_info_castle_towerst.h"
#include "df/site_realization_building_info_castle_wallst.h"
#if DF_VERSION_INT > 34011
#include "df/site_realization_building_info_trenchesst.h"
#endif
#include "df/tissue.h"
#include "df/tissue_style_raw.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/unit_inventory_item.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/vehicle.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_geo_biome.h"
#include "df/world_geo_layer.h"
#include "df/world_history.h"
#include "df/world_population.h"
#include "df/world_region.h"
#include "df/world_region_details.h"
#include "df/world_site.h"
#include "df/world_site_realization.h"
#include "df/entity_position.h"

#if DF_VERSION_INT > 40001
#include "df/plant_growth.h"
#include "df/plant_growth_print.h"
#include "df/plant_tree_info.h"
#include "df/plant_tree_tile.h"
#endif

#include "df/unit_relationship_type.h"

#include "adventure_control.h"
#include "building_reader.h"
#include "item_reader.h"

using namespace DFHack;
using namespace df::enums;
using namespace RemoteFortressReader;
using namespace std;

DFHACK_PLUGIN("RemoteFortressReader");

#ifndef REQUIRE_GLOBAL
using namespace df::global;
#else
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(gamemode);
REQUIRE_GLOBAL(ui_advmode);
#endif

// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom

static command_result GetGrowthList(color_ostream &stream, const EmptyMessage *in, MaterialList *out);
static command_result GetMaterialList(color_ostream &stream, const EmptyMessage *in, MaterialList *out);
static command_result GetTiletypeList(color_ostream &stream, const EmptyMessage *in, TiletypeList *out);
static command_result GetBlockList(color_ostream &stream, const BlockRequest *in, BlockList *out);
static command_result GetPlantList(color_ostream &stream, const BlockRequest *in, PlantList *out);
static command_result CheckHashes(color_ostream &stream, const EmptyMessage *in);
static command_result GetUnitList(color_ostream &stream, const EmptyMessage *in, UnitList *out);
static command_result GetUnitListInside(color_ostream &stream, const BlockRequest *in, UnitList *out);
static command_result GetViewInfo(color_ostream &stream, const EmptyMessage *in, ViewInfo *out);
static command_result GetMapInfo(color_ostream &stream, const EmptyMessage *in, MapInfo *out);
static command_result ResetMapHashes(color_ostream &stream, const EmptyMessage *in);
static command_result GetWorldMap(color_ostream &stream, const EmptyMessage *in, WorldMap *out);
static command_result GetWorldMapNew(color_ostream &stream, const EmptyMessage *in, WorldMap *out);
static command_result GetWorldMapCenter(color_ostream &stream, const EmptyMessage *in, WorldMap *out);
static command_result GetRegionMaps(color_ostream &stream, const EmptyMessage *in, RegionMaps *out);
static command_result GetRegionMapsNew(color_ostream &stream, const EmptyMessage *in, RegionMaps *out);
static command_result GetCreatureRaws(color_ostream &stream, const EmptyMessage *in, CreatureRawList *out);
static command_result GetPartialCreatureRaws(color_ostream &stream, const ListRequest *in, CreatureRawList *out);
static command_result GetPlantRaws(color_ostream &stream, const EmptyMessage *in, PlantRawList *out);
static command_result GetPartialPlantRaws(color_ostream &stream, const ListRequest *in, PlantRawList *out);
static command_result CopyScreen(color_ostream &stream, const EmptyMessage *in, ScreenCapture *out);
static command_result PassKeyboardEvent(color_ostream &stream, const KeyboardEvent *in);
static command_result SendDigCommand(color_ostream &stream, const DigCommand *in);
static command_result SetPauseState(color_ostream & stream, const SingleBool * in);
static command_result GetPauseState(color_ostream & stream, const EmptyMessage * in, SingleBool * out);
static command_result GetVersionInfo(color_ostream & stream, const EmptyMessage * in, RemoteFortressReader::VersionInfo * out);
static command_result GetReports(color_ostream & stream, const EmptyMessage * in, RemoteFortressReader::Status * out);
static command_result GetLanguage(color_ostream & stream, const EmptyMessage * in, RemoteFortressReader::Language * out);


void CopyBlock(df::map_block * DfBlock, RemoteFortressReader::MapBlock * NetBlock, MapExtras::MapCache * MC, DFCoord pos);

const char* growth_locations[] = {
    "TWIGS",
    "LIGHT_BRANCHES",
    "HEAVY_BRANCHES",
    "TRUNK",
    "ROOTS",
    "CAP",
    "SAPLING",
    "SHRUB"
};
#define GROWTH_LOCATIONS_SIZE 8


#include "df/art_image.h"
#include "df/art_image_chunk.h"
#include "df/art_image_ref.h"
command_result loadArtImageChunk(color_ostream &out, vector <string> & parameters)
{
    if (parameters.size() != 1)
        return CR_WRONG_USAGE;

    if (!Core::getInstance().isWorldLoaded())
    {
        out.printerr("No world loaded\n");
        return CR_FAILURE;
    }

    GET_ART_IMAGE_CHUNK GetArtImageChunk = reinterpret_cast<GET_ART_IMAGE_CHUNK>(Core::getInstance().vinfo->getAddress("get_art_image_chunk"));
    if (GetArtImageChunk)
    {
        int index = atoi(parameters[0].c_str());
        auto chunk = GetArtImageChunk(&(world->art_image_chunks), index);
        out.print("Loaded chunk id: %d\n", chunk->id);
    }
    return CR_OK;
}

command_result dump_bp_mods(color_ostream &out, vector <string> & parameters)
{
    remove("bp_appearance_mods.csv");
    ofstream output;
    output.open("bp_appearance_mods.csv");

    output << "Race Index;Race;Caste;Bodypart Token;Bodypart Name;Tissue Layer;Modifier Type;Range\n";

    for (int creatureIndex = 0; creatureIndex < world->raws.creatures.all.size(); creatureIndex++)
    {
        auto creatureRaw = world->raws.creatures.all[creatureIndex];
        for (int casteIndex = 0; casteIndex < creatureRaw->caste.size(); casteIndex++)
        {
            df::caste_raw *casteRaw = creatureRaw->caste[casteIndex];
            for (int partIndex = 0; partIndex < casteRaw->bp_appearance.part_idx.size(); partIndex++)
            {
                output << creatureIndex << ";";
                output << creatureRaw->creature_id << ";";
                output << casteRaw->caste_id << ";";
                output << casteRaw->body_info.body_parts[casteRaw->bp_appearance.part_idx[partIndex]]->token << ";";
                output << casteRaw->body_info.body_parts[casteRaw->bp_appearance.part_idx[partIndex]]->name_singular[0]->c_str() << ";";
                int layer = casteRaw->bp_appearance.layer_idx[partIndex];
                if (layer < 0)
                    output << "N/A;";
                else
                    output << casteRaw->body_info.body_parts[casteRaw->bp_appearance.part_idx[partIndex]]->layers[layer]->layer_name << ";";
                output << ENUM_KEY_STR(appearance_modifier_type, casteRaw->bp_appearance.modifiers[casteRaw->bp_appearance.modifier_idx[partIndex]]->type) << ";";
                auto appMod = casteRaw->bp_appearance.modifiers[casteRaw->bp_appearance.modifier_idx[partIndex]];
#if DF_VERSION_INT > 34011
                if (appMod->growth_rate > 0)
                {
                    output << appMod->growth_min << " - " << appMod->growth_max << "\n";
                }
                else
#endif
                {
                    output << casteRaw->bp_appearance.modifiers[casteRaw->bp_appearance.modifier_idx[partIndex]]->ranges[0] << " - ";
                    output << casteRaw->bp_appearance.modifiers[casteRaw->bp_appearance.modifier_idx[partIndex]]->ranges[6] << "\n";
                }
            }
        }
    }

    output.close();

    return CR_OK;
}

command_result RemoteFortressReader_version(color_ostream &out, vector<string> &parameters)
{
    out.print(RFR_VERSION);
    return CR_OK;
}

DFHACK_PLUGIN_IS_ENABLED(enableUpdates);

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands)
{
    //// Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "dump-bp-mods", "Dump bodypart mods for debugging",
        dump_bp_mods, false, /* true means that the command can't be used from non-interactive user interface */
                             // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  This command does nothing at all.\n"
        "Example:\n"
        "  isoworldremote\n"
        "    Does nothing.\n"
    ));
    commands.push_back(PluginCommand("RemoteFortressReader_version", "List the loaded RemoteFortressReader version", RemoteFortressReader_version, false, "This is used for plugin version checking."));
    commands.push_back(PluginCommand(
        "load-art-image-chunk",
        "Gets an art image chunk by index, loading from disk if necessary",
        loadArtImageChunk, false,
        "Usage: load_art_image_chunk N, where N is the id of the chunk to get."));
    enableUpdates = true;
    return CR_OK;
}

#ifndef SF_ALLOW_REMOTE
#define SF_ALLOW_REMOTE 0
#endif // !SF_ALLOW_REMOTE

DFhackCExport RPCService *plugin_rpcconnect(color_ostream &)
{
    RPCService *svc = new RPCService();
    svc->addFunction("GetMaterialList", GetMaterialList, SF_ALLOW_REMOTE);
    svc->addFunction("GetGrowthList", GetGrowthList, SF_ALLOW_REMOTE);
    svc->addFunction("GetBlockList", GetBlockList, SF_ALLOW_REMOTE);
    svc->addFunction("CheckHashes", CheckHashes, SF_ALLOW_REMOTE);
    svc->addFunction("GetTiletypeList", GetTiletypeList, SF_ALLOW_REMOTE);
    svc->addFunction("GetPlantList", GetPlantList, SF_ALLOW_REMOTE);
    svc->addFunction("GetUnitList", GetUnitList, SF_ALLOW_REMOTE);
    svc->addFunction("GetUnitListInside", GetUnitListInside, SF_ALLOW_REMOTE);
    svc->addFunction("GetViewInfo", GetViewInfo, SF_ALLOW_REMOTE);
    svc->addFunction("GetMapInfo", GetMapInfo, SF_ALLOW_REMOTE);
    svc->addFunction("ResetMapHashes", ResetMapHashes, SF_ALLOW_REMOTE);
    svc->addFunction("GetItemList", GetItemList, SF_ALLOW_REMOTE);
    svc->addFunction("GetBuildingDefList", GetBuildingDefList, SF_ALLOW_REMOTE);
    svc->addFunction("GetWorldMap", GetWorldMap, SF_ALLOW_REMOTE);
    svc->addFunction("GetWorldMapNew", GetWorldMapNew, SF_ALLOW_REMOTE);
    svc->addFunction("GetRegionMaps", GetRegionMaps, SF_ALLOW_REMOTE);
    svc->addFunction("GetRegionMapsNew", GetRegionMapsNew, SF_ALLOW_REMOTE);
    svc->addFunction("GetCreatureRaws", GetCreatureRaws, SF_ALLOW_REMOTE);
    svc->addFunction("GetPartialCreatureRaws", GetPartialCreatureRaws, SF_ALLOW_REMOTE);
    svc->addFunction("GetWorldMapCenter", GetWorldMapCenter, SF_ALLOW_REMOTE);
    svc->addFunction("GetPlantRaws", GetPlantRaws, SF_ALLOW_REMOTE);
    svc->addFunction("GetPartialPlantRaws", GetPartialPlantRaws, SF_ALLOW_REMOTE);
    svc->addFunction("CopyScreen", CopyScreen, SF_ALLOW_REMOTE);
    svc->addFunction("PassKeyboardEvent", PassKeyboardEvent, SF_ALLOW_REMOTE);
    svc->addFunction("SendDigCommand", SendDigCommand, SF_ALLOW_REMOTE);
    svc->addFunction("SetPauseState", SetPauseState, SF_ALLOW_REMOTE);
    svc->addFunction("GetPauseState", GetPauseState, SF_ALLOW_REMOTE);
    svc->addFunction("GetVersionInfo", GetVersionInfo, SF_ALLOW_REMOTE);
    svc->addFunction("GetReports", GetReports, SF_ALLOW_REMOTE);
    svc->addFunction("MoveCommand", MoveCommand, SF_ALLOW_REMOTE);
    svc->addFunction("JumpCommand", JumpCommand, SF_ALLOW_REMOTE);
    svc->addFunction("MenuQuery", MenuQuery, SF_ALLOW_REMOTE);
    svc->addFunction("MovementSelectCommand", MovementSelectCommand, SF_ALLOW_REMOTE);
    svc->addFunction("MiscMoveCommand", MiscMoveCommand, SF_ALLOW_REMOTE);
    svc->addFunction("GetLanguage", GetLanguage, SF_ALLOW_REMOTE);
    return svc;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    // You *MUST* kill all threads you created before this returns.
    // If everything fails, just return CR_FAILURE. Your plugin will be
    // in a zombie state, but things won't crash.
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (!enableUpdates)
        return CR_OK;
    KeyUpdate();
    return CR_OK;
}

uint16_t fletcher16(uint8_t const *data, size_t bytes)
{
    uint16_t sum1 = 0xff, sum2 = 0xff;

    while (bytes) {
        size_t tlen = bytes > 20 ? 20 : bytes;
        bytes -= tlen;
        do {
            sum2 += sum1 += *data++;
        } while (--tlen);
        sum1 = (sum1 & 0xff) + (sum1 >> 8);
        sum2 = (sum2 & 0xff) + (sum2 >> 8);
    }
    /* Second reduction step to reduce sums to 8 bits */
    sum1 = (sum1 & 0xff) + (sum1 >> 8);
    sum2 = (sum2 & 0xff) + (sum2 >> 8);
    return sum2 << 8 | sum1;
}

void ConvertDfColor(int16_t index, RemoteFortressReader::ColorDefinition * out)
{
    if (!df::global::enabler)
        return;

    auto enabler = df::global::enabler;

    out->set_red((int)(enabler->ccolor[index][0] * 255));
    out->set_green((int)(enabler->ccolor[index][1] * 255));
    out->set_blue((int)(enabler->ccolor[index][2] * 255));
}

void ConvertDfColor(int16_t in[3], RemoteFortressReader::ColorDefinition * out)
{
    int index = in[0] | (8 * in[2]);
    ConvertDfColor(index, out);
}

void ConvertDFColorDescriptor(int16_t index, RemoteFortressReader::ColorDefinition * out)
{
    df::descriptor_color *color = world->raws.descriptors.colors[index];
    out->set_red(color->red * 255);
    out->set_green(color->green * 255);
    out->set_blue(color->blue * 255);
}

void ConvertDFCoord(DFCoord in, RemoteFortressReader::Coord * out)
{
    out->set_x(in.x);
    out->set_y(in.y);
    out->set_z(in.z);
}

void ConvertDFCoord(int x, int y, int z, RemoteFortressReader::Coord * out)
{
    out->set_x(x);
    out->set_y(y);
    out->set_z(z);
}


RemoteFortressReader::TiletypeMaterial TranslateMaterial(df::tiletype_material material)
{
    switch (material)
    {
    case df::enums::tiletype_material::NONE:
        return RemoteFortressReader::NO_MATERIAL;
        break;
    case df::enums::tiletype_material::AIR:
        return RemoteFortressReader::AIR;
        break;
    case df::enums::tiletype_material::SOIL:
        return RemoteFortressReader::SOIL;
        break;
    case df::enums::tiletype_material::STONE:
        return RemoteFortressReader::STONE;
        break;
    case df::enums::tiletype_material::FEATURE:
        return RemoteFortressReader::FEATURE;
        break;
    case df::enums::tiletype_material::LAVA_STONE:
        return RemoteFortressReader::LAVA_STONE;
        break;
    case df::enums::tiletype_material::MINERAL:
        return RemoteFortressReader::MINERAL;
        break;
    case df::enums::tiletype_material::FROZEN_LIQUID:
        return RemoteFortressReader::FROZEN_LIQUID;
        break;
    case df::enums::tiletype_material::CONSTRUCTION:
        return RemoteFortressReader::CONSTRUCTION;
        break;
    case df::enums::tiletype_material::GRASS_LIGHT:
        return RemoteFortressReader::GRASS_LIGHT;
        break;
    case df::enums::tiletype_material::GRASS_DARK:
        return RemoteFortressReader::GRASS_DARK;
        break;
    case df::enums::tiletype_material::GRASS_DRY:
        return RemoteFortressReader::GRASS_DRY;
        break;
    case df::enums::tiletype_material::GRASS_DEAD:
        return RemoteFortressReader::GRASS_DEAD;
        break;
    case df::enums::tiletype_material::PLANT:
        return RemoteFortressReader::PLANT;
        break;
    case df::enums::tiletype_material::HFS:
        return RemoteFortressReader::HFS;
        break;
    case df::enums::tiletype_material::CAMPFIRE:
        return RemoteFortressReader::CAMPFIRE;
        break;
    case df::enums::tiletype_material::FIRE:
        return RemoteFortressReader::FIRE;
        break;
    case df::enums::tiletype_material::ASHES:
        return RemoteFortressReader::ASHES;
        break;
    case df::enums::tiletype_material::MAGMA:
        return RemoteFortressReader::MAGMA;
        break;
    case df::enums::tiletype_material::DRIFTWOOD:
        return RemoteFortressReader::DRIFTWOOD;
        break;
    case df::enums::tiletype_material::POOL:
        return RemoteFortressReader::POOL;
        break;
    case df::enums::tiletype_material::BROOK:
        return RemoteFortressReader::BROOK;
        break;
    case df::enums::tiletype_material::RIVER:
        return RemoteFortressReader::RIVER;
        break;
#if DF_VERSION_INT > 40001
    case df::enums::tiletype_material::ROOT:
        return RemoteFortressReader::ROOT;
        break;
    case df::enums::tiletype_material::TREE:
        return RemoteFortressReader::TREE_MATERIAL;
        break;
    case df::enums::tiletype_material::MUSHROOM:
        return RemoteFortressReader::MUSHROOM;
        break;
    case df::enums::tiletype_material::UNDERWORLD_GATE:
        return RemoteFortressReader::UNDERWORLD_GATE;
        break;
#endif
    default:
        return RemoteFortressReader::NO_MATERIAL;
        break;
    }
    return RemoteFortressReader::NO_MATERIAL;
}

RemoteFortressReader::TiletypeSpecial TranslateSpecial(df::tiletype_special special)
{
    switch (special)
    {
    case df::enums::tiletype_special::NONE:
        return RemoteFortressReader::NO_SPECIAL;
        break;
    case df::enums::tiletype_special::NORMAL:
        return RemoteFortressReader::NORMAL;
        break;
    case df::enums::tiletype_special::RIVER_SOURCE:
        return RemoteFortressReader::RIVER_SOURCE;
        break;
    case df::enums::tiletype_special::WATERFALL:
        return RemoteFortressReader::WATERFALL;
        break;
    case df::enums::tiletype_special::SMOOTH:
        return RemoteFortressReader::SMOOTH;
        break;
    case df::enums::tiletype_special::FURROWED:
        return RemoteFortressReader::FURROWED;
        break;
    case df::enums::tiletype_special::WET:
        return RemoteFortressReader::WET;
        break;
    case df::enums::tiletype_special::DEAD:
        return RemoteFortressReader::DEAD;
        break;
    case df::enums::tiletype_special::WORN_1:
        return RemoteFortressReader::WORN_1;
        break;
    case df::enums::tiletype_special::WORN_2:
        return RemoteFortressReader::WORN_2;
        break;
    case df::enums::tiletype_special::WORN_3:
        return RemoteFortressReader::WORN_3;
        break;
    case df::enums::tiletype_special::TRACK:
        return RemoteFortressReader::TRACK;
        break;
#if DF_VERSION_INT > 40001
    case df::enums::tiletype_special::SMOOTH_DEAD:
        return RemoteFortressReader::SMOOTH_DEAD;
        break;
#endif
    default:
        return RemoteFortressReader::NO_SPECIAL;
        break;
    }
    return RemoteFortressReader::NO_SPECIAL;
}

RemoteFortressReader::TiletypeShape TranslateShape(df::tiletype_shape shape)
{
    switch (shape)
    {
    case df::enums::tiletype_shape::NONE:
        return RemoteFortressReader::NO_SHAPE;
        break;
    case df::enums::tiletype_shape::EMPTY:
        return RemoteFortressReader::EMPTY;
        break;
    case df::enums::tiletype_shape::FLOOR:
        return RemoteFortressReader::FLOOR;
        break;
    case df::enums::tiletype_shape::BOULDER:
        return RemoteFortressReader::BOULDER;
        break;
    case df::enums::tiletype_shape::PEBBLES:
        return RemoteFortressReader::PEBBLES;
        break;
    case df::enums::tiletype_shape::WALL:
        return RemoteFortressReader::WALL;
        break;
    case df::enums::tiletype_shape::FORTIFICATION:
        return RemoteFortressReader::FORTIFICATION;
        break;
    case df::enums::tiletype_shape::STAIR_UP:
        return RemoteFortressReader::STAIR_UP;
        break;
    case df::enums::tiletype_shape::STAIR_DOWN:
        return RemoteFortressReader::STAIR_DOWN;
        break;
    case df::enums::tiletype_shape::STAIR_UPDOWN:
        return RemoteFortressReader::STAIR_UPDOWN;
        break;
    case df::enums::tiletype_shape::RAMP:
        return RemoteFortressReader::RAMP;
        break;
    case df::enums::tiletype_shape::RAMP_TOP:
        return RemoteFortressReader::RAMP_TOP;
        break;
    case df::enums::tiletype_shape::BROOK_BED:
        return RemoteFortressReader::BROOK_BED;
        break;
    case df::enums::tiletype_shape::BROOK_TOP:
        return RemoteFortressReader::BROOK_TOP;
        break;
#if DF_VERSION_INT > 40001
    case df::enums::tiletype_shape::BRANCH:
        return RemoteFortressReader::BRANCH;
        break;
#endif
#if DF_VERSION_INT < 40001
    case df::enums::tiletype_shape::TREE:
        return RemoteFortressReader::TREE_SHAPE;
        break;
#endif
#if DF_VERSION_INT > 40001

    case df::enums::tiletype_shape::TRUNK_BRANCH:
        return RemoteFortressReader::TRUNK_BRANCH;
        break;
    case df::enums::tiletype_shape::TWIG:
        return RemoteFortressReader::TWIG;
        break;
#endif
    case df::enums::tiletype_shape::SAPLING:
        return RemoteFortressReader::SAPLING;
        break;
    case df::enums::tiletype_shape::SHRUB:
        return RemoteFortressReader::SHRUB;
        break;
    case df::enums::tiletype_shape::ENDLESS_PIT:
        return RemoteFortressReader::EMPTY;
        break;
    default:
        return RemoteFortressReader::NO_SHAPE;
        break;
    }
    return RemoteFortressReader::NO_SHAPE;
}

RemoteFortressReader::TiletypeVariant TranslateVariant(df::tiletype_variant variant)
{
    switch (variant)
    {
    case df::enums::tiletype_variant::NONE:
        return RemoteFortressReader::NO_VARIANT;
        break;
    case df::enums::tiletype_variant::VAR_1:
        return RemoteFortressReader::VAR_1;
        break;
    case df::enums::tiletype_variant::VAR_2:
        return RemoteFortressReader::VAR_2;
        break;
    case df::enums::tiletype_variant::VAR_3:
        return RemoteFortressReader::VAR_3;
        break;
    case df::enums::tiletype_variant::VAR_4:
        return RemoteFortressReader::VAR_4;
        break;
    default:
        return RemoteFortressReader::NO_VARIANT;
        break;
    }
    return RemoteFortressReader::NO_VARIANT;
}

static command_result CheckHashes(color_ostream &stream, const EmptyMessage *in)
{
    clock_t start = clock();
    for (int i = 0; i < world->map.map_blocks.size(); i++)
    {
        df::map_block * block = world->map.map_blocks[i];
        fletcher16((uint8_t*)(block->tiletype), 16 * 16 * sizeof(df::enums::tiletype::tiletype));
    }
    clock_t end = clock();
    double elapsed_secs = double(end - start) / CLOCKS_PER_SEC;
    stream.print("Checking all hashes took %f seconds.", elapsed_secs);
    return CR_OK;
}

void CopyMat(RemoteFortressReader::MatPair * mat, int type, int index)
{
    if (type >= MaterialInfo::FIGURE_BASE && type < MaterialInfo::PLANT_BASE)
    {
        df::historical_figure * figure = df::historical_figure::find(index);
        if (figure)
        {
            type -= MaterialInfo::GROUP_SIZE;
            index = figure->race;
        }
    }
    mat->set_mat_type(type);
    mat->set_mat_index(index);

}

map<DFCoord, uint16_t> hashes;

bool IsTiletypeChanged(DFCoord pos)
{
    uint16_t hash;
    df::map_block * block = Maps::getBlock(pos);
    if (block)
        hash = fletcher16((uint8_t*)(block->tiletype), 16 * 16 * (sizeof(df::enums::tiletype::tiletype)));
    else
        hash = 0;
    if (hashes[pos] != hash)
    {
        hashes[pos] = hash;
        return true;
    }
    return false;
}

map<DFCoord, uint16_t> waterHashes;

bool IsDesignationChanged(DFCoord pos)
{
    uint16_t hash;
    df::map_block * block = Maps::getBlock(pos);
    if (block)
        hash = fletcher16((uint8_t*)(block->designation), 16 * 16 * (sizeof(df::tile_designation)));
    else
        hash = 0;
    if (waterHashes[pos] != hash)
    {
        waterHashes[pos] = hash;
        return true;
    }
    return false;
}

map<DFCoord, uint8_t> buildingHashes;

bool IsBuildingChanged(DFCoord pos)
{
    df::map_block * block = Maps::getBlock(pos);
    bool changed = false;
    for (int x = 0; x < 16; x++)
        for (int y = 0; y < 16; y++)
        {
            DFCoord localPos = DFCoord(pos.x * 16 + x, pos.y * 16 + y, pos.z);
            auto bld = block->occupancy[x][y].bits.building;
            if (buildingHashes[pos] != bld)
            {
                buildingHashes[pos] = bld;
                changed = true;
            }
        }
    return changed;
}

map<DFCoord, uint16_t> spatterHashes;

bool IsspatterChanged(DFCoord pos)
{
    df::map_block * block = Maps::getBlock(pos);
    bool changed = false;
    std::vector<df::block_square_event_material_spatterst *> materials;
#if DF_VERSION_INT > 34011
    std::vector<df::block_square_event_item_spatterst *> items;
    if (!Maps::SortBlockEvents(block, NULL, NULL, &materials, NULL, NULL, NULL, &items))
        return false;
#else
    if (!Maps::SortBlockEvents(block, NULL, NULL, &materials, NULL, NULL))
        return false;
#endif

    uint16_t hash = 0;

    for (int i = 0; i < materials.size(); i++)
    {
        auto mat = materials[i];
        hash ^= fletcher16((uint8_t*)mat, sizeof(df::block_square_event_material_spatterst));
    }
#if DF_VERSION_INT > 34011
    for (int i = 0; i < items.size(); i++)
    {
        auto item = items[i];
        hash ^= fletcher16((uint8_t*)item, sizeof(df::block_square_event_item_spatterst));
    }
#endif
    if (spatterHashes[pos] != hash)
    {
        spatterHashes[pos] = hash;
        return true;
    }
    return false;
}

map<int, uint16_t> itemHashes;

bool isItemChanged(int i)
{
    uint16_t hash = 0;
    auto item = df::item::find(i);
    if (item)
    {
        hash = fletcher16((uint8_t*)item, sizeof(df::item));
    }
    if (itemHashes[i] != hash)
    {
        itemHashes[i] = hash;
        return true;
    }
    return false;
}

bool areItemsChanged(vector<int> * items)
{
    bool result = false;
    for (int i = 0; i < items->size(); i++)
    {
        if (isItemChanged(items->at(i)))
            result = true;
    }
    return result;
}

map<int, int> engravingHashes;

bool isEngravingNew(int index)
{
    if (engravingHashes[index])
        return false;
    engravingHashes[index] = true;
    return true;
}

void engravingIsNotNew(int index)
{
    engravingHashes[index] = false;
}

static command_result ResetMapHashes(color_ostream &stream, const EmptyMessage *in)
{
    hashes.clear();
    waterHashes.clear();
    buildingHashes.clear();
    spatterHashes.clear();
    itemHashes.clear();
    engravingHashes.clear();
    return CR_OK;
}

df::matter_state GetState(df::material * mat, uint16_t temp = 10015)
{
    df::matter_state state = matter_state::Solid;
    if (temp >= mat->heat.melting_point)
        state = df::matter_state::Liquid;
    if (temp >= mat->heat.boiling_point)
        state = matter_state::Gas;
    return state;
}

static command_result GetMaterialList(color_ostream &stream, const EmptyMessage *in, MaterialList *out)
{
    if (!Core::getInstance().isWorldLoaded()) {
        //out->set_available(false);
        return CR_OK;
    }



    df::world_raws *raws = &world->raws;
    df::world_history *history = &world->history;
    MaterialInfo mat;
    for (int i = 0; i < raws->inorganics.size(); i++)
    {
        mat.decode(0, i);
        MaterialDefinition *mat_def = out->add_material_list();
        mat_def->mutable_mat_pair()->set_mat_type(0);
        mat_def->mutable_mat_pair()->set_mat_index(i);
        mat_def->set_id(mat.getToken());
        mat_def->set_name(mat.toString()); //find the name at cave temperature;
        if (raws->inorganics[i]->material.state_color[GetState(&raws->inorganics[i]->material)] < raws->descriptors.colors.size())
        {
            ConvertDFColorDescriptor(raws->inorganics[i]->material.state_color[GetState(&raws->inorganics[i]->material)], mat_def->mutable_state_color());
        }
    }
    for (int i = 0; i < 19; i++)
    {
        int k = -1;
        if (i == 7)
            k = 1;// for coal.
        for (int j = -1; j <= k; j++)
        {
            mat.decode(i, j);
            MaterialDefinition *mat_def = out->add_material_list();
            mat_def->mutable_mat_pair()->set_mat_type(i);
            mat_def->mutable_mat_pair()->set_mat_index(j);
            mat_def->set_id(mat.getToken());
            mat_def->set_name(mat.toString()); //find the name at cave temperature;
            if (raws->mat_table.builtin[i]->state_color[GetState(raws->mat_table.builtin[i])] < raws->descriptors.colors.size())
            {
                ConvertDFColorDescriptor(raws->mat_table.builtin[i]->state_color[GetState(raws->mat_table.builtin[i])], mat_def->mutable_state_color());
            }
        }
    }
    for (int i = 0; i < raws->creatures.all.size(); i++)
    {
        df::creature_raw * creature = raws->creatures.all[i];
        for (int j = 0; j < creature->material.size(); j++)
        {
            mat.decode(j + MaterialInfo::CREATURE_BASE, i);
            MaterialDefinition *mat_def = out->add_material_list();
            mat_def->mutable_mat_pair()->set_mat_type(j + 19);
            mat_def->mutable_mat_pair()->set_mat_index(i);
            mat_def->set_id(mat.getToken());
            mat_def->set_name(mat.toString()); //find the name at cave temperature;
            if (creature->material[j]->state_color[GetState(creature->material[j])] < raws->descriptors.colors.size())
            {
                ConvertDFColorDescriptor(creature->material[j]->state_color[GetState(creature->material[j])], mat_def->mutable_state_color());
            }
        }
    }
    //for (int i = 0; i < history->figures.size(); i++)
    //{
    //    df::historical_figure * figure = history->figures[i];
    //    if (figure->race < 0)
    //        continue;
    //    df::creature_raw * creature = raws->creatures.all[figure->race];
    //    for (int j = 0; j < creature->material.size(); j++)
    //    {
    //        mat.decode(j + MaterialInfo::FIGURE_BASE, i);
    //        MaterialDefinition *mat_def = out->add_material_list();
    //        mat_def->mutable_mat_pair()->set_mat_type(j + MaterialInfo::FIGURE_BASE);
    //        mat_def->mutable_mat_pair()->set_mat_index(i);
    //        stringstream id;
    //        id << "HF" << i << mat.getToken();
    //        mat_def->set_id(id.str());
    //        mat_def->set_name(mat.toString()); //find the name at cave temperature;
    //        if (creature->material[j]->state_color[GetState(creature->material[j])] < raws->descriptors.colors.size())
    //        {
    //            df::descriptor_color *color = raws->descriptors.colors[creature->material[j]->state_color[GetState(creature->material[j])]];
    //            mat_def->mutable_state_color()->set_red(color->red * 255);
    //            mat_def->mutable_state_color()->set_green(color->green * 255);
    //            mat_def->mutable_state_color()->set_blue(color->blue * 255);
    //        }
    //    }
    //}
    for (int i = 0; i < raws->plants.all.size(); i++)
    {
        df::plant_raw * plant = raws->plants.all[i];
        for (int j = 0; j < plant->material.size(); j++)
        {
            mat.decode(j + 419, i);
            MaterialDefinition *mat_def = out->add_material_list();
            mat_def->mutable_mat_pair()->set_mat_type(j + 419);
            mat_def->mutable_mat_pair()->set_mat_index(i);
            mat_def->set_id(mat.getToken());
            mat_def->set_name(mat.toString()); //find the name at cave temperature;
            if (plant->material[j]->state_color[GetState(plant->material[j])] < raws->descriptors.colors.size())
            {
                ConvertDFColorDescriptor(plant->material[j]->state_color[GetState(plant->material[j])], mat_def->mutable_state_color());
            }
        }
    }
    return CR_OK;
}

static command_result GetGrowthList(color_ostream &stream, const EmptyMessage *in, MaterialList *out)
{
    if (!Core::getInstance().isWorldLoaded()) {
        //out->set_available(false);
        return CR_OK;
    }



    df::world_raws *raws = &world->raws;
    if (!raws)
        return CR_OK;//'.


    for (int i = 0; i < raws->plants.all.size(); i++)
    {
        df::plant_raw * pp = raws->plants.all[i];
        if (!pp)
            continue;
        MaterialDefinition * basePlant = out->add_material_list();
        basePlant->set_id(pp->id + ":BASE");
        basePlant->set_name(pp->name);
        basePlant->mutable_mat_pair()->set_mat_type(-1);
        basePlant->mutable_mat_pair()->set_mat_index(i);
#if DF_VERSION_INT > 40001
        for (int g = 0; g < pp->growths.size(); g++)
        {
            df::plant_growth* growth = pp->growths[g];
            if (!growth)
                continue;
            for (int l = 0; l < GROWTH_LOCATIONS_SIZE; l++)
            {
                MaterialDefinition * out_growth = out->add_material_list();
                out_growth->set_id(pp->id + ":" + growth->id + +":" + growth_locations[l]);
                out_growth->set_name(growth->name);
                out_growth->mutable_mat_pair()->set_mat_type(g * 10 + l);
                out_growth->mutable_mat_pair()->set_mat_index(i);
            }
        }
#endif
    }
    return CR_OK;
}

void CopyBlock(df::map_block * DfBlock, RemoteFortressReader::MapBlock * NetBlock, MapExtras::MapCache * MC, DFCoord pos)
{
    NetBlock->set_map_x(DfBlock->map_pos.x);
    NetBlock->set_map_y(DfBlock->map_pos.y);
    NetBlock->set_map_z(DfBlock->map_pos.z);

    MapExtras::Block * block = MC->BlockAtTile(DfBlock->map_pos);

    int trunk_percent[16][16];
    int tree_x[16][16];
    int tree_y[16][16];
    int tree_z[16][16];
    for (int xx = 0; xx < 16; xx++)
        for (int yy = 0; yy < 16; yy++)
        {
            trunk_percent[xx][yy] = 255;
            tree_x[xx][yy] = -3000;
            tree_y[xx][yy] = -3000;
            tree_z[xx][yy] = -3000;
        }

#if DF_VERSION_INT > 34011
    df::map_block_column * column = df::global::world->map.column_index[(DfBlock->map_pos.x / 48) * 3][(DfBlock->map_pos.y / 48) * 3];
    for (int i = 0; i < column->plants.size(); i++)
    {
        df::plant* plant = column->plants[i];
        if (plant->tree_info == NULL)
            continue;
        df::plant_tree_info * tree_info = plant->tree_info;
        if (
            plant->pos.z - tree_info->roots_depth > DfBlock->map_pos.z
            || (plant->pos.z + tree_info->body_height) <= DfBlock->map_pos.z
            || (plant->pos.x - tree_info->dim_x / 2) > (DfBlock->map_pos.x + 16)
            || (plant->pos.x + tree_info->dim_x / 2) < (DfBlock->map_pos.x)
            || (plant->pos.y - tree_info->dim_y / 2) > (DfBlock->map_pos.y + 16)
            || (plant->pos.y + tree_info->dim_y / 2) < (DfBlock->map_pos.y)
            )
            continue;
        DFCoord localPos = plant->pos - DfBlock->map_pos;
        for (int xx = 0; xx < tree_info->dim_x; xx++)
            for (int yy = 0; yy < tree_info->dim_y; yy++)
            {
                int xxx = localPos.x - (tree_info->dim_x / 2) + xx;
                int yyy = localPos.y - (tree_info->dim_y / 2) + yy;
                if (xxx < 0
                    || yyy < 0
                    || xxx >= 16
                    || yyy >= 16
                    )
                    continue;
                df::plant_tree_tile tile;
                if (-localPos.z < 0)
                {
                    tile = tree_info->roots[-1 + localPos.z][xx + (yy*tree_info->dim_x)];
                }
                else
                {
                    tile = tree_info->body[-localPos.z][xx + (yy*tree_info->dim_x)];
                }
                if (!tile.whole || tile.bits.blocked)
                    continue;
                if (tree_info->body_height <= 1)
                    trunk_percent[xxx][yyy] = 0;
                else
                    trunk_percent[xxx][yyy] = -localPos.z * 100 / (tree_info->body_height - 1);
                tree_x[xxx][yyy] = xx - tree_info->dim_x / 2;
                tree_y[xxx][yyy] = yy - tree_info->dim_y / 2;
                tree_z[xxx][yyy] = localPos.z;
            }
    }
#endif
    for (int yy = 0; yy < 16; yy++)
        for (int xx = 0; xx < 16; xx++)
        {
            df::tiletype tile = DfBlock->tiletype[xx][yy];
            NetBlock->add_tiles(tile);
            df::coord2d p = df::coord2d(xx, yy);
            t_matpair baseMat = block->baseMaterialAt(p);
            t_matpair staticMat = block->staticMaterialAt(p);
            switch (tileMaterial(tile))
            {
            case tiletype_material::FROZEN_LIQUID:
                staticMat.mat_type = builtin_mats::WATER;
                staticMat.mat_index = -1;
                break;
            default:
                break;
            }
            CopyMat(NetBlock->add_materials(), staticMat.mat_type, staticMat.mat_index);
            CopyMat(NetBlock->add_layer_materials(), 0, block->layerMaterialAt(p));
            CopyMat(NetBlock->add_vein_materials(), 0, block->veinMaterialAt(p));
            CopyMat(NetBlock->add_base_materials(), baseMat.mat_type, baseMat.mat_index);
            RemoteFortressReader::MatPair * constructionItem = NetBlock->add_construction_items();
            CopyMat(constructionItem, -1, -1);
            if (tileMaterial(tile) == tiletype_material::CONSTRUCTION)
            {
                df::construction *con = df::construction::find(DfBlock->map_pos + df::coord(xx, yy, 0));
                if (con)
                {
                    CopyMat(constructionItem, con->item_type, con->item_subtype);
                }
            }
            NetBlock->add_tree_percent(trunk_percent[xx][yy]);
            NetBlock->add_tree_x(tree_x[xx][yy]);
            NetBlock->add_tree_y(tree_y[xx][yy]);
            NetBlock->add_tree_z(tree_z[xx][yy]);
        }
}

void CopyDesignation(df::map_block * DfBlock, RemoteFortressReader::MapBlock * NetBlock, MapExtras::MapCache * MC, DFCoord pos)
{
    NetBlock->set_map_x(DfBlock->map_pos.x);
    NetBlock->set_map_y(DfBlock->map_pos.y);
    NetBlock->set_map_z(DfBlock->map_pos.z);

    for (int yy = 0; yy < 16; yy++)
        for (int xx = 0; xx < 16; xx++)
        {
            df::tile_designation designation = DfBlock->designation[xx][yy];
            df::tile_occupancy occupancy = DfBlock->occupancy[xx][yy];
            int lava = 0;
            int water = 0;
            if (designation.bits.liquid_type == df::enums::tile_liquid::Magma)
                lava = designation.bits.flow_size;
            else
                water = designation.bits.flow_size;
            NetBlock->add_magma(lava);
            NetBlock->add_water(water);
            NetBlock->add_aquifer(designation.bits.water_table);
            NetBlock->add_light(designation.bits.light);
            NetBlock->add_outside(designation.bits.outside);
            NetBlock->add_subterranean(designation.bits.subterranean);
            NetBlock->add_water_salt(designation.bits.water_salt);
            NetBlock->add_water_stagnant(designation.bits.water_stagnant);
            if (gamemode && (*gamemode == game_mode::ADVENTURE))
            {
                auto fog_of_war = DfBlock->fog_of_war[xx][yy];
                NetBlock->add_hidden(designation.bits.dig == TileDigDesignation::NO_DIG || designation.bits.hidden);
                NetBlock->add_tile_dig_designation(TileDigDesignation::NO_DIG);
                NetBlock->add_tile_dig_designation_marker(false);
                NetBlock->add_tile_dig_designation_auto(false);
            }
            else
            {
                NetBlock->add_hidden(designation.bits.hidden);
#if DF_VERSION_INT > 34011
                NetBlock->add_tile_dig_designation_marker(occupancy.bits.dig_marked);
                NetBlock->add_tile_dig_designation_auto(occupancy.bits.dig_auto);
#endif
                switch (designation.bits.dig)
                {
                case df::enums::tile_dig_designation::No:
                    NetBlock->add_tile_dig_designation(TileDigDesignation::NO_DIG);
                    break;
                case df::enums::tile_dig_designation::Default:
                    NetBlock->add_tile_dig_designation(TileDigDesignation::DEFAULT_DIG);
                    break;
                case df::enums::tile_dig_designation::UpDownStair:
                    NetBlock->add_tile_dig_designation(TileDigDesignation::UP_DOWN_STAIR_DIG);
                    break;
                case df::enums::tile_dig_designation::Channel:
                    NetBlock->add_tile_dig_designation(TileDigDesignation::CHANNEL_DIG);
                    break;
                case df::enums::tile_dig_designation::Ramp:
                    NetBlock->add_tile_dig_designation(TileDigDesignation::RAMP_DIG);
                    break;
                case df::enums::tile_dig_designation::DownStair:
                    NetBlock->add_tile_dig_designation(TileDigDesignation::DOWN_STAIR_DIG);
                    break;
                case df::enums::tile_dig_designation::UpStair:
                    NetBlock->add_tile_dig_designation(TileDigDesignation::UP_STAIR_DIG);
                    break;
                default:
                    NetBlock->add_tile_dig_designation(TileDigDesignation::NO_DIG);
                    break;
                }
            }
        }
#if DF_VERSION_INT > 34011
    for (int i = 0; i < world->jobs.postings.size(); i++)
    {
        auto job = world->jobs.postings[i]->job;
        if (job == nullptr)
            continue;
        if (
            job->pos.z > DfBlock->map_pos.z
            || job->pos.z < DfBlock->map_pos.z
            || job->pos.x >= (DfBlock->map_pos.x + 16)
            || job->pos.x < (DfBlock->map_pos.x)
            || job->pos.y >= (DfBlock->map_pos.y + 16)
            || job->pos.y < (DfBlock->map_pos.y)
            )
            continue;

        int index = (job->pos.x - DfBlock->map_pos.x) + (16 * (job->pos.y - DfBlock->map_pos.y));

        switch (job->job_type)
        {
        case job_type::Dig:
            NetBlock->set_tile_dig_designation(index, TileDigDesignation::DEFAULT_DIG);
            break;
        case job_type::CarveUpwardStaircase:
            NetBlock->set_tile_dig_designation(index, TileDigDesignation::UP_STAIR_DIG);
            break;
        case job_type::CarveDownwardStaircase:
            NetBlock->set_tile_dig_designation(index, TileDigDesignation::DOWN_STAIR_DIG);
            break;
        case job_type::CarveUpDownStaircase:
            NetBlock->set_tile_dig_designation(index, TileDigDesignation::UP_DOWN_STAIR_DIG);
            break;
        case job_type::CarveRamp:
            NetBlock->set_tile_dig_designation(index, TileDigDesignation::RAMP_DIG);
            break;
        case job_type::DigChannel:
            NetBlock->set_tile_dig_designation(index, TileDigDesignation::CHANNEL_DIG);
            break;
        case job_type::FellTree:
            NetBlock->set_tile_dig_designation(index, TileDigDesignation::DEFAULT_DIG);
            break;
        case job_type::GatherPlants:
            NetBlock->set_tile_dig_designation(index, TileDigDesignation::DEFAULT_DIG);
            break;
        default:
            break;
        }
    }
#endif
}

void CopyProjectiles(RemoteFortressReader::MapBlock * NetBlock)
{
    for (auto proj = world->proj_list.next; proj != NULL; proj = proj->next)
    {
        STRICT_VIRTUAL_CAST_VAR(projectile, df::proj_itemst, proj->item);
        if (projectile == NULL)
            continue;
        auto NetItem = NetBlock->add_items();
        CopyItem(NetItem, projectile->item);
        NetItem->set_projectile(true);
        if (projectile->flags.bits.parabolic)
        {
            NetItem->set_subpos_x(projectile->pos_x / 100000.0);
            NetItem->set_subpos_y(projectile->pos_y / 100000.0);
            NetItem->set_subpos_z(projectile->pos_z / 140000.0);
            NetItem->set_velocity_x(projectile->speed_x / 100000.0);
            NetItem->set_velocity_y(projectile->speed_y / 100000.0);
            NetItem->set_velocity_z(projectile->speed_z / 140000.0);
        }
        else
        {
            DFCoord diff = projectile->target_pos - projectile->origin_pos;
            float max_dist = max(max(abs(diff.x), abs(diff.y)), abs(diff.z));
            NetItem->set_subpos_x(projectile->origin_pos.x + (diff.x / max_dist * projectile->distance_flown) - projectile->cur_pos.x);
            NetItem->set_subpos_y(projectile->origin_pos.y + (diff.y / max_dist * projectile->distance_flown) - projectile->cur_pos.y);
            NetItem->set_subpos_z(projectile->origin_pos.z + (diff.z / max_dist * projectile->distance_flown) - projectile->cur_pos.z);
            NetItem->set_velocity_x(diff.x / max_dist);
            NetItem->set_velocity_y(diff.y / max_dist);
            NetItem->set_velocity_z(diff.z / max_dist);
        }
    }
    for (int i = 0; i < world->vehicles.active.size(); i++)
    {
        bool isProj = false;
        auto vehicle = world->vehicles.active[i];
        for (auto proj = world->proj_list.next; proj != NULL; proj = proj->next)
        {
            STRICT_VIRTUAL_CAST_VAR(projectile, df::proj_itemst, proj->item);
            if (!projectile)
                continue;
            if (projectile->item->id == vehicle->item_id)
            {
                isProj = true;
                break;
            }
        }
        if (isProj)
            continue;

        auto item = Items::findItemByID(vehicle->item_id);
        if (!item)
            continue;
        auto NetItem = NetBlock->add_items();
        CopyItem(NetItem, item);
        NetItem->set_subpos_x(vehicle->offset_x / 100000.0);
        NetItem->set_subpos_y(vehicle->offset_y / 100000.0);
        NetItem->set_subpos_z(vehicle->offset_z / 140000.0);
        NetItem->set_velocity_x(vehicle->speed_x / 100000.0);
        NetItem->set_velocity_y(vehicle->speed_y / 100000.0);
        NetItem->set_velocity_z(vehicle->speed_z / 140000.0);

    }
}

void CopyBuildings(DFCoord min, DFCoord max, RemoteFortressReader::MapBlock * NetBlock, MapExtras::MapCache * MC)
{

    for (int i = 0; i < df::global::world->buildings.all.size(); i++)
    {
        df::building * bld = df::global::world->buildings.all[i];
        if (bld->x1 >= max.x || bld->y1 >= max.y || bld->x2 < min.x || bld->y2 < min.y)
        {
            auto out_bld = NetBlock->add_buildings();
            out_bld->set_index(bld->id);
            continue;
        }

        int z2 = bld->z;

        if (bld->getType() == building_type::Well)
        {
            df::building_wellst * well_building = virtual_cast<df::building_wellst>(bld);
            if (well_building)
            {
                z2 = well_building->bucket_z;
            }
        }
        if (bld->z < min.z || z2 >= max.z)
        {
            auto out_bld = NetBlock->add_buildings();
            out_bld->set_index(bld->id);
            continue;
        }
        auto out_bld = NetBlock->add_buildings();
        CopyBuilding(i, out_bld);
        df::building_actual* actualBuilding = virtual_cast<df::building_actual>(bld);
        if (actualBuilding)
        {
            for (int i = 0; i < actualBuilding->contained_items.size(); i++)
            {
                auto buildingItem = out_bld->add_items();
                buildingItem->set_mode(actualBuilding->contained_items[i]->use_mode);
                CopyItem(buildingItem->mutable_item(), actualBuilding->contained_items[i]->item);
            }
        }
    }
}

void Copyspatters(df::map_block * DfBlock, RemoteFortressReader::MapBlock * NetBlock, MapExtras::MapCache * MC, DFCoord pos)
{
    NetBlock->set_map_x(DfBlock->map_pos.x);
    NetBlock->set_map_y(DfBlock->map_pos.y);
    NetBlock->set_map_z(DfBlock->map_pos.z);
    std::vector<df::block_square_event_material_spatterst *> materials;
#if DF_VERSION_INT > 34011
    std::vector<df::block_square_event_item_spatterst *> items;
    std::vector<df::block_square_event_grassst *> grasses;
    if (!Maps::SortBlockEvents(DfBlock, NULL, NULL, &materials, &grasses, NULL, NULL, &items))
        return;
#else
    if (!Maps::SortBlockEvents(DfBlock, NULL, NULL, &materials, NULL, NULL))
        return;
#endif
    for (int yy = 0; yy < 16; yy++)
        for (int xx = 0; xx < 16; xx++)
        {
            auto send_pile = NetBlock->add_spatterpile();
            for (int i = 0; i < materials.size(); i++)
            {
                auto mat = materials[i];
                if (mat->amount[xx][yy] == 0)
                    continue;
                auto send_spat = send_pile->add_spatters();
                send_spat->set_state((MatterState)mat->mat_state);
                CopyMat(send_spat->mutable_material(), mat->mat_type, mat->mat_index);
                send_spat->set_amount(mat->amount[xx][yy]);
            }
#if DF_VERSION_INT > 34011
            for (int i = 0; i < items.size(); i++)
            {
                auto item = items[i];
                if (item->amount[xx][yy] == 0)
                    continue;
                auto send_spat = send_pile->add_spatters();
                CopyMat(send_spat->mutable_material(), item->mattype, item->matindex);
                send_spat->set_amount(item->amount[xx][yy]);
                auto send_item = send_spat->mutable_item();
                send_item->set_mat_type(item->item_type);
                send_item->set_mat_index(item->item_subtype);
            }
            int grassPercent = 0;
            for (int i = 0; i < grasses.size(); i++)
            {
                auto grass = grasses[i];
                if (grass->amount[xx][yy] > grassPercent)
                    grassPercent = grass->amount[xx][yy];
            }
            NetBlock->add_grass_percent(grassPercent);
#endif
        }
}

void CopyItems(df::map_block * DfBlock, RemoteFortressReader::MapBlock * NetBlock, MapExtras::MapCache * MC, DFCoord pos)
{
    NetBlock->set_map_x(DfBlock->map_pos.x);
    NetBlock->set_map_y(DfBlock->map_pos.y);
    NetBlock->set_map_z(DfBlock->map_pos.z);
    for (int i = 0; i < DfBlock->items.size(); i++)
    {
        int id = DfBlock->items[i];

        auto item = df::item::find(id);
        if (item)
            CopyItem(NetBlock->add_items(), item);
    }
}

void CopyFlow(df::flow_info * localFlow, RemoteFortressReader::FlowInfo * netFlow, int index)
{
    netFlow->set_type((FlowType)localFlow->type);
    netFlow->set_density(localFlow->density);
    ConvertDFCoord(localFlow->pos, netFlow->mutable_pos());
    ConvertDFCoord(localFlow->dest, netFlow->mutable_dest());
    netFlow->set_expanding(localFlow->expanding);
    netFlow->set_reuse(localFlow->reuse);
    netFlow->set_guide_id(localFlow->guide_id);
    auto mat = netFlow->mutable_material();
    mat->set_mat_index(localFlow->mat_index);
    mat->set_mat_type(localFlow->mat_type);
    if (localFlow->guide_id >= 0 && localFlow->type == flow_type::ItemCloud)
    {
        auto guide = df::flow_guide::find(localFlow->guide_id);
        if (guide)
        {
            VIRTUAL_CAST_VAR(cloud, df::flow_guide_item_cloudst, guide);
            if (cloud)
            {
                mat->set_mat_index(cloud->matindex);
                mat->set_mat_type(cloud->mattype);
                auto item = netFlow->mutable_item();
                item->set_mat_index(cloud->item_subtype);
                item->set_mat_type(cloud->item_type);
            }
        }
    }
}

void CopyFlows(df::map_block * DfBlock, RemoteFortressReader::MapBlock * NetBlock)
{
    NetBlock->set_map_x(DfBlock->map_pos.x);
    NetBlock->set_map_y(DfBlock->map_pos.y);
    NetBlock->set_map_z(DfBlock->map_pos.z);
    for (int i = 0; i < DfBlock->flows.size(); i++)
    {
        CopyFlow(DfBlock->flows[i], NetBlock->add_flows(), i);
    }
}

static command_result GetBlockList(color_ostream &stream, const BlockRequest *in, BlockList *out)
{
    int x, y, z;
    DFHack::Maps::getPosition(x, y, z);
    out->set_map_x(x);
    out->set_map_y(y);
    MapExtras::MapCache MC;
    int center_x = (in->min_x() + in->max_x()) / 2;
    int center_y = (in->min_y() + in->max_y()) / 2;

    int NUMBER_OF_POINTS = ((in->max_x() - center_x + 1) * 2) * ((in->max_y() - center_y + 1) * 2);
    int blocks_needed;
    if (in->has_blocks_needed())
        blocks_needed = in->blocks_needed();
    else
        blocks_needed = NUMBER_OF_POINTS * (in->max_z() - in->min_z());
    int blocks_sent = 0;
    int min_x = in->min_x();
    int min_y = in->min_y();
    int max_x = in->max_x();
    int max_y = in->max_y();
    int min_z = in->min_z();
    int max_z = in->max_z();
    bool firstBlock = true; //Always send all the buildings needed on the first block, and none on the rest.
                                //stream.print("Got request for blocks from (%d, %d, %d) to (%d, %d, %d).\n", in->min_x(), in->min_y(), in->min_z(), in->max_x(), in->max_y(), in->max_z());
    for (int zz = max_z - 1; zz >= min_z; zz--)
    {
        // (di, dj) is a vector - direction in which we move right now
        int di = 1;
        int dj = 0;
        // length of current segment
        int segment_length = 1;
        // current position (i, j) and how much of current segment we passed
        int i = center_x;
        int j = center_y;
        int segment_passed = 0;
        for (int k = 0; k < NUMBER_OF_POINTS; ++k)
        {
            if (blocks_sent >= blocks_needed)
                break;
            if (!(i < min_x || i >= max_x || j < min_y || j >= max_y))
            {
                DFCoord pos = DFCoord(i, j, zz);
                df::map_block * block = DFHack::Maps::getBlock(pos);
                if (block != NULL)
                {
                    bool nonAir = false;
                    for (int xxx = 0; xxx < 16; xxx++)
                        for (int yyy = 0; yyy < 16; yyy++)
                        {
                            if ((DFHack::tileShapeBasic(DFHack::tileShape(block->tiletype[xxx][yyy])) != df::tiletype_shape_basic::None &&
                                DFHack::tileShapeBasic(DFHack::tileShape(block->tiletype[xxx][yyy])) != df::tiletype_shape_basic::Open)
                                || block->designation[xxx][yyy].bits.flow_size > 0
                                || block->occupancy[xxx][yyy].bits.building > 0)
                            {
                                nonAir = true;
                                goto ItsAir;
                            }
                        }
                    ItsAir:
                    if (block->flows.size() > 0)
                        nonAir = true;
                    if (nonAir || firstBlock)
                    {
                        bool tileChanged = IsTiletypeChanged(pos);
                        bool desChanged = IsDesignationChanged(pos);
                        bool spatterChanged = IsspatterChanged(pos);
                        bool itemsChanged = block->items.size() > 0;
                        bool flows = block->flows.size() > 0;
                        RemoteFortressReader::MapBlock *net_block;
                        if (tileChanged || desChanged || spatterChanged || firstBlock || itemsChanged || flows)
                            net_block = out->add_map_blocks();
                        if (tileChanged)
                        {
                            CopyBlock(block, net_block, &MC, pos);
                            blocks_sent++;
                        }
                        if (desChanged)
                            CopyDesignation(block, net_block, &MC, pos);
                        if (firstBlock)
                        {
                            CopyBuildings(DFCoord(min_x * 16, min_y * 16, min_z), DFCoord(max_x * 16, max_y * 16, max_z), net_block, &MC);
                            CopyProjectiles(net_block);
                            firstBlock = false;
                        }
                        if (spatterChanged)
                            Copyspatters(block, net_block, &MC, pos);
                        if (itemsChanged)
                            CopyItems(block, net_block, &MC, pos);
                        if (flows)
                        {
                            CopyFlows(block, net_block);
                        }
                    }
                }
            }

            // make a step, add 'direction' vector (di, dj) to current position (i, j)
            i += di;
            j += dj;
            ++segment_passed;
            //System.out.println(i + " " + j);

            if (segment_passed == segment_length)
            {
                // done with current segment
                segment_passed = 0;

                // 'rotate' directions
                int buffer = di;
                di = -dj;
                dj = buffer;

                // increase segment length if necessary
                if (dj == 0) {
                    ++segment_length;
                }
            }
        }
    }

    for (int i = 0; i < world->engravings.size(); i++)
    {
        auto engraving = world->engravings[i];
        if (engraving->pos.x < (min_x * 16) || engraving->pos.x >(max_x * 16))
            continue;
        if (engraving->pos.y < (min_y * 16) || engraving->pos.y >(max_y * 16))
            continue;
        if (engraving->pos.z < min_z || engraving->pos.z > max_z)
            continue;
        if (!isEngravingNew(i))
            continue;

        df::art_image_chunk * chunk = NULL;
        GET_ART_IMAGE_CHUNK GetArtImageChunk = reinterpret_cast<GET_ART_IMAGE_CHUNK>(Core::getInstance().vinfo->getAddress("get_art_image_chunk"));
        if (GetArtImageChunk)
        {
            chunk = GetArtImageChunk(&(world->art_image_chunks), engraving->art_id);
        }
        else
        {
            for (int i = 0; i < world->art_image_chunks.size(); i++)
            {
                if (world->art_image_chunks[i]->id == engraving->art_id)
                    chunk = world->art_image_chunks[i];
            }
        }
        if (!chunk)
        {
            engravingIsNotNew(i);
            continue;
        }
        auto netEngraving = out->add_engravings();
        ConvertDFCoord(engraving->pos, netEngraving->mutable_pos());
        netEngraving->set_quality(engraving->quality);
        netEngraving->set_tile(engraving->tile);
        CopyImage(chunk->images[engraving->art_subid], netEngraving->mutable_image());
        netEngraving->set_floor(engraving->flags.bits.floor);
        netEngraving->set_west(engraving->flags.bits.west);
        netEngraving->set_east(engraving->flags.bits.east);
        netEngraving->set_north(engraving->flags.bits.north);
        netEngraving->set_south(engraving->flags.bits.south);
        netEngraving->set_hidden(engraving->flags.bits.hidden);
        netEngraving->set_northwest(engraving->flags.bits.northwest);
        netEngraving->set_northeast(engraving->flags.bits.northeast);
        netEngraving->set_southwest(engraving->flags.bits.southwest);
        netEngraving->set_southeast(engraving->flags.bits.southeast);
    }
    for (int i = 0; i < world->ocean_waves.size(); i++)
    {
        auto wave = world->ocean_waves[i];
        auto netWave = out->add_ocean_waves();
        ConvertDFCoord(wave->x1, wave->y1, wave->z, netWave->mutable_dest());
        ConvertDFCoord(wave->x2, wave->y2, wave->z, netWave->mutable_pos());
    }
    MC.trash();
    return CR_OK;
}

static command_result GetTiletypeList(color_ostream &stream, const EmptyMessage *in, TiletypeList *out)
{
    int count = 0;
    FOR_ENUM_ITEMS(tiletype, tt)
    {
        Tiletype * type = out->add_tiletype_list();
        type->set_id(tt);
        type->set_name(ENUM_KEY_STR(tiletype, tt));
        const char * name = tileName(tt);
        if (name != NULL && name[0] != 0)
            type->set_caption(name);
        type->set_shape(TranslateShape(tileShape(tt)));
        type->set_special(TranslateSpecial(tileSpecial(tt)));
        type->set_material(TranslateMaterial(tileMaterial(tt)));
        type->set_variant(TranslateVariant(tileVariant(tt)));
        type->set_direction(tileDirection(tt).getStr());
        count++;
    }
    return CR_OK;
}

static command_result GetPlantList(color_ostream &stream, const BlockRequest *in, PlantList *out)
{
    int min_x = in->min_x() / 3;
    int min_y = in->min_y() / 3;
    int min_z = in->min_z();
    int max_x = in->max_x() / 3;
    int max_y = in->max_y() / 3;
    int max_z = in->max_z();

#if DF_VERSION_INT < 40001
    //plants are gotten differently here
#else
    for (int xx = min_x; xx < max_x; xx++)
        for (int yy = min_y; yy < max_y; yy++)
        {
            if (xx < 0 || yy < 0 || xx >= world->map.x_count_block || yy >= world->map.y_count_block)
                continue;
            df::map_block_column * column = world->map.column_index[xx][yy];
            for (int i = 0; i < column->plants.size(); i++)
            {
                df::plant * plant = column->plants[i];
                if (!plant->tree_info)
                {
                    if (plant->pos.z < min_z || plant->pos.z >= max_z)
                        continue;
                    if (plant->pos.x < in->min_x() * 16 || plant->pos.x >= in->max_x() * 16)
                        continue;
                    if (plant->pos.y < in->min_y() * 16 || plant->pos.y >= in->max_y() * 16)
                        continue;
                }
                else
                {
                    if (plant->pos.z - plant->tree_info->roots_depth < min_z || plant->pos.z + plant->tree_info->body_height > max_z)
                        continue;
                    if (plant->pos.x - plant->tree_info->dim_x / 2 < in->min_x() * 16 || plant->pos.x + plant->tree_info->dim_x / 2 >= in->max_x() * 16)
                        continue;
                    if (plant->pos.y - plant->tree_info->dim_y / 2 < in->min_y() * 16 || plant->pos.y + plant->tree_info->dim_y / 2 >= in->max_y() * 16)
                        continue;
                }
                RemoteFortressReader::PlantDef * out_plant = out->add_plant_list();
                out_plant->set_index(plant->material);
                out_plant->set_pos_x(plant->pos.x);
                out_plant->set_pos_y(plant->pos.y);
                out_plant->set_pos_z(plant->pos.z);
            }
        }
#endif
    return CR_OK;
}

static command_result GetUnitList(color_ostream &stream, const EmptyMessage *in, UnitList *out)
{
    return GetUnitListInside(stream, NULL, out);
}

static command_result GetUnitListInside(color_ostream &stream, const BlockRequest *in, UnitList *out)
{
    auto world = df::global::world;
    for (int i = 0; i < world->units.all.size(); i++)
    {
        df::unit * unit = world->units.all[i];
        auto send_unit = out->add_creature_list();
        send_unit->set_id(unit->id);
        send_unit->set_pos_x(unit->pos.x);
        send_unit->set_pos_y(unit->pos.y);
        send_unit->set_pos_z(unit->pos.z);
        send_unit->mutable_race()->set_mat_type(unit->race);
        send_unit->mutable_race()->set_mat_index(unit->caste);
        if (in != NULL)
        {
            if (unit->pos.z < in->min_z() || unit->pos.z >= in->max_z())
                continue;
            if (unit->pos.x < in->min_x() * 16 || unit->pos.x >= in->max_x() * 16)
                continue;
            if (unit->pos.y < in->min_y() * 16 || unit->pos.y >= in->max_y() * 16)
                continue;
        }
        ConvertDfColor(Units::getProfessionColor(unit), send_unit->mutable_profession_color());
        send_unit->set_flags1(unit->flags1.whole);
        send_unit->set_flags2(unit->flags2.whole);
        send_unit->set_flags3(unit->flags3.whole);
        send_unit->set_is_soldier(ENUM_ATTR(profession, military, unit->profession));
        auto size_info = send_unit->mutable_size_info();
        size_info->set_size_cur(unit->body.size_info.size_cur);
        size_info->set_size_base(unit->body.size_info.size_base);
        size_info->set_area_cur(unit->body.size_info.area_cur);
        size_info->set_area_base(unit->body.size_info.area_base);
        size_info->set_length_cur(unit->body.size_info.length_cur);
        size_info->set_length_base(unit->body.size_info.length_base);
        if (unit->name.has_name)
        {
            send_unit->set_name(DF2UTF(Translation::TranslateName(Units::getVisibleName(unit))));
        }

        auto appearance = send_unit->mutable_appearance();
        for (int j = 0; j < unit->appearance.body_modifiers.size(); j++)
            appearance->add_body_modifiers(unit->appearance.body_modifiers[j]);
        for (int j = 0; j < unit->appearance.bp_modifiers.size(); j++)
            appearance->add_bp_modifiers(unit->appearance.bp_modifiers[j]);
        for (int j = 0; j < unit->appearance.colors.size(); j++)
            appearance->add_colors(unit->appearance.colors[j]);
        appearance->set_size_modifier(unit->appearance.size_modifier);

        send_unit->set_profession_id(unit->profession);

        std::vector<Units::NoblePosition> pvec;

        if (Units::getNoblePositions(&pvec, unit))
        {
            for (int j = 0; j < pvec.size(); j++)
            {
                auto noble_positon = pvec[j];
                send_unit->add_noble_positions(noble_positon.position->code);
            }
        }

        send_unit->set_rider_id(unit->relationship_ids[df::unit_relationship_type::RiderMount]);

        auto creatureRaw = world->raws.creatures.all[unit->race];
        auto casteRaw = creatureRaw->caste[unit->caste];

        for (int j = 0; j < unit->appearance.tissue_style_type.size(); j++)
        {
            auto type = unit->appearance.tissue_style_type[j];
            if (type < 0)
                continue;
            int style_raw_index = binsearch_index(casteRaw->tissue_styles, &df::tissue_style_raw::id, type);
            auto styleRaw = casteRaw->tissue_styles[style_raw_index];
            if (styleRaw->token == "HAIR")
            {
                auto send_style = appearance->mutable_hair();
                send_style->set_length(unit->appearance.tissue_length[j]);
                send_style->set_style((HairStyle)unit->appearance.tissue_style[j]);
            }
            else if (styleRaw->token == "BEARD")
            {
                auto send_style = appearance->mutable_beard();
                send_style->set_length(unit->appearance.tissue_length[j]);
                send_style->set_style((HairStyle)unit->appearance.tissue_style[j]);
            }
            else if (styleRaw->token == "MOUSTACHE")
            {
                auto send_style = appearance->mutable_moustache();
                send_style->set_length(unit->appearance.tissue_length[j]);
                send_style->set_style((HairStyle)unit->appearance.tissue_style[j]);
            }
            else if (styleRaw->token == "SIDEBURNS")
            {
                auto send_style = appearance->mutable_sideburns();
                send_style->set_length(unit->appearance.tissue_length[j]);
                send_style->set_style((HairStyle)unit->appearance.tissue_style[j]);
            }
        }

        for (int j = 0; j < unit->inventory.size(); j++)
        {
            auto inventory_item = unit->inventory[j];
            auto sent_item = send_unit->add_inventory();
            sent_item->set_mode((InventoryMode)inventory_item->mode);
            CopyItem(sent_item->mutable_item(), inventory_item->item);
        }

        if (unit->flags1.bits.projectile)
        {
            for (auto proj = world->proj_list.next; proj != NULL; proj = proj->next)
            {
                STRICT_VIRTUAL_CAST_VAR(item, df::proj_unitst, proj->item);
                if (item == NULL)
                    continue;
                if (item->unit != unit)
                    continue;
                send_unit->set_subpos_x(item->pos_x / 100000.0);
                send_unit->set_subpos_y(item->pos_y / 100000.0);
                send_unit->set_subpos_z(item->pos_z / 140000.0);
            }
        }
    }
    return CR_OK;
}

static command_result GetViewInfo(color_ostream &stream, const EmptyMessage *in, ViewInfo *out)
{
    int x, y, z, w, h, cx, cy, cz;
    Gui::getWindowSize(w, h);
    Gui::getViewCoords(x, y, z);
    Gui::getCursorCoords(cx, cy, cz);

#if DF_VERSION_INT > 34011
    auto embark = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    if (embark)
    {
        df::embark_location location = embark->location;
        df::world_data * data = df::global::world->world_data;
        if (data && data->region_map)
        {
            z = data->region_map[location.region_pos.x][location.region_pos.y].elevation;
        }
    }
#endif

    out->set_view_pos_x(x);
    out->set_view_pos_y(y);
    out->set_view_pos_z(z);
    out->set_view_size_x(w);
    out->set_view_size_y(h);
    out->set_cursor_pos_x(cx);
    out->set_cursor_pos_y(cy);
    out->set_cursor_pos_z(cz);

    if (gamemode && *gamemode == GameMode::ADVENTURE)
        out->set_follow_unit_id(world->units.active[0]->id);
    else
        out->set_follow_unit_id(ui->follow_unit);
    out->set_follow_item_id(ui->follow_item);
    return CR_OK;
}

static command_result GetMapInfo(color_ostream &stream, const EmptyMessage *in, MapInfo *out)
{
    if (!Maps::IsValid())
        return CR_FAILURE;
    uint32_t size_x, size_y, size_z;
    int32_t pos_x, pos_y, pos_z;
    Maps::getSize(size_x, size_y, size_z);
    Maps::getPosition(pos_x, pos_y, pos_z);
    out->set_block_size_x(size_x);
    out->set_block_size_y(size_y);
    out->set_block_size_z(size_z);
    out->set_block_pos_x(pos_x);
    out->set_block_pos_y(pos_y);
    out->set_block_pos_z(pos_z);
    out->set_world_name(DF2UTF(Translation::TranslateName(&df::global::world->world_data->name, false)));
    out->set_world_name_english(DF2UTF(Translation::TranslateName(&df::global::world->world_data->name, true)));
    out->set_save_name(df::global::world->cur_savegame.save_dir);
    return CR_OK;
}

DFCoord GetMapCenter()
{
    DFCoord output;
#if DF_VERSION_INT > 34011
    auto embark = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    if (embark)
    {
        df::embark_location location = embark->location;
        output.x = (location.region_pos.x * 16) + 8;
        output.y = (location.region_pos.y * 16) + 8;
        output.z = 100;
        df::world_data * data = df::global::world->world_data;
        if (data && data->region_map)
        {
            output.z = data->region_map[location.region_pos.x][location.region_pos.y].elevation;
        }
    }
    else
#endif
        if (Maps::IsValid())
        {
            int x, y, z;
            Maps::getPosition(x, y, z);
            output = DFCoord(x, y, z);
        }
#if DF_VERSION_INT > 34011
        else
            for (int i = 0; i < df::global::world->armies.all.size(); i++)
            {
                df::army * thisArmy = df::global::world->armies.all[i];
                if (thisArmy->flags.is_set(df::enums::army_flags::player))
                {
                    output.x = (thisArmy->pos.x / 3) - 1;
                    output.y = (thisArmy->pos.y / 3) - 1;
                    output.z = thisArmy->pos.z;
                }
            }
#endif
    return output;
}

static command_result GetWorldMapCenter(color_ostream &stream, const EmptyMessage *in, WorldMap *out)
{
    if (!df::global::world->world_data)
    {
        out->set_world_width(0);
        out->set_world_height(0);
        return CR_FAILURE;
    }
    df::world_data * data = df::global::world->world_data;
    int width = data->world_width;
    int height = data->world_height;
    out->set_world_width(width);
    out->set_world_height(height);
    DFCoord pos = GetMapCenter();
    out->set_center_x(pos.x);
    out->set_center_y(pos.y);
    out->set_center_z(pos.z);
    out->set_name(Translation::TranslateName(&(data->name), false));
    out->set_name_english(Translation::TranslateName(&(data->name), true));
    out->set_cur_year(World::ReadCurrentYear());
    out->set_cur_year_tick(World::ReadCurrentTick());
    return CR_OK;
}

static command_result GetWorldMap(color_ostream &stream, const EmptyMessage *in, WorldMap *out)
{
    if (!df::global::world->world_data)
    {
        out->set_world_width(0);
        out->set_world_height(0);
        return CR_FAILURE;
    }
    df::world_data * data = df::global::world->world_data;
    if (!data->region_map)
    {
        out->set_world_width(0);
        out->set_world_height(0);
        return CR_FAILURE;
    }
    int width = data->world_width;
    int height = data->world_height;
    out->set_world_width(width);
    out->set_world_height(height);
    out->set_name(Translation::TranslateName(&(data->name), false));
    out->set_name_english(Translation::TranslateName(&(data->name), true));
    auto poles = data->flip_latitude;
#if DF_VERSION_INT > 34011
    switch (poles)
    {
    case df::world_data::None:
        out->set_world_poles(WorldPoles::NO_POLES);
        break;
    case df::world_data::North:
        out->set_world_poles(WorldPoles::NORTH_POLE);
        break;
    case df::world_data::South:
        out->set_world_poles(WorldPoles::SOUTH_POLE);
        break;
    case df::world_data::Both:
        out->set_world_poles(WorldPoles::BOTH_POLES);
        break;
    default:
        break;
    }
#else
    out->set_world_poles(WorldPoles::NO_POLES);
#endif
    for (int yy = 0; yy < height; yy++)
        for (int xx = 0; xx < width; xx++)
        {
            df::region_map_entry * map_entry = &data->region_map[xx][yy];
            df::world_region * region = data->regions[map_entry->region_id];
            out->add_elevation(map_entry->elevation);
            out->add_rainfall(map_entry->rainfall);
            out->add_vegetation(map_entry->vegetation);
            out->add_temperature(map_entry->temperature);
            out->add_evilness(map_entry->evilness);
            out->add_drainage(map_entry->drainage);
            out->add_volcanism(map_entry->volcanism);
            out->add_savagery(map_entry->savagery);
            out->add_salinity(map_entry->salinity);
            auto clouds = out->add_clouds();
#if DF_VERSION_INT > 34011
            clouds->set_cirrus(map_entry->clouds.bits.cirrus);
            clouds->set_cumulus((RemoteFortressReader::CumulusType)map_entry->clouds.bits.cumulus);
            clouds->set_fog((RemoteFortressReader::FogType)map_entry->clouds.bits.fog);
            clouds->set_front((RemoteFortressReader::FrontType)map_entry->clouds.bits.front);
            clouds->set_stratus((RemoteFortressReader::StratusType)map_entry->clouds.bits.stratus);
#else
            clouds->set_cirrus(map_entry->clouds.bits.striped);
            clouds->set_cumulus((RemoteFortressReader::CumulusType)map_entry->clouds.bits.density);
            clouds->set_fog((RemoteFortressReader::FogType)map_entry->clouds.bits.fog);
            clouds->set_stratus((RemoteFortressReader::StratusType)map_entry->clouds.bits.darkness);
#endif
            if (region->type == world_region_type::Lake)
            {
                out->add_water_elevation(region->lake_surface);
            }
            else
                out->add_water_elevation(99);
        }
    DFCoord pos = GetMapCenter();
    out->set_center_x(pos.x);
    out->set_center_y(pos.y);
    out->set_center_z(pos.z);


    out->set_cur_year(World::ReadCurrentYear());
    out->set_cur_year_tick(World::ReadCurrentTick());
    return CR_OK;
}

static void SetRegionTile(RegionTile * out, df::region_map_entry * e1)
{
    df::world_region * region = df::world_region::find(e1->region_id);
    df::world_geo_biome * geoBiome = df::world_geo_biome::find(e1->geo_index);
    out->set_rainfall(e1->rainfall);
    out->set_vegetation(e1->vegetation);
    out->set_temperature(e1->temperature);
    out->set_evilness(e1->evilness);
    out->set_drainage(e1->drainage);
    out->set_volcanism(e1->volcanism);
    out->set_savagery(e1->savagery);
    out->set_salinity(e1->salinity);
    if (region->type == world_region_type::Lake)
        out->set_water_elevation(region->lake_surface);
    else
        out->set_water_elevation(99);

    int topLayer = 0;
    for (int i = 0; i < geoBiome->layers.size(); i++)
    {
        auto layer = geoBiome->layers[i];
        if (layer->top_height == 0)
        {
            topLayer = layer->mat_index;
        }
        if (layer->type != geo_layer_type::SOIL
            && layer->type != geo_layer_type::SOIL_OCEAN
            && layer->type != geo_layer_type::SOIL_SAND)
        {
            auto mat = out->add_stone_materials();
            mat->set_mat_index(layer->mat_index);
            mat->set_mat_type(0);
        }
    }
    auto surfaceMat = out->mutable_surface_material();
    surfaceMat->set_mat_index(topLayer);
    surfaceMat->set_mat_type(0);

    for (int i = 0; i < region->population.size(); i++)
    {
        auto pop = region->population[i];
        if (pop->type == world_population_type::Grass)
        {
            auto plantMat = out->add_plant_materials();

            plantMat->set_mat_index(pop->plant);
            plantMat->set_mat_type(419);
        }
        else if (pop->type == world_population_type::Tree)
        {
            auto plantMat = out->add_tree_materials();

            plantMat->set_mat_index(pop->plant);
            plantMat->set_mat_type(419);
        }
    }
#if DF_VERSION_INT >= 43005
    out->set_snow(e1->snowfall);
#endif
}

static command_result GetWorldMapNew(color_ostream &stream, const EmptyMessage *in, WorldMap *out)
{
    if (!df::global::world->world_data)
    {
        out->set_world_width(0);
        out->set_world_height(0);
        return CR_FAILURE;
    }
    df::world_data * data = df::global::world->world_data;
    if (!data->region_map)
    {
        out->set_world_width(0);
        out->set_world_height(0);
        return CR_FAILURE;
    }
    int width = data->world_width;
    int height = data->world_height;
    out->set_world_width(width);
    out->set_world_height(height);
    out->set_name(Translation::TranslateName(&(data->name), false));
    out->set_name_english(Translation::TranslateName(&(data->name), true));
#if DF_VERSION_INT > 34011
    auto poles = data->flip_latitude;
    switch (poles)
    {
    case df::world_data::None:
        out->set_world_poles(WorldPoles::NO_POLES);
        break;
    case df::world_data::North:
        out->set_world_poles(WorldPoles::NORTH_POLE);
        break;
    case df::world_data::South:
        out->set_world_poles(WorldPoles::SOUTH_POLE);
        break;
    case df::world_data::Both:
        out->set_world_poles(WorldPoles::BOTH_POLES);
        break;
    default:
        break;
    }
#else
    out->set_world_poles(WorldPoles::NO_POLES);
#endif
    for (int yy = 0; yy < height; yy++)
        for (int xx = 0; xx < width; xx++)
        {
            df::region_map_entry * map_entry = &data->region_map[xx][yy];
            df::world_region * region = data->regions[map_entry->region_id];

            auto regionTile = out->add_region_tiles();
            regionTile->set_elevation(map_entry->elevation);
            SetRegionTile(regionTile, map_entry);
            auto clouds = out->add_clouds();
#if DF_VERSION_INT > 34011
            clouds->set_cirrus(map_entry->clouds.bits.cirrus);
            clouds->set_cumulus((RemoteFortressReader::CumulusType)map_entry->clouds.bits.cumulus);
            clouds->set_fog((RemoteFortressReader::FogType)map_entry->clouds.bits.fog);
            clouds->set_front((RemoteFortressReader::FrontType)map_entry->clouds.bits.front);
            clouds->set_stratus((RemoteFortressReader::StratusType)map_entry->clouds.bits.stratus);
#else
            clouds->set_cirrus(map_entry->clouds.bits.striped);
            clouds->set_cumulus((RemoteFortressReader::CumulusType)map_entry->clouds.bits.density);
            clouds->set_fog((RemoteFortressReader::FogType)map_entry->clouds.bits.fog);
            clouds->set_stratus((RemoteFortressReader::StratusType)map_entry->clouds.bits.darkness);
#endif
        }
    DFCoord pos = GetMapCenter();
    out->set_center_x(pos.x);
    out->set_center_y(pos.y);
    out->set_center_z(pos.z);


    out->set_cur_year(World::ReadCurrentYear());
    out->set_cur_year_tick(World::ReadCurrentTick());
    return CR_OK;
}

static void AddRegionTiles(WorldMap * out, df::region_map_entry * e1, df::world_data * worldData)
{
    df::world_region * region = worldData->regions[e1->region_id];
    out->add_rainfall(e1->rainfall);
    out->add_vegetation(e1->vegetation);
    out->add_temperature(e1->temperature);
    out->add_evilness(e1->evilness);
    out->add_drainage(e1->drainage);
    out->add_volcanism(e1->volcanism);
    out->add_savagery(e1->savagery);
    out->add_salinity(e1->salinity);
    if (region->type == world_region_type::Lake)
        out->add_water_elevation(region->lake_surface);
    else
        out->add_water_elevation(99);
}


static void AddRegionTiles(WorldMap * out, df::coord2d pos, df::world_data * worldData)
{
    if (pos.x < 0)
        pos.x = 0;
    if (pos.y < 0)
        pos.y = 0;
    if (pos.x >= worldData->world_width)
        pos.x = worldData->world_width - 1;
    if (pos.y >= worldData->world_height)
        pos.y = worldData->world_height - 1;
    AddRegionTiles(out, &worldData->region_map[pos.x][pos.y], worldData);
}

static void AddRegionTiles(RegionTile * out, df::coord2d pos, df::world_data * worldData)
{
    if (pos.x < 0)
        pos.x = 0;
    if (pos.y < 0)
        pos.y = 0;
    if (pos.x >= worldData->world_width)
        pos.x = worldData->world_width - 1;
    if (pos.y >= worldData->world_height)
        pos.y = worldData->world_height - 1;
    SetRegionTile(out, &worldData->region_map[pos.x][pos.y]);
}

static df::coord2d ShiftCoords(df::coord2d source, int direction)
{
    switch (direction)
    {
    case 1:
        return df::coord2d(source.x - 1, source.y + 1);
    case 2:
        return df::coord2d(source.x, source.y + 1);
    case 3:
        return df::coord2d(source.x + 1, source.y + 1);
    case 4:
        return df::coord2d(source.x - 1, source.y);
    case 5:
        return source;
    case 6:
        return df::coord2d(source.x + 1, source.y);
    case 7:
        return df::coord2d(source.x - 1, source.y - 1);
    case 8:
        return df::coord2d(source.x, source.y - 1);
    case 9:
        return df::coord2d(source.x + 1, source.y - 1);
    default:
        return source;
    }
}

static void CopyLocalMap(df::world_data * worldData, df::world_region_details* worldRegionDetails, WorldMap * out)
{
    int pos_x = worldRegionDetails->pos.x;
    int pos_y = worldRegionDetails->pos.y;
    out->set_map_x(pos_x);
    out->set_map_y(pos_y);
    out->set_world_width(17);
    out->set_world_height(17);
    char name[256];
    sprintf(name, "Region %d, %d", pos_x, pos_y);
    out->set_name_english(name);
    out->set_name(name);
#if DF_VERSION_INT > 34011
    auto poles = worldData->flip_latitude;
    switch (poles)
    {
    case df::world_data::None:
        out->set_world_poles(WorldPoles::NO_POLES);
        break;
    case df::world_data::North:
        out->set_world_poles(WorldPoles::NORTH_POLE);
        break;
    case df::world_data::South:
        out->set_world_poles(WorldPoles::SOUTH_POLE);
        break;
    case df::world_data::Both:
        out->set_world_poles(WorldPoles::BOTH_POLES);
        break;
    default:
        break;
    }
#else
    out->set_world_poles(WorldPoles::NO_POLES);
#endif

    df::world_region_details * south = NULL;
    df::world_region_details * east = NULL;
    df::world_region_details * southEast = NULL;

    for (int i = 0; i < worldData->region_details.size(); i++)
    {
        auto region = worldData->region_details[i];
        if (region->pos.x == pos_x + 1 && region->pos.y == pos_y + 1)
            southEast = region;
        else if (region->pos.x == pos_x + 1 && region->pos.y == pos_y)
            east = region;
        else if (region->pos.x == pos_x && region->pos.y == pos_y + 1)
            south = region;
    }

    for (int yy = 0; yy < 17; yy++)
        for (int xx = 0; xx < 17; xx++)
        {
            //This is because the bottom row doesn't line up.
            if (xx == 16 && yy == 16 && southEast != NULL)
            {
                out->add_elevation(southEast->elevation[0][0]);
                AddRegionTiles(out, ShiftCoords(df::coord2d(pos_x + 1, pos_y + 1), (southEast->biome[0][0])), worldData);
            }
            else if (xx == 16 && east != NULL)
            {
                out->add_elevation(east->elevation[0][yy]);
                AddRegionTiles(out, ShiftCoords(df::coord2d(pos_x + 1, pos_y), (east->biome[0][yy])), worldData);
            }
            else if (yy == 16 && south != NULL)
            {
                out->add_elevation(south->elevation[xx][0]);
                AddRegionTiles(out, ShiftCoords(df::coord2d(pos_x, pos_y + 1), (south->biome[xx][0])), worldData);
            }
            else
            {
                out->add_elevation(worldRegionDetails->elevation[xx][yy]);
                AddRegionTiles(out, ShiftCoords(df::coord2d(pos_x, pos_y), (worldRegionDetails->biome[xx][yy])), worldData);
            }

            if (xx == 16 || yy == 16)
            {
                out->add_river_tiles();
            }
            else
            {
                auto riverTile = out->add_river_tiles();
                auto east = riverTile->mutable_east();
                auto north = riverTile->mutable_north();
                auto south = riverTile->mutable_south();
                auto west = riverTile->mutable_west();

                north->set_active(worldRegionDetails->rivers_vertical.active[xx][yy]);
                north->set_elevation(worldRegionDetails->rivers_vertical.elevation[xx][yy]);
                north->set_min_pos(worldRegionDetails->rivers_vertical.x_min[xx][yy]);
                north->set_max_pos(worldRegionDetails->rivers_vertical.x_max[xx][yy]);

                south->set_active(worldRegionDetails->rivers_vertical.active[xx][yy + 1]);
                south->set_elevation(worldRegionDetails->rivers_vertical.elevation[xx][yy + 1]);
                south->set_min_pos(worldRegionDetails->rivers_vertical.x_min[xx][yy + 1]);
                south->set_max_pos(worldRegionDetails->rivers_vertical.x_max[xx][yy + 1]);

                west->set_active(worldRegionDetails->rivers_horizontal.active[xx][yy]);
                west->set_elevation(worldRegionDetails->rivers_horizontal.elevation[xx][yy]);
                west->set_min_pos(worldRegionDetails->rivers_horizontal.y_min[xx][yy]);
                west->set_max_pos(worldRegionDetails->rivers_horizontal.y_max[xx][yy]);

                east->set_active(worldRegionDetails->rivers_horizontal.active[xx + 1][yy]);
                east->set_elevation(worldRegionDetails->rivers_horizontal.elevation[xx + 1][yy]);
                east->set_min_pos(worldRegionDetails->rivers_horizontal.y_min[xx + 1][yy]);
                east->set_max_pos(worldRegionDetails->rivers_horizontal.y_max[xx + 1][yy]);
            }
        }
}

static void CopyLocalMap(df::world_data * worldData, df::world_region_details* worldRegionDetails, RegionMap * out)
{
    int pos_x = worldRegionDetails->pos.x;
    int pos_y = worldRegionDetails->pos.y;
    out->set_map_x(pos_x);
    out->set_map_y(pos_y);
    char name[256];
    sprintf(name, "Region %d, %d", pos_x, pos_y);
    out->set_name_english(name);
    out->set_name(name);


    df::world_region_details * south = NULL;
    df::world_region_details * east = NULL;
    df::world_region_details * southEast = NULL;

    for (int i = 0; i < worldData->region_details.size(); i++)
    {
        auto region = worldData->region_details[i];
        if (region->pos.x == pos_x + 1 && region->pos.y == pos_y + 1)
            southEast = region;
        else if (region->pos.x == pos_x + 1 && region->pos.y == pos_y)
            east = region;
        else if (region->pos.x == pos_x && region->pos.y == pos_y + 1)
            south = region;
    }

    RegionTile* outputTiles[17][17];

    for (int yy = 0; yy < 17; yy++)
        for (int xx = 0; xx < 17; xx++)
        {
            auto tile = out->add_tiles();
            outputTiles[xx][yy] = tile;
            //This is because the bottom row doesn't line up.
            if (xx == 16 && yy == 16 && southEast != NULL)
            {
                tile->set_elevation(southEast->elevation[0][0]);
                AddRegionTiles(tile, ShiftCoords(df::coord2d(pos_x + 1, pos_y + 1), (southEast->biome[0][0])), worldData);
            }
            else if (xx == 16 && east != NULL)
            {
                tile->set_elevation(east->elevation[0][yy]);
                AddRegionTiles(tile, ShiftCoords(df::coord2d(pos_x + 1, pos_y), (east->biome[0][yy])), worldData);
            }
            else if (yy == 16 && south != NULL)
            {
                tile->set_elevation(south->elevation[xx][0]);
                AddRegionTiles(tile, ShiftCoords(df::coord2d(pos_x, pos_y + 1), (south->biome[xx][0])), worldData);
            }
            else
            {
                tile->set_elevation(worldRegionDetails->elevation[xx][yy]);
                AddRegionTiles(tile, ShiftCoords(df::coord2d(pos_x, pos_y), (worldRegionDetails->biome[xx][yy])), worldData);
            }

            auto riverTile = tile->mutable_river_tiles();
            auto east = riverTile->mutable_east();
            auto north = riverTile->mutable_north();
            auto south = riverTile->mutable_south();
            auto west = riverTile->mutable_west();

            if (xx < 16)
            {
                north->set_active(worldRegionDetails->rivers_vertical.active[xx][yy]);
                north->set_elevation(worldRegionDetails->rivers_vertical.elevation[xx][yy]);
                north->set_min_pos(worldRegionDetails->rivers_vertical.x_min[xx][yy]);
                north->set_max_pos(worldRegionDetails->rivers_vertical.x_max[xx][yy]);
            }
            else
            {
                north->set_active(0);
                north->set_elevation(100);
                north->set_min_pos(-30000);
                north->set_max_pos(-30000);
            }

            if (yy < 16 && xx < 16)
            {
                south->set_active(worldRegionDetails->rivers_vertical.active[xx][yy + 1]);
                south->set_elevation(worldRegionDetails->rivers_vertical.elevation[xx][yy + 1]);
                south->set_min_pos(worldRegionDetails->rivers_vertical.x_min[xx][yy + 1]);
                south->set_max_pos(worldRegionDetails->rivers_vertical.x_max[xx][yy + 1]);
            }
            else
            {
                south->set_active(0);
                south->set_elevation(100);
                south->set_min_pos(-30000);
                south->set_max_pos(-30000);
            }

            if (yy < 16)
            {
                west->set_active(worldRegionDetails->rivers_horizontal.active[xx][yy]);
                west->set_elevation(worldRegionDetails->rivers_horizontal.elevation[xx][yy]);
                west->set_min_pos(worldRegionDetails->rivers_horizontal.y_min[xx][yy]);
                west->set_max_pos(worldRegionDetails->rivers_horizontal.y_max[xx][yy]);
            }
            else
            {
                west->set_active(0);
                west->set_elevation(100);
                west->set_min_pos(-30000);
                west->set_max_pos(-30000);
            }

            if (xx < 16 && yy < 16)
            {
                east->set_active(worldRegionDetails->rivers_horizontal.active[xx + 1][yy]);
                east->set_elevation(worldRegionDetails->rivers_horizontal.elevation[xx + 1][yy]);
                east->set_min_pos(worldRegionDetails->rivers_horizontal.y_min[xx + 1][yy]);
                east->set_max_pos(worldRegionDetails->rivers_horizontal.y_max[xx + 1][yy]);
            }
            else
            {
                east->set_active(0);
                east->set_elevation(100);
                east->set_min_pos(-30000);
                east->set_max_pos(-30000);
            }
        }

    auto regionMap = worldData->region_map[pos_x][pos_y];

    for (int i = 0; i < worldData->sites.size(); i++)
    {
        df::world_site* site = worldData->sites[i];
        if (!site)
            continue;

        int region_min_x = pos_x * 16;
        int region_min_y = pos_y * 16;

        if ((site->global_min_x > (region_min_x + 16)) ||
            (site->global_min_y > (region_min_y + 16)) ||
            (site->global_max_x < (region_min_x)) ||
            (site->global_max_y < (region_min_y)))
            continue;

        if (site->realization == NULL)
            continue;

        auto realization = site->realization;
        for (int site_x = 0; site_x < 17; site_x++)
            for (int site_y = 0; site_y < 17; site_y++)
            {
                int region_x = site->global_min_x - region_min_x + site_x;
                int region_y = site->global_min_y - region_min_y + site_y;

                if (region_x < 0 || region_y < 0 || region_x >= 16 || region_y >= 16)
                    continue;

                for (int j = 0; j < realization->building_map[site_x][site_y].buildings.size(); j++)
                {
                    auto in_building = realization->building_map[site_x][site_y].buildings[j];
                    auto out_building = outputTiles[region_x][region_y]->add_buildings();

                    out_building->set_id(in_building->id);
#if DF_VERSION_INT > 34011
                    out_building->set_type((SiteRealizationBuildingType)in_building->type);
#endif
                    out_building->set_min_x(in_building->min_x - (site_x * 48));
                    out_building->set_min_y(in_building->min_y - (site_y * 48));
                    out_building->set_max_x(in_building->max_x - (site_x * 48));
                    out_building->set_max_y(in_building->max_y - (site_y * 48));

                    CopyMat(out_building->mutable_material(), in_building->item.mat_type, in_building->item.mat_index);

#if DF_VERSION_INT >= 43005
                    STRICT_VIRTUAL_CAST_VAR(tower_info, df::site_realization_building_info_castle_towerst, in_building->building_info);
                    if (tower_info)
                    {
                        CopyMat(out_building->mutable_material(), tower_info->wall_item.mat_type, tower_info->wall_item.mat_index);

                        auto out_tower = out_building->mutable_tower_info();
                        out_tower->set_roof_z(tower_info->roof_z);
                        out_tower->set_round(tower_info->shape.bits.round);
                        out_tower->set_goblin(tower_info->shape.bits.goblin);
                    }
                    STRICT_VIRTUAL_CAST_VAR(wall_info, df::site_realization_building_info_castle_wallst, in_building->building_info);
                    if (wall_info)
                    {
                        CopyMat(out_building->mutable_material(), wall_info->wall_item.mat_type, wall_info->wall_item.mat_index);

                        auto out_wall = out_building->mutable_wall_info();

                        out_wall->set_start_x(wall_info->start_x - (site_x * 48));
                        out_wall->set_start_y(wall_info->start_y - (site_y * 48));
                        out_wall->set_start_z(wall_info->start_z);
                        out_wall->set_end_x(wall_info->end_x - (site_x * 48));
                        out_wall->set_end_y(wall_info->end_y - (site_y * 48));
                        out_wall->set_end_z(wall_info->end_z);
                    }
#endif
                }

            }
    }
}

static command_result GetRegionMaps(color_ostream &stream, const EmptyMessage *in, RegionMaps *out)
{
    if (!df::global::world->world_data)
    {
        return CR_FAILURE;
    }
    df::world_data * data = df::global::world->world_data;
    for (int i = 0; i < data->region_details.size(); i++)
    {
        df::world_region_details * region = data->region_details[i];
        if (!region)
            continue;
        WorldMap * regionMap = out->add_world_maps();
        CopyLocalMap(data, region, regionMap);
    }
    return CR_OK;
}

static command_result GetRegionMapsNew(color_ostream &stream, const EmptyMessage *in, RegionMaps *out)
{
    if (!df::global::world->world_data)
    {
        return CR_FAILURE;
    }
    df::world_data * data = df::global::world->world_data;
    for (int i = 0; i < data->region_details.size(); i++)
    {
        df::world_region_details * region = data->region_details[i];
        if (!region)
            continue;
        RegionMap * regionMap = out->add_region_maps();
        CopyLocalMap(data, region, regionMap);
    }
    return CR_OK;
}

static command_result GetCreatureRaws(color_ostream &stream, const EmptyMessage *in, CreatureRawList *out)
{
    return GetPartialCreatureRaws(stream, NULL, out);
}

static command_result GetPartialCreatureRaws(color_ostream &stream, const ListRequest *in, CreatureRawList *out)
{
    if (!df::global::world)
        return CR_FAILURE;

    df::world * world = df::global::world;

    int list_start = 0;
    int list_end = world->raws.creatures.all.size();

    if (in != nullptr)
    {
        list_start = in->list_start();
        if (in->list_end() < list_end)
            list_end = in->list_end();
    }

    for (int i = list_start; i < list_end; i++)
    {
        df::creature_raw * orig_creature = world->raws.creatures.all[i];

        auto send_creature = out->add_creature_raws();

        send_creature->set_index(i);
        send_creature->set_creature_id(orig_creature->creature_id);
        send_creature->add_name(orig_creature->name[0]);
        send_creature->add_name(orig_creature->name[1]);
        send_creature->add_name(orig_creature->name[2]);

        send_creature->add_general_baby_name(orig_creature->general_baby_name[0]);
        send_creature->add_general_baby_name(orig_creature->general_baby_name[1]);

        send_creature->add_general_child_name(orig_creature->general_child_name[0]);
        send_creature->add_general_child_name(orig_creature->general_child_name[1]);

        send_creature->set_creature_tile(orig_creature->creature_tile);
        send_creature->set_creature_soldier_tile(orig_creature->creature_soldier_tile);

        ConvertDfColor(orig_creature->color, send_creature->mutable_color());

        send_creature->set_adultsize(orig_creature->adultsize);

        for (int j = 0; j < orig_creature->caste.size(); j++)
        {
            auto orig_caste = orig_creature->caste[j];
            if (!orig_caste)
                continue;
            auto send_caste = send_creature->add_caste();

            send_caste->set_index(j);

            send_caste->set_caste_id(orig_caste->caste_id);

            send_caste->add_caste_name(orig_caste->caste_name[0]);
            send_caste->add_caste_name(orig_caste->caste_name[1]);
            send_caste->add_caste_name(orig_caste->caste_name[2]);

            send_caste->add_baby_name(orig_caste->baby_name[0]);
            send_caste->add_baby_name(orig_caste->baby_name[1]);

            send_caste->add_child_name(orig_caste->child_name[0]);
            send_caste->add_child_name(orig_caste->child_name[1]);
            send_caste->set_gender(orig_caste->gender);

            for (int partIndex = 0; partIndex < orig_caste->body_info.body_parts.size(); partIndex++)
            {
                auto orig_part = orig_caste->body_info.body_parts[partIndex];
                if (!orig_part)
                    continue;
                auto send_part = send_caste->add_body_parts();

                send_part->set_token(orig_part->token);
                send_part->set_category(orig_part->category);
                send_part->set_parent(orig_part->con_part_id);

                for (int partFlagIndex = 0; partFlagIndex <= ENUM_LAST_ITEM(body_part_raw_flags); partFlagIndex++)
                {
                    send_part->add_flags(orig_part->flags.is_set((body_part_raw_flags::body_part_raw_flags)partFlagIndex));
                }

                for (int layerIndex = 0; layerIndex < orig_part->layers.size(); layerIndex++)
                {
                    auto orig_layer = orig_part->layers[layerIndex];
                    if (!orig_layer)
                        continue;
                    auto send_layer = send_part->add_layers();

                    send_layer->set_layer_name(orig_layer->layer_name);
                    send_layer->set_tissue_id(orig_layer->tissue_id);
                    send_layer->set_layer_depth(orig_layer->layer_depth);
                    for (int layerModIndex = 0; layerModIndex < orig_layer->bp_modifiers.size(); layerModIndex++)
                    {
                        send_layer->add_bp_modifiers(orig_layer->bp_modifiers[layerModIndex]);
                    }
                }

                send_part->set_relsize(orig_part->relsize);
            }

            send_caste->set_total_relsize(orig_caste->body_info.total_relsize);

            for (int k = 0; k < orig_caste->bp_appearance.modifiers.size(); k++)
            {
                auto send_mod = send_caste->add_modifiers();
                auto orig_mod = orig_caste->bp_appearance.modifiers[k];
                send_mod->set_type(ENUM_KEY_STR(appearance_modifier_type, orig_mod->type));

#if DF_VERSION_INT > 34011
                if (orig_mod->growth_rate > 0)
                {
                    send_mod->set_mod_min(orig_mod->growth_min);
                    send_mod->set_mod_max(orig_mod->growth_max);
                }
                else
#endif
                {
                    send_mod->set_mod_min(orig_mod->ranges[0]);
                    send_mod->set_mod_max(orig_mod->ranges[6]);
                }

            }
            for (int k = 0; k < orig_caste->bp_appearance.modifier_idx.size(); k++)
            {
                send_caste->add_modifier_idx(orig_caste->bp_appearance.modifier_idx[k]);
                send_caste->add_part_idx(orig_caste->bp_appearance.part_idx[k]);
                send_caste->add_layer_idx(orig_caste->bp_appearance.layer_idx[k]);
            }
            for (int k = 0; k < orig_caste->body_appearance_modifiers.size(); k++)
            {
                auto send_mod = send_caste->add_body_appearance_modifiers();
                auto orig_mod = orig_caste->body_appearance_modifiers[k];

                send_mod->set_type(ENUM_KEY_STR(appearance_modifier_type, orig_mod->type));

#if DF_VERSION_INT > 34011
                if (orig_mod->growth_rate > 0)
                {
                    send_mod->set_mod_min(orig_mod->growth_min);
                    send_mod->set_mod_max(orig_mod->growth_max);
                }
                else
#endif
                {
                    send_mod->set_mod_min(orig_mod->ranges[0]);
                    send_mod->set_mod_max(orig_mod->ranges[6]);
                }
            }
            for (int k = 0; k < orig_caste->color_modifiers.size(); k++)
            {
                auto send_mod = send_caste->add_color_modifiers();
                auto orig_mod = orig_caste->color_modifiers[k];

                for (int l = 0; l < orig_mod->pattern_index.size(); l++)
                {
                    auto orig_pattern = world->raws.descriptors.patterns[orig_mod->pattern_index[l]];
                    auto send_pattern = send_mod->add_patterns();

                    for (int m = 0; m < orig_pattern->colors.size(); m++)
                    {
                        auto send_color = send_pattern->add_colors();
                        auto orig_color = world->raws.descriptors.colors[orig_pattern->colors[m]];
                        send_color->set_red(orig_color->red * 255.0);
                        send_color->set_green(orig_color->green * 255.0);
                        send_color->set_blue(orig_color->blue * 255.0);
                    }

                    send_pattern->set_id(orig_pattern->id);
                    send_pattern->set_pattern((PatternType)orig_pattern->pattern);
                }

                for (int l = 0; l < orig_mod->body_part_id.size(); l++)
                {
                    send_mod->add_body_part_id(orig_mod->body_part_id[l]);
                    send_mod->add_tissue_layer_id(orig_mod->tissue_layer_id[l]);
                    send_mod->set_start_date(orig_mod->start_date);
                    send_mod->set_end_date(orig_mod->end_date);
                    send_mod->set_part(orig_mod->part);
                }
            }

            send_caste->set_description(orig_caste->description);
            send_caste->set_adult_size(orig_caste->misc.adult_size);
        }

        for (int j = 0; j < orig_creature->tissue.size(); j++)
        {
            auto orig_tissue = orig_creature->tissue[j];
            auto send_tissue = send_creature->add_tissues();

            send_tissue->set_id(orig_tissue->id);
            send_tissue->set_name(orig_tissue->tissue_name_singular);
            send_tissue->set_subordinate_to_tissue(orig_tissue->subordinate_to_tissue);

            CopyMat(send_tissue->mutable_material(), orig_tissue->mat_type, orig_tissue->mat_index);
        }
    }

    return CR_OK;
}

static command_result GetPlantRaws(color_ostream &stream, const EmptyMessage *in, PlantRawList *out)
{
    GetPartialPlantRaws(stream, nullptr, out);
    return CR_OK;
}

static command_result GetPartialPlantRaws(color_ostream &stream, const ListRequest *in, PlantRawList *out)
{
    if (!df::global::world)
        return CR_FAILURE;

    df::world * world = df::global::world;

    int list_start = 0;
    int list_end = world->raws.plants.all.size();

    if (in != nullptr)
    {
        list_start = in->list_start();
        if (in->list_end() < list_end)
            list_end = in->list_end();
    }

    for (int i = 0; i < world->raws.plants.all.size(); i++)
    {
        df::plant_raw* plant_local = world->raws.plants.all[i];
        PlantRaw* plant_remote = out->add_plant_raws();

        plant_remote->set_index(i);
        plant_remote->set_id(plant_local->id);
        plant_remote->set_name(plant_local->name);
        if (!plant_local->flags.is_set(df::plant_raw_flags::TREE))
            plant_remote->set_tile(plant_local->tiles.shrub_tile);
        else
            plant_remote->set_tile(plant_local->tiles.tree_tile);
#if DF_VERSION_INT > 34011
        for (int j = 0; j < plant_local->growths.size(); j++)
        {
            df::plant_growth* growth_local = plant_local->growths[j];
            TreeGrowth * growth_remote = plant_remote->add_growths();
            growth_remote->set_index(j);
            growth_remote->set_id(growth_local->id);
            growth_remote->set_name(growth_local->name);
            for (int k = 0; k < growth_local->prints.size(); k++)
            {
                df::plant_growth_print* print_local = growth_local->prints[k];
                GrowthPrint* print_remote = growth_remote->add_prints();
                print_remote->set_priority(print_local->priority);
                print_remote->set_color(print_local->color[0] | (print_local->color[2] * 8));
                print_remote->set_timing_start(print_local->timing_start);
                print_remote->set_timing_end(print_local->timing_end);
                print_remote->set_tile(print_local->tile_growth);
            }
            growth_remote->set_timing_start(growth_local->timing_1);
            growth_remote->set_timing_end(growth_local->timing_2);
            growth_remote->set_twigs(growth_local->locations.bits.twigs);
            growth_remote->set_light_branches(growth_local->locations.bits.light_branches);
            growth_remote->set_heavy_branches(growth_local->locations.bits.heavy_branches);
            growth_remote->set_trunk(growth_local->locations.bits.trunk);
            growth_remote->set_roots(growth_local->locations.bits.roots);
            growth_remote->set_cap(growth_local->locations.bits.cap);
            growth_remote->set_sapling(growth_local->locations.bits.sapling);
            growth_remote->set_timing_start(growth_local->timing_1);
            growth_remote->set_timing_end(growth_local->timing_2);
            growth_remote->set_trunk_height_start(growth_local->trunk_height_perc_1);
            growth_remote->set_trunk_height_end(growth_local->trunk_height_perc_2);
            CopyMat(growth_remote->mutable_mat(), growth_local->mat_type, growth_local->mat_index);
        }
#endif
    }
    return CR_OK;
}

static command_result CopyScreen(color_ostream &stream, const EmptyMessage *in, ScreenCapture *out)
{
    df::graphic * gps = df::global::gps;
    out->set_width(gps->dimx);
    out->set_height(gps->dimy);
    for (int i = 0; i < (gps->dimx * gps->dimy); i++)
    {
        int index = i * 4;
        auto tile = out->add_tiles();
        tile->set_character(gps->screen[index]);
        tile->set_foreground(gps->screen[index + 1] | (gps->screen[index + 3] * 8));
        tile->set_background(gps->screen[index + 2]);
    }

    return CR_OK;
}

static command_result PassKeyboardEvent(color_ostream &stream, const KeyboardEvent *in)
{
#if DF_VERSION_INT > 34011
    SDL::Event e;
    e.key.type = in->type();
    e.key.state = in->state();
    e.key.ksym.mod = (SDL::Mod)in->mod();
    e.key.ksym.scancode = in->scancode();
    e.key.ksym.sym = (SDL::Key)in->sym();
    e.key.ksym.unicode = in->unicode();
    SDL_PushEvent(&e);
#endif
    return CR_OK;
}

static command_result SendDigCommand(color_ostream &stream, const DigCommand *in)
{
    MapExtras::MapCache mc;

    for (int i = 0; i < in->locations_size(); i++)
    {
        auto pos = in->locations(i);
        auto des = mc.designationAt(DFCoord(pos.x(), pos.y(), pos.z()));
        switch (in->designation())
        {
        case NO_DIG:
            des.bits.dig = tile_dig_designation::No;
            break;
        case DEFAULT_DIG:
            des.bits.dig = tile_dig_designation::Default;
            break;
        case UP_DOWN_STAIR_DIG:
            des.bits.dig = tile_dig_designation::UpDownStair;
            break;
        case CHANNEL_DIG:
            des.bits.dig = tile_dig_designation::Channel;
            break;
        case RAMP_DIG:
            des.bits.dig = tile_dig_designation::Ramp;
            break;
        case DOWN_STAIR_DIG:
            des.bits.dig = tile_dig_designation::DownStair;
            break;
        case UP_STAIR_DIG:
            des.bits.dig = tile_dig_designation::UpStair;
            break;
        default:
            break;
        }
        mc.setDesignationAt(DFCoord(pos.x(), pos.y(), pos.z()), des);

#if DF_VERSION_INT >= 43005
        //remove and job postings related.
        for (df::job_list_link * listing = &(world->jobs.list); listing != NULL; listing = listing->next)
        {
            if (listing->item == NULL)
                continue;
            auto type = listing->item->job_type;
            switch (type)
            {
            case df::enums::job_type::CarveFortification:
            case df::enums::job_type::DetailWall:
            case df::enums::job_type::DetailFloor:
            case df::enums::job_type::Dig:
            case df::enums::job_type::CarveUpwardStaircase:
            case df::enums::job_type::CarveDownwardStaircase:
            case df::enums::job_type::CarveUpDownStaircase:
            case df::enums::job_type::CarveRamp:
            case df::enums::job_type::DigChannel:
            case df::enums::job_type::FellTree:
            case df::enums::job_type::GatherPlants:
            case df::enums::job_type::RemoveConstruction:
            case df::enums::job_type::CarveTrack:
            {
                if (listing->item->pos == DFCoord(pos.x(), pos.y(), pos.z()))
                {
                    Job::removeJob(listing->item);
                    goto JOB_FOUND;
                }
                break;
            }
            default:
                continue;
            }
        }
    JOB_FOUND:
        continue;
#endif
    }

    mc.WriteAll();
    return CR_OK;
}

static command_result SetPauseState(color_ostream &stream, const SingleBool *in)
{
    DFHack::World::SetPauseState(in->value());
    return CR_OK;
}

static command_result GetPauseState(color_ostream &stream, const EmptyMessage *in, SingleBool *out)
{
    out->set_value(World::ReadPauseState());
    return CR_OK;
}

static command_result GetVersionInfo(color_ostream & stream, const EmptyMessage * in, RemoteFortressReader::VersionInfo * out)
{
    out->set_dfhack_version(DFHACK_VERSION);
#if DF_VERSION_INT == 34011
    out->set_dwarf_fortress_version("0.34.11");
#else
    out->set_dwarf_fortress_version(DF_VERSION);
#endif
    out->set_remote_fortress_reader_version(RFR_VERSION);
    return CR_OK;
}

int lastSentReportID = -1;

static command_result GetReports(color_ostream & stream, const EmptyMessage * in, RemoteFortressReader::Status * out)
{
    //First find the last report we sent, so it doesn't get resent.
    int lastSentIndex = -1;
    for (int i = world->status.reports.size() - 1; i >= 0; i--)
    {
        auto local_rep = world->status.reports[i];
        if (local_rep->id <= lastSentReportID)
        {
            lastSentIndex = i;
            break;
        }
    }
    for (int i = lastSentIndex + 1; i < world->status.reports.size(); i++)
    {
        auto local_rep = world->status.reports[i];
        if (!local_rep)
            continue;
        auto send_rep = out->add_reports();
        send_rep->set_type(local_rep->type);
        send_rep->set_text(DF2UTF(local_rep->text));
        ConvertDfColor(local_rep->color | (local_rep->bright ? 8 : 0), send_rep->mutable_color());
        send_rep->set_duration(local_rep->duration);
        send_rep->set_continuation(local_rep->flags.bits.continuation);
        send_rep->set_unconscious(local_rep->flags.bits.unconscious);
        send_rep->set_announcement(local_rep->flags.bits.announcement);
        send_rep->set_repeat_count(local_rep->repeat_count);
        ConvertDFCoord(local_rep->pos, send_rep->mutable_pos());
        send_rep->set_id(local_rep->id);
        send_rep->set_year(local_rep->year);
        send_rep->set_time(local_rep->time);
        lastSentReportID = local_rep->id;
    }
    return CR_OK;
}

static command_result GetLanguage(color_ostream & stream, const EmptyMessage * in, RemoteFortressReader::Language * out)
{
    if (!world)
        return CR_FAILURE;

    for (int i = 0; i < world->raws.descriptors.shapes.size(); i++)
    {
        auto shape = world->raws.descriptors.shapes[i];
        auto netShape = out->add_shapes();
        netShape->set_id(shape->id);
        netShape->set_tile(shape->tile);
    }
    return CR_OK;
}
