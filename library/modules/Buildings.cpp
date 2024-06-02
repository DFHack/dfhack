/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "Internal.h"

#include "ColorText.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "Types.h"
#include "Error.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "TileTypes.h"
#include "MiscUtils.h"
#include "DataDefs.h"

#include "modules/Buildings.h"
#include "modules/Maps.h"
#include "modules/Job.h"

#include "df/building_axle_horizontalst.h"
#include "df/building_bars_floorst.h"
#include "df/building_bars_verticalst.h"
#include "df/building_bridgest.h"
#include "df/building_cagest.h"
#include "df/building_civzonest.h"
#include "df/building_coffinst.h"
#include "df/building_def.h"
#include "df/building_design.h"
#include "df/building_floodgatest.h"
#include "df/building_furnacest.h"
#include "df/building_grate_floorst.h"
#include "df/building_grate_wallst.h"
#include "df/building_rollersst.h"
#include "df/building_screw_pumpst.h"
#include "df/building_stockpilest.h"
#include "df/building_trapst.h"
#include "df/building_water_wheelst.h"
#include "df/building_weaponst.h"
#include "df/building_wellst.h"
#include "df/building_workshopst.h"
#include "df/buildings_other_id.h"
#include "df/d_init.h"
#include "df/dfhack_room_quality_level.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_contains_unitst.h"
#include "df/item.h"
#include "df/item_cagest.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/map_block.h"
#include "df/tile_occupancy.h"
#include "df/inorganic_raw.h"
#include "df/plotinfost.h"
#include "df/squad.h"
#include "df/ui_look_list.h"
#include "df/unit.h"
#include "df/unit_relationship_type.h"
#include "df/world.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

using namespace DFHack;
using namespace df::enums;

using df::global::plotinfo;
using df::global::world;
using df::global::d_init;
using df::global::building_next_id;
using df::global::process_jobs;
using df::building_def;

using std::map;
using std::string;
using std::unordered_map;
using std::vector;

struct CoordHash {
    size_t operator()(const df::coord pos) const {
        return pos.x*65537 + pos.y*17 + pos.z;
    }
};

static unordered_map<df::coord, int32_t, CoordHash> locationToBuilding;

static df::building_extents_type *getExtentTile(df::building_extents &extent, df::coord2d tile)
{
    if (!extent.extents)
        return NULL;
    int dx = tile.x - extent.x;
    int dy = tile.y - extent.y;
    if (dx < 0 || dy < 0 || dx >= extent.width || dy >= extent.height)
        return NULL;
    return &extent.extents[dx + dy*extent.width];
}

/*
 * A monitor to work around this bug, in its application to buildings:
 *
 * http://www.bay12games.com/dwarves/mantisbt/view.php?id=1416
 */
bool buildings_do_onupdate = false;

void buildings_onStateChange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        buildings_do_onupdate = true;
        break;
    case SC_MAP_UNLOADED:
        buildings_do_onupdate = false;
        break;
    default:
        break;
    }
}

void buildings_onUpdate(color_ostream &out)
{
    buildings_do_onupdate = false;

    df::job_list_link *link = world->jobs.list.next;
    for (; link; link = link->next) {
        df::job *job = link->item;

        if (job->job_type != job_type::ConstructBuilding)
            continue;
        if (job->job_items.empty())
            continue;

        buildings_do_onupdate = true;

        for (size_t i = 0; i < job->items.size(); i++)
        {
            df::job_item_ref *iref = job->items[i];
            if (iref->role != df::job_item_ref::Reagent)
                continue;
            df::job_item *item = vector_get(job->job_items, iref->job_item_idx);
            if (!item)
                continue;
            // Convert Reagent to Hauled, while decrementing quantity
            item->quantity = std::max(0, item->quantity-1);
            iref->role = df::job_item_ref::Hauled;
            iref->job_item_idx = -1;
        }
    }
}

static void building_into_zone_unidir(df::building* bld, df::building_civzonest* zone)
{
    for (auto contained_building : zone->contained_buildings)
    {
        if (contained_building == bld)
            return;
    }

    insert_into_vector(zone->contained_buildings, &df::building::id, bld);
}

static void zone_into_building_unidir(df::building* bld, df::building_civzonest* zone)
{
    for (auto relation : bld->relations)
    {
        if (relation == zone)
            return;
    }

    insert_into_vector(bld->relations, &df::building_civzonest::id, zone);
}

static bool is_suitable_building_for_zoning(df::building* bld)
{
    return bld->canMakeRoom();
}

static void add_building_to_zone(df::building* bld, df::building_civzonest* zone)
{
    if (!is_suitable_building_for_zoning(bld))
        return;

    building_into_zone_unidir(bld, zone);
    zone_into_building_unidir(bld, zone);
}

static void add_building_to_all_zones(df::building* bld)
{
    if (!is_suitable_building_for_zoning(bld))
        return;

    df::coord coord(bld->centerx, bld->centery, bld->z);

    std::vector<df::building_civzonest*> cv;
    Buildings::findCivzonesAt(&cv, coord);

    for (auto zone : cv)
        add_building_to_zone(bld, zone);
}

static void add_zone_to_all_buildings(df::building* zone_as_building)
{
    if (zone_as_building->getType() != building_type::Civzone)
        return;

    auto zone = strict_virtual_cast<df::building_civzonest>(zone_as_building);

    if (zone == nullptr)
        return;

    for (auto bld : world->buildings.other.IN_PLAY)
    {
        if (zone->z != bld->z)
            continue;

        if (!is_suitable_building_for_zoning(bld))
            continue;

        int32_t cx = bld->centerx;
        int32_t cy = bld->centery;

        df::coord2d coord(cx, cy);

        //can a zone without extents exist?
        if (zone->room.extents && zone->isExtentShaped())
        {
            auto etile = getExtentTile(zone->room, coord);
            if (!etile || !*etile)
                continue;

            add_building_to_zone(bld, zone);
        }
    }
}

static void remove_building_from_zone(df::building* bld, df::building_civzonest* zone)
{
    for (int bid = 0; bid < (int)zone->contained_buildings.size(); bid++)
    {
        if (zone->contained_buildings[bid] == bld)
        {
            zone->contained_buildings.erase(zone->contained_buildings.begin() + bid);
            bid--;
        }
    }

    for (int bid = 0; bid < (int)bld->relations.size(); bid++)
    {
        if (bld->relations[bid] == zone)
        {
            bld->relations.erase(bld->relations.begin() + bid);
            bid--;
        }
    }
}

static void remove_building_from_all_zones(df::building* bld)
{
    df::coord coord(bld->centerx, bld->centery, bld->z);

    std::vector<df::building_civzonest*> cv;
    Buildings::findCivzonesAt(&cv, coord);

    for (auto zone : cv)
        remove_building_from_zone(bld, zone);
}

static void remove_zone_from_all_buildings(df::building* zone_as_building)
{
    if (zone_as_building->getType() != building_type::Civzone)
        return;

    auto zone = strict_virtual_cast<df::building_civzonest>(zone_as_building);

    if (zone == nullptr)
        return;

    //this is a paranoid check and slower than it could be. Zones contain a list of children
    //good for fixing potentially bad game states when deleting a zone
    for (auto bld : world->buildings.other.IN_PLAY)
        remove_building_from_zone(bld, zone);
}

uint32_t Buildings::getNumBuildings()
{
    return world->buildings.all.size();
}

bool Buildings::Read (const uint32_t index, t_building & building)
{
    df::building *bld = world->buildings.all[index];

    building.x1 = bld->x1;
    building.x2 = bld->x2;
    building.y1 = bld->y1;
    building.y2 = bld->y2;
    building.z = bld->z;
    building.material.index = bld->mat_index;
    building.material.type = bld->mat_type;
    building.type = bld->getType();
    building.subtype = bld->getSubtype();
    building.custom_type = bld->getCustomType();
    building.origin = bld;
    return true;
}

bool Buildings::ReadCustomWorkshopTypes(map <uint32_t, string> & btypes)
{
    vector <building_def *> & bld_def = world->raws.buildings.all;
    btypes.clear();

    for (building_def *temp : bld_def)
    {
        btypes[temp->id] = temp->code;
    }
    return true;
}

df::general_ref *Buildings::getGeneralRef(df::building *building, df::general_ref_type type)
{
    CHECK_NULL_POINTER(building);

    return findRef(building->general_refs, type);
}

df::specific_ref *Buildings::getSpecificRef(df::building *building, df::specific_ref_type type)
{
    CHECK_NULL_POINTER(building);

    return findRef(building->specific_refs, type);
}

std::string Buildings::getName(df::building* building)
{
    CHECK_NULL_POINTER(building);

    std::string tmp;
    building->getName(&tmp);
    return tmp;
}

bool Buildings::setOwner(df::building_civzonest *bld, df::unit *unit)
{
    CHECK_NULL_POINTER(bld);

    if (bld->assigned_unit == unit)
        return true;

    if (bld->assigned_unit)
    {
        auto &blist = bld->assigned_unit->owned_buildings;
        vector_erase_at(blist, linear_index(blist, bld));

        if (auto spouse = df::unit::find(bld->assigned_unit->relationship_ids[df::unit_relationship_type::Spouse]))
        {
            auto &blist = spouse->owned_buildings;
            vector_erase_at(blist, linear_index(blist, bld));
        }
    }

    bld->assigned_unit = unit;

    if (unit)
    {
        bld->assigned_unit_id = unit->id;
        unit->owned_buildings.push_back(bld);

        if (auto spouse = df::unit::find(unit->relationship_ids[df::unit_relationship_type::Spouse]))
        {
            auto &blist = spouse->owned_buildings;
            if (bld->canUseSpouseRoom() && linear_index(blist, bld) < 0)
                blist.push_back(bld);
        }
    }
    else
    {
        bld->assigned_unit_id = -1;
    }

    return true;
}

df::building *Buildings::findAtTile(df::coord pos)
{
    auto occ = Maps::getTileOccupancy(pos);
    if (!occ || !occ->bits.building)
        return NULL;

    // Try cache lookup in case it works:
    auto cached = locationToBuilding.find(pos);
    if (cached != locationToBuilding.end())
    {
        auto building = df::building::find(cached->second);

        if (building && building->z == pos.z &&
            building->isSettingOccupancy() &&
            containsTile(building, pos))
        {
            return building;
        }
    }

    // The authentic method, i.e. how the game generally does this:
    auto &vec = df::building::get_vector();
    for (size_t i = 0; i < vec.size(); i++)
    {
        auto bld = vec[i];

        if (pos.z != bld->z ||
            pos.x < bld->x1 || pos.x > bld->x2 ||
            pos.y < bld->y1 || pos.y > bld->y2)
            continue;

        if (!bld->isSettingOccupancy())
            continue;

        if (bld->room.extents && bld->isExtentShaped())
        {
            auto etile = getExtentTile(bld->room, pos);
            if (!etile || !*etile)
                continue;
        }

        return bld;
    }

    return NULL;
}

static unordered_map<int32_t, df::coord> corner1;
static unordered_map<int32_t, df::coord> corner2;
static void cacheBuilding(df::building *building) {
    int32_t id = building->id;
    df::coord p1(std::min(building->x1, building->x2), std::min(building->y1,building->y2), building->z);
    df::coord p2(std::max(building->x1, building->x2), std::max(building->y1,building->y2), building->z);

    corner1[id] = p1;
    corner2[id] = p2;

    for (int32_t x = p1.x; x <= p2.x; x++) {
        for (int32_t y = p1.y; y <= p2.y; y++) {
            df::coord pt(x, y, building->z);
            if (Buildings::containsTile(building, pt)) {
                locationToBuilding[pt] = id;
            }
        }
    }
}

bool Buildings::findCivzonesAt(std::vector<df::building_civzonest*> *pvec,
                               df::coord pos) {
    pvec->clear();

    for (df::building_civzonest* zone : world->buildings.other.ANY_ZONE)
    {
        if (pos.z != zone->z)
            continue;

        if (zone->room.extents && zone->isExtentShaped())
        {
            auto etile = getExtentTile(zone->room, pos);
            if (!etile || !*etile)
                continue;

            pvec->push_back(zone);
        }
    }

    return !pvec->empty();
}

df::building *Buildings::allocInstance(df::coord pos, df::building_type type, int subtype, int custom)
{
    if (!building_next_id)
        return NULL;

    // Allocate object
    const char *classname = ENUM_ATTR(building_type, classname, type);
    if (!classname)
        return NULL;

    auto id = virtual_identity::find(classname);
    if (!id)
        return NULL;

    df::building *bld = (df::building*)id->allocate();
    if (!bld)
        return NULL;

    // Init base fields
    bld->x1 = bld->x2 = bld->centerx = pos.x;
    bld->y1 = bld->y2 = bld->centery = pos.y;
    bld->z = pos.z;

    bld->race = plotinfo->race_id;

    if (subtype != -1)
        bld->setSubtype(subtype);
    if (custom != -1)
        bld->setCustomType(custom);

    bld->setMaterialAmount(1);

    // Type specific init
    switch (type)
    {
    case building_type::Well:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_wellst, bld))
            obj->bucket_z = bld->z;
        break;
    }
    case building_type::Workshop:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_workshopst, bld))
            obj->profile.max_general_orders = 5;
        break;
    }
    case building_type::Furnace:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_furnacest, bld))
        {
            obj->melt_remainder.resize(df::inorganic_raw::get_vector().size(), 0);
            obj->profile.max_general_orders = 5;
        }
        break;
    }
/* TODO: understand how this changes for v50
    case building_type::Coffin:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_coffinst, bld))
            obj->initBurialFlags(); // DF has this copy&pasted
        break;
    }
*/
    case building_type::Trap:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_trapst, bld))
        {
            if (obj->trap_type == trap_type::PressurePlate)
                obj->ready_timeout = 500;
        }
        break;
    }
    case building_type::Floodgate:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_floodgatest, bld))
            obj->gate_flags.bits.closed = true;
        break;
    }
    case building_type::GrateWall:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_grate_wallst, bld))
            obj->gate_flags.bits.closed = true;
        break;
    }
    case building_type::GrateFloor:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_grate_floorst, bld))
            obj->gate_flags.bits.closed = true;
        break;
    }
    case building_type::BarsVertical:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_bars_verticalst, bld))
            obj->gate_flags.bits.closed = true;
        break;
    }
    case building_type::BarsFloor:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_bars_floorst, bld))
            obj->gate_flags.bits.closed = true;
        break;
    }
    case building_type::Bridge:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_bridgest, bld))
            obj->gate_flags.bits.closed = false;
        break;
    }
    case building_type::Weapon:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_weaponst, bld))
            obj->gate_flags.bits.closed = false;
        break;
    }
    default:
        break;
    }

    return bld;
}

static void makeOneDim(df::coord2d &size, df::coord2d &center, bool vertical)
{
    if (vertical)
        size.x = 1;
    else
        size.y = 1;
    center = size/2;
}

bool Buildings::getCorrectSize(df::coord2d &size, df::coord2d &center,
                               df::building_type type, int subtype, int custom, int direction)
{
    using namespace df::enums::building_type;

    if (size.x <= 0)
        size.x = 1;
    if (size.y <= 0)
        size.y = 1;

    switch (type)
    {
    case FarmPlot:
    case Bridge:
    case RoadDirt:
    case RoadPaved:
    case Stockpile:
    case Civzone:
        center = size/2;
        return true;

    case TradeDepot:
    case Shop:
        size = df::coord2d(5,5);
        center = df::coord2d(2,2);
        return false;

    case SiegeEngine:
    case Windmill:
    case Wagon:
        size = df::coord2d(3,3);
        center = df::coord2d(1,1);
        return false;

    case AxleHorizontal:
        makeOneDim(size, center, direction);
        return true;

    case Rollers:
        makeOneDim(size, center, (direction&1) == 0);
        return true;

    case WaterWheel:
        size = df::coord2d(3,3);
        makeOneDim(size, center, direction);
        return false;

    case Workshop:
    {
        using namespace df::enums::workshop_type;

        switch ((df::workshop_type)subtype)
        {
            case Quern:
            case Millstone:
            case Tool:
                size = df::coord2d(1,1);
                center = df::coord2d(0,0);
                break;

            case Siege:
            case Kennels:
                size = df::coord2d(5,5);
                center = df::coord2d(2,2);
                break;

            case Custom:
                if (auto def = df::building_def::find(custom))
                {
                    size = df::coord2d(def->dim_x, def->dim_y);
                    center = df::coord2d(def->workloc_x, def->workloc_y);
                    break;
                }

            default:
                size = df::coord2d(3,3);
                center = df::coord2d(1,1);
        }

        return false;
    }

    case Furnace:
    {
        using namespace df::enums::furnace_type;

        switch ((df::furnace_type)subtype)
        {
            case Custom:
                if (auto def = df::building_def::find(custom))
                {
                    size = df::coord2d(def->dim_x, def->dim_y);
                    center = df::coord2d(def->workloc_x, def->workloc_y);
                    break;
                }

            default:
                size = df::coord2d(3,3);
                center = df::coord2d(1,1);
        }

        return false;
    }

    case ScrewPump:
    {
        using namespace df::enums::screw_pump_direction;

        switch ((df::screw_pump_direction)direction)
        {
            case FromEast:
                size = df::coord2d(2,1);
                center = df::coord2d(1,0);
                break;
            case FromSouth:
                size = df::coord2d(1,2);
                center = df::coord2d(0,1);
                break;
            case FromWest:
                size = df::coord2d(2,1);
                center = df::coord2d(0,0);
                break;
            default:
                size = df::coord2d(1,2);
                center = df::coord2d(0,0);
        }

        return false;
    }

    default:
        size = df::coord2d(1,1);
        center = df::coord2d(0,0);
        return false;
    }
}

static void init_extents(df::building_extents *ext, const df::coord &pos,
                         const df::coord2d &size)
{
    ext->extents = new df::building_extents_type[size.x*size.y];
    ext->x = pos.x;
    ext->y = pos.y;
    ext->width = size.x;
    ext->height = size.y;

    memset(ext->extents, 1, size.x*size.y);
}

bool Buildings::checkFreeTiles(df::coord pos, df::coord2d size,
                               df::building_extents *ext,
                               bool create_ext,
                               bool allow_occupied,
                               bool allow_wall,
                               bool allow_flow)
{
    bool found_any = false;

    for (int dx = 0; dx < size.x; dx++)
    {
        for (int dy = 0; dy < size.y; dy++)
        {
            df::coord tile = pos + df::coord(dx,dy,0);
            df::building_extents_type *etile = NULL;

            // Exclude using extents
            if (ext && ext->extents)
            {
                etile = getExtentTile(*ext, tile);
                if (!etile || !*etile)
                    continue;
            }

            // Look up map block
            df::map_block *block = Maps::getTileBlock(tile);
            if (!block)
                return false;

            df::coord2d btile = df::coord2d(tile) & 15;

            bool allowed = true;

            // Check occupancy and tile type
            if (!allow_occupied &&
                block->occupancy[btile.x][btile.y].bits.building)
                allowed = false;
            else if (!allow_wall)
            {
                auto &tt = block->tiletype[btile.x][btile.y];
                if (!HighPassable(tt))
                    allowed = false;
                auto &des = block->designation[btile.x][btile.y];
                if (!allow_flow && (des.bits.flow_size > 1 ||
                        (des.bits.flow_size >= 1 && des.bits.liquid_type == df::tile_liquid::Magma)))
                    allowed = false;
            }

            // Create extents if requested
            if (allowed)
                found_any = true;
            else
            {
                if (!ext || !create_ext)
                    return false;

                if (!ext->extents)
                {
                    init_extents(ext, pos, size);
                    etile = getExtentTile(*ext, tile);
                }

                if (!etile)
                    return false;

                *etile = df::building_extents_type::None;
            }
        }
    }

    return found_any;
}

std::pair<df::coord,df::coord2d> Buildings::getSize(df::building *bld)
{
    CHECK_NULL_POINTER(bld);

    df::coord pos(bld->x1,bld->y1,bld->z);

    return std::pair<df::coord,df::coord2d>(pos, df::coord2d(bld->x2+1,bld->y2+1) - pos);
}

static bool checkBuildingTiles(df::building *bld, bool can_change,
                               bool force_extents = false)
{
    auto psize = Buildings::getSize(bld);

    if (force_extents && !bld->room.extents)
    {
        // populate the room structure if it's not set already
        init_extents(&bld->room, psize.first, psize.second);
    }

    return Buildings::checkFreeTiles(psize.first, psize.second, &bld->room,
                                     can_change && bld->isExtentShaped(),
                                     !bld->isSettingOccupancy(),
                                     bld->getType() ==
                                        df::building_type::Civzone,
                                     !bld->isActual());
}

int Buildings::countExtentTiles(df::building_extents *ext, int defval)
{
    if (!ext || !ext->extents)
        return defval;

    int cnt = 0;
    for (int i = 0; i < ext->width * ext->height; i++)
        if (ext->extents[i])
            cnt++;
    return cnt;
}

bool Buildings::containsTile(df::building *bld, df::coord2d tile) {
    CHECK_NULL_POINTER(bld);

    if (!bld->isExtentShaped() || !bld->room.extents) {
        if (tile.x < bld->x1 || tile.x > bld->x2 || tile.y < bld->y1 || tile.y > bld->y2)
            return false;
    }

    if (!bld->room.extents)
        return true;

    df::building_extents_type *etile = getExtentTile(bld->room, tile);
    return etile && *etile;
}

bool Buildings::hasSupport(df::coord pos, df::coord2d size)
{
    for (int dx = -1; dx <= size.x; dx++)
    {
        for (int dy = -1; dy <= size.y; dy++)
        {
            // skip corners
            if ((dx < 0 || dx == size.x) && (dy < 0 || dy == size.y))
                continue;

            df::coord tile = pos + df::coord(dx,dy,0);
            df::map_block *block = Maps::getTileBlock(tile);
            if (!block)
                continue;

            df::coord2d btile = df::coord2d(tile) & 15;
            if (!isOpenTerrain(block->tiletype[btile.x][btile.y]))
                return true;
        }
    }

    return false;
}

static int computeMaterialAmount(df::building *bld)
{
    auto size = Buildings::getSize(bld).second;
    int cnt = size.x * size.y;

    if (bld->room.extents && bld->isExtentShaped())
        cnt = Buildings::countExtentTiles(&bld->room, cnt);

    return cnt/4 + 1;
}

bool Buildings::setSize(df::building *bld, df::coord2d size, int direction)
{
    CHECK_NULL_POINTER(bld);
    CHECK_INVALID_ARGUMENT(bld->id == -1);

    // Compute correct size and apply it
    df::coord2d old_size = size;
    df::coord2d center;
    getCorrectSize(size, center, bld->getType(), bld->getSubtype(),
                   bld->getCustomType(), direction);

    // Delete old extents if size has changed.
    if (old_size != size && bld->room.extents)
    {
        delete[] bld->room.extents;
        bld->room.extents = NULL;
    }

    bld->x2 = bld->x1 + size.x - 1;
    bld->y2 = bld->y1 + size.y - 1;
    bld->centerx = bld->x1 + center.x;
    bld->centery = bld->y1 + center.y;

    auto type = bld->getType();

    using namespace df::enums::building_type;

    switch (type)
    {
    case WaterWheel:
        {
            auto obj = (df::building_water_wheelst*)bld;
            obj->is_vertical = !!direction;
            break;
        }
    case AxleHorizontal:
        {
            auto obj = (df::building_axle_horizontalst*)bld;
            obj->is_vertical = !!direction;
            break;
        }
    case ScrewPump:
        {
            auto obj = (df::building_screw_pumpst*)bld;
            obj->direction = (df::screw_pump_direction)direction;
            break;
        }
    case Rollers:
        {
            auto obj = (df::building_rollersst*)bld;
            obj->direction = (df::screw_pump_direction)direction;
            break;
        }
    case Bridge:
        {
            auto obj = (df::building_bridgest*)bld;
            auto psize = getSize(bld);
            obj->gate_flags.bits.has_support = hasSupport(psize.first, psize.second);
            obj->direction = (df::building_bridgest::T_direction)direction;
            break;
        }
    default:
        break;
    }

    bool ok = checkBuildingTiles(bld, true);

    if (type != Construction)
        bld->setMaterialAmount(computeMaterialAmount(bld));

    return ok;
}

static void markBuildingTiles(df::building *bld, bool remove)
{
    bool use_extents = bld->room.extents && bld->isExtentShaped();
    bool stockpile = (bld->getType() == building_type::Stockpile);
    bool complete = (bld->getBuildStage() >= bld->getMaxBuildStage());

    if (remove)
        stockpile = complete = false;

    for (int tx = bld->x1; tx <= bld->x2; tx++)
    {
        for (int ty = bld->y1; ty <= bld->y2; ty++)
        {
            df::coord tile(tx,ty,bld->z);

            if (use_extents)
            {
                df::building_extents_type *etile = getExtentTile(bld->room, tile);
                if (!etile || !*etile)
                    continue;
            }

            df::map_block *block = Maps::getTileBlock(tile);
            df::coord2d btile = df::coord2d(tile) & 15;

            auto &des = block->designation[btile.x][btile.y];

            des.bits.pile = stockpile;
            if (!remove)
                des.bits.dig = tile_dig_designation::No;

            if (complete)
                bld->updateOccupancy(tx, ty);
            else
            {
                auto &occ = block->occupancy[btile.x][btile.y];

                if (remove)
                    occ.bits.building = tile_building_occ::None;
                else
                    occ.bits.building = tile_building_occ::Planned;
            }
        }
    }
}

static void linkRooms(df::building *bld)
{
/* TODO: understand how this changes for v50
    auto &vec = world->buildings.other[buildings_other_id::IN_PLAY];

    bool changed = false;

    for (size_t i = 0; i < vec.size(); i++)
    {
        auto room = vec[i];
        if (!room->is_room || room->z != bld->z || room == bld)
            continue;

        df::building_extents_type *pext = getExtentTile(room->room, df::coord2d(bld->x1, bld->y1));
        if (!pext || !*pext)
            continue;

        changed = true;
        room->children.push_back(bld);
        bld->parents.push_back(room);

        // TODO: the game updates room rent here if economy is enabled
    }

    if (changed)
        df::global::plotinfo->equipment.update.bits.buildings = true;
*/
}

static void unlinkRooms(df::building *bld)
{
/* TODO: understand how this changes for v50
    for (size_t i = 0; i < bld->parents.size(); i++)
    {
        auto parent = bld->parents[i];
        int idx = linear_index(parent->children, bld);
        vector_erase_at(parent->children, idx);
    }

    bld->parents.clear();
*/
}

static void linkBuilding(df::building *bld)
{
    bld->id = (*building_next_id)++;

    world->buildings.all.push_back(bld);
    bld->categorize(true);

    if (bld->isSettingOccupancy())
        markBuildingTiles(bld, false);

    linkRooms(bld);

    Job::checkBuildingsNow();
}

static void createDesign(df::building *bld, bool rough)
{
    auto job = bld->jobs[0];

    job->mat_type = bld->mat_type;
    job->mat_index = bld->mat_index;

    if (bld->needsDesign())
    {
        auto act = (df::building_actual*)bld;
        act->design = new df::building_design();

        act->design->flags.bits.rough = rough;
    }
}

static int getMaxStockpileId()
{
    int max_id = 0;
    for (auto bld : world->buildings.other.STOCKPILE)
        max_id = std::max(max_id, bld->stockpile_number);
    return max_id;
}

static int getMaxCivzoneId()
{
    int max_id = 0;
    for (auto bld : world->buildings.other.ANY_ZONE)
        max_id = std::max(max_id, bld->zone_num);
    return max_id;
}

bool Buildings::constructAbstract(df::building *bld)
{
    CHECK_NULL_POINTER(bld);
    CHECK_INVALID_ARGUMENT(bld->id == -1);
    CHECK_INVALID_ARGUMENT(!bld->isActual());

    if (!checkBuildingTiles(bld, true, true))
        return false;

    switch (bld->getType())
    {
        case building_type::Stockpile:

            if (auto stock = strict_virtual_cast<df::building_stockpilest>(bld))
                stock->stockpile_number = getMaxStockpileId() + 1;
            break;

        case building_type::Civzone:
            if (auto zone = strict_virtual_cast<df::building_civzonest>(bld))
            {
                zone->zone_num = getMaxCivzoneId() + 1;

                add_zone_to_all_buildings(zone);
            }
            break;

        default:
            break;
    }

    linkBuilding(bld);

    if (!bld->flags.bits.exists)
    {
        bld->flags.bits.exists = true;
        bld->initFarmSeasons();
    }

    return true;
}

static bool linkForConstruct(df::job* &job, df::building *bld)
{
    if (!checkBuildingTiles(bld, false))
        return false;

    auto ref = df::allocate<df::general_ref_building_holderst>();
    if (!ref)
    {
        Core::printerr("Could not allocate general_ref_building_holderst\n");
        return false;
    }

    linkBuilding(bld);

    ref->building_id = bld->id;

    job = new df::job();
    job->job_type = df::job_type::ConstructBuilding;
    job->pos = df::coord(bld->centerx, bld->centery, bld->z);
    job->general_refs.push_back(ref);

    bld->jobs.push_back(job);

    Job::linkIntoWorld(job);

    return true;
}

static bool needsItems(df::building *bld)
{
    if (!bld->isActual())
        return false;

    switch (bld->getType())
    {
        case building_type::FarmPlot:
        case building_type::RoadDirt:
            return false;

        default:
            return true;
    }
}

bool Buildings::constructWithItems(df::building *bld, std::vector<df::item*> items)
{
    CHECK_NULL_POINTER(bld);
    CHECK_INVALID_ARGUMENT(bld->id == -1);
    CHECK_INVALID_ARGUMENT(bld->isActual());
    CHECK_INVALID_ARGUMENT(!items.empty() == needsItems(bld));

    for (size_t i = 0; i < items.size(); i++)
    {
        CHECK_NULL_POINTER(items[i]);

        if (items[i]->flags.bits.in_job)
            return false;
    }

    df::job *job = NULL;
    if (!linkForConstruct(job, bld))
        return false;

    bool rough = false;

    for (size_t i = 0; i < items.size(); i++)
    {
        Job::attachJobItem(job, items[i], df::job_item_ref::Hauled);

        if (items[i]->getType() == item_type::BOULDER)
            rough = true;
        if (bld->mat_type == -1)
            bld->mat_type = items[i]->getMaterial();
        if (bld->mat_index == -1)
            bld->mat_index = items[i]->getMaterialIndex();
    }

    add_building_to_all_zones(bld);

    createDesign(bld, rough);
    return true;
}

bool Buildings::constructWithFilters(df::building *bld, std::vector<df::job_item*> items)
{
    CHECK_NULL_POINTER(bld);
    CHECK_INVALID_ARGUMENT(bld->id == -1);
    CHECK_INVALID_ARGUMENT(bld->isActual());
    CHECK_INVALID_ARGUMENT(!items.empty() == needsItems(bld));

    for (size_t i = 0; i < items.size(); i++)
        CHECK_NULL_POINTER(items[i]);

    df::job *job = NULL;
    if (!linkForConstruct(job, bld))
    {
        for (size_t i = 0; i < items.size(); i++)
            delete items[i];

        return false;
    }

    bool rough = false;

    for (size_t i = 0; i < items.size(); i++)
    {
        if (items[i]->quantity < 0)
            items[i]->quantity = computeMaterialAmount(bld);

        /* The game picks up explicitly listed items in reverse
         * order, but processes filters straight. This reverses
         * the order of filters so as to produce the same final
         * contained_items ordering as the normal ui way. */
        vector_insert_at(job->job_items, 0, items[i]);

        if (items[i]->item_type == item_type::BOULDER)
            rough = true;
        if (bld->mat_type == -1)
            bld->mat_type = items[i]->mat_type;
        if (bld->mat_index == -1)
            bld->mat_index = items[i]->mat_index;
    }

    buildings_do_onupdate = true;

    add_building_to_all_zones(bld);

    createDesign(bld, rough);
    return true;
}

static void delete_civzone_squad_links(df::building_civzonest* zone)
{
    for (df::building_civzonest::T_squad_room_info* room_info : zone->squad_room_info)
    {
        int32_t squad_id = room_info->squad_id;

        df::squad* squad = df::squad::find(squad_id);

        //if this is null, something has gone just *terribly* wrong
        if (squad)
        {
            for (int i=(int)squad->rooms.size() - 1; i >= 0; i--)
            {
                if (squad->rooms[i]->building_id == zone->id)
                {
                    auto room = squad->rooms[i];
                    squad->rooms.erase(squad->rooms.begin() + i);
                    delete room;
                }
            }
        }

        delete room_info;
    }

    zone->squad_room_info.clear();
}

//unit owned_building pointers are known-bad as of 50.05 and dangle on zone delete
//do not use anything that touches anything other than the pointer value
//this means also that if dwarf fortress reuses a memory allocation, we will end up with duplicates
//this vector is also not sorted by id
//it also turns out that multiple units eg (solely?) spouses can point to one room
static void delete_assigned_unit_links(df::building_civzonest* zone)
{
    //not clear if this is always true
    /*if (zone->assigned_unit_id == -1)
        return;*/

    for (df::unit* unit : world->units.active)
    {
        for (int i=(int)unit->owned_buildings.size() - 1; i >= 0; i--)
        {
            if (unit->owned_buildings[i] == zone)
                unit->owned_buildings.erase(unit->owned_buildings.begin() + i);
        }
    }
}

static void on_civzone_delete(df::building_civzonest* civzone)
{
    remove_zone_from_all_buildings(civzone);
    delete_civzone_squad_links(civzone);
    delete_assigned_unit_links(civzone);
}

bool Buildings::deconstruct(df::building *bld)
{
    using df::global::plotinfo;
    using df::global::world;
    using df::global::ui_look_list;

    CHECK_NULL_POINTER(bld);

    if (bld->isActual() && bld->getBuildStage() > 0)
    {
        bld->queueDestroy();
        return false;
    }

    /* Immediate destruction code path.
       Should only happen for abstract and unconstructed buildings.*/

    if (bld->isSettingOccupancy())
    {
        markBuildingTiles(bld, true);
        bld->cleanupMap();
    }

    bld->removeUses(false, false);
    // Assume: no parties.
    unlinkRooms(bld);
    // Assume: not unit destroy target
    int id = bld->id;
    vector_erase_at(plotinfo->tax_collection.rooms, linear_index(plotinfo->tax_collection.rooms, id));
    // Assume: not used in punishment
    // Assume: not used in non-own jobs
    // Assume: does not affect pathfinding
    bld->deconstructItems(false, false, false);
    // Don't clear arrows.

    bld->uncategorize();

    remove_building_from_all_zones(bld);

    if (bld->getType() == df::building_type::Civzone)
    {
        auto zone = strict_virtual_cast<df::building_civzonest>(bld);

        if (zone)
            on_civzone_delete(zone);
    }

    delete bld;

    if (world->selected_building == bld)
    {
        world->selected_building = NULL;
        world->update_selected_building = true;
    }

    for (int i = ui_look_list->items.size()-1; i >= 0; i--)
    {
        auto item = ui_look_list->items[i];
        if (item->type == df::ui_look_list::T_items::Building &&
            item->data.building.bld_id == id)
        {
            vector_erase_at(ui_look_list->items, i);
            delete item;
        }
    }

    Job::checkBuildingsNow();
    Job::checkDesignationsNow();

    return true;
}

bool Buildings::markedForRemoval(df::building *bld)
{
    CHECK_NULL_POINTER(bld);
    for (df::job *job : bld->jobs)
    {
        if (job && job->job_type == df::job_type::DestroyBuilding)
        {
            return true;
        }
    }
    return false;
}

void Buildings::notifyCivzoneModified(df::building* bld)
{
    if (bld->getType() != building_type::Civzone)
        return;

    //remove zone here needs to be the slow method
    remove_zone_from_all_buildings(bld);
    add_zone_to_all_buildings(bld);
}

void Buildings::clearBuildings(color_ostream& out) {
    corner1.clear();
    corner2.clear();
    locationToBuilding.clear();
}

void Buildings::updateBuildings(color_ostream&, void* ptr)
{
    int32_t id = (int32_t)(intptr_t)ptr;
    auto building = df::building::find(id);

    if (building)
    {
        bool is_civzone = !building->isSettingOccupancy();
        if (!corner1.count(id) && !is_civzone)
            cacheBuilding(building);
    }
    else if (corner1.count(id))
    {
        // existing building: destroy it
        // note that civzones are lazy-destroyed in findCivzonesAt() and are
        // not handled here
        df::coord p1 = corner1[id];
        df::coord p2 = corner2[id];

        for ( int32_t x = p1.x; x <= p2.x; x++ ) {
            for ( int32_t y = p1.y; y <= p2.y; y++ ) {
                df::coord pt(x,y,p1.z);

                auto cur = locationToBuilding.find(pt);
                if (cur != locationToBuilding.end() && cur->second == id)
                    locationToBuilding.erase(cur);
            }
        }

        corner1.erase(id);
        corner2.erase(id);
    }
}

static std::map<df::building_type, std::vector<std::string>> room_quality_names = {
    {df::building_type::Bed, {
        "Meager Quarters",
        "Modest Quarters",
        "Quarters",
        "Decent Quarters",
        "Fine Quarters",
        "Great Bedroom",
        "Grand Bedroom",
        "Royal Bedroom"}},
    {df::building_type::Table, {
        "Meager Dining Room",
        "Modest Dining Room",
        "Dining Room",
        "Decent Dining Room",
        "Fine Dining Room",
        "Great Dining Room",
        "Grand Dining Room",
        "Royal Dining Room"}},
    {df::building_type::Chair, {
        "Meager Office",
        "Modest Office",
        "Office",
        "Decent Office",
        "Splendid Office",
        "Throne Room",
        "Opulent Throne Room",
        "Royal Throne Room"}},
    {df::building_type::Coffin, {
        "Grave",
        "Servant's Burial Chamber",
        "Burial Chamber",
        "Tomb",
        "Fine Tomb",
        "Mausoleum",
        "Grand Mausoleum",
        "Royal Mausoleum"}}
};

std::string Buildings::getRoomDescription(df::building *building, df::unit *unit)
{
    CHECK_NULL_POINTER(building);
    // unit can be null

/* TODO: understand how this changes for v50
    if (!building->is_room)
        return "";

    auto btype = building->getType();
    if (room_quality_names.find(btype) == room_quality_names.end())
        return "";

    int32_t value = building->getRoomValue(unit);
    auto level = ENUM_FIRST_ITEM(dfhack_room_quality_level);
    for (auto i_level = level; is_valid_enum_item(i_level); i_level = next_enum_item(i_level, false))
    {
        if (value >= ENUM_ATTR(dfhack_room_quality_level, min_value, i_level))
        {
            level = i_level;
        }
    }

    return vector_get(room_quality_names[btype], size_t(level), string(""));
*/ return "";
}

void Buildings::getStockpileContents(df::building_stockpilest *stockpile, std::vector<df::item*> *items)
{
    CHECK_NULL_POINTER(stockpile);

    items->clear();

    Buildings::StockpileIterator stored;
    for (stored.begin(stockpile); !stored.done(); ++stored) {
        df::item *item = *stored;
        items->push_back(item);
    }
}

bool Buildings::isActivityZone(df::building * building)
{
    CHECK_NULL_POINTER(building);
    return building->getType() == building_type::Civzone
/* TODO: understand how this changes for v50
            && building->getSubtype() == (short)civzone_type::ActivityZone;
*/ ;
}

bool Buildings::isPenPasture(df::building * building)
{
    if (!isActivityZone(building))
        return false;

    return ((df::building_civzonest*)building)->type == civzone_type::Pen;
}

bool Buildings::isPitPond(df::building * building)
{
    if (!isActivityZone(building))
        return false;

    return ((df::building_civzonest*)building)->type == civzone_type::Pond;
}

bool Buildings::isActive(df::building * building)
{
    if (!isActivityZone(building))
        return false;
    return ((df::building_civzonest*)building)->spec_sub_flag.bits.active;
}

 bool Buildings::isAnimalTraining(df::building * building)
 {
     if (!isActivityZone(building))
         return false;

     return ((df::building_civzonest*)building)->type == civzone_type::AnimalTraining;
 }

// returns building of pen/pit at cursor position (NULL if nothing found)
df::building* Buildings::findPenPitAt(df::coord coord)
{
    vector<df::building_civzonest*> zones;
    Buildings::findCivzonesAt(&zones, coord);
    for (auto zone = zones.begin(); zone != zones.end(); ++zone)
    {
        if (isPenPasture(*zone) || isPitPond(*zone))
            return (*zone);
    }
    return NULL;
}

using Buildings::StockpileIterator;
StockpileIterator& StockpileIterator::operator++() {
    while (stockpile) {
        if (block) {
            // Check the next item in the current block.
            ++current;
        }
        else if (stockpile->x2 < 0 || stockpile->y2 < 0 || stockpile->z < 0 || stockpile->x1 > world->map.x_count - 1 || stockpile->y1 > world->map.y_count - 1 || stockpile->z > world->map.z_count - 1) {
            // if the stockpile bounds exist outside of valid map plane then no items can be in the stockpile
            block = NULL;
            item = NULL;
            return *this;
        } else {
            // Start with the top-left block covering the stockpile.
            block = Maps::getTileBlock(std::min(std::max(stockpile->x1, 0), world->map.x_count-1), std::min(std::max(stockpile->y1, 0), world->map.y_count-1), stockpile->z);
            current = 0;
        }

        while (current >= block->items.size()) {
            // Out of items in this block; find the next block to search.
            if (block->map_pos.x + 16 <= std::min(stockpile->x2, world->map.x_count-1)) {
                block = Maps::getTileBlock(block->map_pos.x + 16, block->map_pos.y, stockpile->z);
                current = 0;
            } else if (block->map_pos.y + 16 <= std::min(stockpile->y2, world->map.y_count-1)) {
                block = Maps::getTileBlock(std::max(stockpile->x1, 0), block->map_pos.y + 16, stockpile->z);
                current = 0;
            } else {
                // All items in all blocks have been checked.
                block = NULL;
                item = NULL;
                return *this;
            }
        }

        // If the current item isn't properly stored, move on to the next.
        item = df::item::find(block->items[current]);
        if (!item->flags.bits.on_ground) {
            continue;
        }

        if (!Buildings::containsTile(stockpile, item->pos)) {
            continue;
        }

        // Ignore empty bins, barrels, and wheelbarrows assigned here.
        if (item->isAssignedToThisStockpile(stockpile->id)) {
            auto ref = Items::getGeneralRef(item, df::general_ref_type::CONTAINS_ITEM);
            if (!ref) continue;
        }

        // Found a valid item; yield it.
        break;
    }

    return *this;
}

bool Buildings::getCageOccupants(df::building_cagest *cage, vector<df::unit*> &units)
{
    CHECK_NULL_POINTER(cage);
    if (!world)
        return false;
    if (cage->contained_items.empty())
        return false;

    auto *cage_item = virtual_cast<df::item_cagest>(cage->contained_items[0]->item);
    if (!cage_item)
        return false;

    units.clear();
    for (df::general_ref *gref : cage_item->general_refs)
    {
        auto ref = virtual_cast<df::general_ref_contains_unitst>(gref);
        if (ref)
        {
            df::unit *unit = df::unit::find(ref->unit_id);
            if (unit)
                units.push_back(unit);
        }
    }

    return true;
}

void Buildings::completeBuild(df::building* bld)
{
    CHECK_NULL_POINTER(bld);

    auto fp = df::global::buildingst_completebuild;
    CHECK_NULL_POINTER(fp);

    // whether to add to the IN_PLAY vector, which we always want to do
    char in_play = 1;

    using FT = std::function<void(df::building* bld, char)>;
    auto f = reinterpret_cast<FT*>(fp);
    (*f)(bld, in_play);
}
