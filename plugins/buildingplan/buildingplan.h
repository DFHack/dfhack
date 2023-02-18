#pragma once

#include "modules/Persistence.h"

#include "df/job_item.h"
#include "df/job_item_vector_id.h"

#include <deque>

typedef std::deque<std::pair<int32_t, int>> Bucket;
typedef std::map<df::job_item_vector_id, std::map<std::string, Bucket>> Tasks;

extern const std::string BLD_CONFIG_KEY;

enum ConfigValues {
    CONFIG_BLOCKS = 1,
    CONFIG_BOULDERS = 2,
    CONFIG_LOGS = 3,
    CONFIG_BARS = 4,
};

enum BuildingConfigValues {
    BLD_CONFIG_ID = 0,
};

int get_config_val(DFHack::PersistentDataItem &c, int index);
bool get_config_bool(DFHack::PersistentDataItem &c, int index);
void set_config_val(DFHack::PersistentDataItem &c, int index, int value);
void set_config_bool(DFHack::PersistentDataItem &c, int index, bool value);

std::vector<df::job_item_vector_id> getVectorIds(DFHack::color_ostream &out, df::job_item *job_item);
bool itemPassesScreen(df::item * item);
bool matchesFilters(df::item * item, df::job_item * job_item);
bool isJobReady(DFHack::color_ostream &out, const std::vector<df::job_item *> &jitems);
void finalizeBuilding(DFHack::color_ostream &out, df::building *bld);
