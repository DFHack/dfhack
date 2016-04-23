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

#include "../../../include/Mac_compat.h"
#include "../../../include/ExportMaps.h"

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
bool elevation_water_raw_do_work(MapsExporter* maps_exporter);

int  elevation_water(RegionDetailsElevationWater& rdew,
                     int x,
                     int y,
                     int biome_type,
                     df::world_region* region
                     );


/*****************************************************************************
Module main function.
This is the function that the thread executes
*****************************************************************************/
void consumer_elevation_water_raw(void* arg)
{
  bool                finish  = false;
  MapsExporter* maps_exporter = (MapsExporter*)arg;

  if (arg != nullptr)
  {
    while(!finish)
    {
      if (maps_exporter->is_elevation_water_raw_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = elevation_water_raw_do_work(maps_exporter);
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
bool elevation_water_raw_do_work(MapsExporter* maps_exporter)
{
  // Get the data from the queue
  RegionDetailsElevationWater rdew = maps_exporter->pop_elevation_water_raw();

  // Check if is the marker for no more data from the producer
  if (rdew.is_end_marker())
  {
    // All the data has been processed. Finish this thread execution
    return true;
  }

  // The map where we'll write to
  ExportedMapBase* elevation_water_raw_map = maps_exporter->get_elevation_water_raw_map();

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

      df::world_region* region = df::global::world->world_data->regions[rme.region_id];

      int corrected_elevation = elevation_water(rdew,
                                                x,
                                                y,
                                                biome_type,
                                                region
                                                );

      // Write value to the map
      elevation_water_raw_map->write_data(rdew.get_pos_x(),
                                          rdew.get_pos_y(),
                                          x,
                                          y,
                                          corrected_elevation
                                          );
  }
  return false; // Continue working
}

//----------------------------------------------------------------------------//
// Utility function
// Return the corrected elevation value
//----------------------------------------------------------------------------//
int elevation_water(RegionDetailsElevationWater& rdew,
                             int x,
                             int y,
                             int biome_type,
                             df::world_region* region
                             )
{
  int elevation  = rdew.get_elevation(x,y);
  bool has_river = true;

  int river_horiz_y_min              = rdew.get_rivers_horizontal().y_min[x  ][y];
  int river_horiz_y_min_plus_one_row = rdew.get_rivers_horizontal().y_min[x+1][y];

  int river_vert_x_min               = rdew.get_rivers_vertical().x_min[x][y  ];
  int river_vert_x_min_plus_one_col  = rdew.get_rivers_vertical().x_min[x][y+1];

  if ((river_horiz_y_min == -30000) && (river_horiz_y_min_plus_one_row == -30000))
    if ((river_vert_x_min == -30000) && (river_vert_x_min_plus_one_col == -30000))
    {
      elevation = rdew.get_elevation(x,y);
      has_river = false;
    }

  if (has_river)
  {
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
                  if ((biome_type < 27) ||
                      (biome_type > 29) ||
                      (elevation  < 99))
                      {
                        has_river = false;
                        break;
                      }
      default:    break;
    }
  }

  if (has_river)
  {
    elevation = 30000;
    if ((river_horiz_y_min != -30000) &&
        (rdew.get_rivers_horizontal().elevation[x][y] < 30000)
        )
      elevation = rdew.get_rivers_horizontal().elevation[x][y];

    if ((river_horiz_y_min_plus_one_row != -30000) &&
        (rdew.get_rivers_horizontal().elevation[x+1][y] < elevation)
        )
      elevation = rdew.get_rivers_horizontal().elevation[x+1][y];

    if ((river_vert_x_min != -30000) &&
        (rdew.get_rivers_vertical().elevation[x][y] < elevation)
        )
      elevation = rdew.get_rivers_vertical().elevation[x][y];

    if ((river_vert_x_min_plus_one_col != -30000) &&
        (rdew.get_rivers_vertical().elevation[x][y+1] < elevation)
        )
      elevation = rdew.get_rivers_vertical().elevation[x][y+1];
  }

  if (region->lake_surface != -30000)
    elevation = region->lake_surface;

  int corrected_elevation = elevation;

  if ( corrected_elevation < 98)
    corrected_elevation = 98;

  return corrected_elevation;
}

