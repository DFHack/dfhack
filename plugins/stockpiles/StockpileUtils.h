#pragma once

#include "MiscUtils.h"

#include "df/world.h"
#include "df/world_data.h"
#include "df/creature_raw.h"
#include "df/plant_raw.h"

#include <string>
#include <algorithm>
#include <functional>

//  os
#include <sys/stat.h>

// Utility Functions {{{
// A set of convenience functions for doing common lookups


/**
 * Retrieve creature raw from index
 */
static df::creature_raw* find_creature ( int32_t idx )
{
    return df::global::world->raws.creatures.all[idx];
}

/**
 * Retrieve creature index from id string
 * @return -1 if not found
 */
static int16_t find_creature ( const std::string &creature_id )
{
    return linear_index ( df::global::world->raws.creatures.all, &df::creature_raw::creature_id, creature_id );
}

/**
 * Retrieve plant raw from index
*/
static df::plant_raw* find_plant ( size_t idx )
{
    return df::global::world->raws.plants.all[idx];
}

/**
 * Retrieve plant index from id string
 * @return -1 if not found
 */
static size_t find_plant ( const std::string &plant_id )
{
    return linear_index ( df::global::world->raws.plants.all, &df::plant_raw::id, plant_id );
}

struct less_than_no_case: public std::binary_function< char,char,bool >
{
   bool operator () (char x, char y) const
   {
       return toupper( static_cast< unsigned char >(x)) < toupper( static_cast< unsigned char >(y));
   }
};

static bool CompareNoCase(const std::string &a, const std::string &b)
{
    return std::lexicographical_compare( a.begin(),a.end(), b.begin(),b.end(), less_than_no_case() );
}


/**
 * Checks if the parameter has the dfstock extension.
 * Doesn't check if the file exists or not.
 */
static bool is_dfstockfile ( const std::string& filename )
{
    return filename.rfind ( ".dfstock" ) !=  std::string::npos;
}

// }}} utility Functions


