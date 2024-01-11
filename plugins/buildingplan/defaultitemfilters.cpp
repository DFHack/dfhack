#include "defaultitemfilters.h"

#include "Debug.h"
#include "MiscUtils.h"

#include "modules/World.h"

namespace DFHack {
    DBG_EXTERN(buildingplan, control);
}

using std::string;
using std::vector;
using namespace DFHack;

BuildingTypeKey DefaultItemFilters::getKey(PersistentDataItem &filter_config) {
    return BuildingTypeKey(
        (df::building_type)filter_config.get_int(FILTER_CONFIG_TYPE),
                           filter_config.get_int(FILTER_CONFIG_SUBTYPE),
                           filter_config.get_int(FILTER_CONFIG_CUSTOM));
}

static int get_max_quality(const df::job_item *jitem) {
    if (jitem->flags2.bits.building_material ||
            jitem->item_type == df::item_type::WOOD ||
            jitem->item_type == df::item_type::BLOCKS ||
            jitem->item_type == df::item_type::BAR ||
            jitem->item_type == df::item_type::BOULDER)
        return df::item_quality::Ordinary;

    return df::item_quality::Masterful;
}

static string serialize(const std::vector<ItemFilter> &item_filters, const std::set<std::string> &specials) {
    std::ostringstream out;
    out << serialize_item_filters(item_filters);
    out << "|" << join_strings(",", specials);
    return out.str();
}

DefaultItemFilters::DefaultItemFilters(color_ostream &out, BuildingTypeKey key, const std::vector<const df::job_item *> &jitems)
        : key(key), choose_items(ItemSelectionChoice::ITEM_SELECTION_CHOICE_FILTER) {
    DEBUG(control,out).print("creating persistent data for filter key %d,%d,%d\n",
                            std::get<0>(key), std::get<1>(key), std::get<2>(key));
    filter_config = World::AddPersistentSiteData(FILTER_CONFIG_KEY);
    filter_config.set_int(FILTER_CONFIG_TYPE, std::get<0>(key));
    filter_config.set_int(FILTER_CONFIG_SUBTYPE, std::get<1>(key));
    filter_config.set_int(FILTER_CONFIG_CUSTOM, std::get<2>(key));
    filter_config.set_int(FILTER_CONFIG_CHOOSE_ITEMS, choose_items);
    item_filters.resize(jitems.size());
    for (size_t idx = 0; idx < jitems.size(); ++idx) {
        item_filters[idx].setMaxQuality(get_max_quality(jitems[idx]), true);
    }
    filter_config.val() = serialize(item_filters, specials);
}

DefaultItemFilters::DefaultItemFilters(color_ostream &out, PersistentDataItem &filter_config, const std::vector<const df::job_item *> &jitems)
        : key(getKey(filter_config)), filter_config(filter_config) {
    choose_items = filter_config.get_int(FILTER_CONFIG_CHOOSE_ITEMS);
    if (choose_items < ItemSelectionChoice::ITEM_SELECTION_CHOICE_FILTER ||
            choose_items > ItemSelectionChoice::ITEM_SELECTION_CHOICE_AUTOMATERIAL)
        choose_items = ItemSelectionChoice::ITEM_SELECTION_CHOICE_FILTER;
    auto &serialized = filter_config.val();
    DEBUG(control,out).print("deserializing default item filters for key %d,%d,%d: %s\n",
        std::get<0>(key), std::get<1>(key), std::get<2>(key), serialized.c_str());
    if (!jitems.size())
        return;
    std::vector<std::string> elems;
    split_string(&elems, serialized, "|");
    std::vector<ItemFilter> filters = deserialize_item_filters(out, elems[0]);
    if (filters.size() != jitems.size()) {
        WARN(control,out).print("ignoring invalid filters_str for key %d,%d,%d: '%s'\n",
            std::get<0>(key), std::get<1>(key), std::get<2>(key), serialized.c_str());
        item_filters.resize(jitems.size());
    } else
        item_filters = filters;
    if (elems.size() > 1) {
        vector<string> specs;
        split_string(&specs, elems[1], ",");
        for (auto & special : specs) {
            if (special.size())
                specials.emplace(special);
        }
    }
}

void DefaultItemFilters::setChooseItems(int choose) {
    choose_items = choose;
    filter_config.set_int(FILTER_CONFIG_CHOOSE_ITEMS, choose);
}

void DefaultItemFilters::setSpecial(const std::string &special, bool val) {
    if (val)
        specials.emplace(special);
    else
        specials.erase(special);
    filter_config.val() = serialize(item_filters, specials);
}

void DefaultItemFilters::setItemFilter(DFHack::color_ostream &out, const ItemFilter &filter, int index) {
    if (index < 0 || item_filters.size() <= (size_t)index) {
        WARN(control,out).print("invalid index for filter key %d,%d,%d: %d\n",
                std::get<0>(key), std::get<1>(key), std::get<2>(key), index);
        return;
    }

    item_filters[index] = filter;
    filter_config.val() = serialize(item_filters, specials);
    DEBUG(control,out).print("updated item filter and persisted for key %d,%d,%d: %s\n",
        std::get<0>(key), std::get<1>(key), std::get<2>(key), filter_config.val().c_str());
}
