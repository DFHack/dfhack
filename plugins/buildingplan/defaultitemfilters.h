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

    void setChooseItems(int choose);
    void setItemFilter(DFHack::color_ostream &out, const ItemFilter &filter, int index);
    void setSpecial(const std::string &special, bool val);

    int getChooseItems() const { return choose_items; }
    const std::vector<ItemFilter> & getItemFilters() const { return item_filters; }
    const std::set<std::string> & getSpecials() const { return specials; }

private:
    DFHack::PersistentDataItem filter_config;
    int choose_items;
    std::vector<ItemFilter> item_filters;
    std::set<std::string> specials;
};
