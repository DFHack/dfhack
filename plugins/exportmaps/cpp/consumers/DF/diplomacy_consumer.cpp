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

extern df::history_event*              search_world_history_events(int target);

extern df::history_event_collection*   search_world_history_event_collections(int target);

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
                                                             int x1,
                                                             int y1,
                                                             int x2,
                                                             int y2,
                                                             RGB_color& color_center,
                                                             RGB_color& color_border
                                                             );

extern void                           draw_thick_color_line_2_colors(ExportedMapBase* map,
                                                                     int x1,
                                                                     int y1,
                                                                     int x2,
                                                                     int y2,
                                                                     RGB_color& color_center1,
                                                                     RGB_color& color_border1,
                                                                     RGB_color& color_center2,
                                                                     RGB_color& color_border2
                                                                     );

extern int                             get_site_total_population(df::world_site* world_site);


extern RGB_color                       RGB_from_elevation_water(RegionDetailsElevationWater& rdew,
                                                                int x,
                                                                int y,
                                                                int biome_type,
                                                                DFHack::BitArray<df::region_map_entry_flags>& flags
                                                                );

extern void                            get_nobility_diplomacy_site_color(int            site_type,
                                                                         unsigned char& pixel_R,
                                                                         unsigned char& pixel_G,
                                                                         unsigned char& pixel_B
                                                                         );

extern void                            draw_nobility_diplomacy_site(ExportedMapBase* map,
                                                                    df::world_site* world_site
                                                                    );

extern void                            draw_nobility_diplomacy_sites(ExportedMapBase* map);

extern df::historical_entity*          f2(df::historical_entity*);

extern df::world_site*                 find_world_site_linked_with_entity_links_flags_value(df::historical_entity* entity,
                                                                                            int flag_value_to_check
                                                                                            );

extern df::historical_entity*          f(df::historical_entity* entity);

extern bool                            process_nob_dip_trad_sites_common(ExportedMapBase* map,
                                                                         RegionDetailsElevationWater& rdew,
                                                                         int x,
                                                                         int y
                                                                         );
extern void process_world_structures(ExportedMapBase* map);

extern void draw_nobility_holdings_sites(ExportedMapBase* map);

/*****************************************************************************
 Local functions forward declaration
*****************************************************************************/
void draw_diplomacy_map(MapsExporter* maps_exporter);

bool diplomacy_do_work(MapsExporter* maps_exporter);

void diplomacy_1st_pass(MapsExporter* maps_exporter);

void diplomacy_2nd_pass(MapsExporter* maps_exporter);

void diplomacy_3rd_pass(MapsExporter* maps_exporter);

int  get_parameter(df::historical_entity* entity1,
                  df::historical_entity* entity2
                  );

int  get_parameter2(df::historical_entity::T_unknown1b::T_diplomacy* diplomacy_entry,
                   int entity_id
                   );

/*****************************************************************************
 Module main function.
 This is the function that the thread executes
*****************************************************************************/
void consumer_diplomacy(void* arg)
{
  bool finish = false;
  MapsExporter *maps_exporter = (MapsExporter *) arg;

  if (arg != nullptr)
  {
    while (!finish)
    {
      if (maps_exporter->is_diplomacy_queue_empty())
        // No data on the queue. Try again later
        tthread::this_thread::yield();

      else // There's data in the queue
        finish = diplomacy_do_work(maps_exporter);
    }
  }

  // Function finish -> Thread finish
}


/*****************************************************************************
 Utility function

 Get the data from the queue.
  If is the end marker, the queue is empty and no more work needs to be done in the terrain
  Then process the diplomacy data to draw it over the terrain map
 If it's actual data process it and update the corresponding map
*****************************************************************************/
bool diplomacy_do_work(MapsExporter* maps_exporter)
{
  // Get the data from the queue
  RegionDetailsElevationWater rdew = maps_exporter->pop_diplomacy();

  // Get the map
  ExportedMapDF* diplomacy_map = maps_exporter->get_diplomacy_map();

  // Check if is the marker for no more data from the producer
  if (rdew.is_end_marker())
  {
    // All the terrain data has been processed.
    process_world_structures(diplomacy_map);

    // Now draw world sites and relationships over this base map
    draw_diplomacy_map(maps_exporter);

    // Finish this thread execution
    return true;
  }
  else // There's data to be processed
  {
    // Iterate over the 16 subtiles (x) and (y) that a world tile has
    for (auto x=0; x<16; ++x)
      for (auto y=15; y>=0; --y)
        process_nob_dip_trad_sites_common(diplomacy_map, rdew, x, y);
  }

  return false;
}



//----------------------------------------------------------------------------//
// Utility function
//
// Draws the diplomacy map over the terrain map
//----------------------------------------------------------------------------//
void draw_diplomacy_map(MapsExporter* map_exporter)
{
  // Draw rectangles over ALL sites
  draw_nobility_diplomacy_sites(map_exporter->get_diplomacy_map());

  // 1st pass
  diplomacy_1st_pass(map_exporter);
  // 2nd pass
  diplomacy_2nd_pass(map_exporter);
  // 3rd pass
  diplomacy_3rd_pass(map_exporter);

  // Draw rectangles ONLY over each noble holdings
  draw_nobility_holdings_sites(map_exporter->get_diplomacy_map());
  //draw_capital_sites(map);

  // Map generated. Warn the main thread
  map_exporter->set_percentage_diplomacy(-1);

}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void diplomacy_1st_pass(MapsExporter* map_exporter)
{
  ExportedMapBase* map = map_exporter->get_diplomacy_map();
  for (unsigned int i = 0; i < df::global::world->world_data->sites.size(); i++)
  {
    df::world_site* site1 = df::global::world->world_data->sites[i];

    if (site1 == nullptr) continue;

    df::historical_entity* entity1 = nullptr;
    df::historical_entity* entity2 = nullptr;


    df::world_site* site2 = nullptr;

    // Get a historical entity that is related to this site;
    entity1 = get_historical_entity_from_world_site(site1);

    if (entity1 == nullptr) continue;

    // Check its diplomacy links
    for (unsigned int j = 0; j < entity1->unknown1b.diplomacy.size() ; j++)
    {
      df::historical_entity::T_unknown1b::T_diplomacy*  diplomacy_entry = entity1->unknown1b.diplomacy[j];
      if (diplomacy_entry           == nullptr) continue;
      if (diplomacy_entry->relation != 0)       continue;

      // Locate another entity
      entity2 = search_world_entities(diplomacy_entry->group_id);

      // Discard unwanted entries
      if (entity2                == nullptr) continue;
      if (entity2->flags.whole   &  0x10)    continue;

      if (entity2->type == 1) // Site government
      {
        // Locate residence unsing entity site links
        df::entity_site_link* residence = nullptr;
        for (unsigned int k = 0; k < entity2->site_links.size(); k++)
        {
          df::entity_site_link* site_link = entity2->site_links[k];
          if (site_link == nullptr) continue;

          if (site_link->flags.whole & 0x01) // TODO Use the bitmask properly
          {
            // residence flag
            residence = site_link;
            break;
          }
        }

        // Discard unwanted items
        if (residence         == nullptr) continue;
        if (residence->target == -1)      continue;

        // Get the world site pointed
        site2 = search_world_data_sites(residence->target);
        if (site2 == nullptr) continue;

        // Check site entity site links
        int entity_id = -1;
        for (unsigned int m = 0; m < site2->entity_links.size(); m++)
        {
          df::entity_site_link* site_link3 = site2->entity_links[m];
          if (site_link3           == nullptr) continue;
          if (!(site_link3->flags.whole &  0x01 ))  continue;// TODO Use the bitmask properly

          if (!(site_link3->anon_2) && (site_link3->anon_3 == -1))
          {
            entity_id = site_link3->entity_id;
            break;
          }
        }

        if (entity_id == -1)          continue;
        if (entity_id != entity2->id) continue;
      }
      else // Not site government
      {
        if (entity2->type != 0) continue;

        df::entity_site_link* capital = nullptr;
        for (unsigned int k = 0; k < entity2->site_links.size(); k++)
        {
          df::entity_site_link* site_link = entity2->site_links[k];
          if (site_link == nullptr) continue;

          if (site_link->flags.whole & 0x02)// TODO Use the bitmask properly
          {
            // capital flag
            capital = site_link;
            break;
          }
        }

        // Discard unwanted items
        if (capital         == nullptr) continue;
        if (capital->target == -1)      continue;

        // Get the world site pointed
        site2 = search_world_data_sites(capital->target);
        if (site2 == nullptr) continue;
      }



      if (entity2->id <= entity1->id)
      {
        // Draw a line site1 to site2
        unsigned char pixel_R = 0x00;
        unsigned char pixel_G = 0x00;
        unsigned char pixel_B = 0x00;

        int site1_center_x = (site1->global_min_x + site1->global_max_x) >> 1;
        int site1_center_y = (site1->global_min_y + site1->global_max_y) >> 1;
        int site2_center_x = (site2->global_min_x + site2->global_max_x) >> 1;
        int site2_center_y = (site2->global_min_y + site2->global_max_y) >> 1;

        int parameter = -(get_parameter(entity1, entity2) + get_parameter2(diplomacy_entry,entity1->id));
        if (parameter >= 30)
        {
          pixel_R = 0x40;
          pixel_G = 0x40;
          pixel_B = 0x40;
        }
        else if (parameter < 10)
        {
          int v249 = __OFSUB__(parameter, -10);
          if (((10 + parameter) < 0) ^ v249)
          {
            pixel_R = 0x80;
            pixel_G = 0xff;
            pixel_B = 0x80;


          }
          else
          {
            pixel_R = 0x80;
            pixel_G = 0xff;
            pixel_B = 0xff;
          }
        }
        else // parameter >= 10 and < 30
        {
          pixel_R = 0x80;
          pixel_G = 0x80;
          pixel_B = 0x80;
        }

        RGB_color center_color(pixel_R     , pixel_G     , pixel_B     );
        RGB_color border_color(pixel_R >> 1, pixel_G >> 1, pixel_B >> 1);
        draw_thick_color_line(map,            // Where to draw
                              site1_center_x, // line start x
                              site1_center_y, // line start y
                              site2_center_x, // line end x
                              site2_center_y, // line end y
                              center_color,   // center line color
                              border_color);  // border line color
      }
    }

    // Compute the % of map processing
    float a = i*100;
    float b = df::global::world->entities.all.size() + 2 * df::global::world->world_data->sites.size();
    map_exporter->set_percentage_sites((int)(a/b));
  }

}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void diplomacy_2nd_pass(MapsExporter* map_exporter)
{
  ExportedMapBase* map = map_exporter->get_diplomacy_map();

  for (unsigned int i = 0; i < df::global::world->world_data->sites.size(); i++)
  {
    df::world_site* site1 = df::global::world->world_data->sites[i];

    if (site1 == nullptr) continue;

    df::historical_entity* entity1 = nullptr;
    df::historical_entity* entity2 = nullptr;
    df::world_site* site2 = nullptr;

    // Get a historical entity that is related to this site;
    entity1 = get_historical_entity_from_world_site(site1);

    if (entity1 == nullptr) continue;

    // Check its diplomacy links
    for (unsigned int j = 0; j < entity1->unknown1b.diplomacy.size() ; j++)
    {
      df::historical_entity::T_unknown1b::T_diplomacy* diplomacy_entry = entity1->unknown1b.diplomacy[j];
      if (diplomacy_entry == nullptr) continue;

      // Locate another entity
      entity2 = search_world_entities(diplomacy_entry->group_id);

      // Discard unwanted entries
      if (entity2                == nullptr) continue;
      if (entity2->flags.whole   &  0x10)    continue;

      if (entity2->type == 1) // Site government
      {
        // Locate residence unsing entity site links
        df::entity_site_link* residence = nullptr;
        for (unsigned int k = 0; k < entity2->site_links.size(); k++)
        {
          df::entity_site_link* site_link = entity2->site_links[k];
          if (site_link == nullptr) continue;

          if (site_link->flags.whole & 0x01)// TODO Use the bitmask properly
            if ((!site_link->anon_2) && (site_link->anon_3 == -1))
            {
              // residence flag
              residence = site_link;
              break;
            }
        }

        // Discard unwanted items
        if (residence         == nullptr) continue;
        if (residence->target == -1)      continue;

        // Get the world site pointed
        site2 = search_world_data_sites(residence->target);
        if (site2 == nullptr) continue;

        // Check site entity site links
        int entity_id = -1;
        for (unsigned int m = 0; m < site2->entity_links.size(); m++)
        {
          df::entity_site_link* site_link3 = site2->entity_links[m];
          if (site_link3           == nullptr) continue;
          if (!(site_link3->flags.whole &  0x01 ))  continue;// TODO Use the bitmask properly

          if (!(site_link3->anon_2) && (site_link3->anon_3 == -1))
          {
            entity_id = site_link3->entity_id;
            break;
          }
        }

        if (entity_id == -1)          continue;
        if (entity_id != entity2->id) continue;


      }
      else // Not site government
      {
        if (entity2->type != 0) continue;

        df::entity_site_link* capital = nullptr;
        for (unsigned int k = 0; k < entity2->site_links.size(); k++)
        {
          df::entity_site_link* site_link = entity2->site_links[k];
          if (site_link == nullptr) continue;

          if (site_link->flags.whole & 0x02)// TODO Use the bitmask properly
          {
            // capital flag
            capital = site_link;
            break;
          }
        }

        // Discard unwanted items
        if (capital         == nullptr) continue;
        if (capital->target == -1)      continue;

        // Get the world site pointed
        site2 = search_world_data_sites(capital->target);
        if (site2 == nullptr) continue;

      }

     if ((diplomacy_entry->relation == 1) ||
         (diplomacy_entry->relation == 4) ||
         (diplomacy_entry->relation == 5) )
     {
        // Draw a line

        unsigned char pixel_R1 = 0x00;
        unsigned char pixel_G1 = 0x00;
        unsigned char pixel_B1 = 0x00;
        unsigned char pixel_R2 = 0x00;
        unsigned char pixel_G2 = 0x00;
        unsigned char pixel_B2 = 0x00;

        switch(diplomacy_entry->relation)
        {
          case 4:
                  pixel_R1 = 0x00;
                  pixel_G1 = 0xff;
                  pixel_B1 = 0xff;
                  pixel_R2 = 0x00;
                  pixel_G2 = 0x80;
                  pixel_B2 = 0x80;
                  break;
          case 1:
                  pixel_R1 = 0xff;
                  pixel_G1 = 0x00;
                  pixel_B1 = 0x00;
                  pixel_R2 = 0xff;
                  pixel_G2 = 0x00;
                  pixel_B2 = 0x00;
                  break;
          case 5:
                  pixel_R1 = 0xff;
                  pixel_G1 = 0xff;
                  pixel_B1 = 0x00;
                  pixel_R2 = 0xff;
                  pixel_G2 = 0xff;
                  pixel_B2 = 0x00;
                  break;
        }

        RGB_color center_color1(pixel_R1     ,pixel_G1     ,pixel_B1     );
        RGB_color border_color1(pixel_R1 >> 1,pixel_G1 >> 1,pixel_B1 >> 1);
        RGB_color center_color2(pixel_R2     ,pixel_G2     ,pixel_B2     );
        RGB_color border_color2(pixel_R2 >> 1,pixel_G2 >> 1,pixel_B2 >> 1);

        int site1_center_x = (site1->global_min_x + site1->global_max_x) >> 1;
        int site1_center_y = (site1->global_min_y + site1->global_max_y) >> 1;
        int site2_center_x = (site2->global_min_x + site2->global_max_x) >> 1;
        int site2_center_y = (site2->global_min_y + site2->global_max_y) >> 1;

        draw_thick_color_line_2_colors(map,            // Where to draw
                                       site1_center_x, // line start x
                                       site1_center_y, // line start y
                                       site2_center_x, // line end x
                                       site2_center_y, // line end y
                                       center_color1,  // center line color
                                       border_color1,  // border line color
                                       center_color2,  // center line color
                                       border_color2); // border line color
     }
    }

    // Compute the % of map processing
    float a = (i + df::global::world->world_data->sites.size()) * 100;
    float b = df::global::world->entities.all.size() + 2 * df::global::world->world_data->sites.size();
    map_exporter->set_percentage_sites((int)(a/b));
  }

}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
void diplomacy_3rd_pass(MapsExporter* map_exporter)
{
  ExportedMapBase* map = map_exporter->get_diplomacy_map();

  for (unsigned int p = 0; p < df::global::world->entities.all.size(); p++)
  {
    df::historical_entity* entity1 = df::global::world->entities.all[p];
    if (entity1 == nullptr)                                    continue;
    if ((entity1->type != 0) || (entity1->flags.whole & 0x10)) continue;

    // Locate residence unsing entity site links
    df::entity_site_link* capital = nullptr;

    for (unsigned int q = 0; q < entity1->site_links.size(); q++)
    {
      df::entity_site_link* site_link = entity1->site_links[q];
      if (site_link == nullptr) continue;

      if (site_link->flags.whole & 0x02)// TODO Use the bitmask properly
      {
        // capital flag
        capital = site_link;
        break;
      }
    }

    // Discard unwanted items
    if (capital         == nullptr) continue;
    if (capital->target == -1)      continue;

    // Locate this world site
    df::world_site* site1 = search_world_data_sites(capital->target);
    if (site1 == nullptr) continue;

    // Get a historical entity that is related to this site;
    df::historical_entity* entity2 = get_historical_entity_from_world_site(site1);
    if (entity2 == nullptr) continue;

    if ((entity2 == entity1) || (f2(entity2) == entity1))
    {
      // Check its diplomacy links
      for (unsigned int j = 0; j < entity1->unknown1b.diplomacy.size(); j++)
      {
        auto diplomacy_entry = entity1->unknown1b.diplomacy[j];
        if (diplomacy_entry           == nullptr) continue;
        if (diplomacy_entry->group_id == -1)      continue;

        df::historical_entity* entity3 = search_world_entities(diplomacy_entry->group_id);

        if (entity3              == nullptr) continue;
        if ((entity3->flags.whole &  0x10) || (entity3->type != 0))   continue;

        df::world_site* site2 = find_world_site_linked_with_entity_links_flags_value(entity3, 0x02);

        if (site2 == nullptr) continue;

        df::historical_entity* entity4 = get_historical_entity_from_world_site(site2);
        if (entity4 == nullptr) continue;

        if ((entity4 == entity3) || (f2(entity4) == entity3))
        {
          unsigned char pixel_R1 = 0x00;
          unsigned char pixel_G1 = 0x00;
          unsigned char pixel_B1 = 0x00;
          unsigned char pixel_R2 = 0x00;
          unsigned char pixel_G2 = 0x00;
          unsigned char pixel_B2 = 0x00;

          bool draw_line = false;
          bool draw_double_line = false;

int a,b;
          int parameter;

          switch (diplomacy_entry->relation)
          {
            case 1:   // assign colors
                      pixel_R1 = 0xff;
                      pixel_G1 = 0x00;
                      pixel_B1 = 0x00;
                      // draw line
                      draw_line = true;
                      break;

            case 4:   //
                      pixel_R1 = 0x00;
                      pixel_G1 = 0xff;
                      pixel_B1 = 0xff;
                      pixel_R2 = 0x00;
                      pixel_G2 = 0x80;
                      pixel_B2 = 0x80;

                      // draw line with two colors
                      draw_double_line = true;
                      break;
            case 5:  // assign colors
                      pixel_R1 = 0xff;
                      pixel_G1 = 0xff;
                      pixel_B1 = 0x00;
                      // draw line
                      draw_line = true;
                      break;
            case 0:   if (entity3->id > entity1->id)
                      break;
                      a = get_parameter(entity1, entity3);
                      b = get_parameter2(diplomacy_entry,entity1->id);
                      parameter = -(a + b);
                      if (parameter >= 30)
                      {
                        pixel_R1 = 0x40;
                        pixel_G1 = 0x40;
                        pixel_B1 = 0x40;
                      }
                      else if (parameter >= 10)
                      {
                        pixel_R1 = 0x80;
                        pixel_G1 = 0x80;
                        pixel_B1 = 0x80;

                      }
                      else if (parameter < -10)
                      {
                        pixel_R1 = 0x80;
                        pixel_G1 = 0xff;
                        pixel_B1 = 0x80;
                      }
                      else // parameter >= -10 and parameter < 10
                      {
                        pixel_R1 = 0x00;
                        pixel_G1 = 0xff;
                        pixel_B1 = 0xff;

                      }
                      // draw line
                      draw_line = true;
                      break;
            default:  break;
          }

          if (draw_line)
          {
            RGB_color center_color1(pixel_R1     ,pixel_G1     ,pixel_B1     );
            RGB_color border_color1(pixel_R1 >> 1,pixel_G1 >> 1,pixel_B1 >> 1);


            int site1_center_x = (site1->global_min_x + site1->global_max_x) >> 1;
            int site1_center_y = (site1->global_min_y + site1->global_max_y) >> 1;
            int site2_center_x = (site2->global_min_x + site2->global_max_x) >> 1;
            int site2_center_y = (site2->global_min_y + site2->global_max_y) >> 1;

            draw_thick_color_line(map,            // Where to draw
                                  site1_center_x, // line start x
                                  site1_center_y, // line start y
                                  site2_center_x, // line end x
                                  site2_center_y, // line end y
                                  center_color1,   // center line color
                                  border_color1);  // border line color
          }

          else if (draw_double_line)
          {
            RGB_color center_color1(pixel_R1     ,pixel_G1     ,pixel_B1     );
            RGB_color border_color1(pixel_R1 >> 1,pixel_G1 >> 1,pixel_B1 >> 1);
            RGB_color center_color2(pixel_R2     ,pixel_G2     ,pixel_B2     );
            RGB_color border_color2(pixel_R2 >> 1,pixel_G2 >> 1,pixel_B2 >> 1);


            int site1_center_x = (site1->global_min_x + site1->global_max_x) >> 1;
            int site1_center_y = (site1->global_min_y + site1->global_max_y) >> 1;
            int site2_center_x = (site2->global_min_x + site2->global_max_x) >> 1;
            int site2_center_y = (site2->global_min_y + site2->global_max_y) >> 1;

            draw_thick_color_line_2_colors(map,            // Where to draw
                                           site1_center_x, // line start x
                                           site1_center_y, // line start y
                                           site2_center_x, // line end x
                                           site2_center_y, // line end y
                                           center_color1,  // center line color
                                           border_color1,  // border line color
                                           center_color2,  // center line color
                                           border_color2); // border line color
          }
        }
      }
    }

    // Compute the % of map processing
    float a = (p + 2 * df::global::world->world_data->sites.size())*100;
    float b = df::global::world->entities.all.size() + 2 * df::global::world->world_data->sites.size();
    map_exporter->set_percentage_diplomacy((int)a/b);
  }
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int copy_ethic_response(int a1,
                        int a2
                        )
{
  int result;

  switch (a2)
  {
    case  4:
            switch (a1)
            {
              case  4:  return 2;
              case  5:  return 1;
              case  9:
              case 10:
              case 11:
              case 12:
              case 13:
              case 14:
              case 15: return -1;
              default: return 0;
            }
    case  5:
            switch (a1)
            {
              case  5:  return 2;
              case  4:  return 1;
              case  1:
              case  2:
              case  9:
              case 10:
              case 11:
              case 12:
              case 13:
              case 14:
              case 15:
              case 16: return -1;
              default: return 0;
            }
    case  6:
            switch (a1)
            {
              case  6:  return 2;
              case  5:  return 1;
              case  3:
              case 10:
              case 11:
              case 12:
              case 13:
              case 14:
              case 15: return -1;
              case  1:
              case  2:
              case 16: return -2;
              default: return 0;
            }
    case  7:
            switch (a1)
            {
              case  7:  return 2;
              case  5:  return 1;
              case  3:
              case 10:
              case 11:
              case 12:
              case 13:
              case 14:
              case 15: return -1;
              case  1:
              case  2:
              case 16: return -2;
              default: return 0;
            }
    case  8:
            switch (a1)
            {
              case  8:  return 2;
              case  9:
              case 10:
              case 11:
              case 15: return 1;
              case  1:
              case  2:
              case 14:
              case 16: return -1;
              default: return 0;
            }
    case  9:
            switch (a1)
            {
              case  9: return 2;
              case  8:
              case 10:
              case 11:
              case 15: return 1;
              case  1:
              case  2:
              case 14:
              case 16: return -1;
              default: return 0;
            }
    case 10:
            switch (a1)
            {
              case 10:  return 2;
              case  8:
              case  9:
              case 11:
              case 12:
              case 13:
              case 15: return 1;
              case  5:
              case  6: return -1;
              case  1:
              case  2:
              case 16: return -5;
              case  3: return -3;
              case  4:
              case  7: return -2;
              default: return 0;
            }
    case 11:
            switch (a1)
            {
              case 11:  return 2;
              case  8:  ;
              case  9:
              case 10:
              case 12:
              case 13:
              case 15: return 1;
              case  5:
              case  6: return -1;
              case  1:
              case  2:
              case 16: return -5;
              case  3: return -3;
              case  4:
              case  7: return -2;
              default: return 0;
            }
    case 12:
            switch (a1)
            {
              case 12:  return 2;
              case  9:
              case 10:
              case 13:
              case 14:
              case 15: return 1;
              case  4:
              case  7: return -5;
              case  1:
              case  2:
              case 16: return -10;
              case  3: return -7;
              case  5:
              case  6: return -2;
              default: return 0;
            }
    case 13:
            switch (a1)
            {
              case 13:  return 2;
              case  9:
              case 10:
              case 12:
              case 14:
              case 15: return 1;
              case  4:
              case  7: return -3;
              case  1:
              case  2:
              case 16: return -10;
              case  3: return -7;
              case  5:
              case  6: return -2;
              default: return 0;
            }
    case 14:
            switch (a1)
            {
              case 14:  return 2;
              case 10:
              case 12:
              case 13:
              case 15: return 1;
              case  4:
              case  7: return -5;
              case  5:
              case  6: return -3;
              case  3: return -10;
              case  1:
              case  2:
              case 16: return -15;
              default: return 0;
            }
    case 15:
            switch (a1)
            {
              case 15:  return 2;
              case  9:
              case 10:
              case 12:
              case 13:
              case 14: return 1;
              case  4:
              case  7: return -5;
              case  5:
              case  6: return -3;
              case  3: return -10;
              case  1:
              case  2:
              case 16: return -15;
              default: return 0;
            }
    case  1:
    case  2:
    case  3:
    case 16:
            switch (a1)
            {
              case  1:
              case  2:
              case  3:
              case 16: return 1;
              case  8: return -1;
              case 13:
              case 14: return -5;
              case 12: return -3;
              case  9:
              case 10:
              case 11:
              case 15: return -2;
              default: return 0;
            }
    default: result = 0; break;
  }

  return result;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
bool search_in_raws(df::historical_entity* entity1,
                    df::historical_entity* entity2
                    )
{
  if ( entity1 == nullptr)
    return false;

  if ( entity2 == nullptr)
    return false;

  int race2 = entity2->race;
  int race1 = entity1->race;

  if ( race1 == race2)
    return true;

  int raws_size = df::global::world->raws.creatures.all.size();
  df::creature_raw* creat_raw2 = df::global::world->raws.creatures.all[race2];
  if (creat_raw2 == nullptr)
    return false;

  df::creature_raw* creat_raw1 = df::global::world->raws.creatures.all[race1];
  if (creat_raw1 == nullptr)
    return false;

  if ((race2                            <  0        ) ||
      (race1                            <  0        ) ||
      (race2                            >= raws_size) ||
      (race1                            >= raws_size) ||
      (creat_raw2->flags.size           <= 11       ) ||
      (creat_raw1->flags.size           <= 11       ) ||
      (!(*(creat_raw2->flags.bits + 11) &  2       )) || // CASTE_CAN_SPEAK
      (!(*(creat_raw1->flags.bits + 11) &  2       )))   // CASTE_CAN_SPEAK
          return false;

  return true;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_parameter(df::historical_entity* entity1,
                  df::historical_entity* entity2
                  )
{
  if (entity2 == nullptr) return 0;

  int result = 0;
  int a_iterator = 0;
  int v4;

  while (true)
  {
    switch (a_iterator)
    {
      case  0:  v4 =  3; break;
      case  1:  v4 =  4; break;
      case  2:  v4 =  7; break;
      case  3:  v4 =  8; break;
      case  4:  v4 = 10; break;
      case  5:  v4 = 11; break;
      case  6:  v4 = 17; break;
      case  7:  v4 = 18; break;
      case  8:  v4 = 19; break;
      case  9:  v4 = 20; break;
      case 10:  v4 = 21; break;
      default:  break;
    }

    if ((a_iterator == 4) && !search_in_raws(entity2, entity1))
        break;

    int v6 = copy_ethic_response(entity2->resources.ethic[v4], entity1->resources.ethic[v4]);
    if (v6 != 0)
      result += v6;

    if (++a_iterator >= 11)
      return result;
  }
  result += -20;

  return result;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int dip_func1(df::historical_entity::T_unknown1b::T_diplomacy* diplomacy_entry,
              df::history_event* hist_event,
              int entity_id
              )
{
  int result = hist_event->getWarStatus(entity_id,diplomacy_entry->group_id);
  if (result == 0)
    return 0;

  int delta_time = *df::global::cur_year - hist_event->year;

  switch(delta_time)
  {
    case   9: result >>= 3;
              break;
    case   8: result >>= 2;
              break;
    case   7: result = 3 * result >> 3;
              break;
    case   6: result >>= 1;
              break;
    case   5: result = 5 * result >> 3;
              break;
    case   4: result = 3 * result >> 2;
              break;
    case   3: result = 7 * result >> 3;
              break;
    default : return 0;
              break;
  }

  if (result == 0)
    return result;

  return result;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int dip_func2(df::historical_entity::T_unknown1b::T_diplomacy* diplomacy_entry,
              df::history_event_collection* hist_event_col,
              int entity_id
              )
{
  int result = hist_event_col->isBetweenEntities(entity_id, diplomacy_entry->group_id);
  if (result == 0)
    return 0;

  int delta_time = *df::global::cur_year - hist_event_col->end_year;

  switch(delta_time)
  {
    case   9: result >>= 3;
              break;
    case   8: result >>= 2;
              break;
    case   7: result = 3 * result >> 3;
              break;
    case   6: result >>= 1;
              break;
    case   5: result = 5 * result >> 3;
              break;
    case   4: result = 3 * result >> 2;
              break;
    case   3: result = 7 * result >> 3;
              break;
    default : return 0;
              break;
  }

  if (result == 0)
    return result;

  return result;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_parameter2(df::historical_entity::T_unknown1b::T_diplomacy* diplomacy_entry,
                   int entity_id
                   )
{
  int result = 0;

  if (diplomacy_entry == nullptr)
    return 0;

  for (int i = diplomacy_entry->historic_events.size() - 1; i >= 0; i--)
  {
    int event_id = diplomacy_entry->historic_events[i];
    if (event_id == -1) continue;

    df::history_event* hist_event = search_world_history_events(event_id);
    if (hist_event == nullptr)   continue;

    result += dip_func1(diplomacy_entry,
                        hist_event,
                        entity_id
                        );
  }

  for (int i = diplomacy_entry->historic_events_collection.size() - 1; i >= 0; i--)
  {
    int event_id = diplomacy_entry->historic_events_collection[i];
    if (event_id == -1) continue;

    df::history_event_collection* hist_event_col = search_world_history_event_collections(event_id);
    if (hist_event_col == nullptr)   continue;

    result += dip_func2(diplomacy_entry,
                        hist_event_col,
                        entity_id
                        );
  }

  return result;
}
