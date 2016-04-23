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

bool      biome_type_raw_do_work(MapsExporter* maps_exporter);


/*****************************************************************************
Module main function.
This is the function that the thread executes
*****************************************************************************/
void consumer_biome_type_raw(void* arg)
{
  bool             finish        = false;
  MapsExporter*    maps_exporter = (MapsExporter*)arg;

  if (arg != nullptr)
  {
    while(!finish)
    {
      if (maps_exporter->is_biome_raw_type_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = biome_type_raw_do_work(maps_exporter);
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
bool biome_type_raw_do_work(MapsExporter* maps_exporter)
{
  // Get the data from the queue
  RegionDetailsBiome rdb = maps_exporter->pop_biome_type_raw();

  // Check if is the marker for no more data from the producer
  if (rdb.is_end_marker())
    // All the data has been processed. Done
    return true;

  // Get the data where we'll write to
  ExportedMapBase* map = maps_exporter->get_biome_type_raw_map();

  // There's data to be processed
  // Iterate over the 16 subtiles (x) and (y) that a world tile has
  for (auto x=0; x<16; ++x)
    for (auto y=0; y<16; ++y)
    {
      // Each position of the array is a value that tells us if the local tile
      // belongs to the NW,N,NE,W,center,E,SW,S,SE world region.
      // Returns a world coordinate adjusted from the original one
      std::pair<int,int> adjusted_tile_coordinates = adjust_coordinates_to_region(x,
                                                                                  y,
                                                                                  rdb.get_biome_index(x,y),
                                                                                  rdb.get_pos_x(),
                                                                                  rdb.get_pos_y(),
                                                                                  df::global::world->world_data->world_width,
                                                                                  df::global::world->world_data->world_height
                                                                                  );

      // Get the biome type for this world position
      int biome_type = get_biome_type(adjusted_tile_coordinates.first,
                                      adjusted_tile_coordinates.second
                                      );

      // Write the biome type in the buffer
      map->write_data(rdb.get_pos_x(),
                      rdb.get_pos_y(),
                      x,
                      y,
                      biome_type
                      );


  }
  return false; // Continue working
}

