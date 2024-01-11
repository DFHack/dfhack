#pragma once

#include "itemfilter.h"

#include "modules/Persistence.h"

#include "df/building.h"
#include "df/job_item.h"
#include "df/job_item_vector_id.h"

#include <deque>
#include <set>

typedef std::deque<std::pair<int32_t, int>> Bucket;
typedef std::map<df::job_item_vector_id, std::map<std::string, Bucket>> Tasks;

extern const std::string FILTER_CONFIG_KEY;
extern const std::string BLD_CONFIG_KEY;

enum ConfigValues {
    CONFIG_BLOCKS = 1,
    CONFIG_BOULDERS = 2,
    CONFIG_LOGS = 3,
    CONFIG_BARS = 4,
};

enum FilterConfigValues {
    FILTER_CONFIG_TYPE = 0,
    FILTER_CONFIG_SUBTYPE = 1,
    FILTER_CONFIG_CUSTOM = 2,
    FILTER_CONFIG_CHOOSE_ITEMS = 3,
};

enum BuildingConfigValues {
    BLD_CONFIG_ID = 0,
    BLD_CONFIG_HEAT = 1,
};

enum HeatSafety {
    HEAT_SAFETY_ANY = 0,
    HEAT_SAFETY_FIRE = 1,
    HEAT_SAFETY_MAGMA = 2,
};

enum ItemSelectionChoice {
    ITEM_SELECTION_CHOICE_FILTER = 0,
    ITEM_SELECTION_CHOICE_MANUAL = 1,
    ITEM_SELECTION_CHOICE_AUTOMATERIAL = 2,
};

std::vector<df::job_item_vector_id> getVectorIds(DFHack::color_ostream &out, const df::job_item *job_item, bool ignore_filters);
bool itemPassesScreen(DFHack::color_ostream& out, df::item* item);
bool matchesHeatSafety(int16_t mat_type, int32_t mat_index, HeatSafety heat);
bool matchesFilters(df::item * item, const df::job_item * job_item, HeatSafety heat, const ItemFilter &item_filter, const std::set<std::string> &special);
bool isJobReady(DFHack::color_ostream &out, const std::vector<df::job_item *> &jitems);
void finalizeBuilding(DFHack::color_ostream &out, df::building *bld, bool unsuspend_on_finalize);
