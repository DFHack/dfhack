#include "Debug.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Designations.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
#include "modules/Random.h"

#include "df/map_block.h"
#include "df/map_block_column.h"
#include "df/material.h"
#include "df/material_flags.h"
#include "df/plant.h"
#include "df/plant_growth.h"
#include "df/plant_raw.h"
#include "df/tile_dig_designation.h"
#include "df/plotinfost.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_object_data.h"
#include "df/world_site.h"

using std::string;
using std::vector;
using std::set;

using namespace DFHack;
using namespace df::enums;

namespace DFHack
{
DBG_DECLARE(getplants, log, DebugCategory::LINFO);
}

DFHACK_PLUGIN("getplants");
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(cur_year);
REQUIRE_GLOBAL(cur_year_tick);

enum class selectability {
    Selectable,
    Grass,
    Nonselectable,
    OutOfSeason,
    Unselected
};

//  Determination of whether seeds can be collected is somewhat messy:
//    - Growths of type SEEDS are collected only if they are edible either raw or cooked.
//    - Growths of type PLANT_GROWTH are collected provided the STOCKPILE_PLANT_GROWTH
//      flag is set.
//  The two points above were determined through examination of the DF code, while the ones
//  below were determined through examination of the behavior of bugged, working, and
//  RAW manipulated shrubs on embarks.
//    - If seeds are defined as explicit growths, they are the source of seeds, overriding
//      the default STRUCTURAL part as the source.
//    - If a growth has the reaction that extracts seeds as a side effect of other
//      processing (brewing, eating raw, etc.), this overrides the STRUCTURAL part as the
//      source of seeds. However, for some reason it does not produce seeds from eating
//      raw growths unless the structural part is collected (at least for shrubs: other
//      processing was not examined).
//    - If a growth has a (non vanilla) reaction that produces seeds, seeds are produced,
//      provided there is something (such as a workshop order) that triggers it.
//  The code below is satisfied with detection of a seed producing reaction, and does not
//  detect the bugged case where a seed extraction process is defined but doesn't get
//  triggered. Such a process can be triggered either as a side effect of other
//  processing, or as a workshop reaction, and it would be overkill for this code to
//  try to determine if a workshop reaction exists and has been permitted for the played
//  race.
//  There are two bugged cases of this in the current vanilla RAWs:
//  Both Red Spinach and Elephant-Head Amaranth have the seed extraction reaction
//  explicitly specified for the structural part, but no other use for it. This causes
//  these parts to be collected (a valid reaction is defined), but remain unusable. This
//  is one of the issues in bug 6940 on the bug tracker (the others cases are detected and
//  result in the plants not being usable for farming or even collectable at all).

selectability selectablePlant(color_ostream& out, const df::plant_raw* plant, bool farming) {
    TRACE(log, out).print("analyzing %s\n", plant->id.c_str());
    const DFHack::MaterialInfo basic_mat = DFHack::MaterialInfo(plant->material_defs.type[plant_material_def::basic_mat], plant->material_defs.idx[plant_material_def::basic_mat]);
    bool outOfSeason = false;
    selectability result = selectability::Nonselectable;

    if (plant->flags.is_set(plant_raw_flags::TREE)) {
        DEBUG(log, out).print("%s is a selectable tree\n", plant->id.c_str());
        if (farming) {
            return selectability::Nonselectable;
        }
        else {
            return selectability::Selectable;
        }
    }
    else if (plant->flags.is_set(plant_raw_flags::GRASS)) {
        DEBUG(log, out).print("%s is a non selectable Grass\n", plant->id.c_str());
        return selectability::Grass;
    }

    if (farming && plant->material_defs.type[plant_material_def::seed] == -1) {
        return selectability::Nonselectable;
    }

    if (basic_mat.isValid() &&
        (basic_mat.material->flags.is_set(material_flags::EDIBLE_RAW) ||
         basic_mat.material->flags.is_set(material_flags::EDIBLE_COOKED)))
    {
        DEBUG(log, out).print("%s is edible\n", plant->id.c_str());
        if (farming) {
            if (basic_mat.material->flags.is_set(material_flags::EDIBLE_RAW)) {
                result = selectability::Selectable;
            }
        }
        else {
            return selectability::Selectable;
        }
    }

    if (plant->flags.is_set(plant_raw_flags::THREAD) ||
        plant->flags.is_set(plant_raw_flags::MILL) ||
        plant->flags.is_set(plant_raw_flags::EXTRACT_VIAL) ||
        plant->flags.is_set(plant_raw_flags::EXTRACT_BARREL) ||
        plant->flags.is_set(plant_raw_flags::EXTRACT_STILL_VIAL)) {
        DEBUG(log, out).print("%s is thread/mill/extract\n", plant->id.c_str());
        if (farming) {
            result = selectability::Selectable;
        }
        else {
            return selectability::Selectable;
        }
    }

    if (basic_mat.isValid() &&
        (basic_mat.material->reaction_product.id.size() > 0 ||
         basic_mat.material->reaction_class.size() > 0))
    {
        DEBUG(log, out).print("%s has a reaction\n", plant->id.c_str());
        if (farming) {
            result = selectability::Selectable;
        }
        else {
            return selectability::Selectable;
        }
    }

    for (size_t i = 0; i < plant->growths.size(); i++) {
        if (plant->growths[i]->item_type == df::item_type::SEEDS ||  //  Only trees have seed growths in vanilla, but raws can be modded...
            plant->growths[i]->item_type == df::item_type::PLANT_GROWTH) {
            const DFHack::MaterialInfo growth_mat = DFHack::MaterialInfo(plant->growths[i]->mat_type, plant->growths[i]->mat_index);
            if ((plant->growths[i]->item_type == df::item_type::SEEDS &&
                (growth_mat.material->flags.is_set(material_flags::EDIBLE_COOKED) ||
                    growth_mat.material->flags.is_set(material_flags::EDIBLE_RAW))) ||
                (plant->growths[i]->item_type == df::item_type::PLANT_GROWTH &&
                    growth_mat.material->flags.is_set(material_flags::STOCKPILE_PLANT_GROWTH)))
            {
                bool seedSource = plant->growths[i]->item_type == df::item_type::SEEDS;

                if (plant->growths[i]->item_type == df::item_type::PLANT_GROWTH) {
                    auto& mat = growth_mat.material->reaction_product.material;
                    for (size_t k = 0; k < mat.mat_type.size(); k++) {
                        if (mat.mat_type[k] == plant->material_defs.type[plant_material_def::seed] &&
                            mat.mat_index[k] == plant->material_defs.idx[plant_material_def::seed]) {
                            seedSource = true;
                            break;
                        }
                    }
                }

                if (*cur_year_tick >= plant->growths[i]->timing_1 &&
                    (plant->growths[i]->timing_2 == -1 ||
                        *cur_year_tick <= plant->growths[i]->timing_2)) {
                    DEBUG(log, out).print("%s has an edible seed or a stockpile growth\n", plant->id.c_str());
                    if (!farming || seedSource) {
                        return selectability::Selectable;
                    }
                }
                else {
                    if (!farming || seedSource) {
                        outOfSeason = true;
                    }
                }
            }
        }
        /*            else if (plant->growths[i]->behavior.bits.has_seed)  //  This code designates beans, etc. when DF doesn't, but plant gatherers still fail to collect anything, so it's useless: bug #0006940.
                    {
                        const DFHack::MaterialInfo seed_mat = DFHack::MaterialInfo(plant->material_defs.type[plant_material_def::seed], plant->material_defs.idx[plant_material_def::seed]);

                        if (seed_mat.material->flags.is_set(material_flags::EDIBLE_RAW) ||
                            seed_mat.material->flags.is_set(material_flags::EDIBLE_COOKED))
                        {
                            if (*cur_year_tick >= plant->growths[i]->timing_1 &&
                                (plant->growths[i]->timing_2 == -1 ||
                                    *cur_year_tick <= plant->growths[i]->timing_2))
                            {
                                return selectability::Selectable;
                            }
                            else
                            {
                                outOfSeason = true;
                            }
                        }
                    }  */
    }

    if (outOfSeason) {
        DEBUG(log, out).print("%s has an out of season growth\n", plant->id.c_str());
        return selectability::OutOfSeason;
    }
    else {
        DEBUG(log, out).print("%s cannot be gathered\n", plant->id.c_str());
        return result;
    }
}

//  Formula for determination of the variance in plant growth maturation time, determined via disassembly.
//  The coordinates are relative to the embark region.
bool ripe(int32_t x, int32_t y, int32_t z, int32_t start, int32_t end) {
    DFHack::Random::SplitmixRNG rng((world->map.region_x * 48 + x) + (world->map.region_y * 48 + y) * 10000 + (world->map.region_z + z) * 100000000);
    int32_t time = (rng.df_trandom(2000) + *cur_year_tick) % 403200;

    return time >= start && (end == -1 || time <= end);
}

//  Looks in the local creation zone's picked growths vector to see if a matching growth has been marked as picked.
bool picked(const df::plant* plant, int32_t growth_subtype, int32_t growth_density) {
    int32_t pos_x = plant->pos.x / 48 + world->map.region_x;
    int32_t pos_y = plant->pos.y / 48 + world->map.region_y;
    size_t cz_id = pos_x + pos_y * 16 * world->world_data->world_width;
    auto cz = df::world_object_data::find(cz_id);
    if (!cz) {
        return false;
    }

    for (size_t i = 0; i < cz->picked_growths.x.size(); i++) {
        if (cz->picked_growths.x[i] == (plant->pos.x % 48) &&
            cz->picked_growths.y[i] == (plant->pos.y % 48) &&
            cz->picked_growths.z[i] == (plant->pos.z + world->map.region_z) &&
            cz->picked_growths.density[i] >= growth_density &&
            cz->picked_growths.subtype[i] == growth_subtype &&
            cz->picked_growths.year[i] == *cur_year) {
            return true;
        }
    }

    return false;
}

bool designate(color_ostream& out, const df::plant* plant, bool farming) {
    TRACE(log, out).print("Attempting to designate %s at (%i, %i, %i)\n", world->raws.plants.all[plant->material]->id.c_str(), plant->pos.x, plant->pos.y, plant->pos.z);

    if (!farming) {
        bool istree = (tileMaterial(Maps::getTileBlock(plant->pos)->tiletype[plant->pos.x % 16][plant->pos.y % 16]) == tiletype_material::TREE);
        if (istree)
            return Designations::markPlant(plant);
    }

    df::plant_raw* plant_raw = world->raws.plants.all[plant->material];
    const DFHack::MaterialInfo basic_mat = DFHack::MaterialInfo(plant_raw->material_defs.type[plant_material_def::basic_mat], plant_raw->material_defs.idx[plant_material_def::basic_mat]);

    if (basic_mat.material->flags.is_set(material_flags::EDIBLE_RAW) ||
        basic_mat.material->flags.is_set(material_flags::EDIBLE_COOKED)) {
        return Designations::markPlant(plant);
    }

    if (plant_raw->flags.is_set(plant_raw_flags::THREAD) ||
        plant_raw->flags.is_set(plant_raw_flags::MILL) ||
        plant_raw->flags.is_set(plant_raw_flags::EXTRACT_VIAL) ||
        plant_raw->flags.is_set(plant_raw_flags::EXTRACT_BARREL) ||
        plant_raw->flags.is_set(plant_raw_flags::EXTRACT_STILL_VIAL)) {
        if (!farming) {
            return Designations::markPlant(plant);
        }
    }

    if (basic_mat.material->reaction_product.id.size() > 0 ||
        basic_mat.material->reaction_class.size() > 0) {
        if (!farming) {
            return Designations::markPlant(plant);
        }
    }

    for (size_t i = 0; i < plant_raw->growths.size(); i++) {
        TRACE(log, out).print("growth item type=%d\n", plant_raw->growths[i]->item_type);
        //  Only trees have seed growths in vanilla, but raws can be modded...
        if (plant_raw->growths[i]->item_type != df::item_type::SEEDS &&
            plant_raw->growths[i]->item_type != df::item_type::PLANT_GROWTH)
            continue;

        const DFHack::MaterialInfo growth_mat = DFHack::MaterialInfo(plant_raw->growths[i]->mat_type, plant_raw->growths[i]->mat_index);
        TRACE(log, out).print("edible_cooked=%d edible_raw=%d leaf_mat=%d\n",
            growth_mat.material->flags.is_set(material_flags::EDIBLE_COOKED),
            growth_mat.material->flags.is_set(material_flags::EDIBLE_RAW),
            growth_mat.material->flags.is_set(material_flags::STOCKPILE_PLANT_GROWTH));
        if (!(plant_raw->growths[i]->item_type == df::item_type::SEEDS &&
            (growth_mat.material->flags.is_set(material_flags::EDIBLE_COOKED) ||
                growth_mat.material->flags.is_set(material_flags::EDIBLE_RAW))) &&
            !(plant_raw->growths[i]->item_type == df::item_type::PLANT_GROWTH &&
                growth_mat.material->flags.is_set(material_flags::STOCKPILE_PLANT_GROWTH)))
            continue;

        bool seedSource = plant_raw->growths[i]->item_type == df::item_type::SEEDS;

        if (plant_raw->growths[i]->item_type == df::item_type::PLANT_GROWTH) {
            for (size_t k = 0; growth_mat.material->reaction_product.material.mat_type.size(); k++) {
                if (growth_mat.material->reaction_product.material.mat_type[k] == plant_raw->material_defs.type[plant_material_def::seed] &&
                    growth_mat.material->reaction_product.material.mat_index[k] == plant_raw->material_defs.idx[plant_material_def::seed]) {
                    seedSource = true;
                    break;
                }
            }
        }

        if ((!farming || seedSource) &&
            ripe(plant->pos.x, plant->pos.y, plant->pos.z, plant_raw->growths[i]->timing_1, plant_raw->growths[i]->timing_2) &&
            !picked(plant, i, plant_raw->growths[i]->density))
            return Designations::markPlant(plant);
    }

    return false;
}

command_result df_getplants(color_ostream& out, vector <string>& parameters) {
    string plantMatStr = "";
    std::vector<selectability> plantSelections;
    std::vector<size_t> collectionCount;
    set<string> plantNames;
    bool deselect = false, exclude = false, treesonly = false, shrubsonly = false, all = false, verbose = false, farming = false;
    size_t maxCount = 999999;
    int count = 0;

    plantSelections.resize(world->raws.plants.all.size());
    collectionCount.resize(world->raws.plants.all.size());

    for (size_t i = 0; i < plantSelections.size(); i++) {
        plantSelections[i] = selectability::Unselected;
        collectionCount[i] = 0;
    }

    bool anyPlantsSelected = false;

    for (size_t i = 0; i < parameters.size(); i++) {
        if (parameters[i] == "help" || parameters[i] == "?")
            return CR_WRONG_USAGE;
        else if (parameters[i] == "-t")
            treesonly = true;
        else if (parameters[i] == "-s")
            shrubsonly = true;
        else if (parameters[i] == "-c")
            deselect = true;
        else if (parameters[i] == "-x")
            exclude = true;
        else if (parameters[i] == "-a")
            all = true;
        else if (parameters[i] == "-v")
            verbose = true;
        else if (parameters[i] == "-f")
            farming = true;
        else if (parameters[i] == "-n") {
            if (parameters.size() > i + 1) {
                maxCount = atoi(parameters[i + 1].c_str());
                if (maxCount >= 1) {
                    i++;  //  We've consumed the next parameter, so we need to progress the iterator.
                }
                else {
                    out.printerr("-n requires a positive integer parameter!\n");
                    return CR_WRONG_USAGE;
                }
            }
            else {
                out.printerr("-n requires a positive integer parameter!\n");
                return CR_WRONG_USAGE;
            }
        }
        else
            plantNames.insert(toUpper_cp437(parameters[i]));
    }
    if (treesonly && shrubsonly) {
        out.printerr("Cannot specify both -t and -s at the same time!\n");
        return CR_WRONG_USAGE;
    }
    if (treesonly && farming) {
        out.printerr("Cannot specify both -t and -f at the same time!\n");
        return CR_WRONG_USAGE;
    }
    if (all && exclude) {
        out.printerr("Cannot specify both -a and -x at the same time!\n");
        return CR_WRONG_USAGE;
    }
    if (all && plantNames.size()) {
        out.printerr("Cannot specify -a along with plant IDs!\n");
        return CR_WRONG_USAGE;
    }

    for (size_t i = 0; i < world->raws.plants.all.size(); i++) {
        df::plant_raw* plant = world->raws.plants.all[i];
        if (all) {
            plantSelections[i] = selectablePlant(out, plant, farming);
        }
        else if (plantNames.find(plant->id) != plantNames.end()) {
            plantNames.erase(plant->id);
            plantSelections[i] = selectablePlant(out, plant, farming);
            switch (plantSelections[i]) {
            case selectability::Grass:
                out.printerr("%s is a grass and cannot be gathered\n", plant->id.c_str());
                break;

            case selectability::Nonselectable:
                if (farming) {
                    out.printerr("%s does not have any parts that can be gathered for seeds for farming\n", plant->id.c_str());
                }
                else {
                    out.printerr("%s does not have any parts that can be gathered\n", plant->id.c_str());
                }
                break;

            case selectability::OutOfSeason:
                out.printerr("%s is out of season, with nothing that can be gathered now\n", plant->id.c_str());
                break;

            case selectability::Selectable:
                break;

            case selectability::Unselected:
                break;  //  We won't get to this option
            }
        }
    }
    if (plantNames.size() > 0) {
        out.printerr("Invalid plant ID(s):");
        for (set<string>::const_iterator it = plantNames.begin(); it != plantNames.end(); it++)
            out.printerr(" %s", it->c_str());
        out.printerr("\n");
        return CR_FAILURE;
    }

    for (size_t i = 0; i < plantSelections.size(); i++) {
        if (plantSelections[i] == selectability::OutOfSeason ||
            plantSelections[i] == selectability::Selectable) {
            anyPlantsSelected = true;
            break;
        }
    }

    if (!anyPlantsSelected) {
        out.print("Valid plant IDs:\n");
        for (size_t i = 0; i < world->raws.plants.all.size(); i++) {
            df::plant_raw* plant = world->raws.plants.all[i];
            switch (selectablePlant(out, plant, farming)) {
            case selectability::Grass:
            case selectability::Nonselectable:
                continue;

            case selectability::OutOfSeason:
            {
                if (!treesonly) {
                    out.print("* (shrub) %s - %s is out of season\n", plant->id.c_str(), plant->name.c_str());
                }
                break;
            }

            case selectability::Selectable:
            {
                if ((treesonly && plant->flags.is_set(plant_raw_flags::TREE)) ||
                    (shrubsonly && !plant->flags.is_set(plant_raw_flags::TREE)) ||
                    (!treesonly && !shrubsonly))  // 'farming' weeds out trees when determining selectability, so no need to test that explicitly
                {
                    out.print("* (%s) %s - %s\n", plant->flags.is_set(plant_raw_flags::TREE) ? "tree" : "shrub", plant->id.c_str(), plant->name.c_str());
                }
                break;
            }

            case selectability::Unselected:  //  Should never get this alternative
                break;
            }
        }
        return CR_OK;
    }

    count = 0;
    for (size_t i = 0; i < world->plants.all.size(); i++) {
        const df::plant* plant = world->plants.all[i];
        df::map_block* cur = Maps::getTileBlock(plant->pos);

        TRACE(log, out).print("Examining %s at (%i, %i, %i) [index=%d]\n", world->raws.plants.all[plant->material]->id.c_str(), plant->pos.x, plant->pos.y, plant->pos.z, (int)i);

        int x = plant->pos.x % 16;
        int y = plant->pos.y % 16;
        if (plantSelections[plant->material] == selectability::OutOfSeason ||
            plantSelections[plant->material] == selectability::Selectable) {
            if (exclude ||
                plantSelections[plant->material] == selectability::OutOfSeason)
                continue;
        }
        else {
            if (!exclude)
                continue;
        }
        df::tiletype tt = cur->tiletype[x][y];
        df::tiletype_material mat = tileMaterial(tt);
        if ((treesonly || tt != tiletype::Shrub) && ENUM_ATTR(plant_type, is_shrub, plant->type))
            continue;
        if ((shrubsonly || mat != tiletype_material::TREE) && !ENUM_ATTR(plant_type, is_shrub, plant->type))
            continue;
        if (cur->designation[x][y].bits.hidden)
            continue;
        if (collectionCount[plant->material] >= maxCount)
            continue;
        if (deselect && Designations::unmarkPlant(plant)) {
            collectionCount[plant->material]++;
            ++count;
        }
        if (!deselect && designate(out, plant, farming)) {
            DEBUG(log, out).print("Designated %s at (%i, %i, %i), %d\n", world->raws.plants.all[plant->material]->id.c_str(), plant->pos.x, plant->pos.y, plant->pos.z, (int)i);
            collectionCount[plant->material]++;
            ++count;
        }
    }
    if (count && verbose) {
        for (size_t i = 0; i < plantSelections.size(); i++) {
            if (collectionCount[i] > 0)
                out.print("Updated %d %s designations.\n", (int)collectionCount[i], world->raws.plants.all[i]->id.c_str());
        }
        out.print("\n");
    }
    out.print("Updated %d plant designations.\n", (int)count);

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream& out, vector <PluginCommand>& commands) {
    commands.push_back(PluginCommand(
        "getplants",
        "Designate trees for chopping and shrubs for gathering.",
        df_getplants));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream& out) {
    return CR_OK;
}
