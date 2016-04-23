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

#include "../../include/Mac_compat.h"
#include "../../include/dfhack.h"

using namespace std;
using namespace DFHack;


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::world_site* search_world_data_sites(int target)
{
  vector<df::world_site* >& vec_sites = df::global::world->world_data->sites;

  int start = 0;
  int end = vec_sites.size() - 1;
  while(true)
  {
    int half = (start + end) >> 1;
    int site_id = vec_sites[half]->id;

    if (site_id == target)
      return vec_sites[half];

    if (site_id <= target)
      start = half + 1;
    else
      end = half - 1;

    if (start > end)
      break;
  }
  return nullptr;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::historical_entity* search_world_entities(int target)
{
  vector<df::historical_entity* >*  ptr_vec = &df::global::world->entities.all;
  vector<df::historical_entity* >& vec = *ptr_vec;

  int start = 0;
  int end = vec.size() - 1;
  while(true)
  {
    int half = (start + end) >> 1;
    int id = vec[half]->id;

    if (id == target)
      return vec[half];

    if (id <= target)
      start = half + 1;
    else
      end = half - 1;

    if (start > end)
      break;
  }
  return nullptr;
}




//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::entity_position_assignment* search_entity_positions_assignments(vector<df::entity_position_assignment* >& vec,
                                                                    int target
                                                                    )
{
  int start = 0;
  int end = vec.size() - 1;

  while(true)
  {
    int half = (start + end) >> 1;
    int id = vec[half]->id;

    if (id == target)
      return vec[half];

    if (id <= target)
      start = half + 1;
    else
      end = half - 1;

    if (start > end)
      break;
  }
  return nullptr;
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::entity_position* search_entity_positions_own(vector<df::entity_position* >& vec,
                                                 int target
                                                 )
{
  int start = 0;
  int end = vec.size() - 1;

  while(true)
  {
    int half = (start + end) >> 1;
    int id = vec[half]->id;

    if (id == target)
      return vec[half];

    if (id <= target)
      start = half + 1;
    else
      end = half - 1;

    if (start > end)
      break;
  }
  return nullptr;
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
int get_historical_entity_id_from_world_site(df::world_site* site)
{
  if ( site == nullptr)
    return -1;

  df::entity_site_link* site_link = nullptr;
  bool found = false;

  for (unsigned int i = 0; i < site->entity_links.size(); i++)
  {
    site_link = site->entity_links[i];
    if ( site_link->flags.whole & 1)
      if (!(site_link->anon_2) && (site_link->anon_3 == -1))
      {
        found = true;
        break;
      }
  }

  if (!found) return -1;

  if (site_link != nullptr)
  {
    df::historical_entity* ent = search_world_entities(site_link->entity_id);
    if (ent != nullptr)
      return ent->id;
  }

  return -1;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::historical_entity* get_historical_entity_from_world_site(df::world_site* site)
{
  if ( site == nullptr)
    return nullptr;

  df::entity_site_link* site_link = nullptr;

  bool found = false;
  for (unsigned int i = 0; i < site->entity_links.size(); i++)
  {
    site_link = site->entity_links[i];
    if ( site_link->flags.whole & 1) // is residence?
      if (!(site_link->anon_2) && (site_link->anon_3 == -1))
      {
        found = true;
        break;
      }
  }

  if (!found) return nullptr;

  if (site_link != nullptr)
    if ( site_link->entity_id != -1)
      return search_world_entities(site_link->entity_id);

  return nullptr;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::entity_position* search_entity_positions(vector<df::entity_position* >& vec,
                                             int target
                                             )
{
  int start = 0;
  int end = vec.size() - 1;

  while(true)
  {
    int half = (start + end) >> 1;
    int id = vec[half]->id;

    if (id == target)
      return vec[half];

    if (id <= target)
      start = half + 1;
    else
      end = half - 1;

    if (start > end)
      break;
  }
  return nullptr;
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::entity_site_link* find_entity_site_link_from_world_site_id(vector<df::entity_site_link* >& vec,
                                                               int target
                                                               )
{
  int start = 0;
  int end = vec.size() - 1;

  while(true)
  {
    int half = (start + end) >> 1;
    int id = vec[half]->target;

    if (id == target)
      return vec[half];

    if (id <= target)
      start = half + 1;
    else
      end = half - 1;

    if (start > end)
      break;
  }
  return nullptr;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::historical_entity* f(df::historical_entity* entity)
{

  if (entity == nullptr)              return entity;

  if (entity->site_links.size() == 0) return entity;

  for (unsigned int i = 0; i < entity->entity_links.size(); i++)
  {
    df::entity_entity_link* entity_link = entity->entity_links[i];

    if (entity_link->type != 0) continue; // next one if CHILD
    {
      // PARENT relationship type
      int target = entity_link->target;

      // Find this one in world.entities.all
      df::historical_entity* result = search_world_entities(target);

      if (result && result->type == 0) // 0 = CIVILIZATION
        return result;
    }
  }
  return entity;
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::historical_entity* f2(df::historical_entity* entity)
{
  if (entity->type == 0)
    return entity;

  for (unsigned int i = 0; i < entity->entity_links.size(); i++)
  {
    df::entity_entity_link* entity_link = entity->entity_links[i];
    if (entity_link         ==  nullptr) continue;
    if (entity_link->type   !=  0)       continue;
    if (entity_link->target == -1)       continue;

    df::historical_entity* entity2 = search_world_entities(entity_link->target);
    if (entity2 == nullptr) continue;

    if (entity2->type == 0)
      return entity2;
  }

  return entity;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::world_site* find_world_site_linked_with_entity_links_flags_value(df::historical_entity* entity,
                                                                     int flag_value_to_check
                                                                     )
{
  if (entity == nullptr)
    return nullptr;

  for (unsigned int i = 0; i < entity->site_links.size(); i++)
  {
    df::entity_site_link* site_link = entity->site_links[i];
    if (site_link == nullptr)       continue;
    if (site_link->target == -1) continue;

    if (site_link->flags.whole & flag_value_to_check)
      return search_world_data_sites(site_link->target);
  }

  return nullptr;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::world_construction* find_construction_square_id_in_world_data_constructions(df::world_data::T_constructions& constructions,
                                                                                int start,
                                                                                int end,
                                                                                int target
                                                                                )
{
  const std::vector<df::world_construction* >& list = constructions.list;

  // Binary search

  while(true)
  {
    int half = (start + end) >> 1;
    int id = list[half]->id;

    if (id == target)
      return list[half];

    if (id <= target)
      start = half + 1;
    else
      end = half - 1;

    if (start > end)
      break;
  }
  return nullptr;
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::history_event* search_world_history_events(int target)
{
  vector<df::history_event* >& vec_events = df::global::world->history.events;

  int start = 0;
  int end = vec_events.size() - 1;
  while(true)
  {
    int half = (start + end) >> 1;
    int event_id = vec_events[half]->id;

    if (event_id == target)
      return vec_events[half];

    if (event_id <= target)
      start = half + 1;
    else
      end = half - 1;

    if (start > end)
      break;
  }
  return nullptr;
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::history_event_collection* search_world_history_event_collections(int target)
{
  auto& vec_events = df::global::world->history.event_collections.all;

  int start = 0;
  int end = vec_events.size() - 1;
  while(true)
  {
    int half = (start + end) >> 1;
    int event_id = vec_events[half]->id;

    if (event_id == target)
      return vec_events[half];

    if (event_id <= target)
      start = half + 1;
    else
      end = half - 1;

    if (start > end)
      break;
  }
  return nullptr;
}

//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::world_site_realization::T_areas* search_world_site_realization_areas(int target,
                                                                         df::world_site_realization* wsr
                                                                         )
{
  if (wsr == nullptr) return nullptr;

  vector<df::world_site_realization::T_areas*>& vec = wsr->areas;
  int start = 0;
  int end = vec.size() - 1;
  while(true)
  {
    int half = (start + end) >> 1;
    int index = vec[half]->index;

    if (index == target)
      return vec[half];

    if (index <= target)
      start = half + 1;
    else
      end = half - 1;

    if (start > end)
      break;
  }
  return nullptr;
}


//----------------------------------------------------------------------------//
// Utility function
//
//----------------------------------------------------------------------------//
df::world_region* find_world_region(int target)
{
  auto& vec_regions = df::global::world->world_data->regions;

  int start = 0;
  int end = vec_regions.size() - 1;
  while(true)
  {
    int half = (start + end) >> 1;
    int region_id = vec_regions[half]->index;

    if (region_id == target)
      return vec_regions[half];

    if (region_id <= target)
      start = half + 1;
    else
      end = half - 1;

    if (start > end)
      break;
  }
  return nullptr;
}
