#include "defaultitemfilters.h"

#include "Debug.h"
#include "MiscUtils.h"

#include "modules/World.h"

namespace DFHack {
    DBG_EXTERN(buildingplan, status);
}

using std::string;
using std::vector;
using namespace DFHack;

BuildingTypeKey DefaultItemFilters::getKey(PersistentDataItem &filter_config) {
    return BuildingTypeKey(
        (df::building_type)get_config_val(filter_config, FILTER_CONFIG_TYPE),
                           get_config_val(filter_config, FILTER_CONFIG_SUBTYPE),
                           get_config_val(filter_config, FILTER_CONFIG_CUSTOM));
}

DefaultItemFilters::DefaultItemFilters(color_ostream &out, BuildingTypeKey key, const std::vector<const df::job_item *> &jitems)
        : key(key) {
    DEBUG(status,out).print("creating persistent data for filter key %d,%d,%d\n",
                            std::get<0>(key), std::get<1>(key), std::get<2>(key));
    filter_config = World::AddPersistentData(FILTER_CONFIG_KEY);
    set_config_val(filter_config, FILTER_CONFIG_TYPE, std::get<0>(key));
    set_config_val(filter_config, FILTER_CONFIG_SUBTYPE, std::get<1>(key));
    set_config_val(filter_config, FILTER_CONFIG_CUSTOM, std::get<2>(key));
    item_filters.resize(jitems.size());
    filter_config.val() = serialize_item_filters(item_filters);
}

DefaultItemFilters::DefaultItemFilters(color_ostream &out, PersistentDataItem &filter_config, const std::vector<const df::job_item *> &jitems)
        : key(getKey(filter_config)), filter_config(filter_config) {
    auto &serialized = filter_config.val();
    DEBUG(status,out).print("deserializing item filters for key %d,%d,%d: %s\n",
        std::get<0>(key), std::get<1>(key), std::get<2>(key), serialized.c_str());
    std::vector<ItemFilter> filters = deserialize_item_filters(out, serialized);
    if (filters.size() != jitems.size()) {
        WARN(status,out).print("ignoring invalid filters_str for key %d,%d,%d: '%s'\n",
            std::get<0>(key), std::get<1>(key), std::get<2>(key), serialized.c_str());
        item_filters.resize(jitems.size());
    } else
        item_filters = filters;
}

void DefaultItemFilters::setItemFilter(DFHack::color_ostream &out, const ItemFilter &filter, int index) {
    if (item_filters.size() <= index) {
        WARN(status,out).print("invalid index for filter key %d,%d,%d: %d\n",
                std::get<0>(key), std::get<1>(key), std::get<2>(key), index);
        return;
    }

    item_filters[index] = filter;
    filter_config.val() = serialize_item_filters(item_filters);
    DEBUG(status,out).print("updated item filter and persisted for key %d,%d,%d: %s\n",
        std::get<0>(key), std::get<1>(key), std::get<2>(key), filter_config.val().c_str());
}
