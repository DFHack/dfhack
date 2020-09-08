#pragma once

#include "df/item_quality.h"
#include "df/dfhack_material_category.h"
#include "modules/Materials.h"
#include "modules/Persistence.h"

struct ItemFilter
{
    df::dfhack_material_category mat_mask;
    std::vector<DFHack::MaterialInfo> materials;
    df::item_quality min_quality;
    df::item_quality max_quality;
    bool decorated_only;

    ItemFilter();

    bool matchesMask(DFHack::MaterialInfo &mat);
    bool matches(const df::dfhack_material_category mask) const;
    bool matches(DFHack::MaterialInfo &material) const;
    bool matches(df::item *item);

    std::vector<std::string> getMaterialFilterAsVector();
    std::string getMaterialFilterAsSerial();
    bool parseSerializedMaterialTokens(std::string str);

    std::string getMinQuality();
    std::string getMaxQuality();

    bool isValid();
    void clear();

private:
    bool valid;
};

class PlannedBuilding
{
public:
    PlannedBuilding(df::building *building, ItemFilter *filter);
    PlannedBuilding(DFHack::PersistentDataItem &config, DFHack::color_ostream &out);

    bool assignClosestItem(std::vector<df::item *> *items_vector);
    bool assignItem(df::item *item);

    bool isValid();
    void remove();

    df::building_type getType();
    bool isCurrentlySelectedBuilding();

    ItemFilter *getFilter();

private:
    df::building *building;
    DFHack::PersistentDataItem config;
    df::coord pos;
    ItemFilter filter;
};

class Planner
{
public:
    bool in_dummmy_screen;

    Planner();

    bool isPlanableBuilding(const df::building_type type) const;

    void reset(DFHack::color_ostream &out);

    void initialize();

    void addPlannedBuilding(df::building *bld);

    void doCycle();

    bool allocatePlannedBuilding(df::building_type type);

    PlannedBuilding *getSelectedPlannedBuilding();

    void removeSelectedPlannedBuilding();

    ItemFilter *getDefaultItemFilterForType(df::building_type type);

    void adjustMinQuality(df::building_type type, int amount);
    void adjustMaxQuality(df::building_type type, int amount);

    void enableQuickfortMode();
    void disableQuickfortMode();
    bool inQuickFortMode();

private:
    std::map<df::building_type, df::item_type> item_for_building_type;
    std::map<df::building_type, ItemFilter> default_item_filters;
    std::map<df::item_type, std::vector<df::item *>> available_item_vectors;
    std::map<df::item_type, bool> is_relevant_item_type; //Needed for fast check when looping over all items
    bool quickfort_mode;

    std::vector<PlannedBuilding> planned_buildings;

    void boundsCheckItemQuality(df::enums::item_quality::item_quality *quality);

    void gather_available_items();
};

extern std::map<df::building_type, bool> planmode_enabled, saved_planmodes;
extern Planner planner;
