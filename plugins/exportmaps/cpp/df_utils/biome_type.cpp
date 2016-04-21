/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

// You can always find the latest version of this plugin in Github
// https://github.com/ragundo/exportmaps

#include "../../include/dfhack.h"
#include "../../include/util/ofsub.h"


/*****************************************************************************
 Local functions forward declaration
*****************************************************************************/
std::pair<bool,bool> check_tropicality                       (df::region_map_entry& region,
                                                              int a1
                                                              );

int                  get_lake_biome                          (df::region_map_entry& region,
                                                              bool is_possible_tropical_area_by_latitude
                                                              );

int                  get_ocean_biome                         (df::region_map_entry& region,
                                                              bool is_tropical_area_by_latitude
                                                              );

int                  get_desert_biome                        (df::region_map_entry& region);

int                  get_biome_grassland                     (bool is_possible_tropical_area_by_latitude,
                                                              bool is_tropical_area_by_latitude,
                                                              int y,
                                                              int x
                                                              );

int                  get_biome_savanna                       (bool is_possible_tropical_area_by_latitude,
                                                              bool is_tropical_area_by_latitude,
                                                              int y,
                                                              int x
                                                              );

int                  get_biome_shrubland                     (bool is_possible_tropical_area_by_latitude,
                                                              bool is_tropical_area_by_latitude,
                                                              int y,
                                                              int x
                                                              );

int                  get_biome_marsh                         (df::region_map_entry& region,
                                                              bool is_possible_tropical_area_by_latitude,
                                                              bool is_tropical_area_by_latitude,
                                                              int y,
                                                              int x
                                                              );

int                  get_biome_forest                        (df::region_map_entry& region,
                                                              bool is_possible_tropical_area_by_latitude,
                                                              bool is_tropical_area_by_latitude,
                                                              int y,
                                                              int x
                                                              );

int                  get_biome_swamp                         (df::region_map_entry& region,
                                                              bool is_possible_tropical_area_by_latitude,
                                                              bool is_tropical_area_by_latitude,
                                                              int y,
                                                              int x
                                                              );

int                  get_biome_desert_or_grassland_or_savanna (df::region_map_entry& region,
                                                               bool is_possible_tropical_area_by_latitude,
                                                               bool is_tropical_area_by_latitude,
                                                               int y,
                                                               int x
                                                               );

int                  get_biome_shrubland_or_marsh            (df::region_map_entry& region,
                                                              bool is_possible_tropical_area_by_latitude,
                                                              bool is_tropical_area_by_latitude,
                                                              int y,
                                                              int x
                                                              );

/*****************************************************************************
 Module main function.

 Return the biome type, given a position coordinate expressed in world_tiles
*****************************************************************************/
int get_biome_type(int world_coord_x,
                   int world_coord_y
                   )
{
    // Biome is per region, so get the region where this biome exists
    df::region_map_entry& region = df::global::world->world_data->region_map[world_coord_x][world_coord_y];

    // Check if the y position coordinate belongs to a tropical area
    std::pair<bool,bool> p                     = check_tropicality(region,
                                                                   world_coord_y
                                                                   );
    bool is_possible_tropical_area_by_latitude = p.first;
    bool is_tropical_area_by_latitude          = p.second;

    // Begin the discrimination
    if (region.flags.is_set(df::region_map_entry_flags::is_lake)) // is it a lake?
        return get_lake_biome(region,
                              is_possible_tropical_area_by_latitude
                              );

    // Not a lake. Check elevation
    // Elevation greater then 149 means a mountain biome
    // Elevation below 100 means a ocean biome
    // Elevation between 100 and 149 are land biomes

    if (region.elevation >= 150) // is it a mountain?
        return df::enums::biome_type::biome_type::MOUNTAIN; // 0

    if (region.elevation < 100) // is it a ocean?
        return get_ocean_biome(region,
                               is_possible_tropical_area_by_latitude
                               );

    // land biome. Elevation between 100 and 149
    if (region.temperature <= -5)
    {
        if (region.drainage < 75)
            return df::enums::biome_type::biome_type::TUNDRA; // 2
        else
            return df::enums::biome_type::biome_type::GLACIER; // 1
    }

    // Not a lake, mountain, ocean, glacier or tundra
    // Vegetation determines the biome type
    if ( region.vegetation < 66)
    {
        if (region.vegetation < 33)
            return get_biome_desert_or_grassland_or_savanna(region,
                                                            is_possible_tropical_area_by_latitude,
                                                            is_tropical_area_by_latitude,
                                                            world_coord_y,
                                                            world_coord_x
                                                            );
        else // vegetation between 33 and 65
            return get_biome_shrubland_or_marsh(region,
                                                is_possible_tropical_area_by_latitude,
                                                is_tropical_area_by_latitude,
                                                world_coord_y,
                                                world_coord_x
                                                );
    }

    // Not a lake, mountain, ocean, glacier, tundra, desert, grassland or savanna
    // vegetation >= 66
    if (region.drainage >= 33)
        return get_biome_forest(region,
                                is_possible_tropical_area_by_latitude,
                                is_tropical_area_by_latitude,
                                world_coord_y,
                                world_coord_x
                                );

    // Not a lake, mountain, ocean, glacier, tundra, desert, grassland, savanna or forest
    // vegetation >= 66, drainage < 33
    return get_biome_swamp (region,
                            is_possible_tropical_area_by_latitude,
                            is_tropical_area_by_latitude,
                            world_coord_y,
                            world_coord_x);
}



//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
std::pair<bool,bool> check_tropicality_no_poles_world(df::region_map_entry& region,
                                                      int y_pos
                                                      )
{
    int is_possible_tropical_area_by_latitude = 0;
    int is_tropical_area_by_latitude = 0;

    // If there're no poles, tropical area is determined by temperature
    if (region.temperature >= 75)
        is_possible_tropical_area_by_latitude = true;
    int v9 = __OFSUB__(region.temperature,85);
    int v8 = (region.temperature - 85) < 0;
    if (!(v8 ^ v9))
        is_tropical_area_by_latitude = true;

    return std::pair<bool,bool>(is_possible_tropical_area_by_latitude,
                                is_tropical_area_by_latitude
                                );
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
std::pair<bool,bool> check_tropicality_north_pole_only_world(df::region_map_entry& region,
                                                             int y_pos
                                                             )
{
    int v6;
    int is_possible_tropical_area_by_latitude = 0;
    int is_tropical_area_by_latitude = 0;
    df::world_data* wdata = df::global::world->world_data;

    // Scale the smaller worlds to the big one
    if ( wdata->world_height == 17)
        v6 = 16 * y_pos;
    else if ( wdata->world_height == 33)
        v6 = 8 * y_pos;
    else if ( wdata->world_height == 65)
        v6 = 4 * y_pos;
    else if ( wdata->world_height == 129)
        v6 = 2 * y_pos;
    else
        v6 = y_pos;

    is_possible_tropical_area_by_latitude = v6 > 170;
    int v8 = ((v6 - 200) < 0);
    int v9 = __OFSUB__(v6,200);
    if (!(v8 ^ v9))
        is_tropical_area_by_latitude = true;

    return std::pair<bool,bool>(is_possible_tropical_area_by_latitude,
                                is_tropical_area_by_latitude
                                );
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
std::pair<bool,bool> check_tropicality_south_pole_only_world(df::region_map_entry& region,
                                                             int y_pos
                                                             )
{
    int v6 = df::global::world->world_data->world_height - y_pos - 1;
    int is_possible_tropical_area_by_latitude = 0;
    int is_tropical_area_by_latitude = 0;
    df::world_data* wdata = df::global::world->world_data;

    if ( wdata->world_height == 17)
        v6 *= 16;
    else if ( wdata->world_height == 33)
        v6 *= 8;
    else if ( wdata->world_height == 65)
        v6 *= 4;
    else if ( wdata->world_height == 129)
        v6 *= 2;
    else
        v6 *= 1;

    is_possible_tropical_area_by_latitude = v6 > 170;
    int v9 = __OFSUB__(v6,200);
    int v8 = ((v6 - 200) < 0);
    if (!(v8 ^ v9))
        is_tropical_area_by_latitude = true;
//    if ((v6 > 170) && (v6 < 200))
//        is_tropical_area_by_latitude = true;

    return std::pair<bool,bool>(is_possible_tropical_area_by_latitude,
                                is_tropical_area_by_latitude
                                );
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
std::pair<bool,bool> check_tropicality_both_poles_world(df::region_map_entry& region,
                                                        int y_pos
                                                        )
{
    int v6;
    int is_possible_tropical_area_by_latitude = 0;
    int is_tropical_area_by_latitude = 0;
    df::world_data* wdata = df::global::world->world_data;

    if (y_pos < wdata->world_height/2)
        v6 = 2 * y_pos;
    else
    {
        v6 = wdata->world_height + 2*(wdata->world_height / 2 - y_pos) - 1;
        if (v6 < 0)
            v6 = 0;
        if (v6 >= wdata->world_height)
            v6 = wdata->world_height - 1;
    }

    if (wdata->world_height == 17)
        v6 *= 16;
    else if (wdata->world_height == 33)
        v6 *= 8;
    else if (wdata->world_height == 65)
        v6 *= 4;
    else if (wdata->world_height == 129)
        v6 *= 2;
    else
        v6 *= 1;

    is_possible_tropical_area_by_latitude = v6 > 170;
    int v9 = __OFSUB__(v6,200);
    int v8 = (v6-200) < 0;
    if (!(v8 ^ v9))
        is_tropical_area_by_latitude = true;

    return std::pair<bool,bool>(is_possible_tropical_area_by_latitude,
                                is_tropical_area_by_latitude
                                );
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
std::pair<bool,bool> check_tropicality(df::region_map_entry& region,
                                       int y_pos
                                       )
{
    int flip_latitude = df::global::world->world_data->flip_latitude;

    if (flip_latitude == -1)  // NO POLES
        return check_tropicality_no_poles_world(region,
                                                y_pos
                                                );

    else if (flip_latitude == 0) // NORTH POLE ONLY
        return check_tropicality_north_pole_only_world(region,
                                                       y_pos
                                                       );

    else if (flip_latitude == 1) // SOUTH_POLE ONLY
        return check_tropicality_south_pole_only_world(region,
                                                       y_pos
                                                       );

    else if (flip_latitude == 2) // BOTH POLES
        return check_tropicality_both_poles_world(region,
                                                  y_pos
                                                  );

     return std::pair<bool,bool>(false,false);
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_parameter_percentage(int flip_latitude,
                             int y_pos,
                             int rainfall,
                             int world_height
                             )
{
    int result;
    int ypos = y_pos;

    if (flip_latitude == -1) // NO POLES
        return 100;

    else if (flip_latitude == 1) // SOUTH POLE
        ypos = world_height - y_pos - 1;
    else if (flip_latitude == 2) // NORTH & SOUTH POLE
    {
        if (ypos < world_height/2)
            ypos *= 2;
        else
        {
            ypos = world_height + 2 * (world_height/2 - ypos) - 1;
            if (ypos < 0)
                ypos = 0;
            if (ypos >= world_height)
                ypos = world_height - 1;
        }
    }

    int latitude; // 0 - 256 (size of a large world)
    switch ( world_height )
    {
      case 17:                                    // Pocket world
                latitude = 16 * ypos;
                break;
      case 33:                                    // Smaller world
                latitude = 8 * ypos;
                break;
      case 65:                                    // Small world
                latitude = 4 * ypos;
                break;
      case 129:                                  // Medium world
                latitude = 2 * ypos;
                break;
      default:                                  // Large world
                latitude = ypos;
                break;
    }

    // latitude > 220
    if ((latitude - 171) > 49)
        return 100;


    // Latitude between 191 and 200
    if ((latitude > 190) && (latitude <  201))
        return 0;

    // Latitude between 201 and 220
    if ((latitude > 190) && (latitude >= 201))
        result = rainfall + 16 * (latitude - 207);
    else
    // Latitude between 0 and 190
    result = (16 * (184 - latitude) - rainfall);

    if (result < 0)
        return 0;

    if (result > 100)
        return 100;

    return result;
}

//----------------------------------------------------------------------------//
// Utility function
//
// return some unknow parameter as a percentage
//----------------------------------------------------------------------------//
int get_region_parameter(int y,
                         int x,
                         char a4
                         )
{
    int result = 100;

    if ((df::global::cur_season && *df::global::cur_season != 1) || !a4)
    {
        int world_height = df::global::world->world_data->world_height;
        if (world_height > 65) // Medium and large worlds
        {
            // access to region 2D array
            df::region_map_entry& region = df::global::world->world_data->region_map[x][y];
            return get_parameter_percentage(df::global::world->world_data->flip_latitude,
                                            y,
                                            region.rainfall,
                                            world_height
                                            );
        }
    }
    return result;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_lake_biome(df::region_map_entry& region,
                   bool is_possible_tropical_area_by_latitude
                   )
{
    // salinity values tell us the lake type
    // greater than 66 is a salt water lake
    // between 33 and 65 is a brackish water lake
    // less than 33 is a fresh water lake
    if (region.salinity < 66)
    {
        if (region.salinity < 33)
            if (is_possible_tropical_area_by_latitude)
                return df::enums::biome_type::biome_type::LAKE_TROPICAL_FRESHWATER; // 39
            else
                return df::enums::biome_type::biome_type::LAKE_TEMPERATE_FRESHWATER; // 36
        else // salinity >= 33
            if (is_possible_tropical_area_by_latitude)
                return df::enums::biome_type::biome_type::LAKE_TROPICAL_BRACKISHWATER; // 40
            else
                return df::enums::biome_type::biome_type::LAKE_TEMPERATE_BRACKISHWATER; // 37
    }
    else // salinity >= 66
    {
        if (is_possible_tropical_area_by_latitude)
            return df::enums::biome_type::biome_type::LAKE_TROPICAL_SALTWATER;// 41
        else
            return df::enums::biome_type::biome_type::LAKE_TEMPERATE_SALTWATER; // 38
    }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_ocean_biome(df::region_map_entry& region,
                    bool is_tropical_area_by_latitude
                    )
{
    if (is_tropical_area_by_latitude)
        return df::enums::biome_type::biome_type::OCEAN_TROPICAL; // 27
    else
        if ( region.temperature <= -5)
            return df::enums::biome_type::biome_type::OCEAN_ARCTIC ; // 29
        else
            return df::enums::biome_type::biome_type::OCEAN_TEMPERATE; // 28
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_desert_biome(df::region_map_entry& region)
{
    if (region.drainage < 66)
    {
        if (region.drainage < 33)
            return df::enums::biome_type::biome_type::DESERT_SAND; // 26
        else // drainage between 33 and 65
            return df::enums::biome_type::biome_type::DESERT_ROCK; // 25
    }
    // drainage >= 66
    return df::enums::biome_type::biome_type::DESERT_BADLAND; // 24
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_biome_grassland(bool is_possible_tropical_area_by_latitude,
                        bool is_tropical_area_by_latitude,
                        int y,
                        int x
                        )
{
    if ((is_possible_tropical_area_by_latitude && (get_region_parameter(y, x, 0) < 66)) || is_tropical_area_by_latitude)
        return df::enums::biome_type::biome_type::GRASSLAND_TROPICAL; // 21
    else
        return df::enums::biome_type::biome_type::GRASSLAND_TEMPERATE; //18;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_biome_savanna(bool is_possible_tropical_area_by_latitude,
                      bool is_tropical_area_by_latitude,
                      int y,
                      int x
                      )
{
    if ((is_possible_tropical_area_by_latitude && (get_region_parameter(y, x, 0) <= 6)) || is_tropical_area_by_latitude)
        return df::enums::biome_type::biome_type::SAVANNA_TROPICAL; // 22
    else
        return df::enums::biome_type::biome_type::SAVANNA_TEMPERATE; //19;

}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_biome_desert_or_grassland_or_savanna(df::region_map_entry& region,
                                             bool is_possible_tropical_area_by_latitude,
                                             bool is_tropical_area_by_latitude,
                                             int y,
                                             int x
                                             )
{
    if (region.vegetation < 20)
    {
        if (region.vegetation < 10)
            return get_desert_biome(region);
        else // vegetation between 10 and 19
            return get_biome_grassland(is_possible_tropical_area_by_latitude, is_tropical_area_by_latitude,y, x);
    }
    // vegetation between 20 and 32
    return get_biome_savanna(is_possible_tropical_area_by_latitude, is_tropical_area_by_latitude, y, x);
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_biome_shrubland(bool is_possible_tropical_area_by_latitude,
                        bool is_tropical_area_by_latitude,
                        int y,
                        int x
                        )
{
    if (is_possible_tropical_area_by_latitude && (get_region_parameter(y, x, 0) < 66 || is_tropical_area_by_latitude))
        return df::enums::biome_type::biome_type::SHRUBLAND_TROPICAL; // 23
    else
        return df::enums::biome_type::biome_type::SHRUBLAND_TEMPERATE; // 20
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_biome_marsh(df::region_map_entry& region,
                    bool is_possible_tropical_area_by_latitude,
                    bool is_tropical_area_by_latitude,
                    int y,
                    int x
                    )
{
    if (region.salinity < 66)
    {
        if ((is_possible_tropical_area_by_latitude && (get_region_parameter(y, x, 0) < 66)) || is_tropical_area_by_latitude)
            return df::enums::biome_type::biome_type::MARSH_TROPICAL_FRESHWATER; // 10
        else
            return df::enums::biome_type::biome_type::MARSH_TEMPERATE_FRESHWATER; // 5
    }
    else // drainage < 33, salinity >= 66
    {
        if ((is_possible_tropical_area_by_latitude && (get_region_parameter(y, x, 0) < 66)) || is_tropical_area_by_latitude)
            return df::enums::biome_type::biome_type::MARSH_TROPICAL_SALTWATER; // 11
        else
            return df::enums::biome_type::biome_type::MARSH_TEMPERATE_SALTWATER; // 6
    }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_biome_shrubland_or_marsh(df::region_map_entry& region,
                                 bool is_possible_tropical_area_by_latitude,
                                 bool is_tropical_area_by_latitude,
                                 int y,
                                 int x
                                 )
{
    if (region.drainage >= 33)
        return get_biome_shrubland(is_possible_tropical_area_by_latitude,
                                   is_tropical_area_by_latitude,
                                   y,
                                   x
                                   );
    // drainage < 33
    return get_biome_marsh(region,
                           is_possible_tropical_area_by_latitude,
                           is_tropical_area_by_latitude,
                           y,
                           x
                           );
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_biome_forest(df::region_map_entry& region,
                     bool is_possible_tropical_area_by_latitude,
                     bool is_tropical_area_by_latitude,
                     int y,
                     int x
                     )
{
    int parameter = get_region_parameter(y, x, 0);

    // drainage >= 33, not tropical area
    if (!is_possible_tropical_area_by_latitude)
    {
        if ((region.rainfall < 75) || (region.temperature < 65))
        {
            if (region.temperature >= 10)
                return df::enums::biome_type::biome_type::FOREST_TEMPERATE_CONIFER; // 13
            else
                return df::enums::biome_type::biome_type::FOREST_TAIGA; // 12
        }
        else
            return df::enums::biome_type::biome_type::FOREST_TEMPERATE_BROADLEAF; // 14
    }
    else // drainage >= 33, tropical area
    {
        if (((parameter < 66) || is_tropical_area_by_latitude) && (region.rainfall < 75))
            return df::enums::biome_type::biome_type::FOREST_TROPICAL_CONIFER; // 15
        if (parameter < 66)
            return df::enums::biome_type::biome_type::FOREST_TROPICAL_DRY_BROADLEAF; // 16
        if (is_tropical_area_by_latitude)
            return df::enums::biome_type::biome_type::FOREST_TROPICAL_MOIST_BROADLEAF; // 17
        else
        {
            if ((region.rainfall < 75) || (region.temperature < 65))
            {
                if (region.temperature >= 10)
                    return df::enums::biome_type::biome_type::FOREST_TEMPERATE_CONIFER; // 13
                else
                    return df::enums::biome_type::biome_type::FOREST_TAIGA; // 12
            }
            else
                return df::enums::biome_type::biome_type::FOREST_TEMPERATE_BROADLEAF; // 14
        }
    }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_biome_swamp(df::region_map_entry& region,
                    bool is_possible_tropical_area_by_latitude,
                    bool is_tropical_area_by_latitude,
                    int y,
                    int x
                    )
{
    int parameter = get_region_parameter(y, x, 0);

    if (is_possible_tropical_area_by_latitude)
    {
        if (region.salinity < 66)
        {
            if ((parameter < 66) || is_tropical_area_by_latitude)
                return df::enums::biome_type::biome_type::SWAMP_TROPICAL_FRESHWATER; // 7
            else
                return df::enums::biome_type::biome_type::SWAMP_TEMPERATE_FRESHWATER;// 3
        }
        else // elevation between 100 and 149, vegetation >= 66, drainage < 33, salinity >= 66
        {
            if ((parameter < 66) || is_tropical_area_by_latitude)
            {
                if (region.drainage < 10)
                    return df::enums::biome_type::biome_type::SWAMP_MANGROVE; //9
                else // drainage >= 10
                    return df::enums::biome_type::biome_type::SWAMP_TROPICAL_SALTWATER; // 8
            }
            else
                return df::enums::biome_type::biome_type::SWAMP_TEMPERATE_SALTWATER; // 4
        }
    }
    else // elevation between 100 and 149, vegetation >= 66, drainage < 33, not tropical area
    {
        if (region.salinity >= 66)
            return df::enums::biome_type::biome_type::SWAMP_TEMPERATE_SALTWATER; // 4
        else
            return df::enums::biome_type::biome_type::SWAMP_TEMPERATE_FRESHWATER; // 3
    }
}


