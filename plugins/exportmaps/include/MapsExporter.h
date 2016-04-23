/* zlib.h -- interface of the 'zlib' general purpose compression library
  version 1.2.2, October 3rd, 2004

  Copyright (C) 1995-2004 Jean-loup Gailly and Mark Adler

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

  Jean-loup Gailly jloup@gzip.org
  Mark Adler madler@alumni.caltech.edu

*/

#ifndef MAPSEXPORTER_H
#define MAPSEXPORTER_H

#include <tinythread.h>

#include <memory>
#include <queue>
#include <list>

#include <BitArray.h>

#include "Producer.h"
#include "RegionDetails.h"
#include "ExportedMap.h"
#include "Logger.h"

using namespace std;

class Logger;

namespace exportmaps_plugin
{

  class MapsExporter
  {
    // Producer data queues for each different map

    // DF maps
    queue<class RegionDetailsBiome>           biome_queue;
    queue<class RegionDetailsElevationWater>  diplomacy_queue;
    queue<class RegionDetailsBiome>           drainage_queue;
    queue<class RegionDetailsElevation>       elevation_queue;
    queue<class RegionDetailsElevationWater>  elevation_water_queue;
    queue<class RegionDetailsBiome>           evilness_queue;
    queue<class RegionDetailsGeology>         geology_queue;
    queue<class RegionDetailsElevationWater>  hydro_queue;
    queue<class RegionDetailsElevationWater>  nobility_queue;
    queue<class RegionDetailsBiome>           rainfall_queue;
    queue<class RegionDetailsElevationWater>  region_queue;
    queue<class RegionDetailsBiome>           salinity_queue;
    queue<class RegionDetailsBiome>           savagery_queue;
    queue<class RegionDetailsElevationWater>  sites_queue;
    queue<class RegionDetailsBiome>           temperature_queue;
    queue<class RegionDetailsElevationWater>  trading_queue;
    queue<class RegionDetailsBiome>           vegetation_queue;
    queue<class RegionDetailsBiome>           volcanism_queue;

    // Raw maps
    queue<class RegionDetailsBiome>           biome_raw_type_queue;
    queue<class RegionDetailsBiome>           biome_raw_region_queue;
    queue<class RegionDetailsBiome>           drainage_raw_queue;
    queue<class RegionDetailsElevation>       elevation_raw_queue;
    queue<class RegionDetailsElevationWater>  elevation_water_raw_queue;
    queue<class RegionDetailsBiome>           evilness_raw_queue;
    queue<class RegionDetailsElevationWater>  hydro_raw_queue;
    queue<class RegionDetailsBiome>           rainfall_raw_queue;
    queue<class RegionDetailsBiome>           salinity_raw_queue;
    queue<class RegionDetailsBiome>           savagery_raw_queue;
    queue<class RegionDetailsBiome>           temperature_raw_queue;
    queue<class RegionDetailsBiome>           vegetation_raw_queue;
    queue<class RegionDetailsBiome>           volcanism_raw_queue;

    // Heightmaps
    queue<class RegionDetailsElevation>       elevation_hm_queue;
    queue<class RegionDetailsElevationWater>  elevation_water_hm_queue;

    // Enable the generation of each different map
    uint32_t maps_to_generate;      // DF style maps
    uint32_t maps_to_generate_raw;  // Raw binary maps
    uint32_t maps_to_generate_hm;   // Heightmap style maps


    // Different DF data producer for each map
    unique_ptr<class ProducerBiome>                   biome_producer;
    unique_ptr<class ProducerDiplomacy>               diplomacy_producer;
    unique_ptr<class ProducerDrainage>                drainage_producer;
    unique_ptr<class ProducerElevation>               elevation_producer;
    unique_ptr<class ProducerElevationWater>          elevation_water_producer;
    unique_ptr<class ProducerEvilness>                evilness_producer;
    unique_ptr<class ProducerGeology>                 geology_producer;
    unique_ptr<class ProducerHydro>                   hydro_producer;
    unique_ptr<class ProducerNobility>                nobility_producer;
    unique_ptr<class ProducerRainfall>                rainfall_producer;
    unique_ptr<class ProducerRegion>                  region_producer;
    unique_ptr<class ProducerSalinity>                salinity_producer;
    unique_ptr<class ProducerSavagery>                savagery_producer;
    unique_ptr<class ProducerSites>                   sites_producer;
    unique_ptr<class ProducerTemperature>             temperature_producer;
    unique_ptr<class ProducerTrading>                 trading_producer;
    unique_ptr<class ProducerVegetation>              vegetation_producer;
    unique_ptr<class ProducerVolcanism>               volcanism_producer;

    unique_ptr<class ProducerBiomeRawType>            biome_type_raw_producer;
    unique_ptr<class ProducerBiomeRawRegion>          biome_region_raw_producer;
    unique_ptr<class ProducerDrainageRaw>             drainage_raw_producer;
    unique_ptr<class ProducerElevationRaw>            elevation_raw_producer;
    unique_ptr<class ProducerElevationWaterRaw>       elevation_water_raw_producer;
    unique_ptr<class ProducerEvilnessRaw>             evilness_raw_producer;
    unique_ptr<class ProducerHydroRaw>                hydro_raw_producer;
    unique_ptr<class ProducerRainfallRaw>             rainfall_raw_producer;
    unique_ptr<class ProducerSalinityRaw>             salinity_raw_producer;
    unique_ptr<class ProducerSavageryRaw>             savagery_raw_producer;
    unique_ptr<class ProducerTemperatureRaw>          temperature_raw_producer;
    unique_ptr<class ProducerVegetationRaw>           vegetation_raw_producer;
    unique_ptr<class ProducerVolcanismRaw>            volcanism_raw_producer;

    unique_ptr<class ProducerElevationHeightMap>      elevation_hm_producer;
    unique_ptr<class ProducerElevationWaterHeightMap> elevation_water_hm_producer;

    // Pointers to every map that can be exported
    unique_ptr<class ExportedMapBase>           biome_map;
    unique_ptr<class ExportedMapDF>             diplomacy_map;
    unique_ptr<class ExportedMapBase>           drainage_map;
    unique_ptr<class ExportedMapBase>           elevation_map;
    unique_ptr<class ExportedMapBase>           elevation_water_map;
    unique_ptr<class ExportedMapBase>           evilness_map;
    unique_ptr<class ExportedMapDF>             geology_map;
    unique_ptr<class ExportedMapBase>           hydro_map;
    unique_ptr<class ExportedMapDF>             nobility_map;
    unique_ptr<class ExportedMapBase>           rainfall_map;
    unique_ptr<class ExportedMapBase>           region_map;
    unique_ptr<class ExportedMapBase>           salinity_map;
    unique_ptr<class ExportedMapBase>           savagery_map;
    unique_ptr<class ExportedMapDF>             sites_map;
    unique_ptr<class ExportedMapBase>           temperature_map;
    unique_ptr<class ExportedMapDF>             trading_map;
    unique_ptr<class ExportedMapBase>           volcanism_map;
    unique_ptr<class ExportedMapBase>           vegetation_map;

    unique_ptr<class ExportedMapBase>           biome_type_raw_map;
    unique_ptr<class ExportedMapBase>           biome_region_raw_map;
    unique_ptr<class ExportedMapBase>           drainage_raw_map;
    unique_ptr<class ExportedMapBase>           elevation_raw_map;
    unique_ptr<class ExportedMapBase>           elevation_water_raw_map;
    unique_ptr<class ExportedMapBase>           evilness_raw_map;
    unique_ptr<class ExportedMapBase>           hydro_raw_map;
    unique_ptr<class ExportedMapBase>           rainfall_raw_map;
    unique_ptr<class ExportedMapBase>           salinity_raw_map;
    unique_ptr<class ExportedMapBase>           savagery_raw_map;
    unique_ptr<class ExportedMapBase>           temperature_raw_map;
    unique_ptr<class ExportedMapBase>           volcanism_raw_map;
    unique_ptr<class ExportedMapBase>           vegetation_raw_map;

    unique_ptr<class ExportedMapBase>           elevation_hm_map;
    unique_ptr<class ExportedMapBase>           elevation_water_hm_map;

    // Thread synchronization between producer and consumers
    // accessing the different data queues
    tthread::mutex mtx;

    // Threads for each map type
    list<tthread::thread*> consumer_threads;

    // Used to display the percentage of the generation of
    // sites, diplomacy, nobility and trade maps
    int percentage_sites;
    int percentage_trade;
    int percentage_nobility;
    int percentage_diplomacy;

  public:

    void setup_maps(uint32_t maps_to_generate,     // Graphical maps to generate
                    uint32_t maps_to_generate_raw, // Raw maps to generate
                    uint32_t maps_to_generate_hm   // Heightmaps to generate
                    );

    void cleanup();

    bool generate_maps(Logger& logger);

    void write_maps_to_disk();

    void push_data(df::world_region_details* ptr_rd, int x, int y);

    void push_end();

    // Push methods

    void push_biome              (RegionDetailsBiome&          rdb);
    void push_diplomacy          (RegionDetailsElevationWater& rdg);
    void push_drainage           (RegionDetailsBiome&          rdg);
    void push_elevation          (RegionDetailsElevation&      rde);
    void push_elevation_water    (RegionDetailsElevationWater& rdew);
    void push_evilness           (RegionDetailsBiome&          rdg);
    void push_geology            (RegionDetailsGeology&        rdg);
    void push_hydro              (RegionDetailsElevationWater& rdg);
    void push_nobility           (RegionDetailsElevationWater& rdg);
    void push_rainfall           (RegionDetailsBiome&          rdg);
    void push_region             (RegionDetailsElevationWater& rdg);
    void push_salinity           (RegionDetailsBiome&          rdg);
    void push_savagery           (RegionDetailsBiome&          rdg);
    void push_sites              (RegionDetailsElevationWater& rdg);
    void push_temperature        (RegionDetailsBiome&          rdg);
    void push_trading            (RegionDetailsElevationWater& rdg);
    void push_vegetation         (RegionDetailsBiome&          rdg);
    void push_volcanism          (RegionDetailsBiome&          rdg);

    void push_biome_type_raw     (RegionDetailsBiome&          rdb);
    void push_biome_region_raw   (RegionDetailsBiome&          rdb);
    void push_drainage_raw       (RegionDetailsBiome&          rdg);
    void push_elevation_raw      (RegionDetailsElevation&      rde);
    void push_elevation_water_raw(RegionDetailsElevationWater& rdew);
    void push_evilness_raw       (RegionDetailsBiome&          rdg);
    void push_hydro_raw          (RegionDetailsElevationWater& rdg);
    void push_rainfall_raw       (RegionDetailsBiome&          rdg);
    void push_salinity_raw       (RegionDetailsBiome&          rdg);
    void push_savagery_raw       (RegionDetailsBiome&          rdg);
    void push_temperature_raw    (RegionDetailsBiome&          rdg);
    void push_vegetation_raw     (RegionDetailsBiome&          rdg);
    void push_volcanism_raw      (RegionDetailsBiome&          rdg);

    void push_elevation_hm       (RegionDetailsElevation&      rde);
    void push_elevation_water_hm (RegionDetailsElevationWater& rdew);

    // Pop methods

    RegionDetailsBiome          pop_biome();
    RegionDetailsElevationWater pop_diplomacy();
    RegionDetailsBiome          pop_drainage();
    RegionDetailsElevation      pop_elevation();
    RegionDetailsElevationWater pop_elevation_water();
    RegionDetailsBiome          pop_evilness();
    RegionDetailsGeology        pop_geology();
    RegionDetailsElevationWater pop_hydro();
    RegionDetailsElevationWater pop_nobility();
    RegionDetailsBiome          pop_rainfall();
    RegionDetailsElevationWater pop_region();
    RegionDetailsBiome          pop_salinity();
    RegionDetailsBiome          pop_savagery();
    RegionDetailsElevationWater pop_sites();
    RegionDetailsBiome          pop_temperature();
    RegionDetailsElevationWater pop_trading();
    RegionDetailsBiome          pop_vegetation();
    RegionDetailsBiome          pop_volcanism();

    RegionDetailsBiome          pop_biome_type_raw();
    RegionDetailsBiome          pop_biome_region_raw();
    RegionDetailsBiome          pop_drainage_raw();
    RegionDetailsElevation      pop_elevation_raw();
    RegionDetailsElevationWater pop_elevation_water_raw();
    RegionDetailsBiome          pop_evilness_raw();
    RegionDetailsElevationWater pop_hydro_raw();
    RegionDetailsBiome          pop_rainfall_raw();
    RegionDetailsBiome          pop_salinity_raw();
    RegionDetailsBiome          pop_savagery_raw();
    RegionDetailsBiome          pop_temperature_raw();
    RegionDetailsBiome          pop_vegetation_raw();
    RegionDetailsBiome          pop_volcanism_raw();

    RegionDetailsElevation      pop_elevation_hm();
    RegionDetailsElevationWater pop_elevation_water_hm();

    // Maps getters

    ExportedMapBase* get_biome_map();
    ExportedMapDF*   get_diplomacy_map();
    ExportedMapBase* get_drainage_map();
    ExportedMapBase* get_elevation_map();
    ExportedMapBase* get_elevation_water_map();
    ExportedMapBase* get_evilness_map();
    ExportedMapDF*   get_geology_map();
    ExportedMapBase* get_hydro_map();
    ExportedMapDF*   get_nobility_map();
    ExportedMapBase* get_rainfall_map();
    ExportedMapBase* get_region_map();
    ExportedMapBase* get_salinity_map();
    ExportedMapBase* get_savagery_map();
    ExportedMapDF*   get_sites_map();
    ExportedMapBase* get_temperature_map();
    ExportedMapDF*   get_trading_map();
    ExportedMapBase* get_vegetation_map();
    ExportedMapBase* get_volcanism_map();

    ExportedMapBase* get_biome_type_raw_map();
    ExportedMapBase* get_biome_region_raw_map();
    ExportedMapBase* get_drainage_raw_map();
    ExportedMapBase* get_elevation_raw_map();
    ExportedMapBase* get_elevation_water_raw_map();
    ExportedMapBase* get_evilness_raw_map();
    ExportedMapBase* get_hydro_raw_map();
    ExportedMapBase* get_rainfall_raw_map();
    ExportedMapBase* get_salinity_raw_map();
    ExportedMapBase* get_savagery_raw_map();
    ExportedMapBase* get_temperature_raw_map();
    ExportedMapBase* get_vegetation_raw_map();
    ExportedMapBase* get_volcanism_raw_map();

    ExportedMapBase* get_elevation_hm_map();
    ExportedMapBase* get_elevation_water_hm_map();

    // Queue status methods

    bool is_biome_queue_empty();
    bool is_diplomacy_queue_empty();
    bool is_drainage_queue_empty();
    bool is_elevation_queue_empty();
    bool is_elevation_water_queue_empty();
    bool is_evilness_queue_empty();
    bool is_geology_queue_empty();
    bool is_hydro_queue_empty();
    bool is_nobility_queue_empty();
    bool is_rainfall_queue_empty();
    bool is_region_queue_empty();
    bool is_salinity_queue_empty();
    bool is_savagery_queue_empty();
    bool is_sites_queue_empty();
    bool is_temperature_queue_empty();
    bool is_trading_queue_empty();
    bool is_vegetation_queue_empty();
    bool is_volcanism_queue_empty();

    bool is_biome_raw_type_queue_empty();
    bool is_biome_raw_region_queue_empty();
    bool is_drainage_raw_queue_empty();
    bool is_elevation_raw_queue_empty();
    bool is_elevation_water_raw_queue_empty();
    bool is_evilness_raw_queue_empty();
    bool is_hydro_raw_queue_empty();
    bool is_rainfall_raw_queue_empty();
    bool is_salinity_raw_queue_empty();
    bool is_savagery_raw_queue_empty();
    bool is_temperature_raw_queue_empty();
    bool is_vegetation_raw_queue_empty();
    bool is_volcanism_raw_queue_empty();

    bool is_elevation_hm_queue_empty();
    bool is_elevation_water_hm_queue_empty();

    // Thread related methods

    tthread::mutex& get_mutex();
    void            setup_threads();
    void            wait_for_threads();

    //
    void set_percentage_sites    (int percentage);
    void set_percentage_trade    (int percentage);
    void set_percentage_nobility (int percentage);
    void set_percentage_diplomacy(int percentage);

  private:
    void display_progress_special_maps(Logger* logger);

  };
}

#endif // MAPSEXPORTER_H
