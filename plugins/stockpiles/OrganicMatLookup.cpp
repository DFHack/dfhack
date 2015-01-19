#include "OrganicMatLookup.h"

#include "StockpileUtils.h"

#include "modules/Materials.h"
#include "MiscUtils.h"

#include "df/world.h"
#include "df/world_data.h"

#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/material.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;

using std::endl;

/**
 * Helper class for mapping the various organic mats between their material indices
 * and their index in the stockpile_settings structures.
 */

void OrganicMatLookup::food_mat_by_idx ( std::ostream &out, organic_mat_category::organic_mat_category mat_category, std::vector<int16_t>::size_type food_idx, FoodMat & food_mat )
{
    out << "food_lookup: food_idx(" << food_idx << ") ";
    df::world_raws &raws = world->raws;
    df::special_mat_table table = raws.mat_table;
    int32_t main_idx = table.organic_indexes[mat_category][food_idx];
    int16_t type = table.organic_types[mat_category][food_idx];
    if ( mat_category == organic_mat_category::Fish ||
            mat_category == organic_mat_category::UnpreparedFish ||
            mat_category == organic_mat_category::Eggs )
    {
        food_mat.creature = raws.creatures.all[type];
        food_mat.caste = food_mat.creature->caste[main_idx];
        out << " special creature type(" << type << ") caste("<< main_idx <<")" <<endl;
    }
    else
    {
        food_mat.material.decode ( type, main_idx );
        out << " type(" << type << ") index("<< main_idx <<") token(" << food_mat.material.getToken() <<  ")" << endl;
    }
}
std::string OrganicMatLookup::food_token_by_idx ( std::ostream &out, organic_mat_category::organic_mat_category mat_category, std::vector<int16_t>::size_type idx )
{
    FoodMat food_mat;
    food_mat_by_idx ( out, mat_category, idx, food_mat );
    if ( food_mat.material.isValid() )
    {
        return food_mat.material.getToken();
    }
    else if ( food_mat.creature )
    {
        return food_mat.creature->creature_id + ":" + food_mat.caste->caste_id;
    }
    return std::string();
}

size_t OrganicMatLookup::food_max_size ( organic_mat_category::organic_mat_category mat_category )
{
    return world->raws.mat_table.organic_types[mat_category].size();
}

void OrganicMatLookup::food_build_map ( std::ostream &out )
{
    if ( index_built )
        return;
    df::world_raws &raws = world->raws;
    df::special_mat_table table = raws.mat_table;
    using df::enums::organic_mat_category::organic_mat_category;
    df::enum_traits<organic_mat_category> traits;
    for ( int32_t mat_category = traits.first_item_value; mat_category <= traits.last_item_value; ++mat_category )
    {
        for ( size_t i = 0; i < table.organic_indexes[mat_category].size(); ++i )
        {
            int16_t type = table.organic_types[mat_category].at ( i );
            int32_t index = table.organic_indexes[mat_category].at ( i );
            food_index[mat_category].insert ( std::make_pair ( std::make_pair ( type,index ), i ) ); // wtf.. only in c++
        }
    }
    index_built = true;
}

int16_t OrganicMatLookup::food_idx_by_token ( std::ostream &out, organic_mat_category::organic_mat_category mat_category, const std::string & token )
{
    int16_t food_idx = -1;
    df::world_raws &raws = world->raws;
    df::special_mat_table table = raws.mat_table;
    out << "food_idx_by_token: ";
    if ( mat_category == organic_mat_category::Fish ||
            mat_category == organic_mat_category::UnpreparedFish ||
            mat_category == organic_mat_category::Eggs )
    {
        std::vector<std::string> tokens;
        split_string ( &tokens, token, ":" );
        if ( tokens.size() != 2 )
        {
            out << "creature " << "invalid CREATURE:CASTE token: " << token << endl;
        }
        else
        {
            int16_t creature_idx = find_creature ( tokens[0] );
            if ( creature_idx < 0 )
            {
                out << " creature invalid token " << tokens[0];
            }
            else
            {
                food_idx = linear_index ( table.organic_types[mat_category], creature_idx );
                if ( tokens[1] ==  "MALE" )
                    food_idx +=  1;
                if ( table.organic_types[mat_category][food_idx] ==  creature_idx )
                    out << "creature " << token << " caste " <<  tokens[1] <<  " creature_idx(" << creature_idx << ") food_idx("<< food_idx << ")" << endl;
                else
                {
                    out << "ERROR creature caste not found: " << token << " caste " <<  tokens[1] <<  " creature_idx(" << creature_idx << ") food_idx("<< food_idx << ")" << endl;
                    food_idx = -1;
                }
            }
        }
    }
    else
    {
        if ( !index_built )
            food_build_map ( out );
        MaterialInfo mat_info = food_mat_by_token ( out, token );
        int16_t type = mat_info.type;
        int32_t index = mat_info.index;
        int16_t food_idx2 = -1;
        auto it = food_index[mat_category].find ( std::make_pair ( type, index ) );
        if ( it != food_index[mat_category].end() )
        {
            out << "matinfo: " << token << " type(" << mat_info.type << ") idx("  << mat_info.index << ") food_idx(" << it->second << ")" << endl;
            food_idx = it->second;
        }
        else
        {
            out << "matinfo: " << token << " type(" << mat_info.type << ") idx("  << mat_info.index << ") food_idx not found :(" <<  endl;
        }
    }
    return food_idx;
}

MaterialInfo OrganicMatLookup::food_mat_by_token ( std::ostream &out, const std::string & token )
{
    MaterialInfo mat_info;
    mat_info.find ( token );
    return mat_info;
}

bool OrganicMatLookup::index_built = false;
std::vector<OrganicMatLookup::FoodMatMap> OrganicMatLookup::food_index = std::vector<OrganicMatLookup::FoodMatMap> ( 37 );
