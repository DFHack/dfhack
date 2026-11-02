#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"

#include "modules/Buildings.h"
#include "modules/Items.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
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
#include "df/material.h"
#include "df/material_flags.h"
#include "df/plant_growth.h"
#include "df/plant_raw.h"
#include "df/plant_raw_flags.h"
#include "df/plotinfost.h"
#include "df/unit.h"
#include "df/world.h"

#include <algorithm>
#include <set>

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

    // thresholds for specific growth products of a plant, keyed by
    // (plant index, growth id). growth items whose growth has no explicit
    // threshold count toward the plant's pooled threshold instead
    std::map<std::pair<int, std::string>, int> growth_thresholds;

    std::map<int, int> lastCounts;
    // stockpile growth item counts, keyed by (plant index, growth id).
    // populated for reporting; untracked growths also count toward lastCounts
    std::map<std::pair<int, std::string>, int> lastGrowthCounts;

public:
    void initialize()
    {
        thresholds.clear();
        defaultThreshold = 50;

        growth_thresholds.clear();

        lastCounts.clear();
        lastGrowthCounts.clear();
    }

    void setThreshold(int id, int val)
    {
        thresholds[id] = val;
    }

    int getThreshold(int id)
    {
        return (thresholds.count(id) > 0) ? thresholds[id] : defaultThreshold;
    }

    void removeThreshold(int id)
    {
        thresholds.erase(id);
    }

    void setGrowthThreshold(int plant_id, const std::string& growth_id, int val)
    {
        growth_thresholds[std::make_pair(plant_id, growth_id)] = val;
    }

    void removeGrowthThreshold(int plant_id, const std::string& growth_id)
    {
        growth_thresholds.erase(std::make_pair(plant_id, growth_id));
    }

    const std::map<std::pair<int, std::string>, int>& getGrowthThresholds()
    {
        return growth_thresholds;
    }

    const std::map<std::pair<int, std::string>, int>& getLastGrowthCounts()
    {
        return lastGrowthCounts;
    }

    int getDefault()
    {
        return defaultThreshold;
    }

    const std::map<int, int>& getThresholds()
    {
        return thresholds;
    }

    const std::map<int, int>& getLastCounts()
    {
        return lastCounts;
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

    // whether the growth's material produces stockpilable growth items;
    // only these are ever produced by processing jobs and gathering, so
    // they are the only growths that can be meaningfully tracked
    static bool is_stockpile_growth(df::plant_growth* growth)
    {
        MaterialInfo mat(growth->mat_type, growth->mat_index);
        return mat.isValid() &&
            mat.material->flags.is_set(df::material_flags::STOCKPILE_PLANT_GROWTH);
    }

    // the id of the stockpile growth at the given index, or "" if the
    // growth doesn't exist or isn't stockpilable
    static std::string stockpile_growth_id(df::plant_raw* plant, int32_t subtype)
    {
        if (subtype >= 0 && (size_t)subtype < plant->growths.size() &&
            is_stockpile_growth(plant->growths[subtype]))
            return plant->growths[subtype]->id;
        return "";
    }

    static bool has_stockpile_growth(df::plant_raw* plant, const std::string& growth_id)
    {
        for (auto g : plant->growths)
            if (g->id == growth_id && is_stockpile_growth(g))
                return true;
        return false;
    }

    // gathers the set of biomes covered by the farm's tiles, since a farm may
    // span multiple biomes and DF allows planting any crop that can grow in
    // any of them (in the tiles of the respective biome)
    std::set<df::biome_type> get_farm_biomes(df::building_farmplotst* farm)
    {
        std::set<df::biome_type> biomes;
        for (int32_t x = farm->x1; x <= farm->x2; x++)
        {
            for (int32_t y = farm->y1; y <= farm->y2; y++)
            {
                if (!Buildings::containsTile(farm, df::coord2d(x, y)))
                    continue;
                auto des = Maps::getTileDesignation(x, y, farm->z);
                if (!des)
                    continue;
                if (des->bits.subterranean)
                {
                    // any subterranean tile can grow any crop that grows
                    // underground
                    biomes.insert(df::biome_type::SUBTERRANEAN_WATER);
                    biomes.insert(df::biome_type::SUBTERRANEAN_CHASM);
                    biomes.insert(df::biome_type::SUBTERRANEAN_LAVA);
                }
                else if (Maps::getRegionBiome(Maps::getTileBiomeRgn(df::coord(x, y, farm->z))))
                {
                    df::coord2d region(Maps::getTileBiomeRgn(df::coord(x, y, farm->z)));
                    biomes.insert(Maps::getBiomeType(region.x, region.y));
                }
            }
        }
        return biomes;
    }

public:
    bool is_farm_excluded(int32_t farm_id)
    {
        auto item = World::GetPersistentSiteData("autofarm/excluded_farm/" + std::to_string(farm_id));
        return item.isValid() && item.ival(0) != 0;
    }

    void set_farm_excluded(int32_t farm_id, bool excluded)
    {
        const std::string key = "autofarm/excluded_farm/" + std::to_string(farm_id);
        auto item = World::GetPersistentSiteData(key);
        if (excluded)
        {
            if (!item.isValid())
                item = World::AddPersistentSiteData(key);
            if (item.isValid())
                item.ival(0) = 1;
        }
        else if (item.isValid())
        {
            World::DeletePersistentData(item);
        }
    }

    std::vector<int32_t> get_excluded_farms()
    {
        std::vector<PersistentDataItem> items;
        std::vector<int32_t> ret;
        World::GetPersistentSiteData(&items, "autofarm/excluded_farm/", true);
        for (auto& i : items)
        {
            if (!i.isValid() || i.ival(0) == 0)
                continue;
            int32_t farm_id = atoi(i.key().substr(strlen("autofarm/excluded_farm/")).c_str());
            auto bld = df::building::find(farm_id);
            // ignore entries left behind by destroyed farm plots
            if (bld && bld->getType() == df::building_type::FarmPlot)
                ret.push_back(farm_id);
        }
        return ret;
    }

private:
    void set_farm(color_ostream& out, int new_plant_id, df::building_farmplotst* farm, int season)
    {
        int old_plant_id = farm->plant_id[season];
        if (old_plant_id != new_plant_id)
        {
            farm->plant_id[season] = new_plant_id;
            INFO(cycle, out).print("autofarm: changing farm #{} from {} to {}\n", farm->id,
                get_plant_name(old_plant_id),
                get_plant_name(new_plant_id));
        }
    }

public:
    void set_farms(color_ostream& out, const std::map<df::biome_type, std::set<int>>& plants,
                   const std::vector<std::pair<df::building_farmplotst*, std::set<df::biome_type>>>& farms)
    {
        // this algorithm attempts to change as few farms as possible, while
        // ensuring that the number of farms planting each plant is "as equal
        // as possible". unlike previously, each farm may have a different set
        // of plants that it can grow, since a farm can span multiple biomes

        int season = *df::global::cur_season;

        // the plants that each farm is eligible to grow, and the list of
        // farms assigned to each plant
        std::map<df::building_farmplotst*, std::set<int>> eligible;
        std::map<int, std::vector<df::building_farmplotst*>> assigned;
        std::map<int, int> counters;
        std::vector<df::building_farmplotst*> unassigned;

        for (auto& [farm, biomes] : farms)
        {
            for (auto biome : biomes)
            {
                auto it = plants.find(biome);
                if (it != plants.end())
                    eligible[farm].insert(it->second.begin(), it->second.end());
            }
            for (auto plant : eligible[farm])
                assigned[plant]; // ensure every eligible plant can receive farms
        }

        // keep the current assignment for farms whose crop is still eligible
        for (auto& [farm, biomes] : farms)
        {
            int cur = farm->plant_id[season];
            if (cur >= 0 && eligible[farm].count(cur))
            {
                assigned[cur].push_back(farm);
                counters[cur]++;
            }
            else
            {
                unassigned.push_back(farm);
            }
        }

        // assign farms with the fewest options first so that constrained
        // farms don't get crowded out by more flexible ones
        std::sort(unassigned.begin(), unassigned.end(),
            [&](df::building_farmplotst* a, df::building_farmplotst* b)
            {
                return eligible[a].size() < eligible[b].size();
            });

        for (auto farm : unassigned)
        {
            if (eligible[farm].empty())
            {
                set_farm(out, -1, farm, season);
                continue;
            }
            int best = *std::min_element(eligible[farm].begin(), eligible[farm].end(),
                [&](int a, int b) { return counters[a] < counters[b]; });
            set_farm(out, best, farm, season);
            assigned[best].push_back(farm);
            counters[best]++;
        }

        // rebalance: move farms from overrepresented plants to
        // underrepresented ones while their counts differ by more than one.
        // each move strictly decreases the variance, so this terminates
        for (bool moved = true; moved;)
        {
            moved = false;
            for (auto& [src, src_farms] : assigned)
            {
                for (auto& [dst, dst_farms] : assigned)
                {
                    if (counters[src] <= counters[dst] + 1)
                        continue;
                    auto it = std::find_if(src_farms.begin(), src_farms.end(),
                        [&](df::building_farmplotst* f) { return eligible[f].count(dst); });
                    if (it == src_farms.end())
                        continue;
                    auto farm = *it;
                    set_farm(out, dst, farm, season);
                    src_farms.erase(it);
                    dst_farms.push_back(farm);
                    counters[src]--;
                    counters[dst]++;
                    moved = true;
                    break;
                }
                if (moved)
                    break;
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

        lastGrowthCounts.clear();

        auto count_plant = [&, this](df::item* i) {
            auto mat = i->getMaterialIndex();
            if ((i->flags.whole & bad_flags) == 0 &&
                plantable_plants.count(mat) > 0)
            {
                lastCounts[mat] += i->getStackSize();
            }
        };

        auto count_growth = [&, this](df::item* i) {
            auto g = virtual_cast<df::item_plant_growthst>(i);
            auto mat = i->getMaterialIndex();
            if (!g || (i->flags.whole & bad_flags) != 0 ||
                plantable_plants.count(mat) == 0)
                return;
            df::plant_raw* plant = world->raws.plants.all[mat];
            const std::string gid = stockpile_growth_id(plant, g->subtype);
            const auto key = std::make_pair(mat, gid);
            if (!gid.empty() && growth_thresholds.count(key))
            {
                // tracked growths count only toward their own threshold
                lastGrowthCounts[key] += i->getStackSize();
            }
            else
            {
                if (!gid.empty())
                    // unconfigured stockpile growths are tracked
                    // separately for reporting, but still count toward
                    // the plant's pooled threshold
                    lastGrowthCounts[key] += i->getStackSize();
                lastCounts[mat] += i->getStackSize();
            }
        };

        for (auto i : world->items.other[df::items_other_id::PLANT])
            count_plant(i);
        for (auto i : world->items.other[df::items_other_id::PLANT_GROWTH])
            count_growth(i);

        std::map<df::biome_type, std::set<int>> plants;

        for (auto& plantable : plantable_plants)
        {
            df::plant_raw* plant = world->raws.plants.all[plantable.first];
            // a plant is in demand when its pooled stock is under threshold
            // or any explicitly tracked growth of it is under threshold
            bool under = lastCounts[plant->index] < getThreshold(plant->index);
            if (!under)
                for (auto& gt : growth_thresholds)
                {
                    if (gt.first.first == plant->index &&
                        lastGrowthCounts[gt.first] < gt.second)
                    {
                        under = true;
                        break;
                    }
                }
            if (under)
                for (auto biome : plantable.second)
                {
                    plants[biome].insert(plant->index);
                }
        }

        std::vector<std::pair<df::building_farmplotst*, std::set<df::biome_type>>> farms;

        for (auto& bb : world->buildings.other[df::buildings_other_id::FARM_PLOT])
        {
            auto farm = virtual_cast<df::building_farmplotst>(bb);
            if (farm->flags.bits.exists && !is_farm_excluded(farm->id))
            {
                farms.emplace_back(farm, get_farm_biomes(farm));
            }
        }

        set_farms(out, plants, farms);

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

        for (auto& gt : growth_thresholds)
        {
            auto plant = world->raws.plants.all[gt.first.first];
            out << plant->id << ':' << gt.first.second << " limit " << gt.second
                << " current " << lastGrowthCounts[gt.first] << '\n';
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
                const auto colon = id.find(':');
                const std::string plant_id = id.substr(0, colon);
                const auto plant = std::find_if(std::begin(allPlants), std::end(allPlants), [plant_id](df::plant_raw* p) { return p->id == plant_id; });
                if (plant == std::end(allPlants))
                {
                    WARN(control, out).print("threshold for unknown plant {} in saved configuration ignored\n", id);
                }
                else if (colon == std::string::npos)
                {
                    setThreshold((*plant)->index, val);
                    INFO(control, out).print("threshold of {} for plant {} in saved configuration loaded\n", val, id);
                }
                else
                {
                    // "PLANT:GROWTH" entries hold thresholds for specific
                    // stockpile growth products of the plant
                    const std::string growth_id = id.substr(colon + 1);
                    if (has_stockpile_growth(*plant, growth_id))
                    {
                        setGrowthThreshold((*plant)->index, growth_id, val);
                        INFO(control, out).print("threshold of {} for growth {} in saved configuration loaded\n", val, id);
                    }
                    else
                    {
                        WARN(control, out).print("threshold for unknown growth {} in saved configuration ignored\n", id);
                    }
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

        for (const auto& t : growth_thresholds)
        {
            const std::string productID = world->raws.plants.all[t.first.first]->id + ":" + t.first.second;
            const std::string keyName = "autofarm/threshold/" + productID;
            PersistentDataItem cfgThreshold = World::AddPersistentSiteData(keyName);
            cfgThreshold.val() = productID;
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
        out.printerr("Cannot enable {} without a loaded fort.\n", plugin_name);
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

        // a threshold may apply to a plant ("PLANT") or to a specific
        // stockpile growth product of a plant ("PLANT:GROWTH")
        const auto colon = id.find(':');
        const std::string plant_id = id.substr(0, colon);
        const std::string growth_id = colon == std::string::npos ? "" : id.substr(colon + 1);

        bool ok = false;
        for (auto plant : world->raws.plants.all)
        {
            if (!plant->flags.is_set(df::plant_raw_flags::SEED) || (plant->id != plant_id))
                continue;
            if (growth_id.empty())
                autofarmInstance->setThreshold(plant->index, val);
            else if (AutoFarm::has_stockpile_growth(plant, growth_id))
                autofarmInstance->setGrowthThreshold(plant->index, growth_id, val);
            else
                break;
            ok = true;
            break;
        }
        if (!ok)
        {
            out << "Cannot find plant or growth with id " << id << '\n' << std::flush;
            return CR_WRONG_USAGE;
        }
    }
    autofarmInstance->save_state(out);
    return CR_OK;
}

static command_result autofarm(color_ostream& out, std::vector<std::string>& parameters)
{
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run {} without a loaded fort.\n", plugin_name);
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

static void autofarm_setDefaultThreshold(color_ostream& out, int32_t val)
{
    autofarmInstance->setDefault(val);
    autofarmInstance->save_state(out);
}

static void autofarm_setThreshold(color_ostream& out, int32_t plant_index, int32_t val)
{
    autofarmInstance->setThreshold(plant_index, val);
    autofarmInstance->save_state(out);
}

static void autofarm_removeThreshold(color_ostream& out, int32_t plant_index)
{
    autofarmInstance->removeThreshold(plant_index);
    autofarmInstance->save_state(out);
}

static void autofarm_setGrowthThreshold(color_ostream& out, int32_t plant_index, std::string growth_id, int32_t val)
{
    autofarmInstance->setGrowthThreshold(plant_index, growth_id, val);
    autofarmInstance->save_state(out);
}

static void autofarm_removeGrowthThreshold(color_ostream& out, int32_t plant_index, std::string growth_id)
{
    autofarmInstance->removeGrowthThreshold(plant_index, growth_id);
    autofarmInstance->save_state(out);
}

static bool autofarm_isFarmExcluded(int32_t farm_id)
{
    return autofarmInstance->is_farm_excluded(farm_id);
}

static void autofarm_setFarmExcluded(color_ostream& out, int32_t farm_id, bool excluded)
{
    autofarmInstance->set_farm_excluded(farm_id, excluded);
}

// pushes a nested table of the form {plant_index = {growth_id = count}} for a
// map keyed by (plant_index, growth_id)
static void push_growth_map(lua_State* L, const std::map<std::pair<int, std::string>, int>& m)
{
    lua_newtable(L);
    for (auto& [key, val] : m)
    {
        lua_pushinteger(L, key.first);
        lua_gettable(L, -2);
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushinteger(L, key.first);
            lua_pushvalue(L, -2);
            lua_settable(L, -4);
        }
        Lua::Push(L, key.second);
        lua_pushinteger(L, val);
        lua_settable(L, -3);
        lua_pop(L, 1);
    }
}

static int autofarm_getData(lua_State* L)
{
    lua_newtable(L);
    int ctable = lua_gettop(L);
    Lua::SetField(L, autofarmInstance->getDefault(), ctable, "default_threshold");
    Lua::Push(L, autofarmInstance->getThresholds());
    lua_setfield(L, ctable, "thresholds");
    push_growth_map(L, autofarmInstance->getGrowthThresholds());
    lua_setfield(L, ctable, "growth_thresholds");
    Lua::Push(L, autofarmInstance->getLastCounts());
    lua_setfield(L, ctable, "last_counts");
    push_growth_map(L, autofarmInstance->getLastGrowthCounts());
    lua_setfield(L, ctable, "growth_counts");
    Lua::Push(L, autofarmInstance->get_excluded_farms());
    lua_setfield(L, ctable, "excluded_farms");
    return 1;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(autofarm_setDefaultThreshold),
    DFHACK_LUA_FUNCTION(autofarm_setThreshold),
    DFHACK_LUA_FUNCTION(autofarm_removeThreshold),
    DFHACK_LUA_FUNCTION(autofarm_setGrowthThreshold),
    DFHACK_LUA_FUNCTION(autofarm_removeGrowthThreshold),
    DFHACK_LUA_FUNCTION(autofarm_isFarmExcluded),
    DFHACK_LUA_FUNCTION(autofarm_setFarmExcluded),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(autofarm_getData),
    DFHACK_LUA_END
};
