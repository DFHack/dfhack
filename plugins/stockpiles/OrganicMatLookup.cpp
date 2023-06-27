#include "OrganicMatLookup.h"
#include "StockpileUtils.h"

#include "Debug.h"

#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;

namespace DFHack
{
DBG_EXTERN(stockpiles, log);
}

/**
 * Helper class for mapping the various organic mats between their material indices
 * and their index in the stockpile_settings structures.
 */

void OrganicMatLookup::food_mat_by_idx(color_ostream& out, organic_mat_category::organic_mat_category mat_category, std::vector<int16_t>::size_type food_idx, FoodMat& food_mat) {
    DEBUG(log, out).print("food_lookup: food_idx(%zd)\n", food_idx);
    df::world_raws& raws = world->raws;
    df::special_mat_table table = raws.mat_table;
    int32_t main_idx = table.organic_indexes[mat_category][food_idx];
    int16_t type = table.organic_types[mat_category][food_idx];
    if (mat_category == organic_mat_category::Fish ||
        mat_category == organic_mat_category::UnpreparedFish ||
        mat_category == organic_mat_category::Eggs) {
        food_mat.creature = raws.creatures.all[type];
        food_mat.caste = food_mat.creature->caste[main_idx];
        DEBUG(log, out).print("special creature type(%d) caste(%d)\n", type, main_idx);
    }
    else {
        food_mat.material.decode(type, main_idx);
        DEBUG(log, out).print("type(%d) index(%d) token(%s)\n", type, main_idx, food_mat.material.getToken().c_str());
    }
}
std::string OrganicMatLookup::food_token_by_idx(color_ostream& out, organic_mat_category::organic_mat_category mat_category, std::vector<int16_t>::size_type idx) {
    FoodMat food_mat;
    food_mat_by_idx(out, mat_category, idx, food_mat);
    if (food_mat.material.isValid()) {
        return food_mat.material.getToken();
    }
    else if (food_mat.creature) {
        return food_mat.creature->creature_id + ":" + food_mat.caste->caste_id;
    }
    return std::string();
}

size_t OrganicMatLookup::food_max_size(organic_mat_category::organic_mat_category mat_category) {
    return world->raws.mat_table.organic_types[mat_category].size();
}

void OrganicMatLookup::food_build_map() {
    if (index_built)
        return;
    df::world_raws& raws = world->raws;
    df::special_mat_table table = raws.mat_table;
    using df::enums::organic_mat_category::organic_mat_category;
    using traits = df::enum_traits<organic_mat_category>;
    for (int32_t mat_category = traits::first_item_value; mat_category <= traits::last_item_value; ++mat_category) {
        for (size_t i = 0; i < table.organic_indexes[mat_category].size(); ++i) {
            int16_t type = table.organic_types[mat_category].at(i);
            int32_t index = table.organic_indexes[mat_category].at(i);
            food_index[mat_category].insert(std::make_pair(std::make_pair(type, index), i)); // wtf.. only in c++
        }
    }
    index_built = true;
}

int16_t OrganicMatLookup::food_idx_by_token(color_ostream& out, organic_mat_category::organic_mat_category mat_category, const std::string& token) {
    df::world_raws& raws = world->raws;
    df::special_mat_table table = raws.mat_table;
    DEBUG(log, out).print("food_idx_by_token:\n");
    if (mat_category == organic_mat_category::Fish ||
        mat_category == organic_mat_category::UnpreparedFish ||
        mat_category == organic_mat_category::Eggs) {
        std::vector<std::string> tokens;
        split_string(&tokens, token, ":");
        if (tokens.size() != 2) {
            WARN(log, out).print("creature invalid CREATURE:CASTE token: %s\n", token.c_str());
            return -1;
        }
        int16_t creature_idx = find_creature(tokens[0]);
        if (creature_idx < 0) {
            WARN(log, out).print("creature invalid token %s\n", tokens[0].c_str());
            return -1;
        }
        int16_t food_idx = linear_index(table.organic_types[mat_category], creature_idx);
        if (tokens[1] == "MALE")
            food_idx += 1;
        if (table.organic_types[mat_category][food_idx] == creature_idx) {
            DEBUG(log, out).print("creature %s caste %s creature_idx(%d) food_idx(%d)\n", token.c_str(), tokens[1].c_str(), creature_idx, food_idx);
            return food_idx;
        }
        WARN(log, out).print("creature caste not found: %s caste %s creature_idx(%d) food_idx(%d)\n", token.c_str(), tokens[1].c_str(), creature_idx, food_idx);
        return -1;
    }

    if (!index_built)
        food_build_map();
    MaterialInfo mat_info = food_mat_by_token(token);
    int16_t type = mat_info.type;
    int32_t index = mat_info.index;
    auto it = food_index[mat_category].find(std::make_pair(type, index));
    if (it != food_index[mat_category].end()) {
        DEBUG(log, out).print("matinfo: %s type(%d) idx(%d) food_idx(%zd)\n", token.c_str(), mat_info.type, mat_info.index, it->second);
        return it->second;

    }

    WARN(log, out).print("matinfo: %s type(%d) idx(%d) food_idx not found :(\n", token.c_str(), mat_info.type, mat_info.index);
    return -1;
}

MaterialInfo OrganicMatLookup::food_mat_by_token(const std::string& token) {
    MaterialInfo mat_info;
    mat_info.find(token);
    return mat_info;
}

bool OrganicMatLookup::index_built = false;
std::vector<OrganicMatLookup::FoodMatMap> OrganicMatLookup::food_index = std::vector<OrganicMatLookup::FoodMatMap>(df::enum_traits< df::organic_mat_category >::last_item_value + 1);
