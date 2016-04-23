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
#include "../include/MapsExporter.h"
#include "../include/RegionDetails.h"
#include "../include/ExportedMap.h"

using namespace exportmaps_plugin;


//----------------------------------------------------------------------------//
// Check if the queue is empty
// As the producer and the consumer run in different threads
// we must synchronize them in order to avoid a data race
//----------------------------------------------------------------------------//
bool MapsExporter::is_temperature_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = temperature_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_rainfall_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = rainfall_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_region_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = region_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_drainage_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = drainage_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_savagery_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = savagery_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_volcanism_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = volcanism_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_vegetation_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = vegetation_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_evilness_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = evilness_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_salinity_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = salinity_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_hydro_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = hydro_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_elevation_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = elevation_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_elevation_water_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = elevation_water_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_biome_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = biome_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_geology_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = geology_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_trading_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = trading_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_nobility_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = nobility_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_diplomacy_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = diplomacy_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_sites_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = sites_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_biome_raw_type_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = biome_raw_type_queue.empty();
    mtx.unlock();
    return queue_empty;
}


//----------------------------------------------------------------------------//
bool MapsExporter::is_biome_raw_region_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = biome_raw_region_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_drainage_raw_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = drainage_raw_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_elevation_raw_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = elevation_raw_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_elevation_water_raw_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = elevation_water_raw_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_evilness_raw_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = evilness_raw_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_hydro_raw_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = hydro_raw_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_rainfall_raw_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = rainfall_raw_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_salinity_raw_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = salinity_raw_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_savagery_raw_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = savagery_raw_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//

bool MapsExporter::is_temperature_raw_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = temperature_raw_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_volcanism_raw_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = volcanism_raw_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_vegetation_raw_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = vegetation_raw_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_elevation_hm_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = elevation_hm_queue.empty();
    mtx.unlock();
    return queue_empty;
}

//----------------------------------------------------------------------------//
bool MapsExporter::is_elevation_water_hm_queue_empty()
{
    bool queue_empty;
    mtx.lock();
    queue_empty = elevation_water_hm_queue.empty();
    mtx.unlock();
    return queue_empty;
}
