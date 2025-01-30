#include "Debug.h"
#include "PluginManager.h"

#include "modules/Items.h"
#include "modules/Maps.h"
#include "modules/World.h"

#include "df/biome_type.h"
#include "df/building.h"
#include "df/building_farmplotst.h"
#include "df/building_type.h"
#include "df/buildings_other_id.h"
#include "df/global_objects.h"
#include "df/item.h"
#include "df/item_plantst.h"
#include "df/item_plant_growthst.h"
#include "df/item_seedsst.h"
#include "df/items_other_id.h"
#include "df/plant_raw.h"
#include "df/plant_raw_flags.h"
#include "df/plotinfost.h"
#include "df/unit.h"
#include "df/world.h"

#include <queue>

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("autofarm");
DFHACK_PLUGIN_IS_ENABLED(enabled);

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(plotinfo);

static command_result autofarm(color_ostream& out, std::vector<std::string>& parameters);

#define CONFIG_KEY "autofarm/config"

namespace DFHack {
    DBG_DECLARE(autofarm, cycle, DebugCategory::LINFO);
    DBG_DECLARE(autofarm, control, DebugCategory::LINFO);
}

static const int32_t CYCLE_TICKS = 53; // one hour-ish
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

class AutoFarm {
private:
    std::map<int, int> thresholds;
    int defaultThreshold = 50;

    std::map<int, int> lastCounts;

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
    std::map<int, std::set<df::biome_type>> plantable_plants;

    const std::map<df::plant_raw_flags, df::biome_type> biomeFlagMap = {
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

        std::map<int, int> counts;

        const uint32_t bad_flags{
#define F(x) (df::item_flags::Mask::mask_##x)
        F(dump) | F(forbid) | F(garbage_collect) |
        F(hostile) | F(on_fire) | F(rotten) | F(trader) |
        F(in_building) | F(construction) | F(artifact)
#undef F
        };

        for (auto& ii : world->items.other[df::items_other_id::SEEDS])
        {
            auto i = virtual_cast<df::item_seedsst>(ii);
            if (i && (i->flags.whole & bad_flags) == 0)
                counts[i->mat_index] += i->stack_size;
        }

        for (auto& ci : counts)
        {
            df::plant_raw* plant = world->raws.plants.all[ci.first];
            if (is_plantable(plant))
                for (auto& flagmap : biomeFlagMap)
                    if (plant->flags.is_set(flagmap.first))
                        plantable_plants[plant->index].insert(flagmap.second);
        }
    }

    std::string get_plant_name(int plant_id)
    {
        df::plant_raw* raw = df::plant_raw::find(plant_id);
        return raw ? raw->name : "NONE";
    }

    void set_farm(color_ostream& out, int new_plant_id, df::building_farmplotst* farm, int season)
    {
        int old_plant_id = farm->plant_id[season];
        if (old_plant_id != new_plant_id)
        {
            farm->plant_id[season] = new_plant_id;
            INFO(cycle, out).print("autofarm: changing farm #%d from %s to %s\n", farm->id,
                get_plant_name(old_plant_id).c_str(),
                get_plant_name(new_plant_id).c_str());
        }
    }

    void set_farms(color_ostream& out, const std::set<int>& plants, const std::vector<df::building_farmplotst*>& farms)
    {
        // this algorithm attempts to change as few farms as possible, while ensuring that
        // the number of farms planting each eligible plant is "as equal as possible"

        int season = *df::global::cur_season;

        if (farms.empty() || plants.empty())
        {
            // if no more plants were requested, fallow all farms
            // if there were no farms, do nothing
            for (auto farm : farms)
            {
                set_farm(out, -1, farm, season);
            }
            return;
        }

        int min = farms.size() / plants.size(); // the number of farms that should plant each eligible plant, rounded down
        int extra = farms.size() - min * plants.size(); // the remainder that cannot be evenly divided

        std::map<int, int> counters;
        std::queue<df::building_farmplotst*> toChange;

        for (auto farm : farms)
        {
            int o = farm->plant_id[season];
            if (plants.count(o) == 0 || counters[o] > min || (counters[o] == min && extra == 0))
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
                set_farm(out, n, farm, season);
                toChange.pop();
                if (c++ == min)
                    extra--;
            }
        }
    }

    void process(color_ostream& out)
    {
        // mark that we have recently run
        cycle_timestamp = world->frame_counter;

        find_plantable_plants();

        lastCounts.clear();

        const uint32_t bad_flags{
 #define F(x) (df::item_flags::Mask::mask_##x)
         F(dump) | F(forbid) | F(garbage_collect) |
         F(hostile) | F(on_fire) | F(rotten) | F(trader) |
         F(in_building) | F(construction) | F(artifact)
 #undef F
        };

        // have to scan both items[PLANT] and items[PLANT_GROWTH] because agricultural products can be either

        auto count = [&, this](df::item* i) {
            auto mat = i->getMaterialIndex();
            if ((i->flags.whole & bad_flags) == 0 &&
                plantable_plants.count(mat) > 0)
            {
                lastCounts[mat] += i->getStackSize();
            }
        };

        for (auto i : world->items.other[df::items_other_id::PLANT])
            count(i);
        for (auto i : world->items.other[df::items_other_id::PLANT_GROWTH])
            count(i);

        std::map<df::biome_type, std::set<int>> plants;

        for (auto& plantable : plantable_plants)
        {
            df::plant_raw* plant = world->raws.plants.all[plantable.first];
            if (lastCounts[plant->index] < getThreshold(plant->index))
                for (auto biome : plantable.second)
                {
                    plants[biome].insert(plant->index);
                }
        }

        std::map<df::biome_type, std::vector<df::building_farmplotst*>> farms;

        for (auto& bb : world->buildings.other[df::buildings_other_id::FARM_PLOT])
        {
            auto farm = virtual_cast<df::building_farmplotst>(bb);
            if (farm->flags.bits.exists)
            {
                df::biome_type biome;
                if (Maps::getTileDesignation(bb->centerx, bb->centery, bb->z)->bits.subterranean)
                    biome = biome_type::SUBTERRANEAN_WATER;
                else {
                    df::coord2d region(Maps::getTileBiomeRgn(df::coord(bb->centerx, bb->centery, bb->z)));
                    biome = Maps::getBiomeType(region.x, region.y);
                }
                farms[biome].push_back(farm);
            }
        }

        for (auto& ff : farms)
        {
            set_farms(out, plants[ff.first], ff.second);
        }

        out << std::flush;
    }

    void status(color_ostream& out)
    {
        out << "Autofarm is " << (enabled ? "Active." : "Stopped.") << '\n';

        for (auto& lc : lastCounts)
        {
            auto plant = world->raws.plants.all[lc.first];
            out << plant->id << " limit " << getThreshold(lc.first) << " current " << lc.second << '\n';
        }

        for (auto& th : thresholds)
        {
            if (lastCounts.count(th.first) > 0)
                continue;
            auto plant = world->raws.plants.all[th.first];
            out << plant->id << " limit " << getThreshold(th.first) << " current 0" << '\n';
        }
        out << "Default: " << defaultThreshold << '\n';

        out << std::flush;
    }

private:

    PersistentDataItem cfg_default_threshold;
    PersistentDataItem cfg_enabled;

public:
    void load_state(color_ostream& out)
    {
        initialize();

        cfg_enabled = World::GetPersistentSiteData("autofarm/enabled");
        if (cfg_enabled.isValid())
            enabled = cfg_enabled.ival(0) != 0;
        else {
            cfg_enabled = World::AddPersistentSiteData("autofarm/enabled");
            cfg_enabled.ival(0) = enabled;
        }

        cfg_default_threshold = World::GetPersistentSiteData("autofarm/default_threshold");

        if (cfg_default_threshold.isValid())
            defaultThreshold = cfg_default_threshold.ival(0);
        else {
            cfg_default_threshold = World::AddPersistentSiteData("autofarm/default_threshold");
            cfg_default_threshold.ival(0) = defaultThreshold;
        }

        std::vector<PersistentDataItem> items;
        World::GetPersistentSiteData(&items, "autofarm/threshold/", true);
        for (auto& i: items) {
            if (i.isValid())
            {
                const auto allPlants = world->raws.plants.all;
                const std::string id = i.val();
                const int val = i.ival(0);
                const auto plant = std::find_if(std::begin(allPlants), std::end(allPlants), [id](df::plant_raw* p) { return p->id == id; });
                if (plant != std::end(allPlants))
                {
                    setThreshold((*plant)->index, val);
                    INFO(control, out).print("threshold of %d for plant %s in saved configuration loaded\n", val, id.c_str());
                }
                else
                {
                    WARN(control, out).print("threshold for unknown plant %s in saved configuration ignored\n", id.c_str());
                }
            }
        }

    }

    void save_state(color_ostream& out)
    {
        cfg_default_threshold.ival(0) = defaultThreshold;
        cfg_enabled.ival(0) = enabled;

        std::vector<PersistentDataItem> items;
        World::GetPersistentSiteData(&items, "autofarm/threshold/", true);
        for (auto& i : items)
            World::DeletePersistentData(i);

        for (const auto& t : thresholds)
        {
            const std::string& plantID = world->raws.plants.all[t.first]->id;
            const std::string keyName = "autofarm/threshold/" + plantID;
            PersistentDataItem cfgThreshold = World::AddPersistentSiteData(keyName);
            cfgThreshold.val() = plantID;
            cfgThreshold.ival(0) = t.second;
        }
    }

};

static std::unique_ptr<AutoFarm> autofarmInstance;


DFhackCExport command_result plugin_init(color_ostream& out, std::vector <PluginCommand>& commands)
{
    commands.push_back(
        PluginCommand("autofarm",
                        "Automatically manage farm crop selection.",
                        autofarm));

    autofarmInstance = std::move(std::make_unique<AutoFarm>());
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream& out)
{
    autofarmInstance.release();
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream& out)
{
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        autofarmInstance->process(out);

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream& out, bool enable)
{
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    enabled = enable;
    autofarmInstance->save_state(out);
    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    cycle_timestamp = 0;
    autofarmInstance->load_state(out);
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream& out, state_change_event event)
{
    switch (event) {
    case SC_WORLD_UNLOADED:
        enabled = false;
        break;
    default:
        break;
    }

    return CR_OK;
}

static command_result setThresholds(color_ostream& out, std::vector<std::string>& parameters)
{
    int val = atoi(parameters[1].c_str());
    for (size_t i = 2; i < parameters.size(); i++)
    {
        std::string id = parameters[i];
        std::transform(id.begin(), id.end(), id.begin(), ::toupper);

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
            out << "Cannot find plant with id " << id << '\n' << std::flush;
            return CR_WRONG_USAGE;
        }
    }
    autofarmInstance->save_state(out);
    return CR_OK;
}

static command_result autofarm(color_ostream& out, std::vector<std::string>& parameters)
{
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (parameters.size() == 1 && parameters[0] == "runonce")
    {
        if (autofarmInstance) autofarmInstance->process(out);
    }
    else if (parameters.size() == 1 && parameters[0] == "enable")
        plugin_enable(out, true);
    else if (parameters.size() == 1 && parameters[0] == "disable")
        plugin_enable(out, false);
    else if (parameters.size() == 2 && parameters[0] == "default")
    {
        if (autofarmInstance)
        {
            autofarmInstance->setDefault(atoi(parameters[1].c_str()));
            autofarmInstance->save_state(out);
        }
    }
    else if (parameters.size() >= 3 && parameters[0] == "threshold")
    {
        if (autofarmInstance)
            return setThresholds(out, parameters);
    }
    else if (parameters.size() == 0 || (parameters.size() == 1 && parameters[0] == "status"))
        autofarmInstance->status(out);
    else
        return CR_WRONG_USAGE;

    return CR_OK;
}
