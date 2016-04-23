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

using namespace DFHack;
using namespace exportmaps_plugin;


//----------------------------------------------------------------------------//
// Set up the maps to be generated according to the command line parameters
// that we got from DFHack console
//----------------------------------------------------------------------------//
void MapsExporter::setup_maps(uint32_t maps,     // Graphical maps to generate
                              uint32_t maps_raw, // Raw maps to generate
                              uint32_t maps_hm   // Height maps to generate
                              )
{
  // Copy the data received from DFHack command line
  maps_to_generate = maps;
  maps_to_generate_raw = maps_raw;
  maps_to_generate_hm = maps_hm;

  // Get the date elements
  int year  = World::ReadCurrentYear();
  int month = World::ReadCurrentMonth();
  int day   = World::ReadCurrentDay();


  // Compose date
  std::stringstream ss_date;
  ss_date << "-" << year << "-" << month << "-" << day;
  std::string current_date = ss_date.str();
  std::string region_name = World::ReadWorldFolder();

  if (maps_to_generate & MapType::TEMPERATURE)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-tmp.png";

    temperature_producer.reset(new ProducerTemperature);
    if (!temperature_producer) throw std::bad_alloc();

    temperature_map.reset(new ExportedMapDF(file_name.str(),
                                            df::global::world->world_data->world_width,
                                            df::global::world->world_data->world_height,
                                            MapType::TEMPERATURE
                                            )
                          );
    if (!temperature_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::RAINFALL)
  {
    // Compose filename
    std::stringstream file_name;

    file_name << region_name << current_date << "-rain.png";
    rainfall_producer.reset(new ProducerRainfall);
    if (!rainfall_producer) throw std::bad_alloc();

    rainfall_map.reset(new ExportedMapDF(file_name.str(),
                                         df::global::world->world_data->world_width,
                                         df::global::world->world_data->world_height,
                                         MapType::RAINFALL
                                         )
                       );

    if (!rainfall_map) throw std::bad_alloc();
  }

  //----------------------------------------------------------------------------//

    if (maps_to_generate & MapType::REGION)
    {
      // Compose filename
      std::stringstream file_name;

      file_name << region_name << current_date << "-rgn.png";
      region_producer.reset(new ProducerRegion);
      if (!region_producer) throw std::bad_alloc();

      region_map.reset(new ExportedMapDF(file_name.str(),
                                         df::global::world->world_data->world_width,
                                         df::global::world->world_data->world_height,
                                         MapType::REGION
                                         )
                       );

      if (!region_map) throw std::bad_alloc();
    }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::DRAINAGE)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-drn.png";

    drainage_producer.reset(new ProducerDrainage);
    if (!drainage_producer) throw std::bad_alloc();

    drainage_map.reset(new ExportedMapDF(file_name.str(),
                                         df::global::world->world_data->world_width,
                                         df::global::world->world_data->world_height,
                                         MapType::DRAINAGE
                                         )
                       );

    if (!drainage_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::SAVAGERY)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-sav.png";

    savagery_producer.reset(new ProducerSavagery);
    if (!savagery_producer) throw std::bad_alloc();

    savagery_map.reset(new ExportedMapDF(file_name.str(),
                                         df::global::world->world_data->world_width,
                                         df::global::world->world_data->world_height,
                                         MapType::SAVAGERY
                                         )
                       );

    if (!savagery_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::VOLCANISM)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-vol.png";

    volcanism_producer.reset(new ProducerVolcanism);
    if (!volcanism_producer) throw std::bad_alloc();

    volcanism_map.reset(new ExportedMapDF(file_name.str(),
                                          df::global::world->world_data->world_width,
                                          df::global::world->world_data->world_height,
                                          MapType::VOLCANISM
                                          )
                        );

    if (!volcanism_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::VEGETATION)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-veg.png";
    vegetation_producer.reset(new ProducerVegetation);
    if (!vegetation_producer) throw std::bad_alloc();

    vegetation_map.reset(new ExportedMapDF(file_name.str(),
                                           df::global::world->world_data->world_width,
                                           df::global::world->world_data->world_height,
                                           MapType::VEGETATION
                                           )
                         );

    if (!vegetation_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::EVILNESS)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-evil.png";

    evilness_producer.reset(new ProducerEvilness);
    if (!evilness_producer) throw std::bad_alloc();

    evilness_map.reset(new ExportedMapDF(file_name.str(),
                                         df::global::world->world_data->world_width,
                                         df::global::world->world_data->world_height,
                                         MapType::EVILNESS
                                         )
                       );

    if (!evilness_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::SALINITY)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-sal.png";

    salinity_producer.reset(new ProducerSalinity);
    if (!salinity_producer) throw std::bad_alloc();

    salinity_map.reset(new ExportedMapDF(file_name.str(),
                                         df::global::world->world_data->world_width,
                                         df::global::world->world_data->world_height,
                                         MapType::SALINITY
                                         )
                       );

    if (!salinity_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::HYDROSPHERE)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-hyd.png";

    hydro_producer.reset(new ProducerHydro);
    if (!hydro_producer) throw std::bad_alloc();

    hydro_map.reset(new ExportedMapDF(file_name.str(),
                                      df::global::world->world_data->world_width,
                                      df::global::world->world_data->world_height,
                                      MapType::HYDROSPHERE
                                      )
                    );

    if (!hydro_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::ELEVATION)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-el.png";
    elevation_producer.reset(new ProducerElevation);
    if (!elevation_producer) throw std::bad_alloc();

    elevation_map.reset(new ExportedMapDF(file_name.str(),
                                          df::global::world->world_data->world_width,
                                          df::global::world->world_data->world_height,
                                          MapType::ELEVATION
                                          )
                        );

    if (!elevation_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::ELEVATION_WATER)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-elw.png";
    elevation_water_producer.reset(new ProducerElevationWater);
    if (!elevation_water_producer) throw std::bad_alloc();

    elevation_water_map.reset(new ExportedMapDF(file_name.str(),
                                                df::global::world->world_data->world_width,
                                                df::global::world->world_data->world_height,
                                                MapType::ELEVATION_WATER
                                                )
                              );

    if (!elevation_water_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::BIOME)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-bm.png";
    biome_producer.reset(new ProducerBiome);
    if (!biome_producer) throw std::bad_alloc();

    biome_map.reset(new ExportedMapDF(file_name.str(),
                                      df::global::world->world_data->world_width,
                                      df::global::world->world_data->world_height,
                                      MapType::BIOME
                                      )
                    );

    if (!biome_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//
/*
    if (maps_to_generate & MapType::GEOLOGY)
    {
        // Compose filename
        std::stringstream file_name;
        file_name << region_name << current_date << "-geology.png";
        geology_producer.reset(new ProducerGeology);
        if (!geology_producer) throw std::bad_alloc();

        geology_map.reset(new ExportedMapDF(file_name.str(),
                                            df::global::world->world_data->world_width,
                                            df::global::world->world_data->world_height,
                                            MapType::GEOLOGY));
        if (!geology_map) throw std::bad_alloc();
    }
*/

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::TRADING)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-trd.png";
    trading_producer.reset(new ProducerTrading);
    if (!trading_producer) throw std::bad_alloc();

    trading_map.reset(new ExportedMapDF(file_name.str(),
                                        df::global::world->world_data->world_width,
                                        df::global::world->world_data->world_height,
                                        MapType::TRADING
                                        )
                      );

    if (!trading_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::NOBILITY)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-nob.png";
    nobility_producer.reset(new ProducerNobility);
    if (!nobility_producer) throw std::bad_alloc();

    nobility_map.reset(new ExportedMapDF(file_name.str(),
                                         df::global::world->world_data->world_width,
                                         df::global::world->world_data->world_height,
                                         MapType::NOBILITY
                                         )
                       );

    if (!nobility_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::DIPLOMACY)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-dip.png";
    diplomacy_producer.reset(new ProducerDiplomacy);
    if (!diplomacy_producer) throw std::bad_alloc();

    diplomacy_map.reset(new ExportedMapDF(file_name.str(),
                                          df::global::world->world_data->world_width,
                                          df::global::world->world_data->world_height,
                                          MapType::DIPLOMACY
                                          )
                        );

    if (!diplomacy_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_to_generate & MapType::SITES)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-str.png";
    sites_producer.reset(new ProducerSites);
    if (!sites_producer) throw std::bad_alloc();

    sites_map.reset(new ExportedMapDF(file_name.str(),
                                      df::global::world->world_data->world_width,
                                      df::global::world->world_data->world_height,
                                      MapType::SITES
                                      )
                    );

    if (!sites_map) throw std::bad_alloc();
  }

  ////////////////////////////////////
  // Raw Maps
  ////////////////////////////////////

  if (maps_raw & MapTypeRaw::BIOME_TYPE_RAW)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-biome-type.raw";
    biome_type_raw_producer.reset(new ProducerBiomeRawType);
    if (!biome_type_raw_producer) throw std::bad_alloc();

    biome_type_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                                df::global::world->world_data->world_width,
                                                df::global::world->world_data->world_height,
                                                MapTypeRaw::BIOME_TYPE_RAW
                                                )
                             );

    if (!biome_type_raw_map) throw std::bad_alloc();
  }

//----------------------------------------------------------------------------//

  if (maps_raw & MapTypeRaw::BIOME_REGION_RAW)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-biome-region.raw";
    biome_region_raw_producer.reset(new ProducerBiomeRawRegion);
    if (!biome_region_raw_producer) throw std::bad_alloc();

    biome_region_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                                  df::global::world->world_data->world_width,
                                                  df::global::world->world_data->world_height,
                                                  MapTypeRaw::BIOME_REGION_RAW
                                                  )
                               );

    if (!biome_region_raw_map) throw std::bad_alloc();
  }

 //----------------------------------------------------------------------------//

  if (maps_raw & MapTypeRaw::DRAINAGE_RAW)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-drainage.raw";
    drainage_raw_producer.reset(new ProducerDrainageRaw);
    if (!drainage_raw_producer) throw std::bad_alloc();

    drainage_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                              df::global::world->world_data->world_width,
                                              df::global::world->world_data->world_height,
                                              MapTypeRaw::DRAINAGE_RAW
                                              )
                           );

    if (!drainage_raw_map) throw std::bad_alloc();
  }

  //----------------------------------------------------------------------------//

    if (maps_to_generate_raw & MapTypeRaw::ELEVATION_RAW)
    {
      // Compose filename
      std::stringstream file_name;
      file_name << region_name << current_date << "-elevation.raw";
      elevation_raw_producer.reset(new ProducerElevationRaw);
      if (!elevation_raw_producer) throw std::bad_alloc();

      elevation_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                                 df::global::world->world_data->world_width,
                                                 df::global::world->world_data->world_height,
                                                 MapTypeRaw::ELEVATION_RAW
                                                 )
                              );

      if (!elevation_raw_map) throw std::bad_alloc();
    }

  //----------------------------------------------------------------------------//

  if (maps_to_generate_raw & MapTypeRaw::ELEVATION_WATER_RAW)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-elevation-water.raw";
    elevation_water_raw_producer.reset(new ProducerElevationWaterRaw);
    if (!elevation_water_raw_producer) throw std::bad_alloc();

    elevation_water_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                                     df::global::world->world_data->world_width,
                                                     df::global::world->world_data->world_height,
                                                     MapTypeRaw::ELEVATION_WATER_RAW
                                                     )
                                  );

    if (!elevation_water_raw_map) throw std::bad_alloc();
  }

  //----------------------------------------------------------------------------//

  if (maps_to_generate_raw & MapTypeRaw::EVILNESS_RAW)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-evilness.raw";

    evilness_raw_producer.reset(new ProducerEvilnessRaw);
    if (!evilness_raw_producer) throw std::bad_alloc();
    evilness_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                              df::global::world->world_data->world_width,
                                              df::global::world->world_data->world_height,
                                              MapTypeRaw::EVILNESS_RAW
                                              )
                           );

    if (!evilness_raw_map) throw std::bad_alloc();
  }

  //----------------------------------------------------------------------------//

  if (maps_to_generate_raw & MapTypeRaw::HYDROSPHERE_RAW)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-hydrology.raw";

    hydro_raw_producer.reset(new ProducerHydroRaw);
    if (!hydro_raw_producer) throw std::bad_alloc();

    hydro_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                           df::global::world->world_data->world_width,
                                           df::global::world->world_data->world_height,
                                           MapTypeRaw::HYDROSPHERE_RAW
                                           )
                        );
    if (!hydro_raw_map) throw std::bad_alloc();
  }

  //----------------------------------------------------------------------------//

  if (maps_to_generate_raw & MapTypeRaw::RAINFALL_RAW)
  {
    // Compose filename
    std::stringstream file_name;

    file_name << region_name << current_date << "-rainfall.raw";
    rainfall_raw_producer.reset(new ProducerRainfallRaw);
    if (!rainfall_raw_producer) throw std::bad_alloc();

    rainfall_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                              df::global::world->world_data->world_width,
                                              df::global::world->world_data->world_height,
                                              MapTypeRaw::RAINFALL_RAW
                                              )
                           );

    if (!rainfall_raw_map) throw std::bad_alloc();
  }

  //----------------------------------------------------------------------------//

  if (maps_to_generate_raw & MapTypeRaw::SALINITY_RAW)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-salinity.raw";

    salinity_raw_producer.reset(new ProducerSalinityRaw);
    if (!salinity_raw_producer) throw std::bad_alloc();

    salinity_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                              df::global::world->world_data->world_width,
                                              df::global::world->world_data->world_height,
                                              MapTypeRaw::SALINITY_RAW
                                              )
                           );
    if (!salinity_raw_map) throw std::bad_alloc();
  }

  //----------------------------------------------------------------------------//

    if (maps_to_generate_raw & MapTypeRaw::SAVAGERY_RAW)
    {
      // Compose filename
      std::stringstream file_name;
      file_name << region_name << current_date << "-savagery.raw";

      savagery_raw_producer.reset(new ProducerSavageryRaw);
      if (!savagery_raw_producer) throw std::bad_alloc();

      savagery_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                                df::global::world->world_data->world_width,
                                                df::global::world->world_data->world_height,
                                                MapTypeRaw::SAVAGERY_RAW
                                                )
                             );

      if (!savagery_raw_map) throw std::bad_alloc();
    }

  //----------------------------------------------------------------------------//

  if (maps_to_generate_raw & MapTypeRaw::TEMPERATURE_RAW)
  {
    // Compose filename
    std::stringstream file_name;
    file_name << region_name << current_date << "-temperature.raw";

    temperature_raw_producer.reset(new ProducerTemperatureRaw);
    if (!temperature_raw_producer) throw std::bad_alloc();

    temperature_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                            df::global::world->world_data->world_width,
                                            df::global::world->world_data->world_height,
                                            MapTypeRaw::TEMPERATURE_RAW));
    if (!temperature_raw_map) throw std::bad_alloc();
  }

  //----------------------------------------------------------------------------//

    if (maps_to_generate_raw & MapTypeRaw::VOLCANISM_RAW)
    {
      // Compose filename
      std::stringstream file_name;
      file_name << region_name << current_date << "-volcanism.raw";

      volcanism_raw_producer.reset(new ProducerVolcanismRaw);
      if (!volcanism_raw_producer) throw std::bad_alloc();

      volcanism_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                                 df::global::world->world_data->world_width,
                                                 df::global::world->world_data->world_height,
                                                 MapTypeRaw::VOLCANISM_RAW
                                                 )
                              );

      if (!volcanism_raw_map) throw std::bad_alloc();
    }

  //----------------------------------------------------------------------------//

    if (maps_to_generate_raw & MapTypeRaw::VEGETATION_RAW)
    {
      // Compose filename
      std::stringstream file_name;
      file_name << region_name << current_date << "-vegetation.raw";
      vegetation_raw_producer.reset(new ProducerVegetationRaw);
      if (!vegetation_raw_producer) throw std::bad_alloc();

      vegetation_raw_map.reset(new ExportedMapRaw(file_name.str(),
                                                  df::global::world->world_data->world_width,
                                                  df::global::world->world_data->world_height,
                                                  MapTypeRaw::VEGETATION_RAW
                                                  )
                               );

      if (!vegetation_raw_map) throw std::bad_alloc();
    }


  //----------------------------------------------------------------------------//

    if (maps_to_generate_hm & MapTypeHeightMap::ELEVATION_HM)
    {
      // Compose filename
      std::stringstream file_name;
      file_name << region_name << current_date << "-elevation-heightmap.png";
      elevation_hm_producer.reset(new ProducerElevationHeightMap);
      if (!elevation_hm_producer) throw std::bad_alloc();

      elevation_hm_map.reset(new ExportedMapHM(file_name.str(),
                                               df::global::world->world_data->world_width,
                                               df::global::world->world_data->world_height,
                                               MapTypeHeightMap::ELEVATION_HM
                                               )
                             );

      if (!elevation_hm_map) throw std::bad_alloc();
    }

  //----------------------------------------------------------------------------//

    if (maps_to_generate_hm & MapTypeHeightMap::ELEVATION_WATER_HM)
    {
      // Compose filename
      std::stringstream file_name;
      file_name << region_name << current_date << "-elevation-water-heightmap.png";
      elevation_water_hm_producer.reset(new ProducerElevationWaterHeightMap);
      if (!elevation_water_hm_producer) throw std::bad_alloc();

      elevation_water_hm_map.reset(new ExportedMapHM(file_name.str(),
                                                     df::global::world->world_data->world_width,
                                                     df::global::world->world_data->world_height,
                                                     MapTypeHeightMap::ELEVATION_WATER_HM
                                                     )
                                   );

      if (!elevation_water_hm_map) throw std::bad_alloc();
    }



}

