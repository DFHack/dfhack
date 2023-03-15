#pragma once

#include "buildingplan.h"
#include "buildingtypekey.h"

#include "modules/Persistence.h"

class DefaultItemFilters {
public:
    static BuildingTypeKey getKey(DFHack::PersistentDataItem &filter_config);

    const BuildingTypeKey key;

    DefaultItemFilters(DFHack::color_ostream &out, BuildingTypeKey key, const std::vector<const df::job_item *> &jitems);
    DefaultItemFilters(DFHack::color_ostream &out, DFHack::PersistentDataItem &filter_config, const std::vector<const df::job_item *> &jitems);

    void setChooseItems(bool choose);
    void setItemFilter(DFHack::color_ostream &out, const ItemFilter &filter, int index);

    bool getChooseItems() const { return choose_items; }
    const std::vector<ItemFilter> & getItemFilters() const { return item_filters; }

private:
    DFHack::PersistentDataItem filter_config;
    bool choose_items;
    std::vector<ItemFilter> item_filters;
};
