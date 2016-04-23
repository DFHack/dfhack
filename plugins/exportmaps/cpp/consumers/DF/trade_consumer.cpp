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

using namespace std;

/*****************************************************************************
External functions
*****************************************************************************/
extern int get_biome_type(int world_coord_x,
                          int world_coord_y
                          );

extern pair<int,int> adjust_coordinates_to_region(int x,
                                                  int y,
                                                  int delta,
                                                  int pos_x,
                                                  int pos_y,
                                                  int world_width,
                                                  int world_height
                                                  );


extern df::historical_entity* get_historical_entity_from_world_site(df::world_site* site);

extern df::entity_position_assignment* search_entity_positions_assignments(vector<df::entity_position_assignment* >& vec,
                                                                           int target
                                                                           );

extern df::entity_position* search_entity_positions(vector<df::entity_position* >& vec,
                                                    int entity_id
                                                    );

extern df::world_site* search_world_data_sites(int site_id);

extern df::world_site* search_world_entities(int site_id);

extern void draw_thick_color_line(ExportedMapBase* map,
                                  int x1,
                                  int y1,
                                  int x2,
                                  int y2,
                                  RGB_color& color_center,
                                  RGB_color& color_border
                                  );


extern void draw_site_rectangle(ExportedMapBase* map,
                                df::world_site*  world_site,
                                int              site_population,
                                unsigned char    pixel_R,
                                unsigned char    pixel_G,
                                unsigned char    pixel_B
                                );

extern RGB_color RGB_from_elevation_water(RegionDetailsElevationWater& rdew,
                                          int x,
                                          int y,
                                          int biome_type,
                                          DFHack::BitArray<df::region_map_entry_flags>& flags
                                          );

extern bool process_nob_dip_trad_sites_common(ExportedMapBase* map,
                                              RegionDetailsElevationWater& rdew,
                                              int x,
                                              int y
                                              );

 extern void process_world_structures(ExportedMapBase* map);

/*****************************************************************************
 Local functions forward declaration
*****************************************************************************/

bool trading_do_work(MapsExporter* maps_exporter);

void draw_trade_map(MapsExporter* map_exporter);

void draw_regular_sites(ExportedMapBase* map);

void draw_trading_sites(ExportedMapBase* map);

void draw_regular_site(ExportedMapBase* map,
                       df::world_site* world_site
                       );

void draw_trading_site(ExportedMapBase* map,
                       df::world_site* world_site
                       );

void process_trading(ExportedMapBase*       map,
                     df::historical_entity* entity,
                     df::world_site*        world_site
                     );

void get_no_trading_site_color(int            site_type,
                               unsigned char& pixel_R,
                               unsigned char& pixel_G,
                               unsigned char& pixel_B
                               );

void get_trading_site_color(int            site_type,
                            unsigned char& pixel_R,
                            unsigned char& pixel_G,
                            unsigned char& pixel_B
                            );

void draw_trade_relationship_line(ExportedMapBase* map,
                                  df::world_site*  world_site1,
                                  df::world_site*  world_site2,
                                  int              site_population1,
                                  int              site_population2,
                                  int              type
                                  );

int get_site_total_population(df::world_site* world_site);

/*****************************************************************************
 Module main function.
 This is the function that the thread executes
*****************************************************************************/
void consumer_trading(void* arg)
{
  bool finish = false;
  MapsExporter *maps_exporter = (MapsExporter *) arg;

  if (arg != nullptr)
  {
    while (!finish)
    {
      if (maps_exporter->is_trading_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = trading_do_work(maps_exporter);
    }
  }
  // Function finish -> Thread finish
}



/*****************************************************************************
 Utility function

 Get the data from the queue.
 If it's actual data process it and update the basic terrain map.
 If is the end marker, the queue is empty and the basic terrain map was
 generated, but the real trading map must be draw over the terrain map.
 *****************************************************************************/
bool trading_do_work(MapsExporter* maps_exporter)
{
  // Get the data from the queue
  RegionDetailsElevationWater rdew = maps_exporter->pop_trading();

  // The map where we will write to
  ExportedMapBase* trade_map = maps_exporter->get_trading_map();

  // Check if is the marker for no more data from the producer
  if (rdew.is_end_marker())
  {
    // All the terrain data has been processed.
    process_world_structures(trade_map);

    draw_trade_map(maps_exporter);
    return true;
  }
  else // There's data to be processed
  {
    // Iterate over the 16 subtiles (x) and (y) that a world tile has
    for (auto x=0; x<16; ++x)
      for (auto y=15; y>=0; --y)
        process_nob_dip_trad_sites_common(trade_map,
                                          rdew,
                                          x,
                                          y
                                          );
  }
  return false;
}


//----------------------------------------------------------------------------//
// Utility function
//
// Draws the trade map over the "basic world map" after this one was generated.
//  Basic algorithm:
//
//   1.- Draw a rectangle over each site that could have trading relationship
//   2.- Iterate over world.entites
//   3.- For each entity iterate over its vector<entity_site_link*> site_links
//   4.- For each entity_site_link, test its flags and check if trading applies
//   5.- If yes, get the world site linked as world site 1 using its target field
      // as a site_id
//   6.- Repeat again iterating over vector<entity_site_link*> site_links
//   7.- For each entity_site_link test its flags and for different values
      // than step 4.
//   8.- If the flags are ok, get the world site linked as world site 2
//   9.- Draw a line connecting world site 1 and world site 2
//  10 - Iterate over world.world_data.sites vector
//  11 - Check world site flags for a trading one
//  12 - Draw a rectangle over the world site with different color than the
//       regular ones drawed in step 1
//----------------------------------------------------------------------------//
void draw_trade_map(MapsExporter* map_exporter)
{
  int x = 0;
  int y = 0;

  ExportedMapBase* map = map_exporter->get_trading_map();

  // Draw rectangles over each site
  draw_regular_sites(map);

  for (unsigned int l = 0; l < df::global::world->entities.all.size(); ++l)
  {
    // for each world entity
    df::historical_entity *entity = df::global::world->entities.all[l];

    if (entity == nullptr) continue;

    df::world_site *world_site = nullptr;

    // entity_site_link links world sites to historical entities
    for (unsigned int m = 0; m < entity->site_links.size(); ++m)
    {

      df::entity_site_link *ent_site_link = entity->site_links[m];
      if (ent_site_link == nullptr) continue;

      // This historical entity has relationships with some world sites
      // The relationship that we want
      if (ent_site_link->flags.whole & 0x1) // is residence?
        // TODO Use the bitmask properly
      {
        // Get the world site target id
        int site_id = ent_site_link->target;
        if (site_id == -1) continue;

        // get the df::world_site for this site id
        world_site = search_world_data_sites(site_id);
        if (world_site != nullptr)
        {
          // Now we have one entity that has a trading relationship.
          // We know that the entity trades with a world site
          // but we need another site to define the relation
          process_trading(map,           // Where to draw
                          entity,        // Historical entity that trade
                          world_site);   // First world site that the entity trades to
        }
      }
    }

    // Compute the % of map processing
    float a = l*100;
    float b = df::global::world->entities.all.size();
    map_exporter->set_percentage_trade((int)a/b);
  }

  // Draw sites with trading relationships over the trading lines
  draw_trading_sites(map);

  // Map generated. Warn the main thread
  map_exporter->set_percentage_trade(-1);
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void process_trading(ExportedMapBase*       map,
                     df::historical_entity* entity,
                     df::world_site*        world_site
                     )
{
  // Get this site population
  int site_population = get_site_total_population(world_site);

  // We need another site to define the relationship
  df::world_site *world_site2 = nullptr;

  // Repeat searching entity_site_link vector for this entity
  // looking for a different condition
  for (unsigned int n = 0; n < entity->site_links.size(); ++n)
  {
    df::entity_site_link *ent_site_link = entity->site_links[n];

    if (ent_site_link == nullptr) continue;

    // This historical entity has relationships with some world sites
    // The relationship that we want
    // TODO Use the bitmask properly
    if (ent_site_link->flags.whole & 0x8) // local market (for villages to think about their market town)
    {
      int site_id = ent_site_link->target;
      if (site_id != -1)
      {
        // Get the new world site that has a relationship with the other one
        df::world_site* world_site2 = search_world_data_sites(site_id);

        if (world_site2 != nullptr)
        {
          int site_population2 = get_site_total_population(world_site2);

          // Now we have all the elements that define a trading relationship
          // the entity and two world sites.
          // Draw a line connecting both sites
          draw_trade_relationship_line(map,              // Where to draw
                                       world_site,       // 1st site
                                       world_site2,      // 2nd site
                                       site_population,  // 1st site population
                                       site_population2, // 2nd site population
                                       1);               // Trading relation type #1
        }
      }
    }
    else if (ent_site_link->flags.whole & 0x10)// TODO Use the bitmask properly
    {
      int site_id = ent_site_link->target;
      if (site_id != -1)
      {
        // Get the new world site that has a relationship with the other one
        df::world_site* world_site2 = search_world_data_sites(site_id);

        if (world_site2 != nullptr)
        {
          int site_population2 = get_site_total_population(world_site2);

          // Now we have all the elements that define a trading relationship
          // the entity and two world sites.
          // Draw a line connecting both sites
          draw_trade_relationship_line(map,              // Where to draw
                                       world_site,       // 1st site
                                       world_site2,      // 2nd site
                                       site_population,  // 1st site population
                                       site_population2, // 2nd site population
                                       2);               // Trading relation type #2
        }
      }
    }
  }
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void draw_trading_sites(ExportedMapBase* map)
{
  for (unsigned int t = 0; t < df::global::world->world_data->sites.size(); ++t)
  {
    df::world_site *world_site = df::global::world->world_data->sites[t];

    if (world_site == nullptr) continue;

    // Check flags
    int site_flags_size = world_site->flags.size;
    int site_flags_value = world_site->flags.as_int();

    if (site_flags_value & 0x8)
      if (!(site_flags_value & 0x85))
      {
        // Draw a trading site with different color and shape
        // than the regular ones;
        draw_trading_site(map,
                          world_site);
      }
  }
}

//----------------------------------------------------------------------------//
// Utility function
//
// Draws a rectangle over each world site that can have a trading relationship
//----------------------------------------------------------------------------//
void draw_regular_sites(ExportedMapBase* map)
{
  // Draw over the "basic" map a rectangle over each world site

  for (unsigned int i = 0; i < df::global::world->world_data->sites.size(); ++i)
  {
    df::world_site *world_site = df::global::world->world_data->sites[i];

    if (world_site == nullptr) continue;

    // Check flags
    int site_flags_size = world_site->flags.size;
    int site_flags_value = world_site->flags.as_int();

    if (site_flags_size == 0)
      draw_regular_site(map, world_site);
    else if (!(site_flags_value & 0x08))                            // bit 3
              if (!(site_flags_value & 0x01) &&                     // bit 0
                  !(site_flags_value & 0x80))                       // bit 7
                      if (!(site_flags_value & 0x04))               // bit 2
                            draw_regular_site(map, world_site);



  }
}





//----------------------------------------------------------------------------//
// Utility function
//
// Draws a rectangle over a world site that could participates in
// trading relationship. Depending of the site population, the rectangle
// is drawed with different colors
//----------------------------------------------------------------------------//
void draw_regular_site(ExportedMapBase* map,
                       df::world_site* world_site
                       )
{
  unsigned char pixel_R;
  unsigned char pixel_G;
  unsigned char pixel_B;

  // Get the normal site color to draw
  get_no_trading_site_color(world_site->type,
                            pixel_R,
                            pixel_G,
                            pixel_B);

  // Check the population of this site, because it can be empty
  int site_population = get_site_total_population(world_site);

  // Draw the rectangle with the proper color
  draw_site_rectangle(map,             // where to draw
                      world_site,      // The site to draw
                      site_population, // This changes how the site is drawed
                      pixel_R,         // site color
                      pixel_G,
                      pixel_B);
}

//----------------------------------------------------------------------------//
// Utility function
//
// Draws a rectangle over a world site THAT participates in
// trading relationships. Depending of the site population, the rectangle
// is drawed with different colors
//----------------------------------------------------------------------------//
void draw_trading_site(ExportedMapBase* map,
                       df::world_site* world_site
                       )
{
  unsigned char pixel_R;
  unsigned char pixel_G;
  unsigned char pixel_B;

  // Get the site color to draw
  get_trading_site_color(world_site->type,
                         pixel_R,
                         pixel_G,
                         pixel_B);

  // Check the population of this site, because it can be empty
  int site_population = get_site_total_population(world_site);

  // Draw the rectangle with the proper color
  draw_site_rectangle(map,             // where to draw
                      world_site,      // The site to draw
                      site_population, // This changes how the site is drawed
                      pixel_R,         // site color
                      pixel_G,
                      pixel_B);
}



//----------------------------------------------------------------------------//
// Utility function
//
// Returns the base color for a no trading world site.
// It depends of the site type
//----------------------------------------------------------------------------//
void get_no_trading_site_color(int site_type,
                               unsigned char& pixel_R,
                               unsigned char& pixel_G,
                               unsigned char& pixel_B
                               )
{
  switch (site_type)
  {
    case 0: // Player Fortress (dwarves)
    case 3: // Mountain Halls  (dwarves)
      pixel_R = 0x00;
      pixel_G = 0x80;
      pixel_B = 0xff;
      break;

    case 1: // Dark Fortress (Goblin)
      pixel_R = 0xff;
      pixel_G = 0x00;
      pixel_B = 0xff;
      break;

    case 2: // Cave
    case 7: // Lair Shrine
      pixel_R = 0x32;
      pixel_B = 0x32;
      pixel_G = 0x32;
      break;

    case 4: // Forest Retreat (Elves)
      pixel_R = 0xff;
      pixel_G = 0xff;
      pixel_B = 0x00;
      break;

    case 5: // Town     (Human)
    case 8: // Fortress (Human)
      pixel_R = 0xff;
      pixel_G = 0xff;
      pixel_B = 0xff;
      break;

    case 6: // Important Location
      pixel_R = 0xff;
      pixel_G = 0x00;
      pixel_B = 0x00;
      break;

    case 9: // Camp (Bandits)
      pixel_R = 0xff;
      pixel_G = 0x80;
      pixel_B = 0x00;
      break;

    case 10: // Monument
      pixel_R = 0x80;
      pixel_B = 0x80;
      pixel_G = 0x80;
      break;
  }

  pixel_R >>= 1;
  pixel_G >>= 1;
  pixel_B >>= 1;

}



//----------------------------------------------------------------------------//
// Utility function
//
// Returns the base color for a trading world site.
// It depends of the site type
//----------------------------------------------------------------------------//
void get_trading_site_color(int site_type,
                            unsigned char& pixel_R,
                            unsigned char& pixel_G,
                            unsigned char& pixel_B
                            )
{
  switch (site_type)
  {
    case 0: // Player Fortress (Dwarves)
    case 3: // Mountain Halls  (Dwarves)
      pixel_R = 0x00;
      pixel_G = 0x80;
      pixel_B = 0xff;
      break;

    case 1: // Dark Fortress
      pixel_R = 0xff;
      pixel_G = 0x00;
      pixel_B = 0xff;
      break;

    case 2: // Cave
    case 7: // Lair Shrine
      pixel_R = 0x32;
      pixel_G = 0x32;
      pixel_B = 0x32;
      break;

    case 4: // Forest Retreat (Elves)
      pixel_R = 0xff;
      pixel_G = 0xff;
      pixel_B = 0x00;
      break;

    case 5: // Town     (Human)
    case 8: // Fortress (Human)
      pixel_R = 0xff;
      pixel_G = 0xff;
      pixel_B = 0xff;
      break;

    case 6: // Important Location
      pixel_R = 0xff;
      pixel_G = 0x00;
      pixel_B = 0x00;
      break;

    case 9: // Camp (Bandit)
      pixel_R = 0xff;
      pixel_G = 0x00;
      pixel_B = 0xff;
      break;

    case 10: // Monument
      pixel_R = 0x80;
      pixel_G = 0x80;
      pixel_B = 0x80;
      break;
  };
}




//----------------------------------------------------------------------------//
// Utility function
//
// Draws a line over the map connecting two world sites that have a trading
// relationship
//----------------------------------------------------------------------------//
void draw_trade_relationship_line(ExportedMapBase* map,              // The map where to draw
                                  df::world_site*  world_site1,      // trading site 1
                                  df::world_site*  world_site2,      // trading site 2
                                  int              site_population1, // site 1 population
                                  int              site_population2, // site 2 population
                                  int              type              // relatioship type, changes the line color
                                  )
{

  // The limits of site 1
  int site1_global_min_x = world_site1->global_min_x;
  int site1_global_max_x = world_site1->global_max_x;
  int site1_global_min_y = world_site1->global_min_y;
  int site1_global_max_y = world_site1->global_max_y;

// The limits of site 2
  int site2_global_min_x = world_site2->global_min_x;
  int site2_global_max_x = world_site2->global_max_x;
  int site2_global_min_y = world_site2->global_min_y;
  int site2_global_max_y = world_site2->global_max_y;

  // The "center" of each site
  int site1_center_x = (site1_global_min_x + site1_global_max_x) >> 1;
  int site1_center_y = (site1_global_min_y + site1_global_max_y) >> 1;

  int site2_center_x = (site2_global_min_x + site2_global_max_x) >> 1;
  int site2_center_y = (site2_global_min_y + site2_global_max_y) >> 1;

  int color1 = 0x00;
  int color2 = 0x00;

  RGB_color rgb_color1;
  RGB_color rgb_color2;

  if ((site_population1 > 0) || (site_population2 > 0))
  {
    color1 = 0xff;
    color2 = 0xff;

    if ((site_population1 == 0) || (site_population2 == 0))
    {
      color1 = 0x7f;
      color2 = 0x7f;
    }
  }

  color1 >>= 1;


  if (type == 1)
  {
    std::get<0>(rgb_color1) = 0x00;
    std::get<1>(rgb_color1) = color2;
    std::get<2>(rgb_color1) = 0x00;

    std::get<0>(rgb_color2) = 0x00;
    std::get<1>(rgb_color2) = color1;
    std::get<2>(rgb_color2) = 0x00;
  }
  else if (type == 2)
  {
    std::get<0>(rgb_color1) = color2;
    std::get<1>(rgb_color1) = color2;
    std::get<2>(rgb_color1) = 0x00;

    std::get<0>(rgb_color2) = color1;
    std::get<1>(rgb_color2) = color1;
    std::get<2>(rgb_color2) = 0x00;
  }

  // Draw the line in a buffer
  draw_thick_color_line(map,            // Where to draw
                        site1_center_x, // line start x
                        site1_center_y, // line start y
                        site2_center_x, // line end x
                        site2_center_y, // line end y
                        rgb_color1,     // center line color
                        rgb_color2);    // border line color
}



//----------------------------------------------------------------------------//
// Utility function
//
// Returns the total population of a world site
// Total population = site units + site nemesis + site inhabitants
//----------------------------------------------------------------------------//
int get_site_total_population(df::world_site* world_site)
{
  int v2 = world_site->nemesis.size() + world_site->units.size();
  int v4 = 0;

  if (world_site->inhabitants.size() == 0)
    return v2;

  for (unsigned int i = 0; i < world_site->inhabitants.size(); i++)
  {
    df::world_site_inhabitant* inhabitant = world_site->inhabitants[i];

    if (inhabitant != nullptr)
      v4 += inhabitant->count;
  }

  return v2 + v4;
}
