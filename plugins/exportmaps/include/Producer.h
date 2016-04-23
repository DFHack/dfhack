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

#ifndef PRODUCER_H
#define PRODUCER_H

#include "dfhack.h"
#include "RegionDetails.h"
#include "MapsExporter.h"

namespace exportmaps_plugin
{
  class Producer
  {

  public:
        virtual void produce_data(class MapsExporter& destination, int x, int y, df::world_region_details* ptr_rd);
        virtual void produce_end (class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerTemperature : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerRainfall : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerRegion : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerDrainage : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerSavagery : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerVolcanism : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerVegetation : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerEvilness : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerSalinity : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerHydro : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerElevation : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerElevationWater : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerBiome : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerGeology : public Producer
  {
  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerTrading : public Producer
  {
  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerNobility : public Producer
  {
  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerDiplomacy : public Producer
  {
  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerSites : public Producer
  {
  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerBiomeRawType : public Producer
  {
  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerBiomeRawRegion : public Producer
  {
  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerDrainageRaw : public Producer
  {
  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerElevationRaw : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerElevationWaterRaw : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details*
                      ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerEvilnessRaw : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details*
                      ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerHydroRaw : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerRainfallRaw : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerSalinityRaw : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerSavageryRaw : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerTemperatureRaw : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerVolcanismRaw : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerVegetationRaw : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);

  };


/*****************************************************************************
  *****************************************************************************/

  class ProducerElevationHeightMap : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };

  /*****************************************************************************
  *****************************************************************************/

  class ProducerElevationWaterHeightMap : public Producer
  {

  public:
    void produce_data(class MapsExporter& destination,
                      int x,
                      int y,
                      df::world_region_details* ptr_rd
                      );

    void produce_end(class MapsExporter& destination);
  };


}

#endif // PRODUCER_H
