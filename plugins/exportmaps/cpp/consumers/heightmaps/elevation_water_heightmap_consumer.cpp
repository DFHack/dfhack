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

extern int elevation_water(RegionDetailsElevationWater& rdew,
                             int x,
                             int y,
                             int biome_type,
                             df::world_region* region
                             );

extern int find_max_world_elevation();


/*****************************************************************************
Local functions forward declaration
*****************************************************************************/
bool elevation_water_heightmap_do_work(MapsExporter* maps_exporter,
                                       int max_world_elevation
                                       );


/*****************************************************************************
Module main function.
This is the function that the thread executes
*****************************************************************************/
void consumer_elevation_water_heightmap(void* arg)
{
  bool                finish  = false;
  MapsExporter* maps_exporter = (MapsExporter*)arg;

  if (arg != nullptr)
  {
    // Find the maximum height in the world
    int max_world_elevation = find_max_world_elevation();

    while(!finish)
    {
      if (maps_exporter->is_elevation_water_hm_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = elevation_water_heightmap_do_work(maps_exporter, max_world_elevation);
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
bool elevation_water_heightmap_do_work(MapsExporter* maps_exporter, int max_world_elevation)
{
  // Get the data from the queue
  RegionDetailsElevationWater rdew = maps_exporter->pop_elevation_water_hm();

  // Check if is the marker for no more data from the producer
  if (rdew.is_end_marker())
  {
    // All the data has been processed. Finish this thread execution
    return true;
  }

  // The map where we'll write to
  ExportedMapBase* elevation_water_heightmap_map = maps_exporter->get_elevation_water_hm_map();

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

      // Scale the value relative to the world maximum elevation
      // The value for the maximum world elevation must be 255
      int scaled_elevation = (corrected_elevation * 255) / max_world_elevation;


      RGB_color pixel_color = RGB_color(scaled_elevation,
                                        scaled_elevation,
                                        scaled_elevation
                                        );

      // Write value to the map
      elevation_water_heightmap_map->write_world_pixel(rdew.get_pos_x(),
                                                       rdew.get_pos_y(),
                                                       x,
                                                       y,
                                                       pixel_color
                                                       );
  }
  return false; // Continue working
}




