#include "plannedbuilding.h"
#include "buildingplan.h"

#include "Debug.h"
#include "MiscUtils.h"

#include "modules/World.h"

#include "df/job.h"

namespace DFHack {
    DBG_EXTERN(buildingplan, status);
}

using std::set;
using std::string;
using std::vector;

using namespace DFHack;

static vector<vector<df::job_item_vector_id>> get_vector_ids(color_ostream &out, int bld_id) {
    vector<vector<df::job_item_vector_id>> ret;

    df::building *bld = df::building::find(bld_id);

    if (!bld || bld->jobs.size() != 1)
        return ret;

    auto &jitems = bld->jobs[0]->job_items;
    int num_job_items = (int)jitems.size();
    for (int jitem_idx = num_job_items - 1; jitem_idx >= 0; --jitem_idx) {
        ret.emplace_back(getVectorIds(out, jitems[jitem_idx], false));
    }
    return ret;
}

static vector<vector<df::job_item_vector_id>> deserialize_vector_ids(color_ostream &out, PersistentDataItem &bld_config) {
    vector<vector<df::job_item_vector_id>> ret;

    vector<string> rawstrs;
    split_string(&rawstrs, bld_config.val(), "|");
    const string &serialized = rawstrs[0];

    DEBUG(status,out).print("deserializing vector ids for building %d: %s\n",
            get_config_val(bld_config, BLD_CONFIG_ID), serialized.c_str());

    vector<string> joined;
    split_string(&joined, serialized, ";");
    for (auto &str : joined) {
        vector<string> lst;
        split_string(&lst, str, ",");
        vector<df::job_item_vector_id> ids;
        for (auto &s : lst)
            ids.emplace_back(df::job_item_vector_id(string_to_int(s)));
        ret.emplace_back(ids);
    }

    if (!ret.size())
        ret = get_vector_ids(out, get_config_val(bld_config, BLD_CONFIG_ID));

    return ret;
}

static vector<ItemFilter> get_item_filters(color_ostream &out, PersistentDataItem &bld_config) {
    vector<string> rawstrs;
    split_string(&rawstrs, bld_config.val(), "|");
    if (rawstrs.size() < 2)
        return vector<ItemFilter>();
    return deserialize_item_filters(out, rawstrs[1]);
}

static set<string> get_specials(color_ostream &out, PersistentDataItem &bld_config) {
    vector<string> rawstrs;
    split_string(&rawstrs, bld_config.val(), "|");
    set<string> ret;
    if (rawstrs.size() < 3)
        return ret;
    vector<string> specials;
    split_string(&specials, rawstrs[2], ",");
    for (auto & special : specials)
        ret.emplace(special);
    return ret;
}

static string serialize(const vector<vector<df::job_item_vector_id>> &vector_ids, const DefaultItemFilters &item_filters) {
    vector<string> joined;
    for (auto &vec_list : vector_ids) {
        joined.emplace_back(join_strings(",", vec_list));
    }
    std::ostringstream out;
    out << join_strings(";", joined);
    out << "|" << serialize_item_filters(item_filters.getItemFilters());
    out << "|" << join_strings(",", item_filters.getSpecials());
    return out.str();
}

PlannedBuilding::PlannedBuilding(color_ostream &out, df::building *bld, HeatSafety heat, const DefaultItemFilters &item_filters)
        : id(bld->id), vector_ids(get_vector_ids(out, id)), heat_safety(heat),
          item_filters(item_filters.getItemFilters()),
          specials(item_filters.getSpecials()) {
    DEBUG(status,out).print("creating persistent data for building %d\n", id);
    bld_config = World::AddPersistentSiteData(BLD_CONFIG_KEY);
    set_config_val(bld_config, BLD_CONFIG_ID, id);
    set_config_val(bld_config, BLD_CONFIG_HEAT, heat_safety);
    bld_config.val() = serialize(vector_ids, item_filters);
    DEBUG(status,out).print("serialized state for building %d: %s\n", id, bld_config.val().c_str());
}

PlannedBuilding::PlannedBuilding(color_ostream &out, PersistentDataItem &bld_config)
    : id(get_config_val(bld_config, BLD_CONFIG_ID)),
        vector_ids(deserialize_vector_ids(out, bld_config)),
        heat_safety((HeatSafety)get_config_val(bld_config, BLD_CONFIG_HEAT)),
        item_filters(get_item_filters(out, bld_config)),
        specials(get_specials(out, bld_config)),
        bld_config(bld_config) { }

// Ensure the building still exists and is in a valid state. It can disappear
// for lots of reasons, such as running the game with the buildingplan plugin
// disabled, manually removing the building, modifying it via the API, etc.
df::building * PlannedBuilding::getBuildingIfValidOrRemoveIfNot(color_ostream &out, bool skip_remove) {
    auto bld = df::building::find(id);
    bool valid = bld && bld->getBuildStage() == 0;
    if (!valid) {
        if (!skip_remove)
            remove(out);
        return NULL;
    }
    return bld;
}
