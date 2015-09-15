#ifndef BUILDINGPLAN_H
#define BUILDINGPLAN_H

#include "uicommon.h"
#include "listcolumn.h"

#include <functional>

// DF data structure definition headers
#include "DataDefs.h"
#include "Types.h"
#include "df/build_req_choice_genst.h"
#include "df/build_req_choice_specst.h"
#include "df/item.h"
#include "df/ui.h"
#include "df/ui_build_selector.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/items_other_id.h"
#include "df/job.h"
#include "df/world.h"
#include "df/building_constructionst.h"
#include "df/building_design.h"
#include "df/entity_position.h"

#include "modules/Buildings.h"
#include "modules/Maps.h"
#include "modules/Items.h"
#include "modules/Units.h"
#include "modules/Gui.h"

#include "TileTypes.h"
#include "df/job_item.h"
#include "df/dfhack_material_category.h"
#include "df/general_ref_building_holderst.h"
#include "modules/Job.h"
#include "df/building_design.h"
#include "df/buildings_other_id.h"
#include "modules/World.h"
#include "df/building.h"
#include "df/building_doorst.h"

using df::global::ui;
using df::global::ui_build_selector;
using df::global::world;

struct MaterialDescriptor
{
    df::item_type item_type;
    int16_t item_subtype;
    int16_t type;
    int32_t index;
    bool valid;

    bool matches(const MaterialDescriptor &a) const
    {
        return a.valid && valid &&
            a.type == type &&
            a.index == index &&
            a.item_type == item_type &&
            a.item_subtype == item_subtype;
    }
};

#define MAX_MASK 10
#define MAX_MATERIAL 21
#define SIDEBAR_WIDTH 30

static bool canReserveRoom(df::building *building)
{
    if (!building)
        return false;

    if (building->jobs.size() > 0 && building->jobs[0]->job_type == job_type::DestroyBuilding)
        return false;

    return building->is_room;
}

static std::vector<Units::NoblePosition> getUniqueNoblePositions(df::unit *unit)
{
    std::vector<Units::NoblePosition> np;
    Units::getNoblePositions(&np, unit);
    for (auto iter = np.begin(); iter != np.end(); iter++)
    {
        if (iter->position->code == "MILITIA_CAPTAIN")
        {
            np.erase(iter);
            break;
        }
    }

    return np;
}

static void delete_item_fn(df::job_item *x);

static MaterialInfo &material_info_identity_fn(MaterialInfo &m);

static map<df::building_type, bool> planmode_enabled, saved_planmodes;

void enable_quickfort_fn(pair<const df::building_type, bool>& pair);

void debug(const std::string &msg);
static std::string material_to_string_fn(MaterialInfo m);

static bool show_debugging = false;
static bool show_help = false;

struct ItemFilter
{
    df::dfhack_material_category mat_mask;
    std::vector<DFHack::MaterialInfo> materials;
    df::item_quality min_quality;
    bool decorated_only;

    ItemFilter() : min_quality(df::item_quality::Ordinary), decorated_only(false), valid(true)
    {    }

    bool matchesMask(DFHack::MaterialInfo &mat);

    bool matches(const df::dfhack_material_category mask) const;

    bool matches(DFHack::MaterialInfo &material) const;

    bool matches(df::item *item);

    std::vector<std::string> getMaterialFilterAsVector();

    std::string getMaterialFilterAsSerial();

    bool parseSerializedMaterialTokens(std::string str);

    std::string getMinQuality();

    bool isValid();

    void clear();

private:
    bool valid;
};

class ViewscreenChooseMaterial : public dfhack_viewscreen
{
public:
    ViewscreenChooseMaterial(ItemFilter *filter);

    void feed(set<df::interface_key> *input);

    void render();

    std::string getFocusString() { return "buildingplan_choosemat"; }

private:
    ListColumn<df::dfhack_material_category> masks_column;
    ListColumn<MaterialInfo> materials_column;
    int selected_column;
    ItemFilter *filter;

    df::building_type btype;

    void addMaskEntry(df::dfhack_material_category &mask, const std::string &text)
    {
        auto entry = ListEntry<df::dfhack_material_category>(pad_string(text, MAX_MASK, false), mask);
        if (filter->matches(mask))
            entry.selected = true;

        masks_column.add(entry);
    }

    void populateMasks()
    {
        masks_column.clear();
        df::dfhack_material_category mask;

        mask.whole = 0;
        mask.bits.stone = true;
        addMaskEntry(mask, "Stone");

        mask.whole = 0;
        mask.bits.wood = true;
        addMaskEntry(mask, "Wood");

        mask.whole = 0;
        mask.bits.metal = true;
        addMaskEntry(mask, "Metal");

        mask.whole = 0;
        mask.bits.soap = true;
        addMaskEntry(mask, "Soap");

        masks_column.filterDisplay();
    }

    void populateMaterials()
    {
        materials_column.clear();
        df::dfhack_material_category selected_category;
        std::vector<df::dfhack_material_category> selected_masks = masks_column.getSelectedElems();
        if (selected_masks.size() == 1)
            selected_category = selected_masks[0];
        else if (selected_masks.size() > 1)
            return;

        df::world_raws &raws = world->raws;
        for (int i = 1; i < DFHack::MaterialInfo::NUM_BUILTIN; i++)
        {
            auto obj = raws.mat_table.builtin[i];
            if (obj)
            {
                MaterialInfo material;
                material.decode(i, -1);
                addMaterialEntry(selected_category, material, material.toString());
            }
        }

        for (size_t i = 0; i < raws.inorganics.size(); i++)
        {
            df::inorganic_raw *p = raws.inorganics[i];
            MaterialInfo material;
            material.decode(0, i);
            addMaterialEntry(selected_category, material, material.toString());
        }

        decltype(selected_category) wood_flag;
        wood_flag.bits.wood = true;
        if (!selected_category.whole || selected_category.bits.wood)
        {
            for (size_t i = 0; i < raws.plants.all.size(); i++)
            {
                df::plant_raw *p = raws.plants.all[i];
                for (size_t j = 0; p->material.size() > 1 && j < p->material.size(); j++)
                {
                    auto t = p->material[j];
                    if (p->material[j]->id != "WOOD")
                        continue;

                    MaterialInfo material;
                    material.decode(DFHack::MaterialInfo::PLANT_BASE+j, i);
                    auto name = material.toString();
                    ListEntry<MaterialInfo> entry(pad_string(name, MAX_MATERIAL, false), material);
                    if (filter->matches(material))
                        entry.selected = true;

                    materials_column.add(entry);
                }
            }
        }
        materials_column.sort();
    }

    void addMaterialEntry(df::dfhack_material_category &selected_category,
                          MaterialInfo &material, std::string name)
    {
        if (!selected_category.whole || material.matches(selected_category))
        {
            ListEntry<MaterialInfo> entry(pad_string(name, MAX_MATERIAL, false), material);
            if (filter->matches(material))
                entry.selected = true;

            materials_column.add(entry);
        }
    }

    void validateColumn()
    {
        set_to_limit(selected_column, 1);
    }

    void resize(int32_t x, int32_t y)
    {
        dfhack_viewscreen::resize(x, y);
        masks_column.resize();
        materials_column.resize();
    }
};

class ReservedRoom
{
public:
    ReservedRoom(df::building *building, std::string noble_code);

    ReservedRoom(PersistentDataItem &config, color_ostream &out);

    bool checkRoomAssignment();

    void remove() { DFHack::World::DeletePersistentData(config); }

    bool isValid()
    {
        if (!building)
            return false;

        if (Buildings::findAtTile(pos) != building)
            return false;

        return canReserveRoom(building);
    }

    int32_t getId()
    {
        if (!isValid())
            return 0;

        return building->id;
    }

    std::string getCode() { return config.val(); }

    void setCode(const std::string &noble_code) { config.val() = noble_code; }

private:
    df::building *building;
    PersistentDataItem config;
    df::coord pos;

    std::vector<Units::NoblePosition> getOwnersNobleCode()
    {
        if (!building->owner)
            return std::vector<Units::NoblePosition> ();

        return getUniqueNoblePositions(building->owner);
    }
};

class RoomMonitor
{
public:
    RoomMonitor() { }

    std::string getReservedNobleCode(int32_t buildingId);

    void toggleRoomForPosition(int32_t buildingId, std::string noble_code);

    void doCycle();

    void reset(color_ostream &out);

private:
    std::vector<ReservedRoom> reservedRooms;

    void addRoom(ReservedRoom &rr)
    {
        for (auto iter = reservedRooms.begin(); iter != reservedRooms.end(); iter++)
        {
            if (iter->getId() == rr.getId())
                return;
        }

        reservedRooms.push_back(rr);
    }
};

// START Planning
class PlannedBuilding
{
public:
    PlannedBuilding(df::building *building, ItemFilter *filter);

    PlannedBuilding(PersistentDataItem &config, color_ostream &out);

    bool assignClosestItem(std::vector<df::item *> *items_vector);

    bool assignItem(df::item *item);

    bool isValid();

    df::building_type getType() { return building->getType(); }

    bool isCurrentlySelectedBuilding() { return isValid() && (building == world->selected_building); }

    ItemFilter *getFilter() { return &filter; }

    void remove() { DFHack::World::DeletePersistentData(config); }

private:
    df::building *building;
    PersistentDataItem config;
    df::coord pos;
    ItemFilter filter;
};

class Planner
{
public:
    bool in_dummmy_screen;

    Planner() : quickfort_mode(false), in_dummmy_screen(false) { }

    bool isPlanableBuilding(const df::building_type type) const
    {
        return item_for_building_type.find(type) != item_for_building_type.end();
    }

    void reset(color_ostream &out);

    void initialize();

    void addPlannedBuilding(df::building *bld)
    {
        PlannedBuilding pb(bld, &default_item_filters[bld->getType()]);
        planned_buildings.push_back(pb);
    }

    void doCycle();

    bool allocatePlannedBuilding(df::building_type type);

    PlannedBuilding *getSelectedPlannedBuilding();

    void removeSelectedPlannedBuilding() { getSelectedPlannedBuilding()->remove(); }

    ItemFilter *getDefaultItemFilterForType(df::building_type type) { return &default_item_filters[type]; }

    void cycleDefaultQuality(df::building_type type);

    void enableQuickfortMode()
    {
        saved_planmodes = planmode_enabled;
        for_each_(planmode_enabled, enable_quickfort_fn);

        quickfort_mode = true;
    }

    void disableQuickfortMode()
    {
        planmode_enabled = saved_planmodes;
        quickfort_mode = false;
    }

    bool inQuickFortMode() { return quickfort_mode; }

private:
    map<df::building_type, df::item_type> item_for_building_type;
    map<df::building_type, ItemFilter> default_item_filters;
    map<df::item_type, std::vector<df::item *>> available_item_vectors;
    map<df::item_type, bool> is_relevant_item_type; //Needed for fast check when looping over all items
    bool quickfort_mode;

    std::vector<PlannedBuilding> planned_buildings;

    void gather_available_items()
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

        std::vector<df::item*> &items = world->items.other[items_other_id::IN_PLAY];

        for (size_t i = 0; i < items.size(); i++)
        {
            df::item *item = items[i];

            if (item->flags.whole & bad_flags.whole)
                continue;

            df::item_type itype = item->getType();
            if (!is_relevant_item_type[itype])
                continue;

            if (itype == item_type::BOX && item->isBag())
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
};

static Planner planner;

static RoomMonitor roomMonitor;

#endif
