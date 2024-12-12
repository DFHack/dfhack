#pragma once

#include "ColorText.h"

#include "modules/Materials.h"

#include "df/organic_mat_category.h"

namespace df {
    struct creature_raw;
    struct caste_raw;
}

/**
 * Helper class for mapping the various organic mats between their material indices
 * and their index in the stockpile_settings structures.
 */
class OrganicMatLookup {

    //  pair of material type and index
    typedef std::pair<int16_t, int32_t> FoodMatPair;
    //  map for using type,index pairs to find the food index
    typedef std::map<FoodMatPair, size_t> FoodMatMap;

public:
    struct FoodMat {
        DFHack::MaterialInfo material;
        df::creature_raw* creature;
        df::caste_raw* caste;
        FoodMat(): material(-1), creature(0), caste(0) { }
    };

    static void food_mat_by_idx(DFHack::color_ostream& out, df::enums::organic_mat_category::organic_mat_category mat_category, std::vector<int16_t>::size_type food_idx, FoodMat& food_mat);
    static std::string food_token_by_idx(DFHack::color_ostream& out, df::enums::organic_mat_category::organic_mat_category mat_category, std::vector<int16_t>::size_type idx);

    static size_t food_max_size(df::enums::organic_mat_category::organic_mat_category mat_category);
    static void food_build_map();

    static int16_t food_idx_by_token(DFHack::color_ostream& out, df::enums::organic_mat_category::organic_mat_category mat_category, const std::string& token);

    static DFHack::MaterialInfo food_mat_by_token(const std::string& token);

    static bool index_built;
    static std::vector<FoodMatMap> food_index;

private:
    OrganicMatLookup();
};
