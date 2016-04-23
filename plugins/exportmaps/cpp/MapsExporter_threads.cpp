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

#include <set>
#include <modules/World.h>
#include "../include/Mac_compat.h"
#include "../include/MapsExporter.h"
#include "../include/RegionDetails.h"
#include "../include/ExportedMap.h"

using namespace exportmaps_plugin;


/*****************************************************************************
 External functions declaration
 Here comes all the threads functions for the different maps.
*****************************************************************************/
extern void consumer_biome                     (void* arg);
extern void consumer_diplomacy                 (void* arg);
extern void consumer_drainage                  (void* arg);
extern void consumer_elevation                 (void* arg);
extern void consumer_elevation_water           (void* arg);
extern void consumer_evilness                  (void* arg);
extern void consumer_geology                   (void* arg);
extern void consumer_hydro                     (void* arg);
extern void consumer_nobility                  (void* arg);
extern void consumer_rainfall                  (void* arg);
extern void consumer_region                    (void* arg);
extern void consumer_salinity                  (void* arg);
extern void consumer_savagery                  (void* arg);
extern void consumer_sites                     (void* arg);
extern void consumer_temperature               (void* arg);
extern void consumer_trading                   (void* arg);
extern void consumer_vegetation                (void* arg);
extern void consumer_volcanism                 (void* arg);

extern void consumer_biome_type_raw            (void* arg);
extern void consumer_biome_region_raw          (void* arg);
extern void consumer_drainage_raw              (void* arg);
extern void consumer_elevation_raw             (void* arg);
extern void consumer_elevation_water_raw       (void* arg);
extern void consumer_evilness_raw              (void* arg);
extern void consumer_hydro_raw                 (void* arg);
extern void consumer_rainfall_raw              (void* arg);
extern void consumer_salinity_raw              (void* arg);
extern void consumer_savagery_raw              (void* arg);
extern void consumer_temperature_raw           (void* arg);
extern void consumer_vegetation_raw            (void* arg);
extern void consumer_volcanism_raw             (void* arg);

extern void consumer_elevation_heightmap       (void* arg);
extern void consumer_elevation_water_heightmap (void* arg);

/*****************************************************************************
*****************************************************************************/

//----------------------------------------------------------------------------//
// Return the mutex object used to synchronize the producer with the
// different threads
//----------------------------------------------------------------------------//
tthread::mutex& MapsExporter::get_mutex()
{
    return mtx;
}

//----------------------------------------------------------------------------//
// For each map that needs to be generated, a thread is created to do that.
// The MapsExporter object is passed to each thread so it can call its methods
//----------------------------------------------------------------------------//
void MapsExporter::setup_threads()
{
  if (maps_to_generate & MapType::TEMPERATURE)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_temperature,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::RAINFALL)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_rainfall,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::REGION)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_region,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::DRAINAGE)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_drainage,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::SAVAGERY)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_savagery,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::VOLCANISM)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_volcanism,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::VEGETATION)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_vegetation,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::EVILNESS)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_evilness,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::SALINITY)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_salinity,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::HYDROSPHERE)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_hydro,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::ELEVATION)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_elevation,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::ELEVATION_WATER)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_elevation_water,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::BIOME)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_biome,(void*)this);
    consumer_threads.push_back(pthread);
  }


/*
    if (maps_to_generate & MapType::GEOLOGY)
    {
        tthread::thread* pthread =  new tthread::thread(consumer_geology,(void*)this);
        consumer_threads.push_back(pthread);
    }
*/

  if (maps_to_generate & MapType::TRADING)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_trading,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::NOBILITY)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_nobility,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::DIPLOMACY)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_diplomacy,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate & MapType::SITES)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_sites,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }


  if (maps_to_generate_raw & MapTypeRaw::BIOME_TYPE_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_biome_type_raw,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::BIOME_REGION_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_biome_region_raw,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::DRAINAGE_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_drainage_raw,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::ELEVATION_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_elevation_raw,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::ELEVATION_WATER_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_elevation_water_raw,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::EVILNESS_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_evilness_raw,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::HYDROSPHERE_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_hydro_raw,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::RAINFALL_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_rainfall_raw,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::SALINITY_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_salinity_raw,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::SAVAGERY_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_savagery_raw,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::TEMPERATURE_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_temperature_raw,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::VOLCANISM_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_volcanism_raw,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_raw & MapTypeRaw::VEGETATION_RAW)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_vegetation_raw,(void*)this);
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_hm & MapTypeHeightMap::ELEVATION_HM)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_elevation_heightmap,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }

  if (maps_to_generate_hm & MapTypeHeightMap::ELEVATION_WATER_HM)
  {
    tthread::thread* pthread =  new tthread::thread(consumer_elevation_water_heightmap,
                                                    (void*)this
                                                    );
    consumer_threads.push_back(pthread);
  }
}


//----------------------------------------------------------------------------//
// Wait for all consumer threads to finish
//----------------------------------------------------------------------------//
void MapsExporter::wait_for_threads()
{
  for(std::list<tthread::thread *>::iterator i = consumer_threads.begin(); i != consumer_threads.end(); ++i)
  {
    tthread::thread* t = *i;
    if (t!= nullptr)
    {
      // Wait for the thread to finish
      t->join();
      // Destroy this thread
      delete t;
      // No need to check this thread anymore
      *i = nullptr;
    }
  }
}
