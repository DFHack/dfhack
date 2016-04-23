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
RGB_color RGB_from_biome_type(int biome_type);

bool      biome_do_work(MapsExporter* maps_exporter);


/*****************************************************************************
Module main function.
This is the function that the thread executes
*****************************************************************************/
void consumer_biome(void* arg)
{
  bool             finish        = false;
  MapsExporter*    maps_exporter = (MapsExporter*)arg;

  if (arg != nullptr)
  {
    // The map where we will write to
    ExportedMapBase* biome_map = maps_exporter->get_biome_map();

    while(!finish)
    {
      if (maps_exporter->is_biome_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = biome_do_work(maps_exporter);
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
bool biome_do_work(MapsExporter* maps_exporter)
{
  // Get the data from the queue
  RegionDetailsBiome rdb = maps_exporter->pop_biome();

  // Check if is the marker for no more data from the producer
  if (rdb.is_end_marker())
    // All the data has been processed. Done
    return true;

  // Get the data where we'll write to
  ExportedMapBase* map = maps_exporter->get_biome_map();

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

      // Get the RGB values associated to this biome type
      RGB_color rgb_pixel_color = RGB_from_biome_type(biome_type);

      // Write pixels to the bitmap
      map->write_world_pixel(rdb.get_pos_x(),
                             rdb.get_pos_y(),
                             x,
                             y,
                             rgb_pixel_color
                             );
  }
  return false; // Continue working
}


//----------------------------------------------------------------------------//
//Utility function
//Return the RGB values for the biome export map given a biome type.
//It mimics the content of the file biome_color_key.txt generated automatically
//by DF when a biome export map is selected in Legends mode.
//Reverse engineered from DF code.
//----------------------------------------------------------------------------//
RGB_color RGB_from_biome_type(int biome_type)
{
  unsigned char r = -1;
  unsigned char g = -1;
  unsigned char b = -1;

  // The following biome types are not processed
  // POOL_TEMPERATE_FRESHWATER
  // POOL_TEMPERATE_BRACKISHWATER
  // POOL_TEMPERATE_SALTWATER
  // POOL_TROPICAL_FRESHWATER
  // POOL_TROPICAL_BRACKISHWATER
  // POOL_TROPICAL_SALTWATER
  // SUBTERRANEAN_WATER
  // SUBTERRANEAN_CHASM
  // SUBTERRANEAN_LAVA

  switch (biome_type)
  {
    case 0:     // biome type MOUNTAIN
                r = 0x80; g = 0x80; b = 0x80; break;

    case 1:     // biome type GLACIER
                r = 0x40; g = 0xff; b = 0xff; break;

    case 2:     // biome type TUNDRA
                r = 0x80; g = 0xff; b = 0xff; break;

    case 3:     // biome type SWAMP_TEMPERATE_FRESHWATER
                r = 0x60; g = 0xc0; b = 0x80; break;

    case 4:     // biome type SWAMP_TEMPERATE_SALTWATER
                r = 0x40; g = 0xc0; b = 0x80; break;

    case 5:     // biome type MARSH_TEMPERATE_FRESWATER
                r = 0x60; g = 0xff; b = 0x80; break;

    case 6:     // biome type MARSH_TEMPERATE_SALTWATER
                r = 0x40; g = 0xff; b = 0x80; break;

    case 7:     // biome type SWAMP_TROPICAL_FRESHWATER
                r = 0x60; g = 0xc0; b = 0x40; break;

    case 8:     // biome type SWAMP_TROPICAL_SALTWATER
                r = 0x40; g = 0xc0; b = 0x40; break;

    case 9:     // biome type SWAMP_MANGROVE
                r = 0x40; g = 0xff; b = 0x60; break;

    case 10:    // biome type MARSH_TROPICAL_FRESHWATER
                r = 0x60; g = 0xff; b = 0x40; break;

    case 11:    // biome type MARSH_TROPICAL_SALTWATER
                r = 0x40; g = 0xff; b = 0x40; break;

    case 12:    // FOREST_TAIGA
                r = 0x00; g = 0x60; b = 0x40; break;

    case 13:    // FOREST_TEMPERATE_CONIFER
                r = 0x00; g = 0x60; b = 0x20; break;

    case 14:    // FOREST_TEMPERATE_BROADLEAF
                r = 0x00; g = 0xa0; b = 0x20; break;

    case 15:    // FOREST_TROPICAL_CONIFER
                r = 0x00; g = 0x60; b = 0x00; break;

    case 16:    // FOREST_TROPICAL_DRY_BROADLEAF
                r = 0x00; g = 0x80; b = 0x00; break;

    case 17:    // FOREST_TROPICAL_MOIST_BROADLEAF
                r = 0x00; g = 0xa0; b = 0x00; break;

    case 18:    // GRASSLAND_TEMPERATE
                r = 0x00; g = 0xff; b = 0x20; break;

    case 19:    // SAVANNA_TEMPERATE
                r = 0x00; g = 0xe0; b = 0x20; break;

    case 20:    // SHRUBLAND_TEMPERATE
                r = 0x00; g = 0xc0; b = 0x20; break;

    case 21:    // GRASSLAND_TROPICAL
                r = 0xff; g = 0xa0; b = 0x00; break;

    case 22:    // SAVANNA_TROPICAL
                r = 0xff; g = 0xb0; b = 0x00; break;

    case 23:    // SHRUBLAND_TROPICAL
                r = 0xff; g = 0xc0; b = 0x00; break;

    case 24:    // DESERT_BADLAND
                r = 0xff; g = 0x60; b = 0x20; break;

    case 25:    // DESERT_ROCK
                r = 0xff; g = 0x80; b = 0x40; break;

    case 26:    // DESERT_SAND
                r = 0xff; g = 0xff; b = 0x00; break;

    case 27:    // OCEAN_TROPICAL
                r = 0x00; g = 0x00; b = 0xff; break;

    case 28:    // OCEAN_TEMPERATE
                r = 0x00; g = 0x80; b = 0xff; break;

    case 29:    // OCEAN_ARCTIC
                r = 0x00; g = 0xff; b = 0xff; break;

    case 36:    // LAKE_TEMPERATE_FRESHWATER
                r = 0x00; g = 0xe0; b = 0xff; break;

    case 37:    // LAKE_TEMPERATE_BRACKISH_WATER
                r = 0x00; g = 0xc0; b = 0xff; break;

    case 38:    // LAKE_TEMPERATE_SALTWATER
                r = 0x00; g = 0xa0; b = 0xff; break;

    case 39:    // LAKE_TROPICAL_FRESHWATER
                r = 0x00; g = 0x60; b = 0xff; break;

    case 40:    // LAKE_TROPICAL_BRACKISH_WATER
                r = 0x00; g = 0x40; b = 0xff; break;

    case 41:    // LAKE_TROPICAL_SALT_WATER
                r = 0x00; g = 0x20; b = 0xff; break;

    default:    break;
  }

  return RGB_color(r,g,b);
}
