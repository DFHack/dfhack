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

extern RGB_color          no_river_color(int biome_type,
                                         int elevation
                                         );

df::world_construction*   find_construction_square_id_in_world_data_constructions(df::world_data::T_constructions& constructions,
                                                                                  int start,
                                                                                  int end,
                                                                                  int target
                                                                                  );

extern df::world_site_realization::T_areas* search_world_site_realization_areas(int target,
                                                                                df::world_site_realization* wsr
                                                                                );

extern int fill_world_region_details(int world_pos_x,
                                     int world_pos_y
                                     );

extern void delete_world_region_details_vector();

extern void init_world_site_realization(df::world_site* world_site);

extern void delete_world_site_realization(df::world_site* world_site);

RGB_color RGB_from_biome_type(int biome_type);

int get_historical_entity_id_from_world_site(df::world_site* site);


/*****************************************************************************
Local functions forward declaration
*****************************************************************************/
int draw_sites_map(MapsExporter* map_exporter);

bool sites_do_work(MapsExporter* maps_exporter);

void process_nob_dip_trad_common(ExportedMapBase* map,
                                 RegionDetailsElevationWater& rdew,
                                 int x,
                                 int y
                                 );

bool process_nob_dip_trad_sites_common(ExportedMapBase* map,
                                       RegionDetailsElevationWater& rdew,
                                       int x,
                                       int y
                                       );

void process_world_structures(ExportedMapBase* map);

void process_fortress_or_monument(df::world_site* world_site,
                                  ExportedMapBase* map
                                  );

void process_regular_site(df::world_site* world_site,
                          ExportedMapBase* map
                          );

void write_map_pixel(int pos_x,
                     int pos_y,
                     bool is_castle,
                     bool is_village,
                     int area_type,
                     ExportedMapBase* map
                     );

void process_Road(ExportedMapBase* map,
                  df::world_construction* construction
                  );

void process_Tunnel(ExportedMapBase* map,
                    df::world_construction* construction
                    );

void process_Wall(ExportedMapBase* map,
                  df::world_construction* construction
                  );

void process_Bridge(ExportedMapBase* map,
                    df::world_construction* construction
                    );

bool update_inhabitant_count(df::world_site* world_site,
                             df::world_site_inhabitant* inhabitant_in_building
                             );

void delete_site_realization(df::world_site* world_site);

/*****************************************************************************
Module local variables

*****************************************************************************/

// Define some colors to be used in the map
static RGB_color castle_color  (0x80,0x80,0x80);
static RGB_color village_color (0xff,0xff,0xff);
static RGB_color crops1_color  (0xff,0x80,0x00);
static RGB_color crops2_color  (0xff,0xa0,0x00);
static RGB_color crops3_color  (0xff,0xc0,0x00);
static RGB_color pasture_color (0x40,0xff,0x00);
static RGB_color meadows_color (0x00,0xff,0x00);
static RGB_color woodland_color(0x00,0xa0,0x00);
static RGB_color orchard_color (0x00,0x80,0x00);
static RGB_color waste_color   (0x64,0x28,0x0f);

/*****************************************************************************
Module main function.
This is the function that the thread executes
*****************************************************************************/
void consumer_sites(void* arg)
{
  bool             finish        = false;
  MapsExporter*    maps_exporter = (MapsExporter*)arg;

  if (arg != nullptr)
  {
    while(!finish)
    {
      if (maps_exporter->is_sites_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = sites_do_work(maps_exporter);
    }
  }
  // Function finish -> Thread finish
}

/*****************************************************************************
Utility function

Get the data from the queue.
If is the end marker, the queue is empty and no more work needs to be done.Return
If it's actual data process it and update the corresponding map
*****************************************************************************/
bool sites_do_work(MapsExporter*    maps_exporter)
{
  // Get the data from the queue
  RegionDetailsElevationWater rdew = maps_exporter->pop_sites();
  // The map where we will write to
  ExportedMapBase* sites_map = maps_exporter->get_sites_map();

  // Check if is the marker for no more data from the producer
  if (rdew.is_end_marker())
  {
    // All the terrain data has been processed.
    // Now draw world sites over this base map
    process_world_structures(sites_map);
    draw_sites_map(maps_exporter);

    return true;
  }
  else
  {
    // There's data to be processed
    // Iterate over the 16 subtiles (x) and (y) that a world tile has
    for (auto x=0; x<16; ++x)
      for (auto y=0; y<16; ++y)
        process_nob_dip_trad_sites_common(sites_map,
                                          rdew,
                                          x,
                                          y
                                          );
  }
  return false; // Continue working
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
bool process_nob_dip_trad_sites_common(ExportedMapBase*             map,
                                       RegionDetailsElevationWater& rdew,
                                       int                          x,
                                       int                          y
                                       )
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


  int elevation                      = rdew.get_elevation(x,y);
  int river_horiz_y_min              = rdew.get_rivers_horizontal().y_min[x][y  ];
  int river_horiz_y_min_plus_one_row = rdew.get_rivers_horizontal().y_min[x+1][y];
  int river_vert_x_min               = rdew.get_rivers_vertical().x_min[x  ][y];
  int river_vert_x_min_plus_one_col  = rdew.get_rivers_vertical().x_min[x][y+1];

  if ((river_horiz_y_min == -30000) && (river_horiz_y_min_plus_one_row == -30000))
      if ((river_vert_x_min == -30000) && (river_vert_x_min_plus_one_col == -30000))
      {
        RGB_color pixel_color = no_river_color(biome_type, rdew.get_elevation(x,y));
        map->write_world_pixel(rdew.get_pos_x(),
                               rdew.get_pos_y(),
                               x,
                               y,
                               pixel_color
                               );

        return false;
      }


  switch (biome_type)
  {
    case  1: // Glacier
    case  2: // Tundra
    case 27: // Ocean Tropical
    case 28: // Ocean Temperate
    case 29: // Ocean Artic
    case 36: // Lake Temperate Fresh    Water
    case 37: // Lake Temperate Brackish Water
    case 38: // Lake Temperate Salt     Water
    case 39: // Lake Tropical  Fresh    Water
    case 40: // Lake Tropical  Brackish Water
    case 41: // Lake Tropical  Salt     Water
              if ((biome_type < 27) ||
                  (biome_type > 29) ||
                  (elevation  < 99))
              {
                RGB_color pixel_color = no_river_color(biome_type,
                                                       rdew.get_elevation(x,y)
                                                       );
                map->write_world_pixel(rdew.get_pos_x(),
                                       rdew.get_pos_y(),
                                       x,
                                       y,
                                       pixel_color
                                       );
                return false;
              }
             break;
    default: break;
  }

  auto flags = df::global::world->world_data->region_map[rdew.get_pos_x()][rdew.get_pos_y()].flags;
  bool brook_flag = flags.is_set(df::enums::region_map_entry_flags::region_map_entry_flags::is_brook);
  int  v40 = __OFSUB__(flags.size, 1);
  int  v39 = flags.size == 1;
  int  v12 = (flags.size - 1) < 0;

  if (((v12 ^ v40) | v39) || !brook_flag)
  {
    // River
    RGB_color river_pixel_color(0x00, 0xc0, 0xff);
    map->write_world_pixel(rdew.get_pos_x(),
                           rdew.get_pos_y(),
                           x,
                           y,
                           river_pixel_color
                           );
    return false;
  }

  RGB_color pixel_color = no_river_color(biome_type, rdew.get_elevation(x,y));
  map->write_world_pixel(rdew.get_pos_x(),
                         rdew.get_pos_y(),
                         x,
                         y,
                         pixel_color
                         );

  return false;

}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int draw_sites_map(MapsExporter* map_exporter)
{
  ExportedMapBase* map = map_exporter->get_sites_map();
  int result = 0;
  for (int i = df::global::world->world_data->sites.size() - 1; i >= 0; i-- )
  {
    df::world_site* world_site = df::global::world->world_data->sites[i];
    if (world_site == nullptr) continue;

    // Check if the site has a previous realization active
    bool site_has_realization = world_site->realization != nullptr;

    // Do DF initalize the site realization as this is a VERY complex task
    if (!site_has_realization)
      init_world_site_realization(world_site);

    // Get the new/updated site realization after DF work
    df::world_site_realization* site_realization = world_site->realization;
    if (site_realization != nullptr)
    {
      // Check site realization initialization
      if (site_realization->anon_1 & 1) // bit 0 ON means site unitialized
      {
        //process_world_site_realization(world_site);
        site_realization->anon_1 &= 0xFFFFFFFE; // reset bit 0 -> site initialized
      }

      if ((world_site->type == df::enums::world_site_type::world_site_type::Fortress) ||
          (world_site->type == df::enums::world_site_type::world_site_type::Monument))
      {
        process_fortress_or_monument(world_site,
                                     map
                                     );
      }
      else
      {
        process_regular_site(world_site,
                             map
                             );
      }

      if (!site_has_realization && (world_site->realization != nullptr))
        delete_world_site_realization(world_site);
    }

    delete_world_region_details_vector();

    // Compute the % of map processing
    float a = i*100;
    float b = df::global::world->world_data->sites.size();
    map_exporter->set_percentage_sites(100-(int)(a/b));
  }
  // Map generated. Warn the main thread
  map_exporter->set_percentage_sites(-1);

  return result;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void process_regular_site(df::world_site*  world_site,
                          ExportedMapBase* map
                          )
{
  // Not fortress nor monument

  if (world_site == nullptr) return;

  // Castle map. Size = 0x121 bytes = 17 x 17 array
  char castle_map[17][17];
  for (int i = 0; i < 17; i++)
    for (int j = 0; j < 17; j++)
      castle_map[i][j] = 0;

  df::world_site_realization* site_realization = world_site->realization;
  if (site_realization == nullptr) return;

  // Process the castle map
  for (unsigned int l = 0; l < site_realization->buildings.size(); l++)
  {
    df::site_realization_building* site_realiz_building = site_realization->buildings[l];
    if (site_realiz_building == nullptr) continue;

    if ((site_realiz_building->type == 1) || // castle wall
        (site_realiz_building->type == 2) || // castle tower
        (site_realiz_building->type == 3))   // castle courtyard
    {
      int cx = site_realiz_building->min_x/48;
      int cy = site_realiz_building->min_y/48;
      castle_map[cx][cy] = 1;
    }
  }


  // Now iterate over the site from [global_min_x,global_min_y] to [global_max_x,global_max_y]
  for (int pos_x_iterator = world_site->global_min_x; pos_x_iterator <= world_site->global_max_x; pos_x_iterator++)
  {
    int building_map_x_iterator = (pos_x_iterator - world_site->global_min_x);

    for (int pos_y_iterator = world_site->global_min_y; pos_y_iterator <= world_site->global_max_y; pos_y_iterator++)
    {
      int building_map_y_iterator = (pos_y_iterator - world_site->global_min_y);

      // Check that we have already filled the region details for this world position
      df::world_region_details* region_details = nullptr;

      // Locate the region details that contains our position [x,y]
      for (unsigned int p = 0; p < df::global::world->world_data->region_details.size(); p++)
      {
        region_details = df::global::world->world_data->region_details[p];
        if (region_details == nullptr) continue;

        if ((region_details->pos.x == (pos_x_iterator >> 4)) &&
            (region_details->pos.y == (pos_y_iterator >> 4)))
          break;
      }

      if (region_details == nullptr) continue;


      bool is_village = false;
      bool is_castle = castle_map[building_map_x_iterator][building_map_y_iterator] != 0;

      // A world site has a graphic realization of 816x816 pixels
      // 816/48 = 17 which is the size of the building_map array
      // so each entry in the building map covers one area of 48x48 pixels.
      // This way we avoid iterate over ALL the site buildings and process only a small subset

      // iterate over the buildings that are built in this area to determine
      // if is a castle or a town building and update the flags.
      for (unsigned int q = 0;
           q < world_site->realization->building_map[building_map_x_iterator][building_map_y_iterator].buildings.size();
           q++)
      {
        df::site_realization_building* building = world_site->realization->building_map[building_map_x_iterator][building_map_y_iterator].buildings[q];
        switch (building->type)
        {
          case  1:                        // castle wall
          case  2:                        // castle tower
          case  3:                        // castle courtyard
                    is_castle = true;
                    break;
          case  0:                        // cottage_plot
          case  4:                        // house
          case  5:                        // temple
          case  6:                        // tomb
          case  7:                        // shop_house
          case  8:                        // warehouse
          case  9:                        // market_square

          case 13:                        // well
          case 14:                        // vault
          case 15:                        // great_tower
          case 16:                        // trenches
          case 17:                        // tree_house
          case 18:                        // hillock_house
          case 19:                        // mead_hall

          case 21:                        // Tavern
          case 22:                        // Library
                     is_village = true;
                     break;
          default:   continue;
        }
      }

      // Similar to the building map, the area map contains the T_areas for a specifice region of the bitmap
      // T_areas are of 16x16 pixels size. 816/16 = 51, but Toady used instead, a [52]x[52] array

      // The to be located area type
      int area_type = -1;

      if (world_site->realization->areas.size() > 0)
        for (int i = 0; i < 4 ; i++)
        {
          int area_map_start_x = (pos_x_iterator - world_site->global_min_x) * 3;

          if (area_map_start_x + i < 52)
            area_map_start_x += i;

          for (int j = 0; j < 4 ; j++)
          {
            int area_map_start_y = (pos_y_iterator - world_site->global_min_y) * 3;

            if (area_map_start_y + j < 52)
              area_map_start_y += j;

            // Locate the area index in the map
            int area_map_index = world_site->realization->area_map[area_map_start_x][area_map_start_y];
            if (area_map_index != -1)
            {
              // Locate the T_area using its id
              df::world_site_realization::T_areas* area = search_world_site_realization_areas(area_map_index,
                                                                                              world_site->realization
                                                                                              );

              if (area != nullptr)
                area_type = area->type; // area located, get the its type
            }
          }
        }

      // Write the pixel in the map, accdording to its type
      write_map_pixel(pos_x_iterator,
                      pos_y_iterator,
                      is_castle,
                      is_village,
                      area_type,
                      map
                      );

    }
  }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void process_fortress_or_monument(df::world_site*  world_site,
                                  ExportedMapBase* map
                                  )
{
  if (world_site == nullptr) return;

  // Define some colors
  RGB_color color_fortress(0x80,0x80,0x80);
  RGB_color color_monument(0x81,0x81,0x81);

  // The limits of the site in embark coordinates
  int global_min_x = world_site->global_min_x;
  int global_min_y = world_site->global_min_y;
  int global_max_x = world_site->global_max_x;
  int global_max_y = world_site->global_max_y;

  // Paranoia check
  if (global_min_x > global_max_x) return;
  if (global_min_y > global_max_y) return;

  // TODO optimize this code as is not efficient

  for (int pos_x_iterator = global_min_x; pos_x_iterator <= global_max_x; pos_x_iterator++)
    for (int pos_y_iterator = global_min_y; pos_y_iterator <= global_max_y; pos_y_iterator++)
      // Locate the region details for this position
      for (unsigned int i = 0; i < df::global::world->world_data->region_details.size(); i++)
      {
        df::world_region_details* reg_details = df::global::world->world_data->region_details[i];
        if (reg_details == nullptr) continue;

        // region_details pos is in world coordinates
        if ((reg_details->pos.x == pos_x_iterator/16) && (reg_details->pos.y == pos_y_iterator/16))
          // write pixel in the map according to its type: fortress or mounument
          (world_site->type == df::enums::world_site_type::world_site_type::Fortress ?
                               map->write_embark_pixel(pos_x_iterator,pos_y_iterator,color_fortress) :
                               map->write_embark_pixel(pos_x_iterator,pos_y_iterator,color_monument));
      }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void write_map_pixel(int              pos_x,
                     int              pos_y,
                     bool             is_castle,
                     bool             is_village,
                     int              area_type,
                     ExportedMapBase* map
                     )
{
  // According to the building type, draw it in a different color
  if (is_castle)
    map->write_embark_pixel(pos_x,
                            pos_y,
                            castle_color
                            );
  else if (is_village)
    map->write_embark_pixel(pos_x,
                            pos_y,
                            village_color
                            );
  else // Draw the site neighborhood
  {
    switch (area_type)
    {
      case 0:   // Crops1
                map->write_embark_pixel(pos_x,
                                        pos_y,
                                        crops1_color
                                        );
                break;

      case 1:   // Crops2
                map->write_embark_pixel(pos_x,
                                        pos_y,
                                        crops2_color
                                        );
                break;

      case 2:   // Crops3
                map->write_embark_pixel(pos_x,
                                        pos_y,
                                        crops3_color
                                        );
                break;

      case 3:   // Pasture
                map->write_embark_pixel(pos_x,
                                        pos_y,
                                        pasture_color
                                        );
                break;

      case 4:   // Meadows
                map->write_embark_pixel(pos_x,
                                        pos_y,
                                        meadows_color
                                        );
                break;

      case 5:   // Woodland
                map->write_embark_pixel(pos_x,
                                        pos_y,
                                        woodland_color
                                        );
                break;

      case 6:   // Orchard
                map->write_embark_pixel(pos_x,
                                        pos_y,
                                        orchard_color
                                        );
                break;
      case 7:   // Waste
                map->write_embark_pixel(pos_x,
                                        pos_y,
                                        waste_color
                                        );
                break;

      default:  break;
    }
  }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void process_world_structures(ExportedMapBase* map)
{
  for (unsigned int i = 0; i < df::global::world->world_data->constructions.list.size(); i++)
  {
    df::world_construction* construction = df::global::world->world_data->constructions.list[i];
    if (construction == nullptr) continue;

    switch (construction->getType())
    {
      case 0: // Road
              process_Road(map,
                           construction
                           );
              break;
      case 1: // Tunnel
              process_Tunnel(map,
                             construction
                             );
              break;
      case 3: // Wall
              process_Wall(map,
                           construction
                           );
              break;
      case 2: // Bridge
              process_Bridge(map,
                             construction
                             );
              break;
    default:  break;
    }
  }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void process_Road(ExportedMapBase* map,
                  df::world_construction* construction
                  )
{
  for (unsigned int j = 0; j < construction->square_obj.size(); j++)
  {
    df::world_construction_square* constr_square = construction->square_obj[j];
    if (constr_square == nullptr) continue;

    int pos_x = constr_square->region_pos.x;
    int pos_y = constr_square->region_pos.y;

    for (unsigned int k = 0; k < constr_square->embark_x.size(); k++)
    {
      int embark_x = constr_square->embark_x[k];
      int embark_y = constr_square->embark_y[k];

      unsigned char pixel_R = 0;
      unsigned char pixel_G = 0;
      unsigned char pixel_B = 0;

      // cast to the appropiate subclass
      df::world_construction_square_roadst* road = static_cast<df::world_construction_square_roadst*>(constr_square);
      if (road == nullptr) continue;

      if ((road->item_type == -1) || (road->mat_type != 0))
      {
        // Other road
        pixel_R = 0x96;
        pixel_G = 0x7f;
        pixel_B = 0x14;
      }
      else
      {
        // Stone road
        pixel_R = 0xc0;
        pixel_G = 0xc0;
        pixel_B = 0xc0;
      }
      if ((pixel_R == 0) && (pixel_G == 0) && (pixel_B == 0))
        continue;
      RGB_color pixel_color(pixel_R,pixel_G,pixel_B);
      map->write_world_pixel(pos_x,
                             pos_y,
                             embark_x,
                             embark_y,
                             pixel_color
                             );
    }
  }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void process_Tunnel(ExportedMapBase* map,
                    df::world_construction* construction
                    )
{
  for (unsigned int j = 0; j < construction->square_obj.size(); j++)
  {
    df::world_construction_square* constr_square = construction->square_obj[j];
    if (constr_square == nullptr) continue;

    int pos_x = constr_square->region_pos.x;
    int pos_y = constr_square->region_pos.y;

    for (unsigned int k = 0; k < constr_square->embark_x.size(); k++)
    {
      int embark_x = constr_square->embark_x[k];
      int embark_y = constr_square->embark_y[k];

      unsigned char pixel_R = 0x14;
      unsigned char pixel_G = 0x14;
      unsigned char pixel_B = 0x14;

      RGB_color pixel_color(pixel_R,pixel_G,pixel_B);
      map->write_world_pixel(pos_x,
                             pos_y,
                             embark_x,
                             embark_y,
                             pixel_color
                             );


    }
  }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void process_Wall(ExportedMapBase* map,
                  df::world_construction* construction
                  )
{
  for (unsigned int j = 0; j < construction->square_obj.size(); j++)
  {
    df::world_construction_square* constr_square = construction->square_obj[j];
    if (constr_square == nullptr) continue;

    int pos_x = constr_square->region_pos.x;
    int pos_y = constr_square->region_pos.y;

    for (unsigned int k = 0; k < constr_square->embark_x.size(); k++)
    {
      int embark_x = constr_square->embark_x[k];
      int embark_y = constr_square->embark_y[k];

      unsigned char pixel_R = 0;
      unsigned char pixel_G = 0;
      unsigned char pixel_B = 0;

      df::world_construction_square_wallst* wall = static_cast<df::world_construction_square_wallst*>(constr_square);
      if (wall == nullptr) continue;
      if ((wall->item_type == -1) || (wall->mat_type != 0))
      {
        // Other wall
        pixel_R = 0xa0;
        pixel_G = 0x7f;
        pixel_B = 0x14;
      }
      else
      {
        // Stone wall
        pixel_R = 0x60;
        pixel_G = 0x60;
        pixel_B = 0x60;
      }

      if ((pixel_R == 0) && (pixel_G == 0) && (pixel_B == 0))
        continue;

      RGB_color pixel_color(pixel_R,pixel_G,pixel_B);
      map->write_world_pixel(pos_x,
                             pos_y,
                             embark_x,
                             embark_y,
                             pixel_color
                             );
    }
  }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void process_Bridge(ExportedMapBase* map,
                    df::world_construction* construction
                    )
{
  int pos_x = construction->square_pos.x[0];
  int pos_y = construction->square_pos.y[0];
  std::vector<df::world_construction_square* >& vec_constr = df::global::world->world_data->constructions.map[pos_x >> 4][pos_y >> 4];
  for (unsigned int i = 0; i < vec_constr.size(); i++ )
  {
    df::world_construction_square* constr_square = vec_constr[i];
    if (constr_square == nullptr) continue;
    if (constr_square->construction_id != construction->id) continue;

    if ((constr_square->region_pos.x != pos_x) || (constr_square->region_pos.y != pos_y))
      continue;

    for (unsigned int k = 0; k < constr_square->embark_x.size(); k++)
    {
      int embark_x = constr_square->embark_x[k];
      int embark_y = constr_square->embark_y[k];

      unsigned char pixel_R = 0;
      unsigned char pixel_G = 0;
      unsigned char pixel_B = 0;

      df::world_construction_square_bridgest* bridge = static_cast<df::world_construction_square_bridgest*>(constr_square);
      if (bridge == nullptr) continue;

      if (bridge->item_type != 0)
      {
        // Other bridge
        pixel_R = 0xB4;
        pixel_G = 0xA7;
        pixel_B = 0x14;
      }
      else
      {
        // Stone bridge
        pixel_R = 0xe0;
        pixel_G = 0xe0;
        pixel_B = 0xe0;
      }

      if ((pixel_R == 0) && (pixel_G == 0) && (pixel_B == 0))
        continue;

      RGB_color pixel_color(pixel_R,pixel_G,pixel_B);
      map->write_world_pixel(pos_x,
                             pos_y,
                             embark_x,
                             embark_y,
                             pixel_color
                             );
    }
    return;
  }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void delete_site_realization(df::world_site* world_site)
{
  if (world_site->realization == nullptr) return;

  int historical_entity_id = get_historical_entity_id_from_world_site(world_site);
  for (int i = world_site->realization->buildings.size() - 1; i >= 0 ; i--)
  {
    df::site_realization_building* site_building = world_site->realization->buildings[i];
    if (site_building == nullptr) continue;

    df::world_site_inhabitant* inhabitant_in_building = nullptr;
    for (unsigned int j = 0; j < site_building->inhabitants.size(); j++)
    {
      df::world_site_inhabitant* site_building_inhabitant = site_building->inhabitants[j];
      if (site_building_inhabitant == nullptr) continue;

      if (site_building_inhabitant->count > 0)
      {
        inhabitant_in_building = site_building_inhabitant;
        break;
      }
    }

    if (inhabitant_in_building == nullptr) continue;
    update_inhabitant_count(world_site,
                            inhabitant_in_building
                            );



    if (world_site->animals.size() > 0)
    {
      bool found_animal = false;
      for (unsigned int k = 0; k < world_site->animals.size(); k++)
      {
        df::world_population* animal_population = world_site->animals[k];
        if (animal_population == nullptr) continue;

        if ((historical_entity_id           == animal_population->owner)  &&
            (inhabitant_in_building->race   == animal_population->race)   &&
            (inhabitant_in_building->unk_10 == animal_population->unk_10) &&
            (animal_population->type == 0)) // Animal
        {
          found_animal = true;
          if (animal_population->count_min != 10000001)
            animal_population->count_min += inhabitant_in_building->count;
        }
      }

      if (!found_animal)
      {
        // This should not happen
        // If happens, a new empty inhabitant must be created
        //exit(24);
      }
    }
  }


/*
  if (world_site->realization->unk_wsr_vector.size() > 0)
  {
    for (int i = 0; i < world_site->realization->unk_wsr_vector.size(); i++)
    {
      df::world_site_realization::T_unk_wsr_vector* unk_ptr = world_site->realization->unk_wsr_vector[i];
      if (unk_ptr == nullptr) continue;

      unsigned int inhabitant_count_update = 0;
      for (unsigned int j = 0; j < unk_ptr->inhabitants.size(); j++)
      {
        df::world_site_inhabitant* inhabitant_in_building = unk_ptr->inhabitants[j];
        if (inhabitant_in_building != nullptr)
          if (inhabitant_in_building->count > 0)
          {
            inhabitant_count_update = inhabitant_in_building->count;
            update_inhabitant_count(world_site,
                                    inhabitant_in_building);
          }
      }
    }
  }
*/
  delete(world_site->realization);
  world_site->realization = nullptr;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
bool update_inhabitant_count(df::world_site* world_site,
                             df::world_site_inhabitant* inhabitant_in_building
                             )
{
  bool found_world_inhabitant = false;
  if ((inhabitant_in_building->race != -1) &&
      (inhabitant_in_building->race) < (signed int)df::global::world->raws.creatures.all.size())
  {
    df::creature_raw* creature_raw = df::global::world->raws.creatures.all[inhabitant_in_building->race];
    if (creature_raw != nullptr)
      if (creature_raw->flags.size > 8)
      {
        // TODO convert this to the appropiate flag
        if (*(creature_raw->flags.bits+8) & 0x80)
        {
          if ((creature_raw->flags.size <= 14) ||
              !(*(creature_raw->flags.bits+14) & 0x10))
          {
            if (world_site->inhabitants.size() > 0)
            {
              for (unsigned int k = 0; k < world_site->inhabitants.size(); k++)
              {
                df::world_site_inhabitant* site_inhabitant = world_site->inhabitants[k];
                if (site_inhabitant == nullptr) continue;

                if ((inhabitant_in_building->race == site_inhabitant->race) &&
                    (inhabitant_in_building->unk_8 == site_inhabitant->unk_8))
                {
                  if (((inhabitant_in_building->outcast_id == site_inhabitant->outcast_id) ||
                       ((inhabitant_in_building->outcast_id == -1) && (site_inhabitant->outcast_id == -1))) &&
                      (inhabitant_in_building->unk_10 == site_inhabitant->unk_10) &&
                      (inhabitant_in_building->unk_14 == site_inhabitant->unk_14) &&
                      (inhabitant_in_building->unk_18 == site_inhabitant->unk_18) &&
                      (inhabitant_in_building->unk_1c == site_inhabitant->unk_1c) &&
                      (inhabitant_in_building->unk_20 == site_inhabitant->unk_20) &&
                      (inhabitant_in_building->unk_24 == site_inhabitant->unk_24))
                  {
                    // Update the count
                    site_inhabitant->count += inhabitant_in_building->count;
                    found_world_inhabitant = true;
                  }
                }
              }

              if (!found_world_inhabitant)
              {
                // This should not happen
                // If happens, a new empty inhabitant must be created
                //exit(23);
              }
            }
          }
        }
      }
  }
  return found_world_inhabitant;
}
