#include "df/building_design.h"
#include "df/building_doorst.h"
#include "df/building_type.h"
#include "df/general_ref_building_holderst.h"
#include "df/job_item.h"
#include "df/ui_build_selector.h"

#include "modules/Buildings.h"
#include "modules/Gui.h"
#include "modules/Job.h"

#include "uicommon.h"

#include "buildingplan-planner.h"
#include "buildingplan-lib.h"

static const std::string planned_building_persistence_key_v1 = "buildingplan/constraints";

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

bool ItemFilter::deserialize(PersistentDataItem &config)
{
    clear();

    std::vector<std::string> tokens;
    split_string(&tokens, config.val(), "/");
    if (tokens.size() != 2)
    {
        debug("invalid ItemFilter serialization: '%s'", config.val().c_str());
        return false;
    }

    if (!deserializeMaterialMask(tokens[0]) || !deserializeMaterials(tokens[1]))
        return false;

    setMinQuality(config.ival(2) - 1);
    setMaxQuality(config.ival(4) - 1);
    decorated_only = config.ival(3) - 1;
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

void ItemFilter::serialize(PersistentDataItem &config) const
{
    std::ostringstream ser;
    ser << bitfield_to_string(mat_mask, ",") << "/";
    if (!materials.empty())
    {
        ser << materials[0].getToken();
        for (size_t i = 1; i < materials.size(); ++i)
            ser << "," << materials[i].getToken();
    }
    config.val() = ser.str();
    config.ival(2) = min_quality + 1;
    config.ival(4) = max_quality + 1;
    config.ival(3) = static_cast<int>(decorated_only) + 1;
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

static std::vector<ItemFilter> deserializeFilters(PersistentDataItem &config)
{
    // simplified implementation while we can assume there is only one filter
    std::vector<ItemFilter> ret;
    ItemFilter itemFilter;
    itemFilter.deserialize(config);
    ret.push_back(itemFilter);
    return ret;
}

static size_t getNumFilters(BuildingTypeKey key)
{
    // TODO: get num filters in Lua when we handle all building types
    return 1;
}

PlannedBuilding::PlannedBuilding(df::building *building, const std::vector<ItemFilter> &filters)
    : building_id(building->id),
      building(building),
      filters(filters)
{
    config = DFHack::World::AddPersistentData(planned_building_persistence_key_v1);
    config.ival(1) = building_id;
    // assume all filter vectors are length 1 for now
    filters[0].serialize(config);
}

PlannedBuilding::PlannedBuilding(PersistentDataItem &config)
    : config(config),
      building_id(config.ival(1)),
      building(df::building::find(config.ival(1))),
      filters(deserializeFilters(config))
{ }

bool PlannedBuilding::assignClosestItem(std::vector<df::item *> *items_vector)
{
    decltype(items_vector->begin()) closest_item;
    int32_t closest_distance = -1;
    for (auto item_iter = items_vector->begin(); item_iter != items_vector->end(); item_iter++)
    {
        auto item = *item_iter;
        if (!filters[0].matches(item))
            continue;

        auto pos = item->pos;
        auto distance = abs(pos.x - building->centerx) +
            abs(pos.y - building->centery) +
            abs(pos.z - building->z) * 50;

        if (closest_distance > -1 && distance >= closest_distance)
            continue;

        closest_distance = distance;
        closest_item = item_iter;
    }

    if (closest_distance > -1 && assignItem(*closest_item))
    {
        debug("Item assigned");
        items_vector->erase(closest_item);
        remove();
        return true;
    }

    return false;
}

void delete_item_fn(df::job_item *x) { delete x; }

bool PlannedBuilding::assignItem(df::item *item)
{
    auto ref = df::allocate<df::general_ref_building_holderst>();
    if (!ref)
    {
        Core::printerr("Could not allocate general_ref_building_holderst\n");
        return false;
    }

    ref->building_id = building->id;

    if (building->jobs.size() != 1)
        return false;

    auto job = building->jobs[0];

    for_each_(job->job_items, delete_item_fn);
    job->job_items.clear();
    job->flags.bits.suspend = false;

    bool rough = false;
    Job::attachJobItem(job, item, df::job_item_ref::Hauled);
    if (item->getType() == item_type::BOULDER)
        rough = true;
    building->mat_type = item->getMaterial();
    building->mat_index = item->getMaterialIndex();

    job->mat_type = building->mat_type;
    job->mat_index = building->mat_index;

    if (building->needsDesign())
    {
        auto act = (df::building_actual *) building;
        act->design = new df::building_design();
        act->design->flags.bits.rough = rough;
    }

    return true;
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

std::size_t BuildingTypeKeyHash::operator() (const BuildingTypeKey & key) const
{
    return std::hash<df::building_type>()(std::get<0>(key))
        ^ std::hash<int16_t>()(std::get<1>(key))
        ^ std::hash<int32_t>()(std::get<2>(key));
}


/*
 * Planner
 */

void Planner::initialize()
{
#define add_building_type(btype, itype) \
    item_for_building_type[df::building_type::btype] = df::item_type::itype; \
    available_item_vectors[df::item_type::itype] = std::vector<df::item *>(); \
    is_relevant_item_type[df::item_type::itype] = true; \

    FOR_ENUM_ITEMS(item_type, it)
        is_relevant_item_type[it] = false;

    add_building_type(Armorstand, ARMORSTAND);
    add_building_type(Bed, BED);
    add_building_type(Chair, CHAIR);
    add_building_type(Coffin, COFFIN);
    add_building_type(Door, DOOR);
    add_building_type(Floodgate, FLOODGATE);
    add_building_type(Hatch, HATCH_COVER);
    add_building_type(GrateWall, GRATE);
    add_building_type(GrateFloor, GRATE);
    add_building_type(BarsVertical, BAR);
    add_building_type(BarsFloor, BAR);
    add_building_type(Cabinet, CABINET);
    add_building_type(Box, BOX);
    // skip kennels, farm plot
    add_building_type(Weaponrack, WEAPONRACK);
    add_building_type(Statue, STATUE);
    add_building_type(Slab, SLAB);
    add_building_type(Table, TABLE);
    // skip roads ... furnaces
    add_building_type(WindowGlass, WINDOW);
    // skip gem window ... support
    add_building_type(AnimalTrap, ANIMALTRAP);
    add_building_type(Chain, CHAIN);
    add_building_type(Cage, CAGE);
    // skip archery target
    add_building_type(TractionBench, TRACTION_BENCH);
    // skip nest box, hive (tools)

#undef add_building_type
}

void Planner::reset()
{
    debug("resetting Planner state");
    default_item_filters.clear();
    planned_buildings.clear();

    std::vector<PersistentDataItem> items;
    DFHack::World::GetPersistentData(&items, planned_building_persistence_key_v1);
    debug("found data for %zu planned buildings", items.size());

    for (auto i = items.begin(); i != items.end(); i++)
    {
        PlannedBuilding pb(*i);
        if (!pb.isValid())
        {
            pb.remove();
            continue;
        }

        planned_buildings.push_back(pb);
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
    if (getPlannedBuilding(bld))
    {
        debug("building already registered");
        return;
    }

    PlannedBuilding pb(bld, item_filters);
    if (pb.isValid())
    {
        for (auto job : bld->jobs)
            job->flags.bits.suspend = true;

        planned_buildings.push_back(pb);
    }
    else
    {
        pb.remove();
    }
}

PlannedBuilding * Planner::getPlannedBuilding(df::building *bld)
{
    for (auto & pb : planned_buildings)
    {
        if (pb.getBuilding() == bld)
            return &pb;
    }
    return NULL;
}

bool Planner::isPlannableBuilding(BuildingTypeKey key)
{
    return item_for_building_type.count(std::get<0>(key)) > 0;
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

void Planner::doCycle()
{
    debug("Running Cycle");
    if (planned_buildings.size() == 0)
        return;

    debug("Planned count: %zu", planned_buildings.size());

    gather_available_items();
    for (auto building_iter = planned_buildings.begin(); building_iter != planned_buildings.end();)
    {
        if (building_iter->isValid())
        {
            auto type = building_iter->getBuilding()->getType();
            debug("Trying to allocate %s", enum_item_key_str(type));

            auto required_item_type = item_for_building_type[type];
            auto items_vector = &available_item_vectors[required_item_type];
            if (items_vector->size() == 0 || !building_iter->assignClosestItem(items_vector))
            {
                debug("Unable to allocate an item");
                ++building_iter;
                continue;
            }
        }
        debug("Removing building plan");
        building_iter->remove();
        building_iter = planned_buildings.erase(building_iter);
    }
}

void Planner::gather_available_items()
{
    debug("Gather available items");
    for (auto iter = available_item_vectors.begin(); iter != available_item_vectors.end(); iter++)
    {
        iter->second.clear();
    }

    // Precompute a bitmask with the bad flags
    df::item_flags bad_flags;
    bad_flags.whole = 0;

    #define F(x) bad_flags.bits.x = true;
    F(dump); F(forbid); F(garbage_collect);
    F(hostile); F(on_fire); F(rotten); F(trader);
    F(in_building); F(construction); F(artifact);
    #undef F

    std::vector<df::item*> &items = df::global::world->items.other[df::items_other_id::IN_PLAY];

    for (size_t i = 0; i < items.size(); i++)
    {
        df::item *item = items[i];

        if (item->flags.whole & bad_flags.whole)
            continue;

        df::item_type itype = item->getType();
        if (!is_relevant_item_type[itype])
            continue;

        if (itype == df::item_type::BOX && item->isBag())
            continue; //Skip bags

        if (item->flags.bits.artifact)
            continue;

        if (item->flags.bits.in_job ||
            item->isAssignedToStockpile() ||
            item->flags.bits.owned ||
            item->flags.bits.in_chest)
        {
            continue;
        }

        available_item_vectors[itype].push_back(item);
    }
}

Planner planner;
