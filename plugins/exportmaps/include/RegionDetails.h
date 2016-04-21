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

#ifndef REGION_DETAILS_H
#define REGION_DETAILS_H

#include "dfhack.h"

class RegionDetailsBase
{
protected:
    // world coordinates of the current position
    int16_t _pos_x;
    int16_t _pos_y;

public:
    RegionDetailsBase() : _pos_x(-1), _pos_y(-1){}
    RegionDetailsBase(int16_t x, int16_t y) : _pos_x(x), _pos_y(y) {}

    int16_t get_pos_x(){ return _pos_x; }
    int16_t get_pos_y(){ return _pos_y; }
    bool    is_end_marker() { return ((_pos_x == -1) && (_pos_y == -1));}
};

/*****************************************************************************
*****************************************************************************/


class RegionDetailsElevation : public RegionDetailsBase
{
    // Elevation data from world_region_detail
    int16_t elevation[17][17];

public:
    RegionDetailsElevation() : RegionDetailsBase(){}

    RegionDetailsElevation(df::world_region_details* rd) : RegionDetailsBase(rd->pos.x, rd->pos.y)
    {
        // Copy DF data to plugin
        for (auto i = 0; i < 17; ++i)
            for (auto j = 0; j < 17; ++j)
                this->elevation[i][j] = rd->elevation[i][j];
    }

    RegionDetailsElevation(const RegionDetailsElevation& rdd) : RegionDetailsBase(rdd._pos_x, rdd._pos_y)
    {
        // Copy DF data to plugin
        for (auto i = 0; i < 17; ++i)
            for (auto j = 0; j < 17; ++j)
                this->elevation[i][j] = rdd.elevation[i][j];
    }
    int16_t get_elevation(int x, int y)
    {
        return elevation[x][y];
    }
};


/*****************************************************************************
*****************************************************************************/

class RegionDetailsElevationWater : public RegionDetailsBase
{
    // Elevation data from world_region_details
    int16_t elevation[17][17];

    // Biome data from world_region_details
    int16_t biome[17][17];

    // Rivers data from world_region_details
    df::world_region_details::T_rivers_vertical   rivers_vertical;
    df::world_region_details::T_rivers_horizontal rivers_horizontal;

public:
    RegionDetailsElevationWater() : RegionDetailsBase(){}

    RegionDetailsElevationWater(df::world_region_details* rd) : RegionDetailsBase(rd->pos.x, rd->pos.y)
    {
        // Copy elevation and biome data
        for (auto i = 0; i < 17; ++i)
            for (auto j = 0; j < 17; ++j)
            {
                this->elevation[i][j] = rd->elevation[i][j];
                this->biome[i][j]     = rd->biome[i][j];
            }

        // copy rivers data
        memcpy((void*)&rivers_horizontal,
               (void*)&rd->rivers_horizontal,
               sizeof(df::world_region_details::T_rivers_horizontal));

        memcpy((void*)&rivers_vertical,
               (void*)&rd->rivers_vertical,
               sizeof(df::world_region_details::T_rivers_vertical));

    }

    RegionDetailsElevationWater(const RegionDetailsElevationWater& rd) : RegionDetailsBase(rd._pos_x, rd._pos_y)
    {
        // Copy elevation and biome data
        for (auto i = 0; i < 17; ++i)
            for (auto j = 0; j < 17; ++j)
            {
                this->elevation[i][j] = rd.elevation[i][j];
                this->biome[i][j]     = rd.biome[i][j];
            }

        // copy rivers data
        memcpy((void*)&rivers_horizontal,
               (void*)&(rd.rivers_horizontal),
               sizeof(df::world_region_details::T_rivers_horizontal));

        memcpy((void*)&rivers_vertical,
               (void*)&(rd.rivers_vertical),
               sizeof(df::world_region_details::T_rivers_vertical));
    }

    int16_t get_elevation(int x, int y)
    {
        return elevation[x][y];
    }

    int16_t get_biome_index(int x, int y)
    {
        return biome[x][y];
    }

    const df::world_region_details::T_rivers_horizontal& get_rivers_horizontal()
    {
        return rivers_horizontal;
    }

    const df::world_region_details::T_rivers_vertical& get_rivers_vertical()
    {
        return rivers_vertical;
    }
};


/*****************************************************************************
*****************************************************************************/

class RegionDetailsBiome : public RegionDetailsBase
{
    // Biome data from world_region_detail
    int16_t biome[17][17];

public:
    RegionDetailsBiome(){}

    RegionDetailsBiome(df::world_region_details* rd) : RegionDetailsBase(rd->pos.x, rd->pos.y)
    {
        // Copy DF data to plugin
        for (auto i = 0; i < 17; ++i)
            for (auto j = 0; j < 17; ++j)
                this->biome[i][j] = rd->biome[i][j];
    }

    RegionDetailsBiome(const RegionDetailsBiome& rdd) : RegionDetailsBase(rdd._pos_x, rdd._pos_y)
    {
        // Copy DF data to plugin
        for (auto i = 0; i < 17; ++i)
            for (auto j = 0; j < 17; ++j)
                this->biome[i][j] = rdd.biome[i][j];
    }

    int16_t get_biome_index(int x, int y)
    {
        return biome[x][y];
    }
};

/*****************************************************************************
*****************************************************************************/

class RegionDetailsGeology : public RegionDetailsBase
{
    // Biome data from world_region_details
    int16_t biome[17][17];

    // Elevation data from world_region_details
    int16_t elevation[17][17];

    // Underground features
    std::vector<df::world_region_feature* > features[16][16];

public:
    RegionDetailsGeology(){}

    RegionDetailsGeology(df::world_region_details* rd) : RegionDetailsBase(rd->pos.x, rd->pos.y)
    {
        // Copy DF data to plugin
        for (auto i = 0; i < 17; ++i)
            for (auto j = 0; j < 17; ++j)
            {
                this->biome[i][j] = rd->biome[i][j];
                this->elevation[i][j] = rd->elevation[i][j];
            }

        for (auto i = 0; i < 16; ++i)
            for (auto j = 0; j < 16; ++j)
                this->features[i][j] = rd->features[i][j];
    }

    RegionDetailsGeology(const RegionDetailsGeology& rdd) : RegionDetailsBase(rdd._pos_x, rdd._pos_y)
    {
        // Copy DF data to plugin
        for (auto i = 0; i < 17; ++i)
            for (auto j = 0; j < 17; ++j)
            {
                this->biome[i][j] = rdd.biome[i][j];
                this->elevation[i][j] = rdd.elevation[i][j];
            }

        for (auto i = 0; i < 16; ++i)
            for (auto j = 0; j < 16; ++j)
                this->features[i][j] = rdd.features[i][j];
    }

    int16_t get_biome_index(int x, int y)
    {
        return biome[x][y];
    }

    int16_t get_elevation(int x, int y)
    {
        return elevation[x][y];
    }

    const std::vector<df::world_region_feature* >& get_features(int x, int y)
    {
        return features[x][y];
    }

};


#endif // REGION_DETAILS_H
