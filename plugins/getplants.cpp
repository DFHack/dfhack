// (un)designate matching plants for gathering/cutting
#include <set>

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "DataDefs.h"
#include "TileTypes.h"

#include "df/map_block.h"
#include "df/map_block_column.h"
#include "df/plant.h"
#include "df/plant_growth.h"
#include "df/plant_raw.h"
#include "df/tile_dig_designation.h"
#include "df/ui.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_object_data.h"
#include "df/world_site.h"

#include "modules/Designations.h"
#include "modules/Maps.h"
#include "modules/Materials.h"

using std::string;
using std::vector;
using std::set;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("getplants");
REQUIRE_GLOBAL(ui);
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

//selectability selectablePlant(color_ostream &out, const df::plant_raw *plant, bool farming)
selectability selectablePlant(const df::plant_raw *plant, bool farming)
{
    const DFHack::MaterialInfo basic_mat = DFHack::MaterialInfo(plant->material_defs.type[plant_material_def::basic_mat], plant->material_defs.idx[plant_material_def::basic_mat]);
    bool outOfSeason = false;
    selectability result = selectability::Nonselectable;

    if (plant->flags.is_set(plant_raw_flags::TREE))
    {
//        out.print("%s is a selectable tree\n", plant->id.c_str());
        if (farming)
        {
            return selectability::Nonselectable;
        }
        else
        {
            return selectability::Selectable;
        }
    }
    else if (plant->flags.is_set(plant_raw_flags::GRASS))
    {
//        out.print("%s is a non selectable Grass\n", plant->id.c_str());
        return selectability::Grass;
    }

    if (farming && plant->material_defs.type[plant_material_def::seed] == -1)
    {
        return selectability::Nonselectable;
    }

    if (basic_mat.material->flags.is_set(material_flags::EDIBLE_RAW) ||
        basic_mat.material->flags.is_set(material_flags::EDIBLE_COOKED))
    {
//        out.print("%s is edible\n", plant->id.c_str());
        if (farming)
        {
            if (basic_mat.material->flags.is_set(material_flags::EDIBLE_RAW))
            {
                result = selectability::Selectable;
            }
        }
        else
        {
            return selectability::Selectable;
        }
    }

    if (plant->flags.is_set(plant_raw_flags::THREAD) ||
        plant->flags.is_set(plant_raw_flags::MILL) ||
        plant->flags.is_set(plant_raw_flags::EXTRACT_VIAL) ||
        plant->flags.is_set(plant_raw_flags::EXTRACT_BARREL) ||
        plant->flags.is_set(plant_raw_flags::EXTRACT_STILL_VIAL))
    {
//        out.print("%s is thread/mill/extract\n", plant->id.c_str());
        if (farming)
        {
            result = selectability::Selectable;
        }
        else
        {
            return selectability::Selectable;
        }
    }

    if (basic_mat.material->reaction_product.id.size() > 0 ||
        basic_mat.material->reaction_class.size() > 0)
    {
//        out.print("%s has a reaction\n", plant->id.c_str());
        if (farming)
        {
            result = selectability::Selectable;
        }
        else
        {
            return selectability::Selectable;
        }
    }

    for (size_t i = 0; i < plant->growths.size(); i++)
    {
        if (plant->growths[i]->item_type == df::item_type::SEEDS ||  //  Only trees have seed growths in vanilla, but raws can be modded...
            plant->growths[i]->item_type == df::item_type::PLANT_GROWTH)
        {
            const DFHack::MaterialInfo growth_mat = DFHack::MaterialInfo(plant->growths[i]->mat_type, plant->growths[i]->mat_index);
            if ((plant->growths[i]->item_type == df::item_type::SEEDS &&
                 (growth_mat.material->flags.is_set(material_flags::EDIBLE_COOKED) ||
                  growth_mat.material->flags.is_set(material_flags::EDIBLE_RAW))) ||
                (plant->growths[i]->item_type == df::item_type::PLANT_GROWTH &&
                 growth_mat.material->flags.is_set(material_flags::LEAF_MAT)))  //  Will change name to STOCKPILE_PLANT_GROWTH any day now...
            {
                bool seedSource = plant->growths[i]->item_type == df::item_type::SEEDS;

                if (plant->growths[i]->item_type == df::item_type::PLANT_GROWTH)
                {
                    for (size_t k = 0; growth_mat.material->reaction_product.material.mat_type.size(); k++)
                    {
                        if (growth_mat.material->reaction_product.material.mat_type[k] == plant->material_defs.type[plant_material_def::seed] &&
                            growth_mat.material->reaction_product.material.mat_index[k] == plant->material_defs.idx[plant_material_def::seed])
                        {
                            seedSource = true;
                            break;
                        }
                    }
                }

                if (*cur_year_tick >= plant->growths[i]->timing_1 &&
                    (plant->growths[i]->timing_2 == -1 ||
                        *cur_year_tick <= plant->growths[i]->timing_2))
                {
//                     out.print("%s has an edible seed or a stockpile growth\n", plant->id.c_str());
                    if (!farming || seedSource)
                    {
                        return selectability::Selectable;
                    }
                }
                else
                {
                    if (!farming || seedSource)
                    {
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

    if (outOfSeason)
    {
//        out.print("%s has an out of season growth\n", plant->id.c_str());
        return selectability::OutOfSeason;
    }
    else
    {
//        out.printerr("%s cannot be gathered\n", plant->id.c_str());
        return result;
    }
}

//  Formula for determination of the variance in plant growth maturation time, determined via disassembly.
//  The x and y parameters are in tiles relative to the embark.
bool ripe(int32_t x, int32_t y, int32_t start, int32_t end) {
    int32_t time = (((435522653 - (((y + 3) * x + 5) * ((y + 7) * y * 400181475 + 289700012))) & 0x3FFFFFFF) % 2000 + *cur_year_tick) % 403200;

    return time >= start && (end == -1 || time <= end);
}

//  Looks in the picked growths vector to see if a matching growth has been marked as picked.
bool picked(const df::plant *plant, int32_t growth_subtype) {
    df::world_data *world_data = world->world_data;
    df::world_site *site = df::world_site::find(ui->site_id);
    int32_t pos_x = site->global_min_x + plant->pos.x / 48;
    int32_t pos_y = site->global_min_y + plant->pos.y / 48;
    size_t id = pos_x + pos_y * 16 * world_data->world_width;
    df::world_object_data *object_data = df::world_object_data::find(id);
    if (!object_data) {
        return false;
    }
    df::map_block_column *column = world->map.map_block_columns[(plant->pos.x / 16) * world->map.x_count_block + (plant->pos.y / 16)];

    for (size_t i = 0; i < object_data->picked_growths.x.size(); i++) {
        if (object_data->picked_growths.x[i] == plant->pos.x &&
            object_data->picked_growths.y[i] == plant->pos.y &&
            object_data->picked_growths.z[i] - column->z_base == plant->pos.z &&
            object_data->picked_growths.subtype[i] == growth_subtype &&
            object_data->picked_growths.year[i] == *cur_year) {
            return true;
        }
    }

    return false;
}

bool designate(const df::plant *plant, bool farming) {
    df::plant_raw *plant_raw = world->raws.plants.all[plant->material];
    const DFHack::MaterialInfo basic_mat = DFHack::MaterialInfo(plant_raw->material_defs.type[plant_material_def::basic_mat], plant_raw->material_defs.idx[plant_material_def::basic_mat]);

    if (basic_mat.material->flags.is_set(material_flags::EDIBLE_RAW) ||
        basic_mat.material->flags.is_set(material_flags::EDIBLE_COOKED))
    {
        return Designations::markPlant(plant);
    }

    if (plant_raw->flags.is_set(plant_raw_flags::THREAD) ||
        plant_raw->flags.is_set(plant_raw_flags::MILL) ||
        plant_raw->flags.is_set(plant_raw_flags::EXTRACT_VIAL) ||
        plant_raw->flags.is_set(plant_raw_flags::EXTRACT_BARREL) ||
        plant_raw->flags.is_set(plant_raw_flags::EXTRACT_STILL_VIAL))
    {
        if (!farming) {
            return Designations::markPlant(plant);
        }
    }

    if (basic_mat.material->reaction_product.id.size() > 0 ||
        basic_mat.material->reaction_class.size() > 0)
    {
        if (!farming) {
            return Designations::markPlant(plant);
        }
    }

    for (size_t i = 0; i < plant_raw->growths.size(); i++)
    {
        if (plant_raw->growths[i]->item_type == df::item_type::SEEDS ||  //  Only trees have seed growths in vanilla, but raws can be modded...
            plant_raw->growths[i]->item_type == df::item_type::PLANT_GROWTH)
        {
            const DFHack::MaterialInfo growth_mat = DFHack::MaterialInfo(plant_raw->growths[i]->mat_type, plant_raw->growths[i]->mat_index);
            if ((plant_raw->growths[i]->item_type == df::item_type::SEEDS &&
                (growth_mat.material->flags.is_set(material_flags::EDIBLE_COOKED) ||
                    growth_mat.material->flags.is_set(material_flags::EDIBLE_RAW))) ||
                    (plant_raw->growths[i]->item_type == df::item_type::PLANT_GROWTH &&
                        growth_mat.material->flags.is_set(material_flags::LEAF_MAT)))  //  Will change name to STOCKPILE_PLANT_GROWTH any day now...
            {
                bool seedSource = plant_raw->growths[i]->item_type == df::item_type::SEEDS;

                if (plant_raw->growths[i]->item_type == df::item_type::PLANT_GROWTH)
                {
                    for (size_t k = 0; growth_mat.material->reaction_product.material.mat_type.size(); k++)
                    {
                        if (growth_mat.material->reaction_product.material.mat_type[k] == plant_raw->material_defs.type[plant_material_def::seed] &&
                            growth_mat.material->reaction_product.material.mat_index[k] == plant_raw->material_defs.idx[plant_material_def::seed])
                        {
                            seedSource = true;
                            break;
                        }
                    }
                }

                if ((!farming || seedSource) &&
                    ripe(plant->pos.x, plant->pos.y, plant_raw->growths[i]->timing_1, plant_raw->growths[i]->timing_2) &&
                    !picked(plant, i))
                {
                    return Designations::markPlant(plant);
                }
            }
        }
    }

    return false;
}

command_result df_getplants (color_ostream &out, vector <string> & parameters)
{
    string plantMatStr = "";
    std::vector<selectability> plantSelections;
    std::vector<size_t> collectionCount;
    set<string> plantNames;
    bool deselect = false, exclude = false, treesonly = false, shrubsonly = false, all = false, verbose = false, farming = false;
    size_t maxCount = 999999;
    int count = 0;

    plantSelections.resize(world->raws.plants.all.size());
    collectionCount.resize(world->raws.plants.all.size());

    for (size_t i = 0; i < plantSelections.size(); i++)
    {
        plantSelections[i] = selectability::Unselected;
        collectionCount[i] = 0;
    }

    bool anyPlantsSelected = false;

    for (size_t i = 0; i < parameters.size(); i++)
    {
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
        else if (parameters[i] == "-n")
        {
            if (parameters.size() > i + 1)
            {
                maxCount = atoi(parameters[i + 1].c_str());
                if (maxCount >= 1)
                {
                    i++;  //  We've consumed the next parameter, so we need to progress the iterator.
                }
                else
                {
                    out.printerr("-n requires a positive integer parameter!\n");
                    return CR_WRONG_USAGE;
                }
            }
            else
            {
                out.printerr("-n requires a positive integer parameter!\n");
                return CR_WRONG_USAGE;
            }
        }
        else
            plantNames.insert(parameters[i]);
    }
    if (treesonly && shrubsonly)
    {
        out.printerr("Cannot specify both -t and -s at the same time!\n");
        return CR_WRONG_USAGE;
    }
    if (treesonly && farming)
    {
        out.printerr("Cannot specify both -t and -f at the same time!\n");
        return CR_WRONG_USAGE;
    }
    if (all && exclude)
    {
        out.printerr("Cannot specify both -a and -x at the same time!\n");
        return CR_WRONG_USAGE;
    }
    if (all && plantNames.size())
    {
        out.printerr("Cannot specify -a along with plant IDs!\n");
        return CR_WRONG_USAGE;
    }

    CoreSuspender suspend;

    for (size_t i = 0; i < world->raws.plants.all.size(); i++)
    {
        df::plant_raw *plant = world->raws.plants.all[i];
        if (all)
        {
//            plantSelections[i] = selectablePlant(out, plant, farming);
            plantSelections[i] = selectablePlant(plant, farming);
        }
         else if (plantNames.find(plant->id) != plantNames.end())
        {
            plantNames.erase(plant->id);
//            plantSelections[i] = selectablePlant(out, plant, farming);
            plantSelections[i] = selectablePlant(plant, farming);
            switch (plantSelections[i])
            {
            case selectability::Grass:
                out.printerr("%s is a grass and cannot be gathered\n", plant->id.c_str());
                break;

            case selectability::Nonselectable:
                if (farming)
                {
                    out.printerr("%s does not have any parts that can be gathered for seeds for farming\n", plant->id.c_str());
                }
                else
                {
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
    if (plantNames.size() > 0)
    {
        out.printerr("Invalid plant ID(s):");
        for (set<string>::const_iterator it = plantNames.begin(); it != plantNames.end(); it++)
            out.printerr(" %s", it->c_str());
        out.printerr("\n");
        return CR_FAILURE;
    }

    for (size_t i = 0; i < plantSelections.size(); i++)
    {
        if (plantSelections[i] == selectability::OutOfSeason ||
            plantSelections[i] == selectability::Selectable)
        {
            anyPlantsSelected = true;
            break;
        }
    }

    if (!anyPlantsSelected)
    {
        out.print("Valid plant IDs:\n");
        for (size_t i = 0; i < world->raws.plants.all.size(); i++)
        {
            df::plant_raw *plant = world->raws.plants.all[i];
//            switch (selectablePlant(out, plant, farming))
            switch (selectablePlant(plant, farming))
                {
            case selectability::Grass:
            case selectability::Nonselectable:
                continue;

            case selectability::OutOfSeason:
            {
                if (!treesonly)
                {
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
    for (size_t i = 0; i < world->plants.all.size(); i++)
    {
        const df::plant *plant = world->plants.all[i];
        df::map_block *cur = Maps::getTileBlock(plant->pos);
        bool dirty = false;

        int x = plant->pos.x % 16;
        int y = plant->pos.y % 16;
        if (plantSelections[plant->material] == selectability::OutOfSeason ||
            plantSelections[plant->material] == selectability::Selectable)
        {
            if (exclude ||
                plantSelections[plant->material] == selectability::OutOfSeason)
                continue;
        }
        else
        {
            if (!exclude)
                continue;
        }
        df::tiletype_shape shape = tileShape(cur->tiletype[x][y]);
        df::tiletype_material material = tileMaterial(cur->tiletype[x][y]);
        df::tiletype_special special = tileSpecial(cur->tiletype[x][y]);
        if (plant->flags.bits.is_shrub && (treesonly || !(shape == tiletype_shape::SHRUB && special != tiletype_special::DEAD)))
            continue;
        if (!plant->flags.bits.is_shrub && (shrubsonly || !(material == tiletype_material::TREE)))
            continue;
        if (cur->designation[x][y].bits.hidden)
            continue;
        if (collectionCount[plant->material] >= maxCount)
            continue;
        if (deselect && Designations::unmarkPlant(plant))
        {
            collectionCount[plant->material]++;
            ++count;
        }
        if (!deselect && designate(plant, farming))
        {
//            out.print("Designated %s at (%i, %i, %i), %d\n", world->raws.plants.all[plant->material]->id.c_str(), plant->pos.x, plant->pos.y, plant->pos.z, (int)i);
            collectionCount[plant->material]++;
            ++count;
        }
    }
    if (count)
    {
        if (verbose)
        {
            for (size_t i = 0; i < plantSelections.size(); i++)
            {
                if (collectionCount[i] > 0)
                    out.print("Updated %d %s designations.\n", (int)collectionCount[i], world->raws.plants.all[i]->id.c_str());
            }
            out.print("\n");
        }
    }
    out.print("Updated %d plant designations.\n", (int)count);

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "getplants", "Cut down trees or gather shrubs by ID",
        df_getplants, false,
        "  Specify the types of trees to cut down and/or shrubs to gather by their\n"
        "  plant IDs, separated by spaces.\n"
        "Options:\n"
        "  -t - Tree: Select trees only (exclude shrubs)\n"
        "  -s - Shrub: Select shrubs only (exclude trees)\n"
        "  -f - Farming: Designate only shrubs that yield seeds for farming. Implies -s\n"
        "  -c - Clear: Clear designations instead of setting them\n"
        "  -x - eXcept: Apply selected action to all plants except those specified\n"
        "  -a - All: Select every type of plant (obeys -t/-s/-f)\n"
        "  -v - Verbose: List the number of (un)designations per plant\n"
        "  -n * - Number: Designate up to * (an integer number) plants of each species\n"
        "Specifying both -t and -s or -f will have no effect.\n"
        "If no plant IDs are specified, and the -a switch isn't given, all valid plant\n"
        "IDs will be listed with -t, -s, and -f restricting the list to trees, shrubs,\n"
        "and farmable shrubs, respectively.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
