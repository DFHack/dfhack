#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/ui.h"
#include "df/building_type.h"
#include "df/building_farmplotst.h"
#include "df/buildings_other_id.h"
#include "df/global_objects.h"
#include "df/item.h"
#include "df/item_plantst.h"
#include "df/items_other_id.h"
#include "df/unit.h"
#include "df/building.h"
#include "df/plant_raw.h"
#include "df/plant_raw_flags.h"
#include "df/biome_type.h"
#include "modules/Items.h"
#include "modules/Maps.h"
#include "modules/World.h"

#include <queue>

using std::vector;
using std::string;
using std::map;
using std::set;
using std::queue;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;

static command_result autofarm(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("autofarm");

DFHACK_PLUGIN_IS_ENABLED(enabled);

const char *tagline = "Automatically handle crop selection in farm plots based on current plant stocks.";
const char *usage = (
                "``enable autofarm``: Enables the plugin\n"
                "``autofarm runonce``: Updates farm plots (one-time only)\n"
                "``autofarm status``: Prints status information\n"
                "``autofarm default 30``: Sets the default threshold\n"
                "``autofarm threshold 150 helmet_plump tail_pig``: Sets thresholds\n"
                );

class AutoFarm {
private:
    map<int, int> thresholds;
    int defaultThreshold = 50;

    map<int, int> lastCounts;

public:
    void initialize()
    {
        thresholds.clear();
        defaultThreshold = 50;

        lastCounts.clear();
    }

    void setThreshold(int id, int val)
    {
        thresholds[id] = val;
    }

    int getThreshold(int id)
    {
        return (thresholds.count(id) > 0) ? thresholds[id] : defaultThreshold;
    }

    void setDefault(int val)
    {
        defaultThreshold = val;
    }

private:
    const df::plant_raw_flags seasons[4] = { df::plant_raw_flags::SPRING, df::plant_raw_flags::SUMMER, df::plant_raw_flags::AUTUMN, df::plant_raw_flags::WINTER };

public:
    bool is_plantable(df::plant_raw* plant)
    {
        bool has_seed = plant->flags.is_set(df::plant_raw_flags::SEED);
        bool is_tree = plant->flags.is_set(df::plant_raw_flags::TREE);

        int8_t season = *df::global::cur_season;
        int harvest = (*df::global::cur_season_tick) + plant->growdur * 10;
        bool can_plant = has_seed && !is_tree && plant->flags.is_set(seasons[season]);
        while (can_plant && harvest >= 10080) {
            season = (season + 1) % 4;
            harvest -= 10080;
            can_plant = can_plant && plant->flags.is_set(seasons[season]);
        }

        return can_plant;
    }

private:
    map<int, set<df::biome_type>> plantable_plants;

    const map<df::plant_raw_flags, df::biome_type> biomeFlagMap = {
        { df::plant_raw_flags::BIOME_MOUNTAIN, df::biome_type::MOUNTAIN },
        { df::plant_raw_flags::BIOME_GLACIER, df::biome_type::GLACIER },
        { df::plant_raw_flags::BIOME_TUNDRA, df::biome_type::TUNDRA },
        { df::plant_raw_flags::BIOME_SWAMP_TEMPERATE_FRESHWATER, df::biome_type::SWAMP_TEMPERATE_FRESHWATER },
        { df::plant_raw_flags::BIOME_SWAMP_TEMPERATE_SALTWATER, df::biome_type::SWAMP_TEMPERATE_SALTWATER },
        { df::plant_raw_flags::BIOME_MARSH_TEMPERATE_FRESHWATER, df::biome_type::MARSH_TEMPERATE_FRESHWATER },
        { df::plant_raw_flags::BIOME_MARSH_TEMPERATE_SALTWATER, df::biome_type::MARSH_TEMPERATE_SALTWATER },
        { df::plant_raw_flags::BIOME_SWAMP_TROPICAL_FRESHWATER, df::biome_type::SWAMP_TROPICAL_FRESHWATER },
        { df::plant_raw_flags::BIOME_SWAMP_TROPICAL_SALTWATER, df::biome_type::SWAMP_TROPICAL_SALTWATER },
        { df::plant_raw_flags::BIOME_SWAMP_MANGROVE, df::biome_type::SWAMP_MANGROVE },
        { df::plant_raw_flags::BIOME_MARSH_TROPICAL_FRESHWATER, df::biome_type::MARSH_TROPICAL_FRESHWATER },
        { df::plant_raw_flags::BIOME_MARSH_TROPICAL_SALTWATER, df::biome_type::MARSH_TROPICAL_SALTWATER },
        { df::plant_raw_flags::BIOME_FOREST_TAIGA, df::biome_type::FOREST_TAIGA },
        { df::plant_raw_flags::BIOME_FOREST_TEMPERATE_CONIFER, df::biome_type::FOREST_TEMPERATE_CONIFER },
        { df::plant_raw_flags::BIOME_FOREST_TEMPERATE_BROADLEAF, df::biome_type::FOREST_TEMPERATE_BROADLEAF },
        { df::plant_raw_flags::BIOME_FOREST_TROPICAL_CONIFER, df::biome_type::FOREST_TROPICAL_CONIFER },
        { df::plant_raw_flags::BIOME_FOREST_TROPICAL_DRY_BROADLEAF, df::biome_type::FOREST_TROPICAL_DRY_BROADLEAF },
        { df::plant_raw_flags::BIOME_FOREST_TROPICAL_MOIST_BROADLEAF, df::biome_type::FOREST_TROPICAL_MOIST_BROADLEAF },
        { df::plant_raw_flags::BIOME_GRASSLAND_TEMPERATE, df::biome_type::GRASSLAND_TEMPERATE },
        { df::plant_raw_flags::BIOME_SAVANNA_TEMPERATE, df::biome_type::SAVANNA_TEMPERATE },
        { df::plant_raw_flags::BIOME_SHRUBLAND_TEMPERATE, df::biome_type::SHRUBLAND_TEMPERATE },
        { df::plant_raw_flags::BIOME_GRASSLAND_TROPICAL, df::biome_type::GRASSLAND_TROPICAL },
        { df::plant_raw_flags::BIOME_SAVANNA_TROPICAL, df::biome_type::SAVANNA_TROPICAL },
        { df::plant_raw_flags::BIOME_SHRUBLAND_TROPICAL, df::biome_type::SHRUBLAND_TROPICAL },
        { df::plant_raw_flags::BIOME_DESERT_BADLAND, df::biome_type::DESERT_BADLAND },
        { df::plant_raw_flags::BIOME_DESERT_ROCK, df::biome_type::DESERT_ROCK },
        { df::plant_raw_flags::BIOME_DESERT_SAND, df::biome_type::DESERT_SAND },
        { df::plant_raw_flags::BIOME_OCEAN_TROPICAL, df::biome_type::OCEAN_TROPICAL },
        { df::plant_raw_flags::BIOME_OCEAN_TEMPERATE, df::biome_type::OCEAN_TEMPERATE },
        { df::plant_raw_flags::BIOME_OCEAN_ARCTIC, df::biome_type::OCEAN_ARCTIC },
        { df::plant_raw_flags::BIOME_POOL_TEMPERATE_FRESHWATER, df::biome_type::POOL_TEMPERATE_FRESHWATER },
        { df::plant_raw_flags::BIOME_POOL_TEMPERATE_BRACKISHWATER, df::biome_type::POOL_TEMPERATE_BRACKISHWATER },
        { df::plant_raw_flags::BIOME_POOL_TEMPERATE_SALTWATER, df::biome_type::POOL_TEMPERATE_SALTWATER },
        { df::plant_raw_flags::BIOME_POOL_TROPICAL_FRESHWATER, df::biome_type::POOL_TROPICAL_FRESHWATER },
        { df::plant_raw_flags::BIOME_POOL_TROPICAL_BRACKISHWATER, df::biome_type::POOL_TROPICAL_BRACKISHWATER },
        { df::plant_raw_flags::BIOME_POOL_TROPICAL_SALTWATER, df::biome_type::POOL_TROPICAL_SALTWATER },
        { df::plant_raw_flags::BIOME_LAKE_TEMPERATE_FRESHWATER, df::biome_type::LAKE_TEMPERATE_FRESHWATER },
        { df::plant_raw_flags::BIOME_LAKE_TEMPERATE_BRACKISHWATER, df::biome_type::LAKE_TEMPERATE_BRACKISHWATER },
        { df::plant_raw_flags::BIOME_LAKE_TEMPERATE_SALTWATER, df::biome_type::LAKE_TEMPERATE_SALTWATER },
        { df::plant_raw_flags::BIOME_LAKE_TROPICAL_FRESHWATER, df::biome_type::LAKE_TROPICAL_FRESHWATER },
        { df::plant_raw_flags::BIOME_LAKE_TROPICAL_BRACKISHWATER, df::biome_type::LAKE_TROPICAL_BRACKISHWATER },
        { df::plant_raw_flags::BIOME_LAKE_TROPICAL_SALTWATER, df::biome_type::LAKE_TROPICAL_SALTWATER },
        { df::plant_raw_flags::BIOME_RIVER_TEMPERATE_FRESHWATER, df::biome_type::RIVER_TEMPERATE_FRESHWATER },
        { df::plant_raw_flags::BIOME_RIVER_TEMPERATE_BRACKISHWATER, df::biome_type::RIVER_TEMPERATE_BRACKISHWATER },
        { df::plant_raw_flags::BIOME_RIVER_TEMPERATE_SALTWATER, df::biome_type::RIVER_TEMPERATE_SALTWATER },
        { df::plant_raw_flags::BIOME_RIVER_TROPICAL_FRESHWATER, df::biome_type::RIVER_TROPICAL_FRESHWATER },
        { df::plant_raw_flags::BIOME_RIVER_TROPICAL_BRACKISHWATER, df::biome_type::RIVER_TROPICAL_BRACKISHWATER },
        { df::plant_raw_flags::BIOME_RIVER_TROPICAL_SALTWATER, df::biome_type::RIVER_TROPICAL_SALTWATER },
        { df::plant_raw_flags::BIOME_SUBTERRANEAN_WATER, df::biome_type::SUBTERRANEAN_WATER },
        { df::plant_raw_flags::BIOME_SUBTERRANEAN_CHASM, df::biome_type::SUBTERRANEAN_CHASM },
        { df::plant_raw_flags::BIOME_SUBTERRANEAN_LAVA, df::biome_type::SUBTERRANEAN_LAVA }
    };


public:
    void find_plantable_plants()
    {
        plantable_plants.clear();

        map<int, int> counts;

        df::item_flags bad_flags;
        bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
        F(dump); F(forbid); F(garbage_collect);
        F(hostile); F(on_fire); F(rotten); F(trader);
        F(in_building); F(construction); F(artifact);
#undef F

        for (auto ii : world->items.other[df::items_other_id::SEEDS])
        {
            df::item_plantst* i = (df::item_plantst*)ii;
            if ((i->flags.whole & bad_flags.whole) == 0)
                counts[i->mat_index] += i->stack_size;
        }

        for (auto ci : counts)
        {
            if (df::global::ui->tasks.discovered_plants[ci.first])
            {
                df::plant_raw* plant = world->raws.plants.all[ci.first];
                if (is_plantable(plant))
                    for (auto flagmap : biomeFlagMap)
                        if (plant->flags.is_set(flagmap.first))
                            plantable_plants[plant->index].insert(flagmap.second);
            }
        }
    }

    void set_farms(color_ostream& out, set<int> plants, vector<df::building_farmplotst*> farms)
    {
        // this algorithm attempts to change as few farms as possible, while ensuring that
        // the number of farms planting each eligible plant is "as equal as possible"

        if (farms.empty() || plants.empty())
            return; // do nothing if there are no farms or no plantable plants

        int season = *df::global::cur_season;

        int min = farms.size() / plants.size(); // the number of farms that should plant each eligible plant, rounded down
        int extra = farms.size() - min * plants.size(); // the remainder that cannot be evenly divided

        map<int, int> counters;
        counters.empty();

        queue<df::building_farmplotst*> toChange;
        toChange.empty();

        for (auto farm : farms)
        {
            int o = farm->plant_id[season];
            if (plants.count(o)==0 || counters[o] > min || (counters[o] == min && extra == 0))
                toChange.push(farm); // this farm is an excess instance for the plant it is currently planting
            else
            {
                if (counters[o] == min)
                    extra--; // allocate off one of the remainder farms
                counters[o]++;
            }
        }

        for (auto n : plants)
        {
            int c = counters[n];
            while (toChange.size() > 0 && (c < min || (c == min && extra > 0)))
            {
                // pick one of the excess farms and change it to plant this plant
                df::building_farmplotst* farm = toChange.front();
                int o = farm->plant_id[season];
                farm->plant_id[season] = n;
                out << "autofarm: changing farm #" << farm->id <<
                    " from " << ((o == -1) ? "NONE" : world->raws.plants.all[o]->name) <<
                    " to " << ((n == -1) ? "NONE" : world->raws.plants.all[n]->name) << endl;
                toChange.pop();
                if (c++ == min)
                    extra--;
            }
        }
    }

    void process(color_ostream& out)
    {
        if (!enabled)
            return;

        find_plantable_plants();

        lastCounts.clear();

        df::item_flags bad_flags;
        bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
        F(dump); F(forbid); F(garbage_collect);
        F(hostile); F(on_fire); F(rotten); F(trader);
        F(in_building); F(construction); F(artifact);
#undef F

        for (auto ii : world->items.other[df::items_other_id::PLANT])
        {
            df::item_plantst* i = (df::item_plantst*)ii;
            if ((i->flags.whole & bad_flags.whole) == 0 &&
                plantable_plants.count(i->mat_index) > 0)
            {
                lastCounts[i->mat_index] += i->stack_size;
            }
        }

        map<df::biome_type, set<int>> plants;
        plants.clear();

        for (auto plantable : plantable_plants)
        {
            df::plant_raw* plant = world->raws.plants.all[plantable.first];
            if (lastCounts[plant->index] < getThreshold(plant->index))
                for (auto biome : plantable.second)
                {
                    plants[biome].insert(plant->index);
                }
        }

        map<df::biome_type, vector<df::building_farmplotst*>> farms;
        farms.clear();

        for (auto bb : world->buildings.other[df::buildings_other_id::FARM_PLOT])
        {
            df::building_farmplotst* farm = (df::building_farmplotst*) bb;
            if (farm->flags.bits.exists)
            {
                df::biome_type biome;
                if (Maps::getTileDesignation(bb->centerx, bb->centery, bb->z)->bits.subterranean)
                    biome = biome_type::SUBTERRANEAN_WATER;
                else {
                    df::coord2d region(Maps::getTileBiomeRgn(df::coord(bb->centerx, bb->centery, bb->z)));
                    biome = Maps::GetBiomeType(region.x, region.y);
                }
                farms[biome].push_back(farm);
            }
        }

        for (auto ff : farms)
        {
            set_farms(out, plants[ff.first], ff.second);
        }
    }

    void status(color_ostream& out)
    {
        out << (enabled ? "Running." : "Stopped.") << endl;
        for (auto lc : lastCounts)
        {
            auto plant = world->raws.plants.all[lc.first];
            out << plant->id << " limit " << getThreshold(lc.first) << " current " << lc.second << endl;
        }

        for (auto th : thresholds)
        {
            if (lastCounts[th.first] > 0)
                continue;
            auto plant = world->raws.plants.all[th.first];
            out << plant->id << " limit " << getThreshold(th.first) << " current 0" << endl;
        }
        out << "Default: " << defaultThreshold << endl;
    }
};

static AutoFarm* autofarmInstance;


DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (world && ui) {
        commands.push_back(
            PluginCommand("autofarm", tagline,
                autofarm, false, usage
            )
        );
    }
    autofarmInstance = new AutoFarm();
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    delete autofarmInstance;

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (!autofarmInstance)
        return CR_OK;

    if (!Maps::IsValid())
        return CR_OK;

    if (DFHack::World::ReadPauseState())
        return CR_OK;

    if (world->frame_counter % 50 != 0) // Check every hour
        return CR_OK;

    {
        CoreSuspender suspend;
        autofarmInstance->process(out);
    }

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    enabled = enable;
    return CR_OK;
}

static command_result setThresholds(color_ostream& out, vector<string> & parameters)
{
    int val = atoi(parameters[1].c_str());
    for (size_t i = 2; i < parameters.size(); i++)
    {
        string id = parameters[i];
        transform(id.begin(), id.end(), id.begin(), ::toupper);

        bool ok = false;
        for (auto plant : world->raws.plants.all)
        {
            if (plant->flags.is_set(df::plant_raw_flags::SEED) && (plant->id == id))
            {
                autofarmInstance->setThreshold(plant->index, val);
                ok = true;
                break;
            }
        }
        if (!ok)
        {
            out << "Cannot find plant with id " << id << endl;
            return CR_WRONG_USAGE;
        }
    }
    return CR_OK;
}

static command_result autofarm(color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    if (parameters.size() == 1 && parameters[0] == "runonce")
        autofarmInstance->process(out);
    else if (parameters.size() == 1 && parameters[0] == "enable")
        plugin_enable(out, true);
    else if (parameters.size() == 1 && parameters[0] == "disable")
        plugin_enable(out, false);
    else if (parameters.size() == 2 && parameters[0] == "default")
        autofarmInstance->setDefault(atoi(parameters[1].c_str()));
    else if (parameters.size() >= 3 && parameters[0] == "threshold")
        return setThresholds(out, parameters);
    else if (parameters.size() == 0 || (parameters.size() == 1 && parameters[0] == "status"))
        autofarmInstance->status(out);
    else
        return CR_WRONG_USAGE;

    return CR_OK;
}

