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
#include <set>
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

extern RGB_color RGB_from_biome_type(int biome_type);

extern df::world_region* find_world_region(int region_id);


/*****************************************************************************
Local functions forward declaration
*****************************************************************************/
bool region_do_work(MapsExporter* maps_exporter);

// Return the RGB values for the biome export map given a biome type
RGB_color RGB_from_elevation_water(RegionDetailsElevationWater& rdew,
                                   int x,
                                   int y,
                                   int biome_type,
                                   DFHack::BitArray<df::region_map_entry_flags>& flags
                                   );

RGB_color RGB_from_region_type(int region_type);

std::set< std::pair<int,int>, RegionDetailsElevationWater > cache_set;


/*****************************************************************************
Module main function.
This is the function that the thread executes
*****************************************************************************/
void consumer_region(void* arg)
{
  bool                finish  = false;
  MapsExporter* maps_exporter = (MapsExporter*)arg;

  if (arg != nullptr)
  {
    while(!finish)
    {
      if (maps_exporter->is_region_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = region_do_work(maps_exporter);
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
bool region_do_work(MapsExporter* maps_exporter)
{
  // Get the data from the queue
  RegionDetailsElevationWater rdew = maps_exporter->pop_region();

  // Check if is the marker for no more data from the producer
  if (rdew.is_end_marker())
  {
    // All the data has been processed. Finish this thread execution
    return true;
  }

  // Get the map where we'll write to
  ExportedMapBase* region_map = maps_exporter->get_region_map();

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
      // Get the world region where it belongs
      df::world_region* world_region = find_world_region(rme.region_id);
      if (world_region == nullptr) continue;

      // Get a color from the region type
      RGB_color rgb_pixel_color = RGB_from_region_type(world_region->type);

      // Write pixels to the bitmap
      region_map->write_world_pixel(rdew.get_pos_x(),
                                    rdew.get_pos_y(),
                                    x,
                                    y,
                                    rgb_pixel_color
                                    );

  }
  return false; // Continue working
}

//----------------------------------------------------------------------------//
//Utility function
//Return the RGB values for the region export map given a region type.
//----------------------------------------------------------------------------//
RGB_color RGB_from_region_type(int region_type)
{
  unsigned char r = -1;
  unsigned char g = -1;
  unsigned char b = -1;

  switch (region_type)
  {
    case 0: // Swamp
            r = 0x60; g = 0xc0 ; b = 0x40; break;
    case 1: //Desert,
            r = 0xff; g = 0xff ; b = 0x00; break;
    case 2: //Jungle,
            r = 0x00; g = 0x60 ; b = 0x20; break;
    case 3: //Mountains,
            r = 0x80; g = 0x80 ; b = 0x80; break;
    case 4: //Ocean,
            r = 0x00; g = 0x80 ; b = 0xff; break;
    case 5: //Lake,
            r = 0x00; g = 0xe0 ; b = 0xff; break;
    case 6: //Glacier,
            r = 0x40; g = 0xff ; b = 0xff; break;
    case 7: //Tundra,
            r = 0x80; g = 0xff ; b = 0xff; break;
    case 8: //Steppe,
            r = 0x00; g = 0xff ; b = 0x20; break;
    case 9: //Hills
            r = 0x00; g = 0xa0 ; b = 0x00; break;
  }

  return RGB_color(r,g,b);
}
