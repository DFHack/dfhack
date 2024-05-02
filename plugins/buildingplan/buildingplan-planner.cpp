#include <functional>
#include <climits> // for CHAR_BIT

#include "df/building_design.h"
#include "df/building_doorst.h"
#include "df/building_type.h"
#include "df/general_ref_building_holderst.h"
#include "df/job_item.h"
#include "df/buildreq.h"

#include "modules/Buildings.h"
#include "modules/Gui.h"
#include "modules/Job.h"

#include "LuaTools.h"
#include "../uicommon.h"

#include "buildingplan.h"

static const std::string planned_building_persistence_key_v1 = "buildingplan/constraints";
static const std::string planned_building_persistence_key_v2 = "buildingplan/constraints2";
static const std::string global_settings_persistence_key = "buildingplan/global";

/*
 * ItemFilter
 */

ItemFilter::ItemFilter()
{
    clear();
}

void ItemFilter::clear()
{
    min_quality = df::item_quality::Ordinary;
    max_quality = df::item_quality::Masterful;
    decorated_only = false;
    clearMaterialMask();
    materials.clear();
}

bool ItemFilter::deserialize(std::string ser)
{
    clear();

    std::vector<std::string> tokens;
    split_string(&tokens, ser, "/");
    if (tokens.size() != 5)
    {
        debug("invalid ItemFilter serialization: '%s'", ser.c_str());
        return false;
    }

    if (!deserializeMaterialMask(tokens[0]) || !deserializeMaterials(tokens[1]))
        return false;

    setMinQuality(atoi(tokens[2].c_str()));
    setMaxQuality(atoi(tokens[3].c_str()));
    decorated_only = static_cast<bool>(atoi(tokens[4].c_str()));
    return true;
}

bool ItemFilter::deserializeMaterialMask(std::string ser)
{
    if (ser.empty())
        return true;

    if (!parseJobMaterialCategory(&mat_mask, ser))
    {
        debug("invalid job material category serialization: '%s'", ser.c_str());
        return false;
    }
    return true;
}

bool ItemFilter::deserializeMaterials(std::string ser)
{
    if (ser.empty())
        return true;

    std::vector<std::string> mat_names;
    split_string(&mat_names, ser, ",");
    for (auto m = mat_names.begin(); m != mat_names.end(); m++)
    {
        DFHack::MaterialInfo material;
        if (!material.find(*m) || !material.isValid())
        {
            debug("invalid material name serialization: '%s'", ser.c_str());
            return false;
        }
        materials.push_back(material);
    }
    return true;
}

// format: mat,mask,elements/materials,list/minq/maxq/decorated
std::string ItemFilter::serialize() const
{
    std::ostringstream ser;
    ser << bitfield_to_string(mat_mask, ",") << "/";
    if (!materials.empty())
    {
        ser << materials[0].getToken();
        for (size_t i = 1; i < materials.size(); ++i)
            ser << "," << materials[i].getToken();
    }
    ser << "/" << static_cast<int>(min_quality);
    ser << "/" << static_cast<int>(max_quality);
    ser << "/" << static_cast<int>(decorated_only);
    return ser.str();
}

void ItemFilter::clearMaterialMask()
{
    mat_mask.whole = 0;
}

void ItemFilter::addMaterialMask(uint32_t mask)
{
    mat_mask.whole |= mask;
}

void ItemFilter::setMaterials(std::vector<DFHack::MaterialInfo> materials)
{
    this->materials = materials;
}

static void clampItemQuality(df::item_quality *quality)
{
    if (*quality > item_quality::Artifact)
    {
        debug("clamping quality to Artifact");
        *quality = item_quality::Artifact;
    }
    if (*quality < item_quality::Ordinary)
    {
        debug("clamping quality to Ordinary");
        *quality = item_quality::Ordinary;
    }
}

void ItemFilter::setMinQuality(int quality)
{
    min_quality = static_cast<df::item_quality>(quality);
    clampItemQuality(&min_quality);
    if (max_quality < min_quality)
        max_quality = min_quality;
}

void ItemFilter::setMaxQuality(int quality)
{
    max_quality = static_cast<df::item_quality>(quality);
    clampItemQuality(&max_quality);
    if (max_quality < min_quality)
        min_quality = max_quality;
}

void ItemFilter::incMinQuality() { setMinQuality(min_quality + 1); }
void ItemFilter::decMinQuality() { setMinQuality(min_quality - 1); }
void ItemFilter::incMaxQuality() { setMaxQuality(max_quality + 1); }
void ItemFilter::decMaxQuality() { setMaxQuality(max_quality - 1); }

void ItemFilter::toggleDecoratedOnly() { decorated_only = !decorated_only; }

static std::string material_to_string_fn(const MaterialInfo &m) { return m.toString(); }

uint32_t ItemFilter::getMaterialMask() const { return mat_mask.whole; }

std::vector<std::string> ItemFilter::getMaterials() const
{
    std::vector<std::string> descriptions;
    transform_(materials, descriptions, material_to_string_fn);

    if (descriptions.size() == 0)
        bitfield_to_string(&descriptions, mat_mask);

    if (descriptions.size() == 0)
        descriptions.push_back("any");

    return descriptions;
}

std::string ItemFilter::getMinQuality() const
{
    return ENUM_KEY_STR(item_quality, min_quality);
}

std::string ItemFilter::getMaxQuality() const
{
    return ENUM_KEY_STR(item_quality, max_quality);
}

bool ItemFilter::getDecoratedOnly() const
{
    return decorated_only;
}

bool ItemFilter::matchesMask(DFHack::MaterialInfo &mat) const
{
    return mat_mask.whole ? mat.matches(mat_mask) : true;
}

bool ItemFilter::matches(df::dfhack_material_category mask) const
{
    return mask.whole & mat_mask.whole;
}

bool ItemFilter::matches(DFHack::MaterialInfo &material) const
{
    for (auto it = materials.begin(); it != materials.end(); ++it)
        if (material.matches(*it))
            return true;
    return false;
}

bool ItemFilter::matches(df::item *item) const
{
    if (item->getQuality() < min_quality || item->getQuality() > max_quality)
        return false;

    if (decorated_only && !item->hasImprovements())
        return false;

    auto imattype = item->getActualMaterial();
    auto imatindex = item->getActualMaterialIndex();
    auto item_mat = DFHack::MaterialInfo(imattype, imatindex);

    return (materials.size() == 0) ? matchesMask(item_mat) : matches(item_mat);
}


/*
 * PlannedBuilding
 */

// format: itemfilterser|itemfilterser|...
static std::string serializeFilters(const std::vector<ItemFilter> &filters)
{
    std::ostringstream ser;
    if (!filters.empty())
    {
        ser << filters[0].serialize();
        for (size_t i = 1; i < filters.size(); ++i)
            ser << "|" << filters[i].serialize();
    }
    return ser.str();
}

static std::vector<ItemFilter> deserializeFilters(std::string ser)
{
    std::vector<std::string> isers;
    split_string(&isers, ser, "|");
    std::vector<ItemFilter> ret;
    for (auto & iser : isers)
    {
        ItemFilter filter;
        if (filter.deserialize(iser))
            ret.push_back(filter);
    }
    return ret;
}

static size_t getNumFilters(BuildingTypeKey key)
{
    auto L = Lua::Core::State;
    color_ostream_proxy out(Core::getInstance().getConsole());
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 4) || !Lua::PushModulePublic(
            out, L, "plugins.buildingplan", "get_num_filters"))
    {
        debug("failed to push the lua method on the stack");
        return 0;
    }

    Lua::Push(L, std::get<0>(key));
    Lua::Push(L, std::get<1>(key));
    Lua::Push(L, std::get<2>(key));

    if (!Lua::SafeCall(out, L, 3, 1))
    {
        debug("lua call failed");
        return 0;
    }

    int num_filters = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return num_filters;
}

PlannedBuilding::PlannedBuilding(df::building *building, const std::vector<ItemFilter> &filters)
    : building(building),
      building_id(building->id),
      filters(filters)
{
    config = DFHack::World::AddPersistentData(planned_building_persistence_key_v2);
    config.ival(0) = building_id;
    config.val() = serializeFilters(filters);
}

PlannedBuilding::PlannedBuilding(PersistentDataItem &config)
    : config(config),
      building(df::building::find(config.ival(0))),
      building_id(config.ival(0)),
      filters(deserializeFilters(config.val()))
{
    if (building)
    {
        if (filters.size() !=
            getNumFilters(toBuildingTypeKey(building)))
        {
            debug("invalid ItemFilter vector serialization: '%s'",
                config.val().c_str());
            building = NULL;
        }
    }
}

// Ensure the building still exists and is in a valid state. It can disappear
// for lots of reasons, such as running the game with the buildingplan plugin
// disabled, manually removing the building, modifying it via the API, etc.
bool PlannedBuilding::isValid() const
{
    return building && df::building::find(building_id)
        && building->getBuildStage() == 0;
}

void PlannedBuilding::remove()
{
    DFHack::World::DeletePersistentData(config);
    building = NULL;
}

df::building * PlannedBuilding::getBuilding()
{
    return building;
}

const std::vector<ItemFilter> & PlannedBuilding::getFilters() const
{
    // if we want to be able to dynamically change the filters, we'll need to
    // re-bucket the tasks in Planner.
    return filters;
}


/*
 * BuildingTypeKey
 */

BuildingTypeKey toBuildingTypeKey(
    df::building_type btype, int16_t subtype, int32_t custom)
{
    return std::make_tuple(btype, subtype, custom);
}

BuildingTypeKey toBuildingTypeKey(df::building *bld)
{
    return std::make_tuple(
        bld->getType(), bld->getSubtype(), bld->getCustomType());
}

BuildingTypeKey toBuildingTypeKey(df::ui_build_selector *uibs)
{
    return std::make_tuple(
        uibs->building_type, uibs->building_subtype, uibs->custom_type);
}

// rotates a size_t value left by count bits
// assumes count is not 0 or >= size_t_bits
// replace this with std::rotl when we move to C++20
static std::size_t rotl_size_t(size_t val, uint32_t count)
{
    static const int size_t_bits = CHAR_BIT * sizeof(std::size_t);
    return val << count | val >> (size_t_bits - count);
}

std::size_t BuildingTypeKeyHash::operator() (const BuildingTypeKey & key) const
{
    // cast first param to appease gcc-4.8, which is missing the enum
    // specializations for std::hash
    std::size_t h1 = std::hash<int32_t>()(static_cast<int32_t>(std::get<0>(key)));
    std::size_t h2 = std::hash<int16_t>()(std::get<1>(key));
    std::size_t h3 = std::hash<int32_t>()(std::get<2>(key));

    return h1 ^ rotl_size_t(h2, 8) ^ rotl_size_t(h3, 16);
}


/*
 * Planner
 */

// convert v1 persistent data into v2 format
// we can remove this conversion code once v2 has been live for a while
void migrateV1ToV2()
{
    std::vector<PersistentDataItem> configs;
    DFHack::World::GetPersistentData(&configs, planned_building_persistence_key_v1);
    if (configs.empty())
        return;

    debug("migrating %zu persisted configs to new format", configs.size());
    for (auto config : configs)
    {
        df::building *bld = df::building::find(config.ival(1));
        if (!bld)
        {
            debug("buliding no longer exists; removing config");
            DFHack::World::DeletePersistentData(config);
            continue;
        }

        if (bld->getBuildStage() != 0 || bld->jobs.size() != 1
            || bld->jobs[0]->job_items.size() != 1)
        {
            debug("building in invalid state; removing config");
            DFHack::World::DeletePersistentData(config);
            continue;
        }

        // fix up the building so we can set the material properties later
        bld->mat_type = -1;
        bld->mat_index = -1;

        // the v1 filters are not initialized correctly and will match any item.
        // we need to fix them up a bit.
        auto filter = bld->jobs[0]->job_items[0];
        df::item_type type;
        switch (bld->getType())
        {
        case df::building_type::Armorstand: type = df::item_type::ARMORSTAND; break;
        case df::building_type::Bed: type = df::item_type::BED; break;
        case df::building_type::Chair: type = df::item_type::CHAIR; break;
        case df::building_type::Coffin: type = df::item_type::COFFIN; break;
        case df::building_type::Door: type = df::item_type::DOOR; break;
        case df::building_type::Floodgate: type = df::item_type::FLOODGATE; break;
        case df::building_type::Hatch: type = df::item_type::HATCH_COVER; break;
        case df::building_type::GrateWall: type = df::item_type::GRATE; break;
        case df::building_type::GrateFloor: type = df::item_type::GRATE; break;
        case df::building_type::BarsVertical: type = df::item_type::BAR; break;
        case df::building_type::BarsFloor: type = df::item_type::BAR; break;
        case df::building_type::Cabinet: type = df::item_type::CABINET; break;
        case df::building_type::Box: type = df::item_type::BOX; break;
        case df::building_type::Weaponrack: type = df::item_type::WEAPONRACK; break;
        case df::building_type::Statue: type = df::item_type::STATUE; break;
        case df::building_type::Slab: type = df::item_type::SLAB; break;
        case df::building_type::Table: type = df::item_type::TABLE; break;
        case df::building_type::WindowGlass: type = df::item_type::WINDOW; break;
        case df::building_type::AnimalTrap: type = df::item_type::ANIMALTRAP; break;
        case df::building_type::Chain: type = df::item_type::CHAIN; break;
        case df::building_type::Cage: type = df::item_type::CAGE; break;
        case df::building_type::TractionBench: type = df::item_type::TRACTION_BENCH; break;
        default:
            debug("building has unhandled type; removing config");
            DFHack::World::DeletePersistentData(config);
            continue;
        }
        filter->item_type = type;
        filter->item_subtype = -1;
        filter->mat_type = -1;
        filter->mat_index = -1;
        filter->flags1.whole = 0;
        filter->flags2.whole = 0;
        filter->flags2.bits.allow_artifact = true;
        filter->flags3.whole = 0;
        filter->flags4 = 0;
        filter->flags5 = 0;
        filter->metal_ore = -1;
        filter->min_dimension = -1;
        filter->has_tool_use = df::tool_uses::NONE;
        filter->quantity = 1;

        std::vector<std::string> tokens;
        split_string(&tokens, config.val(), "/");
        if (tokens.size() != 2)
        {
            debug("invalid v1 format; removing config");
            DFHack::World::DeletePersistentData(config);
            continue;
        }

        ItemFilter item_filter;
        item_filter.deserializeMaterialMask(tokens[0]);
        item_filter.deserializeMaterials(tokens[1]);
        item_filter.setMinQuality(config.ival(2) - 1);
        item_filter.setMaxQuality(config.ival(4) - 1);
        if (config.ival(3) - 1)
            item_filter.toggleDecoratedOnly();

        // create the v2 record
        std::vector<ItemFilter> item_filters;
        item_filters.push_back(item_filter);
        PlannedBuilding pb(bld, item_filters);

        // remove the v1 record
        DFHack::World::DeletePersistentData(config);
        debug("v1 %s(%d) record successfully migrated",
              ENUM_KEY_STR(building_type, bld->getType()).c_str(),
              bld->id);
    }
}

// assumes no setting has '=' or '|' characters
static std::string serialize_settings(std::map<std::string, bool> & settings)
{
    std::ostringstream ser;
    for (auto & entry : settings)
    {
        ser << entry.first << "=" << (entry.second ? "1" : "0") << "|";
    }
    return ser.str();
}

static void deserialize_settings(std::map<std::string, bool> & settings,
                                 std::string ser)
{
    std::vector<std::string> tokens;
    split_string(&tokens, ser, "|");
    for (auto token : tokens)
    {
        if (token.empty())
            continue;

        std::vector<std::string> parts;
        split_string(&parts, token, "=");
        if (parts.size() != 2)
        {
            debug("invalid serialized setting format: '%s'", token.c_str());
            continue;
        }
        std::string key = parts[0];
        if (settings.count(key) == 0)
        {
            debug("unknown serialized setting: '%s", key.c_str());
            continue;
        }
        settings[key] = static_cast<bool>(atoi(parts[1].c_str()));
        debug("deserialized setting: %s = %d", key.c_str(), settings[key]);
    }
}

static DFHack::PersistentDataItem init_global_settings(
        std::map<std::string, bool> & settings)
{
    settings.clear();
    settings["blocks"] = true;
    settings["boulders"] = true;
    settings["logs"] = true;
    settings["bars"] = false;

    // load persistent global settings if they exist; otherwise create them
    std::vector<PersistentDataItem> items;
    DFHack::World::GetPersistentData(&items, global_settings_persistence_key);
    if (items.size() == 1)
    {
        DFHack::PersistentDataItem & config = items[0];
        deserialize_settings(settings, config.val());
        return config;
    }

    debug("initializing persistent global settings");
    DFHack::PersistentDataItem config =
        DFHack::World::AddPersistentData(global_settings_persistence_key);
    config.val() = serialize_settings(settings);
    return config;
}

const std::map<std::string, bool> & Planner::getGlobalSettings() const
{
    return global_settings;
}

bool Planner::setGlobalSetting(std::string name, bool value)
{
    if (global_settings.count(name) == 0)
    {
        debug("attempted to set invalid setting: '%s'", name.c_str());
        return false;
    }
    debug("global setting '%s' %d -> %d",
          name.c_str(), global_settings[name], value);
    global_settings[name] = value;
    if (config.isValid())
        config.val() = serialize_settings(global_settings);
    return true;
}

void Planner::reset()
{
    debug("resetting Planner state");
    default_item_filters.clear();
    planned_buildings.clear();
    tasks.clear();

    config = init_global_settings(global_settings);

    migrateV1ToV2();

    std::vector<PersistentDataItem> items;
    DFHack::World::GetPersistentData(&items, planned_building_persistence_key_v2);
    debug("found data for %zu planned building(s)", items.size());

    for (auto i = items.begin(); i != items.end(); i++)
    {
        PlannedBuilding pb(*i);
        if (!pb.isValid())
        {
            debug("discarding invalid planned building");
            pb.remove();
            continue;
        }

        if (registerTasks(pb))
            planned_buildings.insert(std::make_pair(pb.getBuilding()->id, pb));
    }
}

void Planner::addPlannedBuilding(df::building *bld)
{
    auto item_filters = getItemFilters(toBuildingTypeKey(bld)).get();
    // not a supported type
    if (item_filters.empty())
    {
        debug("failed to add building: unsupported type");
        return;
    }

    // protect against multiple registrations
    if (planned_buildings.count(bld->id) != 0)
    {
        debug("failed to add building: already registered");
        return;
    }

    PlannedBuilding pb(bld, item_filters);
    if (pb.isValid() && registerTasks(pb))
    {
        for (auto job : bld->jobs)
            job->flags.bits.suspend = true;

        planned_buildings.insert(std::make_pair(bld->id, pb));
    }
    else
    {
        pb.remove();
    }
}

static std::string getBucket(const df::job_item & ji,
                             const std::vector<ItemFilter> & item_filters)
{
    std::ostringstream ser;

    // pull out and serialize only known relevant fields. if we miss a few, then
    // the filter bucket will be slighly less specific than it could be, but
    // that's probably ok. we'll just end up bucketing slightly different items
    // together. this is only a problem if the different filter at the front of
    // the queue doesn't match any available items and blocks filters behind it
    // that could be matched.
    ser << ji.item_type << ':' << ji.item_subtype << ':' << ji.mat_type << ':'
        << ji.mat_index << ':' << ji.flags1.whole << ':' << ji.flags2.whole
        << ':' << ji.flags3.whole << ':' << ji.flags4 << ':' << ji.flags5 << ':'
        << ji.metal_ore << ':' << ji.has_tool_use;

    for (auto & item_filter : item_filters)
    {
        ser << ':' << item_filter.serialize();
    }

    return ser.str();
}

// get a list of item vectors that we should search for matches
static std::vector<df::job_item_vector_id> getVectorIds(df::job_item *job_item,
        const std::map<std::string, bool> & global_settings)
{
    std::vector<df::job_item_vector_id> ret;

    // if the filter already has the vector_id set to something specific, use it
    if (job_item->vector_id > df::job_item_vector_id::IN_PLAY)
    {
        debug("using vector_id from job_item: %s",
              ENUM_KEY_STR(job_item_vector_id, job_item->vector_id).c_str());
        ret.push_back(job_item->vector_id);
        return ret;
    }

    // if the filer is for building material, refer to our global settings for
    // which vectors to search
    if (job_item->flags2.bits.building_material)
    {
        if (global_settings.at("blocks"))
            ret.push_back(df::job_item_vector_id::BLOCKS);
        if (global_settings.at("boulders"))
            ret.push_back(df::job_item_vector_id::BOULDER);
        if (global_settings.at("logs"))
            ret.push_back(df::job_item_vector_id::WOOD);
        if (global_settings.at("bars"))
            ret.push_back(df::job_item_vector_id::BAR);
    }

    // fall back to IN_PLAY if no other vector was appropriate
    if (ret.empty())
        ret.push_back(df::job_item_vector_id::IN_PLAY);
    return ret;
}

bool Planner::registerTasks(PlannedBuilding & pb)
{
    df::building * bld = pb.getBuilding();
    if (bld->jobs.size() != 1)
    {
        debug("unexpected number of jobs: want 1, got %zu", bld->jobs.size());
        return false;
    }
    auto job_items = bld->jobs[0]->job_items;
    int num_job_items = job_items.size();
    if (num_job_items < 1)
    {
        debug("unexpected number of job items: want >0, got %d", num_job_items);
        return false;
    }
    int32_t id = bld->id;
    for (int job_item_idx = 0; job_item_idx < num_job_items; ++job_item_idx)
    {
        auto job_item = job_items[job_item_idx];
        auto bucket = getBucket(*job_item, pb.getFilters());
        auto vector_ids = getVectorIds(job_item, global_settings);

        // if there are multiple vector_ids, schedule duplicate tasks. after
        // the correct number of items are matched, the extras will get popped
        // as invalid
        for (auto vector_id : vector_ids)
        {
            for (int item_num = 0; item_num < job_item->quantity; ++item_num)
            {
                tasks[vector_id][bucket].push(std::make_pair(id, job_item_idx));
                debug("added task: %s/%s/%d,%d; "
                      "%zu vector(s), %zu filter bucket(s), %zu task(s) in bucket",
                      ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                      bucket.c_str(), id, job_item_idx, tasks.size(),
                      tasks[vector_id].size(), tasks[vector_id][bucket].size());
            }
        }
    }
    return true;
}

PlannedBuilding * Planner::getPlannedBuilding(df::building *bld)
{
    if (!bld || planned_buildings.count(bld->id) == 0)
        return NULL;
    return &planned_buildings.at(bld->id);
}

bool Planner::isPlannableBuilding(BuildingTypeKey key)
{
    return getNumFilters(key) >= 1;
}

Planner::ItemFiltersWrapper Planner::getItemFilters(BuildingTypeKey key)
{
    static std::vector<ItemFilter> empty_vector;
    static const ItemFiltersWrapper empty_ret(empty_vector);

    size_t nfilters = getNumFilters(key);
    if (nfilters < 1)
        return empty_ret;
    while (default_item_filters[key].size() < nfilters)
        default_item_filters[key].push_back(ItemFilter());
    return ItemFiltersWrapper(default_item_filters[key]);
}

// precompute a bitmask with bad item flags
struct BadFlags
{
    uint32_t whole;

    BadFlags()
    {
        df::item_flags flags;
        #define F(x) flags.bits.x = true;
        F(dump); F(forbid); F(garbage_collect);
        F(hostile); F(on_fire); F(rotten); F(trader);
        F(in_building); F(construction); F(in_job);
        F(owned); F(in_chest); F(removed); F(encased);
        #undef F
        whole = flags.whole;
    }
};

static bool itemPassesScreen(df::item * item)
{
    static BadFlags bad_flags;
    return !(item->flags.whole & bad_flags.whole)
        && !item->isAssignedToStockpile()
        // TODO: make this configurable
        && !(item->getType() == df::item_type::BOX && item->isBag());
}

static bool matchesFilters(df::item * item,
                           df::job_item * job_item,
                           const ItemFilter & item_filter)
{
    // check the properties that are not checked by Job::isSuitableItem()
    if (job_item->item_type > -1 && job_item->item_type != item->getType())
        return false;

    if (job_item->item_subtype > -1 &&
        job_item->item_subtype != item->getSubtype())
        return false;

    if (job_item->flags2.bits.building_material && !item->isBuildMat())
        return false;

    if (job_item->metal_ore > -1 && !item->isMetalOre(job_item->metal_ore))
        return false;

    if (job_item->has_tool_use > df::tool_uses::NONE
        && !item->hasToolUse(job_item->has_tool_use))
        return false;

    return DFHack::Job::isSuitableItem(
            job_item, item->getType(), item->getSubtype())
        && DFHack::Job::isSuitableMaterial(
            job_item, item->getMaterial(), item->getMaterialIndex(),
            item->getType())
        && item_filter.matches(item);
}

// note that this just removes the PlannedBuilding. the tasks will get dropped
// as we discover them in the tasks queues and they fail their isValid() check.
// this "lazy" task cleaning algorithm works because there is no way to
// re-register a building once it has been removed -- if it fails isValid()
// then it has either been built or desroyed. therefore there is no chance of
// duplicate tasks getting added to the tasks queues.
void Planner::unregisterBuilding(int32_t id)
{
    if (planned_buildings.count(id) > 0)
    {
        planned_buildings.at(id).remove();
        planned_buildings.erase(id);
    }
}

static bool isJobReady(df::job * job)
{
    int needed_items = 0;
    for (auto job_item : job->job_items) { needed_items += job_item->quantity; }
    if (needed_items)
    {
        debug("building needs %d more item(s)", needed_items);
        return false;
    }
    return true;
}

static bool job_item_idx_lt(df::job_item_ref *a, df::job_item_ref *b)
{
    // we want the items in the opposite order of the filters
    return a->job_item_idx > b->job_item_idx;
}

// this function does not remove the job_items since their quantity fields are
// now all at 0, so there is no risk of having extra items attached. we don't
// remove them to keep the "finalize with buildingplan active" path as similar
// as possible to the "finalize with buildingplan disabled" path.
static void finalizeBuilding(df::building * bld)
{
    debug("finalizing building %d", bld->id);
    auto job = bld->jobs[0];

    // sort the items so they get added to the structure in the correct order
    std::sort(job->items.begin(), job->items.end(), job_item_idx_lt);

    // derive the material properties of the building and job from the first
    // applicable item, though if any boulders are involved, it makes the whole
    // structure "rough".
    bool rough = false;
    for (auto attached_item : job->items)
    {
        df::item *item = attached_item->item;
        rough = rough || item->getType() == item_type::BOULDER;
        if (bld->mat_type == -1)
        {
            bld->mat_type = item->getMaterial();
            job->mat_type = bld->mat_type;
        }
        if (bld->mat_index == -1)
        {
            bld->mat_index = item->getMaterialIndex();
            job->mat_index = bld->mat_index;
        }
    }

    if (bld->needsDesign())
    {
        auto act = (df::building_actual *)bld;
        if (!act->design)
            act->design = new df::building_design();
        act->design->flags.bits.rough = rough;
    }

    // we're good to go!
    job->flags.bits.suspend = false;
    Job::checkBuildingsNow();
}

void Planner::popInvalidTasks(std::queue<std::pair<int32_t, int>> & task_queue)
{
    while (!task_queue.empty())
    {
        auto & task = task_queue.front();
        auto id = task.first;
        if (planned_buildings.count(id) > 0)
        {
            PlannedBuilding & pb = planned_buildings.at(id);
            if (pb.isValid() &&
                pb.getBuilding()->jobs[0]->job_items[task.second]->quantity)
            {
                break;
            }
        }
        debug("discarding invalid task: bld=%d, job_item_idx=%d",
              id, task.second);
        task_queue.pop();
        unregisterBuilding(id);
    }
}

void Planner::doVector(df::job_item_vector_id vector_id,
       std::map<std::string, std::queue<std::pair<int32_t, int>>> & buckets)
{
    auto other_id = ENUM_ATTR(job_item_vector_id, other, vector_id);
    auto item_vector = df::global::world->items.other[other_id];
    debug("matching %zu item(s) in vector %s against %zu filter bucket(s)",
          item_vector.size(),
          ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
          buckets.size());
    for (auto item_it = item_vector.rbegin();
         item_it != item_vector.rend();
         ++item_it)
    {
        auto item = *item_it;
        if (!itemPassesScreen(item))
            continue;
        for (auto bucket_it = buckets.begin(); bucket_it != buckets.end();)
        {
            auto & task_queue = bucket_it->second;
            popInvalidTasks(task_queue);
            if (task_queue.empty())
            {
                debug("removing empty bucket: %s/%s; %zu bucket(s) left",
                      ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                      bucket_it->first.c_str(),
                      buckets.size() - 1);
                bucket_it = buckets.erase(bucket_it);
                continue;
            }
            auto & task = task_queue.front();
            auto id = task.first;
            auto & pb = planned_buildings.at(id);
            auto building = pb.getBuilding();
            auto job = building->jobs[0];
            auto filter_idx = task.second;
            if (matchesFilters(item, job->job_items[filter_idx],
                    pb.getFilters()[filter_idx])
               && DFHack::Job::attachJobItem(job, item,
                        df::job_item_ref::Hauled, filter_idx))
            {
                MaterialInfo material;
                material.decode(item);
                ItemTypeInfo item_type;
                item_type.decode(item);
                debug("attached %s %s to filter %d for %s(%d): %s/%s",
                      material.toString().c_str(),
                      item_type.toString().c_str(),
                      filter_idx,
                      ENUM_KEY_STR(building_type, building->getType()).c_str(),
                      id,
                      ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                      bucket_it->first.c_str());
                // keep quantity aligned with the actual number of remaining
                // items so if buildingplan is turned off, the building will
                // be completed with the correct number of items.
                --job->job_items[filter_idx]->quantity;
                task_queue.pop();
                if (isJobReady(job))
                {
                    finalizeBuilding(building);
                    unregisterBuilding(id);
                }
                if (task_queue.empty())
                {
                    debug(
                        "removing empty item bucket: %s/%s; %zu left",
                        ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                        bucket_it->first.c_str(),
                        buckets.size() - 1);
                    buckets.erase(bucket_it);
                }
                // we found a home for this item; no need to look further
                break;
            }
            ++bucket_it;
        }
        if (buckets.empty())
            break;
    }
}

struct VectorsToScanLast
{
    std::vector<df::job_item_vector_id> vectors;
    VectorsToScanLast()
    {
        // order is important here. we want to match boulders before wood and
        // everything before bars. blocks are not listed here since we'll have
        // already scanned them when we did the first pass through the buckets.
        vectors.push_back(df::job_item_vector_id::BOULDER);
        vectors.push_back(df::job_item_vector_id::WOOD);
        vectors.push_back(df::job_item_vector_id::BAR);
    }
};

void Planner::doCycle()
{
    debug("running cycle for %zu registered building(s)",
          planned_buildings.size());
    static const VectorsToScanLast vectors_to_scan_last;
    for (auto it = tasks.begin(); it != tasks.end();)
    {
        auto vector_id = it->first;
        // we could make this a set, but it's only three elements
        if (std::find(vectors_to_scan_last.vectors.begin(),
                      vectors_to_scan_last.vectors.end(),
                      vector_id) != vectors_to_scan_last.vectors.end())
        {
            ++it;
            continue;
        }

        auto & buckets = it->second;
        doVector(vector_id, buckets);
        if (buckets.empty())
        {
            debug("removing empty vector: %s; %zu vector(s) left",
                  ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                  tasks.size() - 1);
            it = tasks.erase(it);
        }
        else
            ++it;
    }
    for (auto vector_id : vectors_to_scan_last.vectors)
    {
        if (tasks.count(vector_id) == 0)
            continue;
        auto & buckets = tasks[vector_id];
        doVector(vector_id, buckets);
        if (buckets.empty())
        {
            debug("removing empty vector: %s; %zu vector(s) left",
                  ENUM_KEY_STR(job_item_vector_id, vector_id).c_str(),
                  tasks.size() - 1);
            tasks.erase(vector_id);
        }
    }
    debug("cycle done; %zu registered building(s) left",
          planned_buildings.size());
}

Planner planner;
