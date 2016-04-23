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
extern int                             get_biome_type(int world_coord_x,
                                                      int world_coord_y
                                                      );

extern pair<int,int>                   adjust_coordinates_to_region(int x,
                                                                    int y,
                                                                    int delta,
                                                                    int pos_x,
                                                                    int pos_y,
                                                                    int world_width,
                                                                    int world_height
                                                                    );

extern df::historical_entity*          get_historical_entity_from_world_site(df::world_site* site);

extern int                             get_historical_entity_id_from_world_site(df::world_site* site);

extern df::entity_position_assignment* search_entity_positions_assignments(vector<df::entity_position_assignment* >& vec,
                                                                           int target
                                                                           );

extern df::entity_position*            search_entity_positions_own(vector<df::entity_position* >& vec,
                                                                   int target
                                                                   );


extern df::entity_position*            search_entity_positions(vector<df::entity_position* >& vec,
                                                               int entity_id
                                                               );

extern df::world_site*                 search_world_data_sites(int site_id);

extern df::historical_entity*          search_world_entities(int site_id);

extern df::entity_site_link*           find_entity_site_link_from_world_site_id(vector<df::entity_site_link* >& vec,
                                                                                int target
                                                                                );

extern vector<df::coord2d>             line_algorithm(int point1_center_x,
                                                      int point1_center_y,
                                                      int point2_center_x,
                                                      int point2_center_y
                                                      );

extern void                            draw_site_rectangle(ExportedMapBase* map,
                                                           df::world_site*  world_site,
                                                           int              site_population,
                                                           unsigned char    pixel_R,
                                                           unsigned char    pixel_G,
                                                           unsigned char    pixel_B
                                                           );


extern void                            draw_site_rectangle_offseted(ExportedMapBase* map,
                                                                    df::world_site*  world_site,
                                                                    unsigned char    pixel_R,
                                                                    unsigned char    pixel_G,
                                                                    unsigned char    pixel_B,
                                                                    int              offset
                                                                    );


extern void                            draw_site_rectangle_filled_offseted(ExportedMapBase* map,
                                                                           df::world_site*  world_site,
                                                                           unsigned char    pixel_R,
                                                                           unsigned char    pixel_G,
                                                                           unsigned char    pixel_B,
                                                                           int              offset
                                                                           );

extern void                            draw_thick_color_line(ExportedMapBase* map,
                                                             int              x1,
                                                             int              y1,
                                                             int              x2,
                                                             int              y2,
                                                             RGB_color&       color_center,
                                                             RGB_color&       color_border
                                                             );


extern int                             get_site_total_population(df::world_site* world_site);

extern RGB_color                       RGB_from_elevation_water(RegionDetailsElevationWater& rdew,
                                                                int x,
                                                                int y,
                                                                int biome_type,
                                                                DFHack::BitArray<df::region_map_entry_flags>& flags
                                                                );

extern df::historical_entity*          f(df::historical_entity* entity);

extern bool                            process_nob_dip_trad_sites_common(ExportedMapBase* map,
                                                                         RegionDetailsElevationWater& rdew,
                                                                         int x,
                                                                         int y
                                                                         );
extern void                            process_world_structures(ExportedMapBase* map);


/*****************************************************************************
 Local functions forward declaration
*****************************************************************************/

void get_nobility_diplomacy_site_color(int site_type,
                                       unsigned char& pixel_R,
                                       unsigned char& pixel_G,
                                       unsigned char& pixel_B
                                       );

void draw_nobility_diplomacy_site(ExportedMapBase* map,
                                  df::world_site* world_site
                                  );

void draw_nobility_diplomacy_sites(ExportedMapBase* map);

void draw_nobility_map(MapsExporter* map_exporter);

bool nobility_do_work(MapsExporter* maps_exporter);

void draw_nobility_holdings_sites(ExportedMapBase* map);

void draw_nobility_relationship_line(ExportedMapBase* map,           // where to draw
                                     df::world_site*  site1,         // line start
                                     df::world_site*  site2,         // line end
                                     RGB_color&       border_color,  // line border color
                                     RGB_color&       center_color   // line center color
                                     );

void draw_holdings_lines(ExportedMapBase* map,                 // where to draw
                         df::historical_entity* entity,        // noble
                         df::entity_site_link*  ent_site_link,
                         df::world_site*        site1          // site where the noble lives now
                         );

void draw_capital_lines(ExportedMapBase*       map,
                        df::historical_entity* entity,
                        df::entity_site_link*  ent_site_link,
                        df::world_site*        site1,
                        df::world_site*        site2
                        );

/*****************************************************************************
 Module main function.
 This is the function that the thread executes
*****************************************************************************/
void consumer_nobility(void* arg)
{
  bool finish = false;
  MapsExporter *maps_exporter = (MapsExporter *) arg;

  if (arg != nullptr)
  {
    while (!finish)
    {
      if (maps_exporter->is_nobility_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = nobility_do_work(maps_exporter);
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
bool nobility_do_work(MapsExporter* maps_exporter)
{
  // Get the data from the queue
  RegionDetailsElevationWater rdew = maps_exporter->pop_nobility();

  // The map where we will write to
  ExportedMapBase *nobility_map = maps_exporter->get_nobility_map();

  // Check if is the marker for no more data from the producer
  if (rdew.is_end_marker())
  {
    // All the terrain data has been processed.
    process_world_structures(nobility_map);

    // Now draw world sites and relationships over this base map
    draw_nobility_map(maps_exporter);

    // Finish this thread execution
    return true;
  }
  else // There's data to be processed
  {
    // Iterate over the 16 subtiles (x) and (y) that a world tile has
    for (auto x=0; x<16; ++x)
      for (auto y=15; y>=0; --y)
        process_nob_dip_trad_sites_common(nobility_map,
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
//----------------------------------------------------------------------------//
void draw_nobility_map(MapsExporter* map_exporter)
{
  ExportedMapBase* map = map_exporter->get_nobility_map();

  // Draw rectangles over ALL sites
  draw_nobility_diplomacy_sites(map);

  // Process all entities looking for nobles
  for (unsigned int l = 0; l < df::global::world->entities.all.size(); ++l)
  {
    df::historical_entity *entity = df::global::world->entities.all[l];

    if (entity == nullptr) continue;

    df::world_site* capital_site = nullptr;

    // For each entity, check its relationships with some sites
    for (unsigned int m = 0; m < entity->site_links.size(); ++m)
    {
      df::entity_site_link* ent_site_link = entity->site_links[m];
      if (ent_site_link == nullptr) continue;

      // The relationship that we want
      // TODO Use the bitmask properly
      if (ent_site_link->flags.whole & 0x2) // is capital?
      {
        // Get the world site target id
        int world_site_id = ent_site_link->target;
        if (world_site_id != -1)
        {
          // get the df::world_site for this site id
          capital_site = search_world_data_sites(world_site_id);
          if (capital_site != nullptr)
            break;
        }
      }
    }

    // Repeat again looking for different flags values
    for (unsigned int m = 0; m < entity->site_links.size(); ++m)
    {
      df::entity_site_link* ent_site_link = entity->site_links[m];

      if (ent_site_link == nullptr) continue;

      // Check flags
      // TODO Use the bitmask properly
      if (!(ent_site_link->flags.whole & 0x200)) continue;

      // land for holding (all regular sites get this if civ has nobles, whether they have a noble or not)

      int world_site_id = ent_site_link->target;
      if (world_site_id == -1) continue;

      // get the df::world_site for this site id
      df::world_site* world_site = search_world_data_sites(world_site_id);
      if (world_site == nullptr) continue;

      if (ent_site_link->anon_1 == -1) continue;

      if (ent_site_link->flags.whole & 0x800)// TODO Use the bitmask properly
        // Land holder residence
        // The regular sites where a baron, etc
        // actually lives
        draw_capital_lines(map,
                           entity,
                           ent_site_link,
                           world_site,
                           capital_site
                           );
      else
        draw_holdings_lines(map,
                            entity,
                            ent_site_link,
                            world_site
                            );
    }

    // Compute the % of map processing
    float a = l*100;
    float b = df::global::world->entities.all.size();
    map_exporter->set_percentage_sites((int)(a/b));
  }

  // Draw rectangles ONLY over each noble holdings
  draw_nobility_holdings_sites(map);

  // Map generated. Warn the main thread
  map_exporter->set_percentage_nobility(-1);
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void draw_nobility_holdings_sites(ExportedMapBase*  map)
{
  int pixel_R;
  int pixel_G;
  int pixel_B;

  for (int i = df::global::world->world_data->sites.size() - 1; i >= 0; --i)
  {
     df::world_site* world_site = df::global::world->world_data->sites[i];
     if (world_site == nullptr) continue;

     int world_site_flags_size  = world_site->flags.size;
     int world_site_flags_value = world_site->flags.as_int();

     if (world_site_flags_size == 0x00) continue;
     if (world_site_flags_value & 0x01) continue;
     if (world_site_flags_value & 0x04) continue;
     if (world_site_flags_value & 0x80) continue;

     df::historical_entity* hist_ent1 = get_historical_entity_from_world_site(world_site);
     if ( hist_ent1 == nullptr) continue;

     df::historical_entity* hist_ent = f(hist_ent1);

     // Find the correct entity site link for this one
     df::entity_site_link* ent_site_link = find_entity_site_link_from_world_site_id(hist_ent->site_links,
                                                                                    world_site->id
                                                                                    );

     if (ent_site_link == nullptr) continue;

     // TODO Use the bitmask properly
     if (ent_site_link->flags.whole & 2) // is capital?
     {
       // site is capital

       // Draw a hollow rectangle (255,0,255) 2 pixels inside the site boundary
       draw_site_rectangle_offseted(map,
                                    world_site,
                                    255,
                                    0,
                                    255,
                                    2
                                    );

       // Draw a hollow rectangle (128,0,128) 3 pixels inside the site boundary
       draw_site_rectangle_offseted(map,
                                    world_site,
                                    128,
                                    0,
                                    128,
                                    3
                                    );
     }
     else // site is NOT capital
     {
       // TODO Use the bitmask properly
       if (!(ent_site_link->flags.whole & 0x800)) continue; // land holder residence (the regular sites where a baron etc. actually lives)

       df::entity_position_assignment* ent_pos_assign = search_entity_positions_assignments(hist_ent->positions.assignments,
                                                                                            ent_site_link->anon_1
                                                                                            );
       if (ent_pos_assign == nullptr) continue;

       df::entity_position* ent_pos = search_entity_positions_own(hist_ent->positions.own,
                                                                  ent_pos_assign->position_id
                                                                  );
       if (ent_pos == nullptr) continue;

       // We have a entity that owns land

       int land_holder = ent_pos->land_holder - 1; // land_holder = 0,1,2,3,...
       if (land_holder != 0) // land_holder = 0,2,3,...
       {
         int land_holder2 = land_holder - 1;
         if (land_holder2 != 0) // 0,3..
         {
           if (land_holder2 != 1)
             continue;

           // land_holder = 3
           pixel_R = 255;
           pixel_G = 0;
           pixel_B = 0;
         }
         else // landholder = 2 ..
         {
           pixel_R = 255;
           pixel_G = 128;
           pixel_B = 0;
         }
       }
       else // land_holder = 1 OK
       {
         pixel_R = 255;
         pixel_G = 255;
         pixel_B = 0;
       }


       if (pixel_R != -128 || pixel_G != -128 || pixel_B != -128)
       {
         draw_site_rectangle_filled_offseted(map,
                                             world_site,
                                             pixel_R,
                                             pixel_G,
                                             pixel_B,
                                             2
                                             );
       }
     }
   }
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void draw_nobility_diplomacy_sites(ExportedMapBase* map)
{
  // Draw over the "basic" map a rectangle over each world site

  for (unsigned int i = 0; i < df::global::world->world_data->sites.size(); i++)
  {
    df::world_site *world_site = df::global::world->world_data->sites[i];

    if (world_site == nullptr) continue;

    // Check site flags
    int site_flags_size = world_site->flags.size;
    int site_flags_value = world_site->flags.as_int();

    if (site_flags_size == 0x00)
      draw_nobility_diplomacy_site(map,
                                   world_site
                                   );

    if (
        !(site_flags_value  & 0x01) &&
        !(site_flags_value  & 0x80)
       )
      draw_nobility_diplomacy_site(map,
                                   world_site
                                   );

  }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void draw_nobility_diplomacy_site(ExportedMapBase* map,
                                  df::world_site* world_site
                                  )
{
  unsigned char pixel_R;
  unsigned char pixel_G;
  unsigned char pixel_B;

  // Get the site color to draw
  get_nobility_diplomacy_site_color(world_site->type,
                                    pixel_R,
                                    pixel_G,
                                    pixel_B
                                    );

  // Check the population of this site, because it can be empty
  int site_population = get_site_total_population(world_site);

  // Draw the rectangle with the proper color
  draw_site_rectangle(map,             // where to draw
                      world_site,      // The site to draw
                      site_population, // This changes how the site is drawed
                      pixel_R,         // site color
                      pixel_G,
                      pixel_B
                      );
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void get_nobility_diplomacy_site_color(int site_type,
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

    case 9: // Camp (Bandits)
      pixel_R = 0xff;
      pixel_G = 0x00;
      pixel_B = 0x80;
      break;

    case 10: // Monument
      pixel_R = 0x80;
      pixel_G = 0x80;
      pixel_B = 0x80;
      break;
  }

  pixel_R >>= 1;
  pixel_G >>= 1;
  pixel_B >>= 1;

}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void draw_nobility_relationship_line(ExportedMapBase* map,    // where to draw
                                     df::world_site* site1,   // line start
                                     df::world_site* site2,   // line end
                                     RGB_color& border_color, // line border color
                                     RGB_color& center_color  // line center color
                                     )
{

  // The limits of site 1
  int site1_global_min_x = site1->global_min_x;
  int site1_global_max_x = site1->global_max_x;
  int site1_global_min_y = site1->global_min_y;
  int site1_global_max_y = site1->global_max_y;

  // The limits of site 2
  int site2_global_min_x = site2->global_min_x;
  int site2_global_max_x = site2->global_max_x;
  int site2_global_min_y = site2->global_min_y;
  int site2_global_max_y = site2->global_max_y;

  // The "center" of each site
  int site1_center_x = (site1_global_min_x + site1_global_max_x) >> 1;
  int site1_center_y = (site1_global_min_y + site1_global_max_y) >> 1;

  int site2_center_x = (site2_global_min_x + site2_global_max_x) >> 1;
  int site2_center_y = (site2_global_min_y + site2_global_max_y) >> 1;

  // Draw the line in a buffer
  draw_thick_color_line(map,            // Where to draw
                        site1_center_x, // line start x
                        site1_center_y, // line start y
                        site2_center_x, // line end x
                        site2_center_y, // line end y
                        center_color,   // center line color
                        border_color    // border line color
                        );
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void draw_holdings_lines(ExportedMapBase*       map,
                         df::historical_entity* entity,
                         df::entity_site_link*  ent_site_link,
                         df::world_site*        site1
                         )
{
  // draw lines from where the noble lives now to each of its holdings
  for (unsigned int t = 0; t < entity->site_links.size(); t++)
  {
    df::entity_site_link* ent_site_link4 = entity->site_links[t];
    if (ent_site_link4 == nullptr) continue;

    // TODO Use the bitmask properly
    if (!(ent_site_link4->flags.whole & 0x800)) continue;

    // land holder residence (the regular sites where a baron etc. actually lives)

    if (ent_site_link4->anon_1 != ent_site_link->anon_1) continue;

    df::world_site* site2 = search_world_data_sites(ent_site_link4->target);

    if (site2 == nullptr) continue;

    // Draw a line
    RGB_color border_color;
    std::get<0>(border_color) = 0;
    std::get<1>(border_color) = 127;
    std::get<2>(border_color) = 0;

    RGB_color center_color;
    std::get<0>(border_color) = 0;
    std::get<1>(center_color) = 255;
    std::get<2>(center_color) = 0;

    draw_nobility_relationship_line(map,
                                    site1,
                                    site2,
                                    border_color,
                                    center_color
                                    );
  }
}



//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void draw_capital_lines(ExportedMapBase*       map,
                        df::historical_entity* entity,
                        df::entity_site_link*  ent_site_link,
                        df::world_site*        site1,
                        df::world_site*        site2
                        )
{
  // land holder residence (the regular sites where a baron etc. actually lives)
  df::entity_position_assignment* ent_pos_assign = search_entity_positions_assignments(entity->positions.assignments,
                                                                                       ent_site_link->anon_1
                                                                                       );
  if (ent_pos_assign == nullptr) return;

  int ent_pos_assign_anon_4 = ent_pos_assign->anon_4; // TODO quitar esto y ver si funciona

  if ((ent_pos_assign->anon_3    != entity->id) ||
      (ent_pos_assign->anon_4    == -1)         ||
      (entity->site_links.size() == 0)
     )
  {

    int r_color = 0xff;
    int g_color = 0x00;
    int b_color = 0xff;

    // TODO Use the bitmask properly
    if (!(ent_site_link->flags.whole & 0x400))
    {
      // Not central holding land (only dwarf fortresses get this for now)
      r_color = 0x80;
      b_color = 0x80;
    }

    // Define line colors
    RGB_color border_color;
    std::get<0>(border_color) = r_color >> 1;
    std::get<1>(border_color) = g_color >> 1;
    std::get<2>(border_color) = b_color >> 1;

    RGB_color center_color;
    std::get<0>(center_color) = r_color;
    std::get<1>(center_color) = g_color;
    std::get<2>(center_color) = b_color;

    if (site1 == nullptr) return;
    if (site2 == nullptr) return;

    // Draw the line
    draw_nobility_relationship_line(map,
                                    site1,
                                    site2,
                                    border_color,
                                    center_color
                                    );
  }
  else
  {
    for (unsigned int n = 0; n < entity->site_links.size(); ++n)
    {
     df::entity_site_link *ent_site_link2 = entity->site_links[n];

     if (ent_site_link2 == nullptr)                        continue;
     // TODO Use the bitmask properly
     if (!(ent_site_link2->flags.whole & 0x800))           continue;
     if (ent_site_link2->anon_1 != ent_pos_assign_anon_4)  continue;

     df::world_site* site3 = search_world_data_sites(ent_site_link2->target);

     if (site3 == nullptr) continue;

     df::entity_position_assignment* ent_pos_assign3 = search_entity_positions_assignments(entity->positions.assignments,
                                                                                           ent_site_link2->anon_1
                                                                                           );
     if (ent_pos_assign3 == nullptr) continue;

     df::entity_position* ent_pos2 = search_entity_positions_own(entity->positions.own,
                                                                 ent_pos_assign3->position_id
                                                                 );
     if (ent_pos2 == nullptr) continue;

     unsigned char r_color   = 0xff;
     unsigned char g_color   = 0xff;
     unsigned char b_color   = 0x00;


     if (ent_pos2->land_holder == 2)
     {
       g_color = 0x80;
     }
     else if (ent_pos2->land_holder == 3)
     {
       g_color = 0x00;
     }

     // Define line colors
     RGB_color border_color;
     std::get<0>(border_color) = r_color >> 1;
     std::get<1>(border_color) = g_color >> 1;
     std::get<2>(border_color) = b_color >> 1;

     RGB_color center_color;
     std::get<0>(center_color) = r_color;
     std::get<1>(center_color) = g_color;
     std::get<2>(center_color) = b_color;

     // Draw the line
     draw_nobility_relationship_line(map,
                                     site1,
                                     site3,
                                     border_color,
                                     center_color
                                     );
    }
  }
}
