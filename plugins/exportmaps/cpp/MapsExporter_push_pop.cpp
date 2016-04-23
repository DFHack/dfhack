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
// Push the end mark in the different queues to signal that there's no
// more data to be processed
//----------------------------------------------------------------------------//

void MapsExporter::push_end()
{
  if (maps_to_generate & MapType::TEMPERATURE)
    temperature_producer->produce_end(*this);

  if (maps_to_generate & MapType::RAINFALL)
    rainfall_producer->produce_end(*this);

  if (maps_to_generate & MapType::REGION)
    region_producer->produce_end(*this);

  if (maps_to_generate & MapType::DRAINAGE)
    drainage_producer->produce_end(*this);

  if (maps_to_generate & MapType::SAVAGERY)
    savagery_producer->produce_end(*this);

  if (maps_to_generate & MapType::VOLCANISM)
    volcanism_producer->produce_end(*this);

  if (maps_to_generate & MapType::VEGETATION)
    vegetation_producer->produce_end(*this);

  if (maps_to_generate & MapType::EVILNESS)
    evilness_producer->produce_end(*this);

  if (maps_to_generate & MapType::SALINITY)
    salinity_producer->produce_end(*this);

  if (maps_to_generate & MapType::HYDROSPHERE)
    hydro_producer->produce_end(*this);

  if (maps_to_generate & MapType::ELEVATION)
    elevation_producer->produce_end(*this);

  if (maps_to_generate & MapType::ELEVATION_WATER)
    elevation_water_producer->produce_end(*this);

  if (maps_to_generate & MapType::BIOME)
    biome_producer->produce_end(*this);

//    if (maps_to_generate & MapType::GEOLOGY)
//        geology_producer->produce_end(*this);

  if (maps_to_generate & MapType::TRADING)
    trading_producer->produce_end(*this);

  if (maps_to_generate & MapType::NOBILITY)
    nobility_producer->produce_end(*this);

  if (maps_to_generate & MapType::DIPLOMACY)
    diplomacy_producer->produce_end(*this);

  if (maps_to_generate & MapType::SITES)
    sites_producer->produce_end(*this);

//----------------------------------------------------------------------------//

  if (maps_to_generate_raw & MapTypeRaw::BIOME_TYPE_RAW)
    biome_type_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::BIOME_REGION_RAW)
    biome_region_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::DRAINAGE_RAW)
    drainage_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::ELEVATION_RAW)
    elevation_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::ELEVATION_WATER_RAW)
    elevation_water_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::EVILNESS_RAW)
    evilness_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::HYDROSPHERE_RAW)
    hydro_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::RAINFALL_RAW)
    rainfall_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::SALINITY_RAW)
    salinity_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::SAVAGERY_RAW)
    savagery_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::TEMPERATURE_RAW)
    temperature_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::VOLCANISM_RAW)
    volcanism_raw_producer->produce_end(*this);

  if (maps_to_generate_raw & MapTypeRaw::VEGETATION_RAW)
    vegetation_raw_producer->produce_end(*this);


//----------------------------------------------------------------------------//

  if (maps_to_generate_hm & MapTypeHeightMap::ELEVATION_HM)
    elevation_hm_producer->produce_end(*this);

  if (maps_to_generate_hm & MapTypeHeightMap::ELEVATION_WATER_HM)
    elevation_water_hm_producer->produce_end(*this);

}

//----------------------------------------------------------------------------//
// Push the world_region_details for this world cooridinate in
// the different queues so each one can process it
//----------------------------------------------------------------------------//
void MapsExporter::push_data(df::world_region_details* ptr_rd, // df::world_region_details data
                             int x,                            // world coordinate x
                             int y                             // world coordinate y
                             )
{
  // Each map has a different producer that generates different info from the data
  // that df::world_region_details contains

  // Push data for the temperature map
  if (maps_to_generate & MapType::TEMPERATURE)
    temperature_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the rainfall map
  if (maps_to_generate & MapType::RAINFALL)
    rainfall_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the region map
  if (maps_to_generate & MapType::REGION)
    region_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the drainage map
  if (maps_to_generate & MapType::DRAINAGE)
    drainage_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the savagery map
  if (maps_to_generate & MapType::SAVAGERY)
    savagery_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the volcanism map
  if (maps_to_generate & MapType::VOLCANISM)
    volcanism_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the vegetation map
  if (maps_to_generate & MapType::VEGETATION)
    vegetation_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the evilness map
  if (maps_to_generate & MapType::EVILNESS)
    evilness_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the salinity map
  if (maps_to_generate & MapType::SALINITY)
    salinity_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the hydrosphere map
  if (maps_to_generate & MapType::HYDROSPHERE)
    hydro_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the Elevation map
  if (maps_to_generate & MapType::ELEVATION)
    elevation_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the Elevation respecting water map
  if (maps_to_generate & MapType::ELEVATION_WATER)
    elevation_water_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for Biome map
  if (maps_to_generate & MapType::BIOME)
    biome_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for geology map
//    if (maps_to_generate & MapType::GEOLOGY)
//        geology_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for trading map
  if (maps_to_generate & MapType::TRADING)
    trading_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for nobility map
  if (maps_to_generate & MapType::NOBILITY)
    nobility_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for diplomacy map
  if (maps_to_generate & MapType::DIPLOMACY)
    diplomacy_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for sites map
  if (maps_to_generate & MapType::SITES)
    sites_producer->produce_data(*this,x,y,ptr_rd);

//----------------------------------------------------------------------------//

  // Push data for Biome type raw map
  if (maps_to_generate_raw & MapTypeRaw::BIOME_TYPE_RAW)
    biome_type_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for Biome region raw map
  if (maps_to_generate_raw & MapTypeRaw::BIOME_REGION_RAW)
    biome_region_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for drainage type raw map
  if (maps_to_generate_raw & MapTypeRaw::DRAINAGE_RAW)
    drainage_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the Elevation raw map
  if (maps_to_generate_raw & MapTypeRaw::ELEVATION_RAW)
    elevation_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the Elevation respecting water raw map
  if (maps_to_generate_raw & MapTypeRaw::ELEVATION_WATER_RAW)
    elevation_water_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the evilness raw map
  if (maps_to_generate_raw & MapTypeRaw::EVILNESS_RAW)
    evilness_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the hydrosphere raw map
  if (maps_to_generate_raw & MapTypeRaw::HYDROSPHERE_RAW)
    hydro_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the rainfall raw map
  if (maps_to_generate_raw & MapTypeRaw::RAINFALL_RAW)
    rainfall_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the salinity raw map
  if (maps_to_generate_raw & MapTypeRaw::SALINITY_RAW)
    salinity_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the savagery raw map
  if (maps_to_generate_raw & MapTypeRaw::SAVAGERY_RAW)
    savagery_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the temperature raw map
  if (maps_to_generate_raw & MapTypeRaw::TEMPERATURE_RAW)
    temperature_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the volcanism raw map
  if (maps_to_generate_raw & MapTypeRaw::VOLCANISM_RAW)
    volcanism_raw_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the vegetation raw map
  if (maps_to_generate_raw & MapTypeRaw::VEGETATION_RAW)
    vegetation_raw_producer->produce_data(*this,x,y,ptr_rd);


//----------------------------------------------------------------------------//

  // Push data for the Elevation heightmap
  if (maps_to_generate_hm & MapTypeHeightMap::ELEVATION_HM)
    elevation_hm_producer->produce_data(*this,x,y,ptr_rd);

  // Push data for the Elevation respecting water heightmap
  if (maps_to_generate_hm & MapTypeHeightMap::ELEVATION_WATER_HM)
    elevation_water_hm_producer->produce_data(*this,x,y,ptr_rd);

}

//----------------------------------------------------------------------------//
// Push the data generated by each producer in its respective queue.
// As the producer and the consumer run in different threads, we must
// synchronize them to avoid a race condition
//----------------------------------------------------------------------------//
void MapsExporter::push_temperature(RegionDetailsBiome& rdb)
{
    mtx.lock();
    temperature_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_rainfall(RegionDetailsBiome& rdb)
{
    mtx.lock();
    rainfall_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_region(RegionDetailsElevationWater& rdb)
{
    mtx.lock();
    region_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_drainage(RegionDetailsBiome& rdb)
{
    mtx.lock();
    drainage_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_savagery(RegionDetailsBiome& rdb)
{
    mtx.lock();
    savagery_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_volcanism(RegionDetailsBiome& rdb)
{
    mtx.lock();
    volcanism_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_vegetation(RegionDetailsBiome& rdb)
{
    mtx.lock();
    vegetation_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_evilness(RegionDetailsBiome& rdb)
{
    mtx.lock();
    evilness_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_salinity(RegionDetailsBiome& rdb)
{
    mtx.lock();
    salinity_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_hydro(RegionDetailsElevationWater& rdb)
{
    mtx.lock();
    hydro_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_elevation(RegionDetailsElevation& rde)
{
    mtx.lock();
    this->elevation_queue.push(rde);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_elevation_water(RegionDetailsElevationWater& rdew)
{
    mtx.lock();
    this->elevation_water_queue.push(rdew);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_biome(RegionDetailsBiome& rdb)
{
    mtx.lock();
    this->biome_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_geology(RegionDetailsGeology& rdg)
{
    mtx.lock();
    this->geology_queue.push(rdg);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_trading(RegionDetailsElevationWater& rdb)
{
    mtx.lock();
    trading_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_nobility(RegionDetailsElevationWater& rdb)
{
    mtx.lock();
    nobility_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_diplomacy(RegionDetailsElevationWater& rdb)
{
    mtx.lock();
    diplomacy_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_sites(RegionDetailsElevationWater& rdb)
{
    mtx.lock();
    sites_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_biome_type_raw(RegionDetailsBiome& rdb)
{
    mtx.lock();
    this->biome_raw_type_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_biome_region_raw(RegionDetailsBiome& rdb)
{
    mtx.lock();
    this->biome_raw_region_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_drainage_raw(RegionDetailsBiome& rdb)
{
    mtx.lock();
    this->drainage_raw_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_elevation_raw(RegionDetailsElevation& rde)
{
    mtx.lock();
    this->elevation_raw_queue.push(rde);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_elevation_water_raw(RegionDetailsElevationWater& rdew)
{
    mtx.lock();
    this->elevation_water_raw_queue.push(rdew);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_evilness_raw(RegionDetailsBiome& rdb)
{
    mtx.lock();
    evilness_raw_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_hydro_raw(RegionDetailsElevationWater& rdb)
{
    mtx.lock();
    hydro_raw_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_rainfall_raw(RegionDetailsBiome& rdb)
{
    mtx.lock();
    rainfall_raw_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_salinity_raw(RegionDetailsBiome& rdb)
{
    mtx.lock();
    salinity_raw_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_savagery_raw(RegionDetailsBiome& rdb)
{
    mtx.lock();
    savagery_raw_queue.push(rdb);
    mtx.unlock();
}

void MapsExporter::push_temperature_raw(RegionDetailsBiome& rdb)
{
    mtx.lock();
    temperature_raw_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_volcanism_raw(RegionDetailsBiome& rdb)
{
    mtx.lock();
    volcanism_raw_queue.push(rdb);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_vegetation_raw(RegionDetailsBiome& rdb)
{
    mtx.lock();
    vegetation_raw_queue.push(rdb);
    mtx.unlock();
}


//----------------------------------------------------------------------------//

void MapsExporter::push_elevation_hm(RegionDetailsElevation& rde)
{
    mtx.lock();
    this->elevation_hm_queue.push(rde);
    mtx.unlock();
}

//----------------------------------------------------------------------------//

void MapsExporter::push_elevation_water_hm(RegionDetailsElevationWater& rdew)
{
    mtx.lock();
    this->elevation_water_hm_queue.push(rdew);
    mtx.unlock();
}


//----------------------------------------------------------------------------//
// Pop data from each queue.
// As the producer and the consumer run in different threads, we must
// synchronize them to avoid a race condition
//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_temperature()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->temperature_queue.front();
    this->temperature_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_rainfall()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->rainfall_queue.front();
    this->rainfall_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsElevationWater MapsExporter::pop_region()
{
    mtx.lock();
    RegionDetailsElevationWater rdb = this->region_queue.front();
    this->region_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_drainage()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->drainage_queue.front();
    this->drainage_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_savagery()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->savagery_queue.front();
    this->savagery_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_volcanism()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->volcanism_queue.front();
    this->volcanism_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_vegetation()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->vegetation_queue.front();
    this->vegetation_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_evilness()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->evilness_queue.front();
    this->evilness_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_salinity()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->salinity_queue.front();
    this->salinity_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsElevationWater MapsExporter::pop_hydro()
{
    mtx.lock();
    RegionDetailsElevationWater rdb = this->hydro_queue.front();
    this->hydro_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsElevation MapsExporter::pop_elevation()
{
    mtx.lock();
    RegionDetailsElevation rde = this->elevation_queue.front();
    this->elevation_queue.pop();
    mtx.unlock();

    return rde;
}

//----------------------------------------------------------------------------//

RegionDetailsElevationWater MapsExporter::pop_elevation_water()
{
    mtx.lock();
    RegionDetailsElevationWater rdew = this->elevation_water_queue.front();
    this->elevation_water_queue.pop();
    mtx.unlock();

    return rdew;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_biome()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->biome_queue.front();
    this->biome_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsGeology MapsExporter::pop_geology()
{
    mtx.lock();
    RegionDetailsGeology rdg = this->geology_queue.front();
    this->geology_queue.pop();
    mtx.unlock();

    return rdg;
}

//----------------------------------------------------------------------------//

RegionDetailsElevationWater MapsExporter::pop_trading()
{
    mtx.lock();
    RegionDetailsElevationWater rdb = this->trading_queue.front();
    this->trading_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsElevationWater MapsExporter::pop_nobility()
{
    mtx.lock();
    RegionDetailsElevationWater rdb = this->nobility_queue.front();
    this->nobility_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsElevationWater MapsExporter::pop_diplomacy()
{
    mtx.lock();
    RegionDetailsElevationWater rdb = this->diplomacy_queue.front();
    this->diplomacy_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsElevationWater MapsExporter::pop_sites()
{
    mtx.lock();
    RegionDetailsElevationWater rdb = this->sites_queue.front();
    this->sites_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_biome_type_raw()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->biome_raw_type_queue.front();
    this->biome_raw_type_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_biome_region_raw()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->biome_raw_region_queue.front();
    this->biome_raw_region_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_drainage_raw()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->drainage_raw_queue.front();
    this->drainage_raw_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsElevation MapsExporter::pop_elevation_raw()
{
    mtx.lock();
    RegionDetailsElevation rde = this->elevation_raw_queue.front();
    this->elevation_raw_queue.pop();
    mtx.unlock();

    return rde;
}

//----------------------------------------------------------------------------//

RegionDetailsElevationWater MapsExporter::pop_elevation_water_raw()
{
    mtx.lock();
    RegionDetailsElevationWater rdew = this->elevation_water_raw_queue.front();
    this->elevation_water_raw_queue.pop();
    mtx.unlock();

    return rdew;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_evilness_raw()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->evilness_raw_queue.front();
    this->evilness_raw_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsElevationWater MapsExporter::pop_hydro_raw()
{
    mtx.lock();
    RegionDetailsElevationWater rdb = this->hydro_raw_queue.front();
    this->hydro_raw_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_rainfall_raw()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->rainfall_raw_queue.front();
    this->rainfall_raw_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_salinity_raw()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->salinity_raw_queue.front();
    this->salinity_raw_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_savagery_raw()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->savagery_raw_queue.front();
    this->savagery_raw_queue.pop();
    mtx.unlock();

    return rdb;
}

RegionDetailsBiome MapsExporter::pop_temperature_raw()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->temperature_raw_queue.front();
    this->temperature_raw_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_volcanism_raw()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->volcanism_raw_queue.front();
    this->volcanism_raw_queue.pop();
    mtx.unlock();

    return rdb;
}

//----------------------------------------------------------------------------//

RegionDetailsBiome MapsExporter::pop_vegetation_raw()
{
    mtx.lock();
    RegionDetailsBiome rdb = this->vegetation_raw_queue.front();
    this->vegetation_raw_queue.pop();
    mtx.unlock();

    return rdb;
}


//----------------------------------------------------------------------------//

RegionDetailsElevation MapsExporter::pop_elevation_hm()
{
    mtx.lock();
    RegionDetailsElevation rde = this->elevation_hm_queue.front();
    this->elevation_hm_queue.pop();
    mtx.unlock();

    return rde;
}

//----------------------------------------------------------------------------//

RegionDetailsElevationWater MapsExporter::pop_elevation_water_hm()
{
    mtx.lock();
    RegionDetailsElevationWater rdew = this->elevation_water_hm_queue.front();
    this->elevation_water_hm_queue.pop();
    mtx.unlock();

    return rdew;
}

