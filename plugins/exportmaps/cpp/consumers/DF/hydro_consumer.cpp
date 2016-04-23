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

#include <utility>
#include <BitArray.h>
#include "../../../include/Mac_compat.h"
#include "../../../include/ExportMaps.h"
#include "../../../include/util/ofsub.h"




using namespace exportmaps_plugin;


/*****************************************************************************
External functions
*****************************************************************************/
extern int                get_biome_type(int world_coord_x,
                                         int world_coord_y
                                         );

extern std::pair<int,int> adjust_coordinates_to_region(int x,
                                                       int y,
                                                       int delta,
                                                       int pos_x,
                                                       int pos_y,
                                                       int world_width,
                                                       int world_height
                                                       );

/*****************************************************************************
Local functions forward declaration
*****************************************************************************/
bool hydro_do_work(MapsExporter* maps_exporter);

// Return the RGB values for the biome export map given a biome type
RGB_color RGB_from_elevation_water(RegionDetailsElevationWater& rdew,
                                   int x,
                                   int y,
                                   int biome_type,
                                   DFHack::BitArray<df::region_map_entry_flags>& flags
                                   );

// Return a world river
std::pair<df::world_river*, int> get_world_river(int x,
                                                 int y
                                                 );


/*****************************************************************************
Module main function.
This is the function that the thread executes
*****************************************************************************/
void consumer_hydro(void* arg)
{
  bool                finish  = false;
  MapsExporter* maps_exporter = (MapsExporter*)arg;

  if (arg != nullptr)
  {
    while(!finish)
    {
      if (maps_exporter->is_hydro_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = hydro_do_work(maps_exporter);
    }
  }
  // Function finish -> Thread finish
}

//----------------------------------------------------------------------------//
// Utility function
//
// Get the data from the queue.
// If is the end marker, the queue is empty and no more work needs to be done, return
// If it's actual data process it and update the corresponding map
//----------------------------------------------------------------------------//
bool hydro_do_work(MapsExporter* maps_exporter)
{
  // Get the data from the queue
  RegionDetailsElevationWater rdew = maps_exporter->pop_hydro();

  // Check if is the marker for no more data from the producer
  if (rdew.is_end_marker())
  {
    // All the data has been processed. Finish this thread execution
    return true;
  }

  // Get the map where we'll write to
  ExportedMapBase* hydro_map = maps_exporter->get_hydro_map();

  // Iterate over the 16 subtiles (x) and (y) that a world tile has
  for (auto x=0; x<16; ++x)
    for (auto y=15; y>=0; --y)
    {
      // Each position of the array is a value that tells us if the local tile
      // belongs to the NW,N,NE,W,center,E,SW,S,SE world region.
      // Returns a world coordinate adjusted from the original one
      std::pair<int,int> adjusted_tile_coordinates = adjust_coordinates_to_region(x,
                                                                                  y,
                                                                                  rdew.get_biome_index(x,y),
                                                                                  rdew.get_pos_x(),
                                                                                  rdew.get_pos_y(),
                                                                                  df::global::world->world_data->world_width,
                                                                                  df::global::world->world_data->world_height
                                                                                  );

      // Get the biome type for this world position
      int biome_type = get_biome_type(adjusted_tile_coordinates.first,
                                      adjusted_tile_coordinates.second
                                      );

      // Get the region where this position belongs to
      df::region_map_entry& rme = df::global::world->world_data->region_map[adjusted_tile_coordinates.first]
                                                                           [adjusted_tile_coordinates.second];

      // Get the RGB values associated to this biome type
      RGB_color rgb_pixel_color = RGB_from_elevation_water(rdew,
                                                           x,
                                                           y,
                                                           biome_type,
                                                           rme.flags
                                                           );

      // Write pixels to the bitmap
      hydro_map->write_world_pixel(rdew.get_pos_x(),
                                   rdew.get_pos_y(),
                                   x,
                                   y,
                                   rgb_pixel_color
                                   );

  }
  return false; // Continue working
}

//----------------------------------------------------------------------------//
// Utility function
// Return the RGB values for the hydrosphere export map
//----------------------------------------------------------------------------//

RGB_color no_river_color(int biome_type,
                         int elevation
                         )
{
    // No river present
    if (biome_type == 0) // MOUNTAIN biome
        return RGB_color(0xff,0xff,0xc0);

    if ((biome_type > 35) && (biome_type <= 41)) // LAKE biome
        return RGB_color(0x00,0x60,0xff);

    if (elevation < 99) // OCEAN biome
        return RGB_color(0x00,0x40,0xff);

    // LAND biome
    return RGB_color(0x80,0x40,0x20);

}

//----------------------------------------------------------------------------//
// Utility function
//----------------------------------------------------------------------------//
RGB_color RGB_from_elevation_water(RegionDetailsElevationWater& rdew,
                                   int x,
                                   int y,
                                   int biome_type,
                                   DFHack::BitArray<df::region_map_entry_flags>& flags
                                   )
{
    int elevation                      = rdew.get_elevation(x,y);
    int river_horiz_y_min              = rdew.get_rivers_horizontal().y_min[x][y  ];
    int river_horiz_y_min_plus_one_row = rdew.get_rivers_horizontal().y_min[x+1][y];
    int river_vert_x_min               = rdew.get_rivers_vertical().x_min[x  ][y];
    int river_vert_x_min_plus_one_col  = rdew.get_rivers_vertical().x_min[x][y+1];

    if ((river_horiz_y_min == -30000) && (river_horiz_y_min_plus_one_row == -30000))
        if ((river_vert_x_min == -30000) && (river_vert_x_min_plus_one_col == -30000))
            return no_river_color(biome_type, elevation);

    // Posibly rivers finishing in ocean or lake
    // Posibly rivers in glacier or tundra are discarted
    switch(biome_type)
    {
        case 1:     // GLACIER
        case 2:     // TUNDRA
        case 27:    // OCEAN_TROPICAL
        case 28:    // OCEAN_TEMPERATE
        case 29:    // OCEAN_ARCTIC
        case 36:    // LAKE_TEMPERATE_FRESHWATER
        case 37:    // LAKE_TEMPERATE_BRACKISH_WATER
        case 38:    // LAKE_TEMPERATE_SALTWATER
        case 39:    // LAKE_TROPICAL_FRESHWATER
        case 40:    // LAKE_TROPICAL_BRACKISH_WATER
        case 41:    // LAKE_TROPICAL_SALT_WATER
                    if ((biome_type < 27) ||  // OCEAN_TROPICAL
                        (biome_type > 29) ||  // OCEAN_ARTIC
                        (elevation  < 99))    // Not shore
                            return no_river_color(biome_type,
                                                  elevation
                                                  );
                    break;
        default:    break;
    }

    bool river_flag = flags.is_set(df::enums::region_map_entry_flags::region_map_entry_flags::has_river);
    bool brook_flag = flags.is_set(df::enums::region_map_entry_flags::region_map_entry_flags::is_brook);

    char* p_void = (char*)&flags;
    p_void += 4;
    uint32_t size = (uint32_t)*p_void;
    bool v41 = __OFSUB__(size,1);
    bool v40 = size == 1;
    bool v12 = (size-1) < 0;
    bool expr = (v12 ^ v41) | v40 || !brook_flag;

    if (!expr)
        return RGB_color(0x00,0xff,0xff); // Brook

    std::pair<df::world_river*, int> river_data = get_world_river(rdew.get_pos_x(),
                                                                  rdew.get_pos_y()
                                                                  );
    df::world_river* ptr_world_river = river_data.first;
    int river_index = river_data.second;

    if (ptr_world_river == nullptr)
        return RGB_color(0x00,0x70,0xff); // Sea or lake shore

    int value = ptr_world_river->unk_8c[river_index];
    if (value >= 20000)
        return RGB_color(0x00,0x80,0xff); // Major river

    if (value >= 10000)
        return RGB_color(0x00,0xa0,0xff); // river

    if (value >= 5000)
        return RGB_color(0x00,0xc0,0xff); // minor river

    return RGB_color(0x00,0xe0,0xff); // stream
}

//----------------------------------------------------------------------------//
// Utility function
//----------------------------------------------------------------------------//
std::pair<df::world_river*, int> get_world_river(int x, int y)
{
    std::pair<df::world_river*, int> NO_RIVER(nullptr,-1);

    // Out of bounds check
    if ((x < 0) || (x >= df::global::world->world_data->world_width) ||
        (y < 0) || (y >= df::global::world->world_data->world_height))
            return NO_RIVER;

    if (df::global::world->world_data->feature_map != nullptr)
    {
        df::world_data::T_feature_map& t_feature = df::global::world->world_data->feature_map[x >> 4][y >> 4];

        if (t_feature.unk_8 != nullptr)
        {
            int offset = 2 * ((y % 16) + sizeof(df::world_data::T_feature_map) * (x % 16));
            int value1  = t_feature.unk_8[offset];
            if (value1 != -1)
            {
                int value2  = t_feature.unk_8[offset + 1];
                df::world_river* river = df::global::world->world_data->rivers[value1];
                return std::pair<df::world_river*, int>(river,value2);
            }
        }
        return NO_RIVER;
    }
    else // No feature map
    {
    }

    return NO_RIVER;
}


