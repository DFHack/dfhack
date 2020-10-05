#pragma once

#include <queue>
#include <unordered_map>

#include "df/building.h"
#include "df/dfhack_material_category.h"
#include "df/item_quality.h"
#include "df/job_item.h"

#include "modules/Materials.h"
#include "modules/Persistence.h"

class ItemFilter
{
public:
    ItemFilter();

    void clear();
    bool deserialize(std::string ser);
    std::string serialize() const;

    void addMaterialMask(uint32_t mask);
    void clearMaterialMask();
    void setMaterials(std::vector<DFHack::MaterialInfo> materials);

    void incMinQuality();
    void decMinQuality();
    void incMaxQuality();
    void decMaxQuality();
    void toggleDecoratedOnly();

    uint32_t getMaterialMask() const;
    std::vector<std::string> getMaterials() const;
    std::string getMinQuality() const;
    std::string getMaxQuality() const;
    bool getDecoratedOnly() const;

    bool matches(df::dfhack_material_category mask) const;
    bool matches(DFHack::MaterialInfo &material) const;
    bool matches(df::item *item) const;

private:
    // remove friend declaration when we no longer need v1 deserialization
    friend void migrateV1ToV2();

    df::dfhack_material_category mat_mask;
    std::vector<DFHack::MaterialInfo> materials;
    df::item_quality min_quality;
    df::item_quality max_quality;
    bool decorated_only;

    bool deserializeMaterialMask(std::string ser);
    bool deserializeMaterials(std::string ser);
    void setMinQuality(int quality);
    void setMaxQuality(int quality);
    bool matchesMask(DFHack::MaterialInfo &mat) const;
};

class PlannedBuilding
{
public:
    PlannedBuilding(df::building *building, const std::vector<ItemFilter> &filters);
    PlannedBuilding(DFHack::PersistentDataItem &config);

    bool isValid() const;
    void remove();

    df::building * getBuilding();
    const std::vector<ItemFilter> & getFilters() const;

private:
    DFHack::PersistentDataItem config;
    df::building *building;
    const df::building::key_field_type building_id;
    const std::vector<ItemFilter> filters;
};

// building type, subtype, custom
typedef std::tuple<df::building_type, int16_t, int32_t> BuildingTypeKey;

BuildingTypeKey toBuildingTypeKey(
    df::building_type btype, int16_t subtype, int32_t custom);
BuildingTypeKey toBuildingTypeKey(df::building *bld);
BuildingTypeKey toBuildingTypeKey(df::ui_build_selector *uibs);

struct BuildingTypeKeyHash
{
    std::size_t operator() (const BuildingTypeKey & key) const;
};

class Planner
{
public:
    class ItemFiltersWrapper
    {
    public:
        ItemFiltersWrapper(std::vector<ItemFilter> & item_filters)
            : item_filters(item_filters) { }
        std::vector<ItemFilter>::reverse_iterator rbegin() const { return item_filters.rbegin(); }
        std::vector<ItemFilter>::reverse_iterator  rend() const { return item_filters.rend(); }
        const std::vector<ItemFilter> & get() const { return item_filters; }
    private:
        std::vector<ItemFilter> &item_filters;
    };

    void reset();

    void addPlannedBuilding(df::building *bld);
    PlannedBuilding *getPlannedBuilding(df::building *bld);

    bool isPlannableBuilding(BuildingTypeKey key);

    // returns an empty vector if the type is not supported
    ItemFiltersWrapper getItemFilters(BuildingTypeKey key);

    void doCycle();

private:
    std::unordered_map<BuildingTypeKey,
                       std::vector<ItemFilter>,
                       BuildingTypeKeyHash> default_item_filters;
    std::unordered_map<df::building *, PlannedBuilding> planned_buildings;
    // vector id -> filter bucket -> queue of (building, job_item index)
    std::map<df::job_item_vector_id, std::map<std::string, std::queue<std::pair<df::building *, int>>>> tasks;

    bool registerTasks(PlannedBuilding &plannedBuilding);
    void unregisterBuilding(df::building *bld);
    void popInvalidTasks(std::queue<std::pair<df::building *, int>> &task_queue);
};

extern Planner planner;
