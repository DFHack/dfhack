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

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

#include "ColorText.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "Types.h"
#include "Error.h"
#include "modules/Buildings.h"
#include "modules/Maps.h"
#include "modules/Job.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "TileTypes.h"
#include "MiscUtils.h"
using namespace DFHack;

#include "DataDefs.h"

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
#include "df/building_wellst.h"
#include "df/building_workshopst.h"
#include "df/buildings_other_id.h"
#include "df/d_init.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_contains_unitst.h"
#include "df/item.h"
#include "df/item_cagest.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/map_block.h"
#include "df/tile_occupancy.h"
#include "df/ui.h"
#include "df/ui_look_list.h"
#include "df/unit.h"
#include "df/unit_relationship_type.h"
#include "df/world.h"

using namespace df::enums;
using df::global::ui;
using df::global::world;
using df::global::d_init;
using df::global::building_next_id;
using df::global::process_jobs;
using df::building_def;

struct CoordHash {
    size_t operator()(const df::coord pos) const {
        return pos.x*65537 + pos.y*17 + pos.z;
    }
};

static unordered_map<df::coord, int32_t, CoordHash> locationToBuilding;

static uint8_t *getExtentTile(df::building_extents &extent, df::coord2d tile)
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

uint32_t Buildings::getNumBuildings()
{
    return world->buildings.all.size();
}

bool Buildings::Read (const uint32_t index, t_building & building)
{
    Core & c = Core::getInstance();
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
    uint32_t size = bld_def.size();
    btypes.clear();

    for (auto iter = bld_def.begin(); iter != bld_def.end();iter++)
    {
        building_def * temp = *iter;
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

bool Buildings::setOwner(df::building *bld, df::unit *unit)
{
    CHECK_NULL_POINTER(bld);

    if (!bld->is_room)
        return false;
    if (bld->owner == unit)
        return true;

    if (bld->owner)
    {
        auto &blist = bld->owner->owned_buildings;
        vector_erase_at(blist, linear_index(blist, bld));

        if (auto spouse = df::unit::find(bld->owner->relationship_ids[df::unit_relationship_type::Spouse]))
        {
            auto &blist = spouse->owned_buildings;
            vector_erase_at(blist, linear_index(blist, bld));
        }
    }

    bld->owner = unit;

    if (unit)
    {
        bld->owner_id = unit->id;
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
        bld->owner_id = -1;
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
            containsTile(building, pos, false))
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

bool Buildings::findCivzonesAt(std::vector<df::building_civzonest*> *pvec, df::coord pos)
{
    pvec->clear();

    auto &vec = world->buildings.other[buildings_other_id::ANY_ZONE];

    for (size_t i = 0; i < vec.size(); i++)
    {
        auto bld = strict_virtual_cast<df::building_civzonest>(vec[i]);

        if (!bld || bld->z != pos.z || !containsTile(bld, pos))
            continue;

        pvec->push_back(bld);
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

    bld->race = ui->race_id;

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
    case building_type::Furnace:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_furnacest, bld))
            obj->melt_remainder.resize(df::inorganic_raw::get_vector().size(), 0);
        break;
    }
    case building_type::Coffin:
    {
        if (VIRTUAL_CAST_VAR(obj, df::building_coffinst, bld))
            obj->initBurialFlags(); // DF has this copy&pasted
        break;
    }
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

bool Buildings::checkFreeTiles(df::coord pos, df::coord2d size,
                               df::building_extents *ext,
                               bool create_ext, bool allow_occupied)
{
    bool found_any = false;

    for (int dx = 0; dx < size.x; dx++)
    {
        for (int dy = 0; dy < size.y; dy++)
        {
            df::coord tile = pos + df::coord(dx,dy,0);
            uint8_t *etile = NULL;

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
            else
            {
                auto tile = block->tiletype[btile.x][btile.y];
                if (!HighPassable(tile))
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
                    ext->extents = new uint8_t[size.x*size.y];
                    ext->x = pos.x;
                    ext->y = pos.y;
                    ext->width = size.x;
                    ext->height = size.y;

                    memset(ext->extents, 1, size.x*size.y);
                    etile = getExtentTile(*ext, tile);
                }

                if (!etile)
                    return false;

                *etile = 0;
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

static bool checkBuildingTiles(df::building *bld, bool can_change)
{
    auto psize = Buildings::getSize(bld);

    return Buildings::checkFreeTiles(psize.first, psize.second, &bld->room,
                                     can_change && bld->isExtentShaped(),
                                     !bld->isSettingOccupancy());
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

bool Buildings::containsTile(df::building *bld, df::coord2d tile, bool room)
{
    CHECK_NULL_POINTER(bld);

    if (room)
    {
        if (!bld->is_room || !bld->room.extents)
            return false;
    }
    else
    {
        if (tile.x < bld->x1 || tile.x > bld->x2 || tile.y < bld->y1 || tile.y > bld->y2)
            return false;
    }

    if (bld->room.extents && (room || bld->isExtentShaped()))
    {
        uint8_t *etile = getExtentTile(bld->room, tile);
        if (!etile || !*etile)
            return false;
    }

    return true;
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

    // Delete old extents
    if (bld->room.extents)
    {
        delete[] bld->room.extents;
        bld->room.extents = NULL;
    }

    // Compute correct size and apply it
    df::coord2d center;
    getCorrectSize(size, center, bld->getType(), bld->getSubtype(),
                   bld->getCustomType(), direction);

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
                uint8_t *etile = getExtentTile(bld->room, tile);
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
    auto &vec = world->buildings.other[buildings_other_id::IN_PLAY];

    bool changed = false;

    for (size_t i = 0; i < vec.size(); i++)
    {
        auto room = vec[i];
        if (!room->is_room || room->z != bld->z)
            continue;

        uint8_t *pext = getExtentTile(room->room, df::coord2d(bld->x1, bld->y1));
        if (!pext || !*pext)
            continue;

        changed = true;
        room->children.push_back(bld);
        bld->parents.push_back(room);

        // TODO: the game updates room rent here if economy is enabled
    }

    if (changed)
        df::global::ui->equipment.update.bits.buildings = true;
}

static void unlinkRooms(df::building *bld)
{
    for (size_t i = 0; i < bld->parents.size(); i++)
    {
        auto parent = bld->parents[i];
        int idx = linear_index(parent->children, bld);
        vector_erase_at(parent->children, idx);
    }

    bld->parents.clear();
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
    auto &vec = world->buildings.other[buildings_other_id::STOCKPILE];
    int max_id = 0;

    for (size_t i = 0; i < vec.size(); i++)
    {
        auto bld = strict_virtual_cast<df::building_stockpilest>(vec[i]);
        if (bld)
            max_id = std::max(max_id, bld->stockpile_number);
    }

    return max_id;
}

static int getMaxCivzoneId()
{
    auto &vec = world->buildings.other[buildings_other_id::ANY_ZONE];
    int max_id = 0;

    for (size_t i = 0; i < vec.size(); i++)
    {
        auto bld = strict_virtual_cast<df::building_civzonest>(vec[i]);
        if (bld)
            max_id = std::max(max_id, bld->zone_num);
    }

    return max_id;
}

bool Buildings::constructAbstract(df::building *bld)
{
    CHECK_NULL_POINTER(bld);
    CHECK_INVALID_ARGUMENT(bld->id == -1);
    CHECK_INVALID_ARGUMENT(!bld->isActual());

    if (!checkBuildingTiles(bld, false))
        return false;

    switch (bld->getType())
    {
        case building_type::Stockpile:
            if (auto stock = strict_virtual_cast<df::building_stockpilest>(bld))
                stock->stockpile_number = getMaxStockpileId() + 1;
            break;

        case building_type::Civzone:
            if (auto zone = strict_virtual_cast<df::building_civzonest>(bld))
                zone->zone_num = getMaxCivzoneId() + 1;
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

    createDesign(bld, rough);
    return true;
}

bool Buildings::deconstruct(df::building *bld)
{
    using df::global::ui;
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
    vector_erase_at(ui->tax_collection.rooms, linear_index(ui->tax_collection.rooms, bld->id));
    // Assume: not used in punishment
    // Assume: not used in non-own jobs
    // Assume: does not affect pathfinding
    bld->deconstructItems(false, false);
    // Don't clear arrows.

    bld->uncategorize();
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
            item->building == bld)
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

static unordered_map<int32_t, df::coord> corner1;
static unordered_map<int32_t, df::coord> corner2;

void Buildings::clearBuildings(color_ostream& out) {
    corner1.clear();
    corner2.clear();
    locationToBuilding.clear();
}

void Buildings::updateBuildings(color_ostream& out, void* ptr)
{
    int32_t id = *((int32_t*)ptr);
    auto building = df::building::find(id);

    if (building)
    {
        // Already cached -> weird, so bail out
        if (corner1.count(id))
            return;
        // Civzones cannot be cached because they can
        // overlap each other and normal buildings.
        if (!building->isSettingOccupancy())
            return;

        df::coord p1(min(building->x1, building->x2), min(building->y1,building->y2), building->z);
        df::coord p2(max(building->x1, building->x2), max(building->y1,building->y2), building->z);

        corner1[id] = p1;
        corner2[id] = p2;

        for ( int32_t x = p1.x; x <= p2.x; x++ ) {
            for ( int32_t y = p1.y; y <= p2.y; y++ ) {
                df::coord pt(x,y,building->z);
                if (containsTile(building, pt, false))
                    locationToBuilding[pt] = id;
            }
        }
    }
    else if (corner1.count(id))
    {
        //existing building: destroy it
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
            && building->getSubtype() == (short)civzone_type::ActivityZone;
}

bool Buildings::isPenPasture(df::building * building)
{
    if (!isActivityZone(building))
        return false;

    return ((df::building_civzonest*) building)->zone_flags.bits.pen_pasture != 0;
}

bool Buildings::isPitPond(df::building * building)
{
    if (!isActivityZone(building))
        return false;
    return ((df::building_civzonest*) building)->zone_flags.bits.pit_pond != 0;
}

bool Buildings::isActive(df::building * building)
{
    if (!isActivityZone(building))
        return false;
    return ((df::building_civzonest*) building)->zone_flags.bits.active != 0;
}

bool Buildings::isHospital(df::building * building)
 {
     if (!isActivityZone(building))
         return false;
     return ((df::building_civzonest*) building)->zone_flags.bits.hospital != 0;
 }

 bool Buildings::isAnimalTraining(df::building * building)
 {
     if (!isActivityZone(building))
         return false;
     return ((df::building_civzonest*) building)->zone_flags.bits.animal_training != 0;
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
        } else {
            // Start with the top-left block covering the stockpile.
            block = Maps::getTileBlock(stockpile->x1, stockpile->y1, stockpile->z);
            current = 0;
        }

        while (current >= block->items.size()) {
            // Out of items in this block; find the next block to search.
            if (block->map_pos.x + 16 < stockpile->x2) {
                block = Maps::getTileBlock(block->map_pos.x + 16, block->map_pos.y, stockpile->z);
                current = 0;
            } else if (block->map_pos.y + 16 < stockpile->y2) {
                block = Maps::getTileBlock(stockpile->x1, block->map_pos.y + 16, stockpile->z);
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

        if (!Buildings::containsTile(stockpile, item->pos, false)) {
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
