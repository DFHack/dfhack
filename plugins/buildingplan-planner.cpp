#include "df/general_ref_building_holderst.h"
#include "df/job_item.h"
#include "df/building_doorst.h"
#include "df/building_design.h"

#include "modules/Job.h"
#include "modules/Buildings.h"
#include "modules/Gui.h"

#include "buildingplan-planner.h"
#include "buildingplan-lib.h"
#include "uicommon.h"

/*
* ItemFilter
*/

ItemFilter::ItemFilter() :
        min_quality(df::item_quality::Ordinary),
        max_quality(df::item_quality::Artifact),
        decorated_only(false),
        valid(true)
{
    clear(); // mat_mask is not cleared by default (see issue #1047)
}

bool ItemFilter::matchesMask(DFHack::MaterialInfo &mat)
{
    return (mat_mask.whole) ? mat.matches(mat_mask) : true;
}

bool ItemFilter::matches(const df::dfhack_material_category mask) const
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

bool ItemFilter::matches(df::item *item)
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

std::string material_to_string_fn(DFHack::MaterialInfo m) { return m.toString(); }

std::vector<std::string> ItemFilter::getMaterialFilterAsVector()
{
    std::vector<std::string> descriptions;

    transform_(materials, descriptions, material_to_string_fn);

    if (descriptions.size() == 0)
        bitfield_to_string(&descriptions, mat_mask);

    if (descriptions.size() == 0)
        descriptions.push_back("any");

    return descriptions;
}

std::string ItemFilter::getMaterialFilterAsSerial()
{
    std::string str;

    str.append(bitfield_to_string(mat_mask, ","));
    str.append("/");
    if (materials.size() > 0)
    {
        for (size_t i = 0; i < materials.size(); i++)
            str.append(materials[i].getToken() + ",");

        if (str[str.size()-1] == ',')
            str.resize(str.size () - 1);
    }

    return str;
}

bool ItemFilter::parseSerializedMaterialTokens(std::string str)
{
    valid = false;
    std::vector<std::string> tokens;
    split_string(&tokens, str, "/");

    if (tokens.size() > 0 && !tokens[0].empty())
    {
        if (!parseJobMaterialCategory(&mat_mask, tokens[0]))
            return false;
    }

    if (tokens.size() > 1 && !tokens[1].empty())
    {
        std::vector<std::string> mat_names;
        split_string(&mat_names, tokens[1], ",");
        for (auto m = mat_names.begin(); m != mat_names.end(); m++)
        {
            DFHack::MaterialInfo material;
            if (!material.find(*m) || !material.isValid())
                return false;

            materials.push_back(material);
        }
    }

    valid = true;
    return true;
}

std::string ItemFilter::getMinQuality()
{
    return ENUM_KEY_STR(item_quality, min_quality);
}

std::string ItemFilter::getMaxQuality()
{
    return ENUM_KEY_STR(item_quality, max_quality);
}

bool ItemFilter::isValid()
{
    return valid;
}

void ItemFilter::clear()
{
    mat_mask.whole = 0;
    materials.clear();
}

/*
* PlannedBuilding
*/

PlannedBuilding::PlannedBuilding(df::building *building, ItemFilter *filter)
{
    this->building = building;
    this->filter = *filter;
    pos = df::coord(building->centerx, building->centery, building->z);
    config = DFHack::World::AddPersistentData("buildingplan/constraints");
    config.val() = filter->getMaterialFilterAsSerial();
    config.ival(1) = building->id;
    config.ival(2) = filter->min_quality + 1;
    config.ival(3) = static_cast<int>(filter->decorated_only) + 1;
    config.ival(4) = filter->max_quality + 1;
}

PlannedBuilding::PlannedBuilding(PersistentDataItem &config, color_ostream &out)
{
    this->config = config;

    if (!filter.parseSerializedMaterialTokens(config.val()))
    {
        out.printerr("Buildingplan: Cannot parse filter: %s\nDiscarding.", config.val().c_str());
        return;
    }

    building = df::building::find(config.ival(1));
    if (!building)
        return;

    pos = df::coord(building->centerx, building->centery, building->z);
    filter.min_quality = static_cast<df::item_quality>(config.ival(2) - 1);
    filter.max_quality = static_cast<df::item_quality>(config.ival(4) - 1);
    filter.decorated_only = config.ival(3) - 1;
}

bool PlannedBuilding::assignClosestItem(std::vector<df::item *> *items_vector)
{
    decltype(items_vector->begin()) closest_item;
    int32_t closest_distance = -1;
    for (auto item_iter = items_vector->begin(); item_iter != items_vector->end(); item_iter++)
    {
        auto item = *item_iter;
        if (!filter.matches(item))
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

bool PlannedBuilding::isValid()
{
    bool valid = filter.isValid() &&
        building && Buildings::findAtTile(pos) == building &&
        building->getBuildStage() == 0;

    if (!valid)
        remove();

    return valid;
}

df::building_type PlannedBuilding::getType()
{
    return building->getType();
}

bool PlannedBuilding::isCurrentlySelectedBuilding()
{
    return isValid() && (building == df::global::world->selected_building);
}

ItemFilter *PlannedBuilding::getFilter()
{
    return &filter;
}

void PlannedBuilding::remove()
{
    DFHack::World::DeletePersistentData(config);
}

/*
* Planner
*/

Planner::Planner() : in_dummmy_screen(false), quickfort_mode(false) { }

void enable_quickfort_fn(pair<const df::building_type, bool>& pair) { pair.second = true; }

bool Planner::isPlanableBuilding(const df::building_type type) const
{
    return item_for_building_type.find(type) != item_for_building_type.end();
}

void Planner::reset(color_ostream &out)
{
    planned_buildings.clear();
    std::vector<PersistentDataItem> items;
    DFHack::World::GetPersistentData(&items, "buildingplan/constraints");

    for (auto i = items.begin(); i != items.end(); i++)
    {
        PlannedBuilding pb(*i, out);
        if (pb.isValid())
            planned_buildings.push_back(pb);
    }
}

void Planner::initialize()
{
#define add_building_type(btype, itype) \
    item_for_building_type[df::building_type::btype] = df::item_type::itype; \
    default_item_filters[df::building_type::btype] =  ItemFilter(); \
    available_item_vectors[df::item_type::itype] = std::vector<df::item *>(); \
    is_relevant_item_type[df::item_type::itype] = true; \
    if (planmode_enabled.find(df::building_type::btype) == planmode_enabled.end()) \
        planmode_enabled[df::building_type::btype] = false

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

void Planner::addPlannedBuilding(df::building *bld)
{
    for (auto iter = bld->jobs.begin(); iter != bld->jobs.end(); iter++)
    {
        (*iter)->flags.bits.suspend = true;
    }

    PlannedBuilding pb(bld, &default_item_filters[bld->getType()]);
    planned_buildings.push_back(pb);
}

void Planner::doCycle()
{
    debug("Running Cycle");
    if (planned_buildings.size() == 0)
        return;

    debug("Planned count: " + int_to_string(planned_buildings.size()));

    gather_available_items();
    for (auto building_iter = planned_buildings.begin(); building_iter != planned_buildings.end();)
    {
        if (building_iter->isValid())
        {
            if (show_debugging)
                debug(std::string("Trying to allocate ") + enum_item_key_str(building_iter->getType()));

            auto required_item_type = item_for_building_type[building_iter->getType()];
            auto items_vector = &available_item_vectors[required_item_type];
            if (items_vector->size() == 0 || !building_iter->assignClosestItem(items_vector))
            {
                debug("Unable to allocate an item");
                ++building_iter;
                continue;
            }
        }
        debug("Removing building plan");
        building_iter = planned_buildings.erase(building_iter);
    }
}

bool Planner::allocatePlannedBuilding(df::building_type type)
{
    coord32_t cursor;
    if (!DFHack::Gui::getCursorCoords(cursor.x, cursor.y, cursor.z))
        return false;

    auto newinst = Buildings::allocInstance(cursor.get_coord16(), type);
    if (!newinst)
        return false;

    df::job_item *filter = new df::job_item();
    filter->item_type = item_type::NONE;
    filter->mat_index = 0;
    filter->flags2.bits.building_material = true;
    std::vector<df::job_item*> filters;
    filters.push_back(filter);

    if (!Buildings::constructWithFilters(newinst, filters))
    {
        delete newinst;
        return false;
    }

    if (type == building_type::Door)
    {
        auto door = virtual_cast<df::building_doorst>(newinst);
        if (door)
            door->door_flags.bits.pet_passable = true;
    }

    addPlannedBuilding(newinst);

    return true;
}

PlannedBuilding *Planner::getSelectedPlannedBuilding()
{
    for (auto building_iter = planned_buildings.begin(); building_iter != planned_buildings.end(); building_iter++)
    {
        if (building_iter->isCurrentlySelectedBuilding())
        {
            return &(*building_iter);
        }
    }

    return nullptr;
}

void Planner::removeSelectedPlannedBuilding() { getSelectedPlannedBuilding()->remove(); }

ItemFilter *Planner::getDefaultItemFilterForType(df::building_type type) { return &default_item_filters[type]; }

void Planner::adjustMinQuality(df::building_type type, int amount)
{
    auto min_quality = &getDefaultItemFilterForType(type)->min_quality;
    *min_quality = static_cast<df::item_quality>(*min_quality + amount);

    boundsCheckItemQuality(min_quality);
    auto max_quality = &getDefaultItemFilterForType(type)->max_quality;
    if (*min_quality > *max_quality)
        (*max_quality) = *min_quality;

}

void Planner::adjustMaxQuality(df::building_type type, int amount)
{
    auto max_quality = &getDefaultItemFilterForType(type)->max_quality;
    *max_quality = static_cast<df::item_quality>(*max_quality + amount);

    boundsCheckItemQuality(max_quality);
    auto min_quality = &getDefaultItemFilterForType(type)->min_quality;
    if (*max_quality < *min_quality)
        (*min_quality) = *max_quality;
}

void Planner::enableQuickfortMode()
{
    saved_planmodes = planmode_enabled;
    for_each_(planmode_enabled, enable_quickfort_fn);

    quickfort_mode = true;
}

void Planner::disableQuickfortMode()
{
    planmode_enabled = saved_planmodes;
    quickfort_mode = false;
}

bool Planner::inQuickFortMode() { return quickfort_mode; }

void Planner::boundsCheckItemQuality(item_quality::item_quality *quality)
{
    *quality = static_cast<df::item_quality>(*quality);
    if (*quality > item_quality::Artifact)
        (*quality) = item_quality::Artifact;
    if (*quality < item_quality::Ordinary)
        (*quality) = item_quality::Ordinary;
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

std::map<df::building_type, bool> planmode_enabled, saved_planmodes;
Planner planner;
