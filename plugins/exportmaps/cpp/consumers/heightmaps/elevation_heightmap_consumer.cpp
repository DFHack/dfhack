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

#include "../../../include/Mac_compat.h"
#include "../../../include/ExportMaps.h"

// You can always find the latest version of this plugin in Github
// https://github.com/ragundo/exportmaps

using namespace exportmaps_plugin;

/*****************************************************************************
Local functions forward declaration
*****************************************************************************/
bool      elevation_heightmap_do_work(MapsExporter* maps_exporter,
                                      int max_world_elevation
                                      );

int find_max_world_elevation();

/*****************************************************************************
Module main function.
This is the function that the thread executes
*****************************************************************************/
void consumer_elevation_heightmap(void* arg)
{
  bool                finish  = false;
  MapsExporter* maps_exporter = (MapsExporter*)arg;

  if (arg != nullptr)
  {
    // Find the maximum height in the world
    int max_world_elevation = find_max_world_elevation();

    while(!finish)
    {
      if (maps_exporter->is_elevation_hm_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = elevation_heightmap_do_work(maps_exporter, max_world_elevation);
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
bool elevation_heightmap_do_work(MapsExporter* maps_exporter, int max_world_elevation)
{
  // Get the data from the queue
  RegionDetailsElevation rde = maps_exporter->pop_elevation_hm();
  // Check if is the marker for no more data from the producer
  if (rde.is_end_marker())
  {
    // All the data has been processed. Finish this thread execution
    return true;
  }

  // Get the map where we'll write to
  ExportedMapBase* elevation_heightmap_map = maps_exporter->get_elevation_hm_map();

  // Iterate over the 16 subtiles (x) and (y) that a world tile has
  for (auto x=0; x<16; ++x)
    for (auto y=0; y<16; ++y)
    {
      // Scale the value relative to the world maximum elevation
      // The value for the maximum world elevation must be 255
      int scaled_elevation = (rde.get_elevation(x,y) * 255) / max_world_elevation;

      RGB_color rgb_pixel_color = RGB_color(scaled_elevation,
                                            scaled_elevation,
                                            scaled_elevation
                                            );

      // Write pixel to the bitmap
      elevation_heightmap_map->write_world_pixel(rde.get_pos_x(),
                                                 rde.get_pos_y(),
                                                 x,
                                                 y,
                                                 rgb_pixel_color
                                                 );

    }
  return false; // Continue working
}

//----------------------------------------------------------------------------//
// Utility function
//
// Iterates over mountain peaks looking for the one with the maximum height
// This height will be the maximum elevation of the complete DF world
//----------------------------------------------------------------------------//
int find_max_world_elevation()
{
  int max_height = -1;
  for (unsigned int i = 0; i < df::global::world->world_data->mountain_peaks.size(); i++)
    if ( max_height <= df::global::world->world_data->mountain_peaks[i]->height)
      max_height = df::global::world->world_data->mountain_peaks[i]->height;

  return max_height;
}
