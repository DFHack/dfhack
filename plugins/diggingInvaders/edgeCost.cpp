#include "edgeCost.h"

#include "modules/Buildings.h"
#include "modules/Maps.h"
#include "modules/MapCache.h"

#include "df/building.h"
#include "df/coord.h"
#include "df/map_block.h"
#include "df/tile_building_occ.h"
#include "df/tiletype.h"
#include "df/tiletype_material.h"
#include "df/tiletype_shape.h"

using namespace std;

int64_t getEdgeCost(color_ostream& out, df::coord pt1, df::coord pt2) {
    //first, list all the facts
    int32_t dx = pt2.x - pt1.x;
    int32_t dy = pt2.y - pt1.y;
    int32_t dz = pt2.z - pt1.z;
    int64_t cost = costWeight[CostDimension::Distance];
    
    Maps::ensureTileBlock(pt1);
    Maps::ensureTileBlock(pt2);
    df::tiletype* type1 = Maps::getTileType(pt1);
    df::tiletype* type2 = Maps::getTileType(pt2);
    df::map_block* block1 = Maps::getTileBlock(pt1);
    df::map_block* block2 = Maps::getTileBlock(pt2);
    df::tiletype_shape shape1 = ENUM_ATTR(tiletype, shape, *type1);
    df::tiletype_shape shape2 = ENUM_ATTR(tiletype, shape, *type2);

    bool construction1 = ENUM_ATTR(tiletype, material, *type1) == df::enums::tiletype_material::CONSTRUCTION;
    bool construction2 = ENUM_ATTR(tiletype, material, *type2) == df::enums::tiletype_material::CONSTRUCTION;
    bool passable1 = block1->walkable[pt1.x&0xF][pt1.y&0xF] != 0;
    bool passable2 = block2->walkable[pt2.x&0xF][pt2.y&0xF] != 0;
    bool passable_high1 = ENUM_ATTR(tiletype_shape, passable_high, shape1);
    bool passable_high2 = ENUM_ATTR(tiletype_shape, passable_high, shape2);
    bool passable_low1 = ENUM_ATTR(tiletype_shape, passable_low, shape1);
    bool passable_low2 = ENUM_ATTR(tiletype_shape, passable_low, shape2);

    bool building1, building2;
    bool sameBuilding = false;
    {
        df::enums::tile_building_occ::tile_building_occ awk = block1->occupancy[pt1.x&0x0F][pt1.y&0x0F].bits.building;
        building1 = awk == df::enums::tile_building_occ::Obstacle || awk == df::enums::tile_building_occ::Impassable;
        awk = block2->occupancy[pt2.x&0x0F][pt2.y&0x0F].bits.building;
        building2 = awk == df::enums::tile_building_occ::Obstacle || awk == df::enums::tile_building_occ::Impassable;
        if ( building1 && building2 ) {
            df::building* b1 = Buildings::findAtTile(pt1);
            df::building* b2 = Buildings::findAtTile(pt2);
            sameBuilding = b1 == b2;
        }
    }

    if ( Maps::canStepBetween(pt1, pt2) ) {
        if ( building2 && !sameBuilding ) {
            cost += costWeight[CostDimension::DestroyBuilding];
        }
        return cost;
    }

    if ( shape2 == df::enums::tiletype_shape::EMPTY ) {
        return -1;
    }

    //cannot step between: find out why
    if ( dz == 0 ) {
        if ( passable2 && !passable1 ) {
            return cost;
        }
        if ( passable1 && passable2 ) {
            out << __FILE__ << ", line " << __LINE__ << ": WTF?" << endl;
            return cost;
        }
        //pt2 is not passable. it must be a construction, a building, or a wall.
        if ( building2 ) {
            if ( sameBuilding ) {
                //don't charge twice for destroying the same building
                return cost;
            }
            cost += costWeight[CostDimension::DestroyBuilding];
            return cost;
        }
        if ( construction2 ) {
            //impassible constructions must be deconstructed
            cost += costWeight[CostDimension::DestroyConstruction];
            return cost;
        }

        if ( shape2 == df::enums::tiletype_shape::TREE ) {
            return -1;
        }

        if ( shape2 == df::enums::tiletype_shape::RAMP_TOP ) {
            return -1;
        }
        
        //it has to be a wall
        if ( shape2 != df::enums::tiletype_shape::WALL ) {
            out << "shape = " << (int32_t)shape2 << endl;
            out << __FILE__ << ", line " << __LINE__ << ": WTF?" << endl;
            return cost;
        }

        cost += costWeight[CostDimension::Dig];
        return cost;
    }

    //dz != 0
    if ( dx == 0 && dy == 0 ) {
        if ( dz > 0 ) {
            if ( passable_low2 )
                return cost;
            if ( building2 || construction2 ) {
                return -1;
            }
            cost += costWeight[CostDimension::Dig];
            return cost;
        }
        
        //descending
        if ( passable_high2 )
            return cost;
        
        if ( building2 || construction2 ) {
            return -1;
        }
        
        //must be a wall?
        if ( shape2 != df::enums::tiletype_shape::WALL ) {
            out.print("%s, line %n: WTF?\n", __FILE__, __LINE__);
            return cost;
        }

        cost += costWeight[CostDimension::Dig];
        return cost;
    }
    
    //moving diagonally
    return -1;
}

vector<Edge>* getEdgeSet(color_ostream &out, df::coord point, MapExtras::MapCache& cache, int32_t xMax, int32_t yMax, int32_t zMax) {
    vector<Edge>* result = new vector<Edge>;
    result->reserve(26);
    
    for ( int32_t dx = -1; dx <= 1; dx++ ) {
        for ( int32_t dy = -1; dy <= 1; dy++ ) {
            for ( int32_t dz = -1; dz <= 1; dz++ ) {
                df::coord neighbor(point.x+dx, point.y+dy, point.z+dz);
                if ( neighbor.x < 0 || neighbor.x >= xMax || neighbor.y < 0 || neighbor.y >= yMax || neighbor.z < 0 || neighbor.z >= zMax )
                    continue;
                if ( dz != 0 && /*(point.x == 0 || point.y == 0 || point.z == 0 || point.x == xMax-1 || point.y == yMax-1 || point.z == zMax-1) ||*/ (neighbor.x == 0 || neighbor.y == 0 || neighbor.z == 0 || neighbor.x == xMax-1 || neighbor.y == yMax-1 || neighbor.z == zMax-1) )
                    continue;
                if ( dx == 0 && dy == 0 && dz == 0 )
                    continue;
                int64_t cost = getEdgeCost(out, point, neighbor);
                if ( cost == -1 )
                    continue;
                Edge edge(point, neighbor, cost);
                result->push_back(edge);
            }
        }
    }

    return result;
}

