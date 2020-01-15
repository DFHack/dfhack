// (un)designate matching plants for gathering/cutting
// Known issue:
// DF is capable of determining that a shrub has already been picked, leaving an unusable structure part
// behind. This code does not perform such a check (as the location of the required information is
// unknown to the writer of this comment). This leads to some shrubs being designated when they
// shouldn't be, causing a plant gatherer to walk there and do nothing (except clearing the designation).
#include <set>

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "DataDefs.h"
#include "TileTypes.h"

#include "df/map_block.h"
#include "df/plant.h"
#include "df/plant_growth.h"
#include "df/plant_raw.h"
#include "df/tile_dig_designation.h"
#include "df/world.h"

#include "modules/Designations.h"
#include "modules/Maps.h"
#include "modules/Materials.h"

using std::string;
using std::vector;
using std::set;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("getplants");
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(cur_year_tick);

enum class selectability {
    Selectable,
    Grass,
    Nonselectable,
    OutOfSeason,
    Unselected
};

//selectability selectablePlant(color_ostream &out, const df::plant_raw *plant)
selectability selectablePlant(const df::plant_raw *plant)
{
    const DFHack::MaterialInfo basic_mat = DFHack::MaterialInfo(plant->material_defs.type_basic_mat, plant->material_defs.idx_basic_mat);
    bool outOfSeason = false;

    if (plant->flags.is_set(plant_raw_flags::TREE))
    {
//        out.print("%s is a selectable tree\n", plant->id.c_str());
        return selectability::Selectable;
    }
    else if (plant->flags.is_set(plant_raw_flags::GRASS))
    {
//        out.print("%s is a non selectable Grass\n", plant->id.c_str());
        return selectability::Grass;
    }

    if (basic_mat.material->flags.is_set(material_flags::EDIBLE_RAW) ||
        basic_mat.material->flags.is_set(material_flags::EDIBLE_COOKED))
    {
//        out.print("%s is edible\n", plant->id.c_str());
        return selectability::Selectable;
    }

    if (plant->flags.is_set(plant_raw_flags::THREAD) ||
        plant->flags.is_set(plant_raw_flags::MILL) ||
        plant->flags.is_set(plant_raw_flags::EXTRACT_VIAL) ||
        plant->flags.is_set(plant_raw_flags::EXTRACT_BARREL) ||
        plant->flags.is_set(plant_raw_flags::EXTRACT_STILL_VIAL))
    {
//        out.print("%s is thread/mill/extract\n", plant->id.c_str());
        return selectability::Selectable;
    }

    if (basic_mat.material->reaction_product.id.size() > 0 ||
        basic_mat.material->reaction_class.size() > 0)
    {
//        out.print("%s has a reaction\n", plant->id.c_str());
        return selectability::Selectable;
    }

    for (auto i = 0; i < plant->growths.size(); i++)
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
                if (*cur_year_tick >= plant->growths[i]->timing_1 &&
                    (plant->growths[i]->timing_2 == -1 ||
                        *cur_year_tick <= plant->growths[i]->timing_2))
                {
//                     out.print("%s has an edible seed or a stockpile growth\n", plant->id.c_str());
                    return selectability::Selectable;
                }
                else
                {
                    outOfSeason = true;
                }
            }
        }
/*            else if (plant->growths[i]->behavior.bits.has_seed)  //  This code designates beans, etc. when DF doesn't, but plant gatherers still fail to collect anything, so it's useless: bug #0006940.
            {
                const DFHack::MaterialInfo seed_mat = DFHack::MaterialInfo(plant->material_defs.type_seed, plant->material_defs.idx_seed);

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
        return selectability::Nonselectable;
    }
}

command_result df_getplants (color_ostream &out, vector <string> & parameters)
{
    string plantMatStr = "";
    std::vector<selectability> plantSelections;
    std::vector<uint16_t> collectionCount;
    set<string> plantNames;
    bool deselect = false, exclude = false, treesonly = false, shrubsonly = false, all = false, verbose = false;

    int count = 0;

    plantSelections.resize(world->raws.plants.all.size());
    collectionCount.resize(world->raws.plants.all.size());

    for (auto i = 0; i < plantSelections.size(); i++)
    {
        plantSelections[i] = selectability::Unselected;
        collectionCount[i] = 0;
    }
    
    bool anyPlantsSelected = false;

    for (size_t i = 0; i < parameters.size(); i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
            return CR_WRONG_USAGE;
        else if(parameters[i] == "-t")
            treesonly = true;
        else if(parameters[i] == "-s")
            shrubsonly = true;
        else if(parameters[i] == "-c")
            deselect = true;
        else if(parameters[i] == "-x")
            exclude = true;
        else if(parameters[i] == "-a")
            all = true;
        else if(parameters[i] == "-v")
            verbose = true;
        else
            plantNames.insert(parameters[i]);
    }
    if (treesonly && shrubsonly)
    {
        out.printerr("Cannot specify both -t and -s at the same time!\n");
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
//            plantSelections[i] = selectablePlant(out, plant);
            plantSelections[i] = selectablePlant(plant);
        }
         else if (plantNames.find(plant->id) != plantNames.end())
        {
            plantNames.erase(plant->id);
//            plantSelections[i] = selectablePlant(out, plant);
            plantSelections[i] = selectablePlant(plant);
            switch (plantSelections[i])
            {
            case selectability::Grass:
            {
                out.printerr("%s is a Grass, and those can not be gathered\n", plant->id.c_str());
                break;
            }

            case selectability::Nonselectable:
            {
                out.printerr("%s does not have any parts that can be gathered\n", plant->id.c_str());
                break;
            }
            case selectability::OutOfSeason:
            {
                out.printerr("%s is out of season, with nothing that can be gathered now\n", plant->id.c_str());
                break;
            }
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

    for (auto i = 0; i < plantSelections.size(); i++)
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
//            switch (selectablePlant(out, plant))
            switch (selectablePlant(plant))
                {
            case selectability::Grass:
            case selectability::Nonselectable:
                continue;

            case selectability::OutOfSeason:
            {
                out.print("* (shrub) %s - %s is out of season\n", plant->id.c_str(), plant->name.c_str());
                break;
            }

            case selectability::Selectable:
            {
                out.print("* (%s) %s - %s\n", plant->flags.is_set(plant_raw_flags::TREE) ? "tree" : "shrub", plant->id.c_str(), plant->name.c_str());
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
        if (deselect && Designations::unmarkPlant(plant))
        {
            collectionCount[plant->material]++;
            ++count;
        }
        if (!deselect && Designations::markPlant(plant))
        {
//            out.print("Designated %s at (%i, %i, %i), %i\n", world->raws.plants.all[plant->material]->id.c_str(), plant->pos.x, plant->pos.y, plant->pos.z, i);
            collectionCount[plant->material]++;
            ++count;
        }
    }
    if (count)
    {
        if (verbose)
        {
            for (auto i = 0; i < plantSelections.size(); i++)
            {
                if (collectionCount[i] > 0)
                    out.print("Updated %i %s designations.\n", collectionCount[i], world->raws.plants.all[i]->id.c_str());
            }
            out.print("\n");
        }
    }
    out.print("Updated %d plant designations.\n", count);

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
        "  -t - Select trees only (exclude shrubs)\n"
        "  -s - Select shrubs only (exclude trees)\n"
        "  -c - Clear designations instead of setting them\n"
        "  -x - Apply selected action to all plants except those specified\n"
        "  -a - Select every type of plant (obeys -t/-s)\n"
        "  -v - Verbose: lists the number of (un)designations per plant\n"
        "Specifying both -t and -s will have no effect.\n"
        "If no plant IDs are specified, all valid plant IDs will be listed.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
