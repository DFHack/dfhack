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
bool      elevation_do_work(MapsExporter* maps_exporter);

// Return the RGB values for the biome export map given a biome type
RGB_color RGB_from_elevation(int elevation);


/*****************************************************************************
Module main function.
This is the function that the thread executes
*****************************************************************************/
void consumer_elevation(void* arg)
{
  bool                finish  = false;
  MapsExporter* maps_exporter = (MapsExporter*)arg;

  if (arg != nullptr)
  {
    while(!finish)
    {
      if (maps_exporter->is_elevation_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = elevation_do_work(maps_exporter);
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
bool elevation_do_work(MapsExporter* maps_exporter)
{
  // Get the data from the queue
  RegionDetailsElevation rde = maps_exporter->pop_elevation();
  // Check if is the marker for no more data from the producer
  if (rde.is_end_marker())
  {
    // All the data has been processed. Finish this thread execution
    return true;
  }

  // Get the map where we'll write to
  ExportedMapBase* elevation_map = maps_exporter->get_elevation_map();

  // Iterate over the 16 subtiles (x) and (y) that a world tile has
  for (auto x=0; x<16; ++x)
    for (auto y=0; y<16; ++y)
    {
      // Get the RGB values associated to this biome type
      RGB_color rgb_pixel_color = RGB_from_elevation(rde.get_elevation(x,y));

      // Write pixels to the bitmap

      elevation_map->write_world_pixel(rde.get_pos_x(),
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
// Return the RGB values for the elevation export map given a elevation value.
//----------------------------------------------------------------------------//
RGB_color RGB_from_elevation(int elevation)
{
  unsigned char r = -1;
  unsigned char g = -1;
  unsigned char b = -1;

  if (elevation < 99)
  {
    elevation = std::max(elevation,
                         0
                         );
    r = 0;
    g = 0;
    b = elevation;
  }
  else
  {
    // Elevation correction
    elevation -= 25;
    elevation = std::min(elevation,
                         255
                         );
    r = elevation;
    g = elevation;
    b = elevation;
  }

  return RGB_color(r,g,b);
}
