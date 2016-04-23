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

// You can always find the latest version of this plugin in Github
// https://github.com/ragundo/exportmaps

#include <utility>
#include <BitArray.h>
#include "modules/Maps.h"
#include "../../../include/Mac_compat.h"
#include "../../../include/ExportMaps.h"
#include "../../../include/util/ofsub.h"

using namespace DFHack;
using namespace exportmaps_plugin;

/*****************************************************************************
External functions
*****************************************************************************/
extern int                get_biome_type(int world_coord_x, int world_coord_y);
extern std::pair<int,int> adjust_coordinates_to_region(int x,
                                                       int y,
                                                       int delta,
                                                       int pos_x,
                                                       int pos_y,
                                                       int world_width,
                                                       int world_height);

extern bool GetLocalFeature(t_feature &feature, df::coord2d rgn_pos, int32_t index);

/*****************************************************************************
Local functions forward declaration
*****************************************************************************/
// Return the RGB values for the biome export map given a biome type
RGB_color RGB_from_elevation_water(RegionDetailsElevationWater& rdew,
                                                                                 int x,
                                                                                 int y,
                                                                                 int biome_type,
                                                                                 DFHack::BitArray<df::region_map_entry_flags>& flags);

// Return a world river
std::pair<df::world_river*, int> get_world_river(int x, int y);


/*****************************************************************************
Module main function.
This is the function that the thread executes
*****************************************************************************/
void consumer_geology(void* arg)
{
    MapsExporter* maps_exporter = (MapsExporter*)arg;

    while(arg != nullptr)
    {
        if (maps_exporter->is_geology_queue_empty())
        {
            // No data on the queue. Sleep 100 ms and try again
            tthread::this_thread::yield();
        }
        else // There's data in the queue
        {
            // Get the data from the queue
            RegionDetailsGeology rdg = maps_exporter->pop_geology();

            // Check if is the marker for no more data from the producer
            // TODO refactor this
            if ((rdg.get_pos_x() == -1) && (rdg.get_pos_y() == -1))
            {
                // All the data has been processed. Finish this thread execution
                break;
            }
            else // There's data to be processed
            {
                // Iterate over the 16 subtiles (x) and (y) that a world tile has
                for (auto x=0; x<16; ++x)
                    for (auto y=15; y>=0; --y)
                    {
                        // Each position of the array is a value that tells us if the local tile
                        // belongs to the NW,N,NE,W,center,E,SW,S,SE world region.
                        // Returns a world coordinate adjusted from the original one
                        std::pair<int,int> adjusted_tile_coordinates = adjust_coordinates_to_region(x,
                                                                                                    y,
                                                                                                    rdg.get_biome_index(x,y),
                                                                                                    rdg.get_pos_x(),
                                                                                                    rdg.get_pos_y(),
                                                                                                    df::global::world->world_data->world_width,
                                                                                                    df::global::world->world_data->world_height);

                        // Get the region where this position belongs to
                        df::region_map_entry& rme = df::global::world->world_data->region_map[adjusted_tile_coordinates.first]
                                                                                             [adjusted_tile_coordinates.second];

                        // Get the geo-biome associated
                        int geo_biome_index = rme.geo_index;
                        df::world_geo_biome* geo_biome = df::world_geo_biome::find(geo_biome_index);

                        // The geobiome has a vector of world_geo_layers
                        for ( unsigned int i=0; i < geo_biome->layers.size(); i++)
                        {
                          int borrame = 0;
                            df::world_geo_layer* layer    = geo_biome->layers[i];
                            df::geo_layer_type layer_type = layer->type;
                            switch (layer_type)
                            {
                              case df::enums::geo_layer_type::SOIL                      : borrame = -1;
                                break;
                              case df::enums::geo_layer_type::SEDIMENTARY               : borrame = -2;
                                break;
                              case df::enums::geo_layer_type::METAMORPHIC               : borrame = -3;
                                break;
                              case df::enums::geo_layer_type::IGNEOUS_EXTRUSIVE         : borrame = -4;
                                break;
                              case df::enums::geo_layer_type::IGNEOUS_INTRUSIVE         : borrame = -5;
                                break;
                              case df::enums::geo_layer_type::SOIL_OCEAN                : borrame = -6;
                                break;
                              case df::enums::geo_layer_type::SOIL_SAND                 : borrame = -7;
                                break;
                              case df::enums::geo_layer_type::SEDIMENTARY_OCEAN_SHALLOW : borrame = -8;
                                break;
                              case df::enums::geo_layer_type::SEDIMENTARY_OCEAN_DEEP    : borrame = -9;
                                break;
                            }
                            int mat_index           = std::max(borrame, layer->mat_index);
                            std::string mat_name    = df::global::world->raws.inorganics[mat_index]->id;
                            int layer_top_height    = layer->top_height;
                            int layer_bottom_height = layer->bottom_height;

                            if (layer->vein_mat.size() > 0)
                            {
                                for (unsigned int j = 0 ; j < layer->vein_mat.size(); j++)
                                {
                                    int vein_mat         = layer->vein_mat[j];
                                    std::string mat_vein = df::global::world->raws.inorganics[vein_mat]->id;
                                    auto vein_type       = layer->vein_type;
                                }
                            }
                        }
                    }
            }
        }
    }
}

/*
                        // Get the features associated
                        const std::vector<df::world_region_feature* >& features = rdg.get_features(x,y);
                        std::map<int, int> layer_bottom, layer_top;
                        int tile_base_z = 255;
                        bool sea_found = false;
                        for (size_t i = 0; i < features.size(); i++)
                        {
                            df::world_region_feature* feature = features[i];
                            df::world_underground_region *layer = df::world_underground_region::find(feature->layer);
                            if (!layer || feature->min_z == -30000) continue;

                            layer_bottom[layer->layer_depth] = feature->min_z;
                            layer_top[layer->layer_depth] = feature->max_z;

                            tile_base_z = std::min(tile_base_z, (int) feature->min_z);

                            switch (layer->type) {
                                case df::world_underground_region::Cavern:
                                    break;
                                case df::world_underground_region::MagmaSea:
                                    sea_found = true;
                                    break;
                                case df::world_underground_region::Underworld:
                                    break;
                            }

                            df::coord pos;
                            pos.x = rdg.get_pos_x();
                            pos.y = rdg.get_pos_y();
                            df::feature_init* local_init_feature = Maps::getLocalInitFeature(pos, feature->feature_idx);

                            //t_feature local_feature;
                            //GetLocalFeature(local_feature, pos, feature->feature_idx);
                        }
*/
