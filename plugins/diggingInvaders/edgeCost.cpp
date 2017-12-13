#include "edgeCost.h"

#include "modules/Buildings.h"
#include "modules/Maps.h"
#include "modules/MapCache.h"

#include "df/building.h"
#include "df/building_bridgest.h"
#include "df/building_hatchst.h"
#include "df/building_type.h"
#include "df/construction.h"
#include "df/coord.h"
#include "df/item_type.h"
#include "df/map_block.h"
#include "df/tile_building_occ.h"
#include "df/tiletype.h"
#include "df/tiletype_material.h"
#include "df/tiletype_shape.h"

#include <iostream>

using namespace DFHack;

/*
cost_t costWeight[] = {
//Distance
1,
//Destroy Building
2,
//Dig
10000,
//DestroyConstruction
100,
};

int32_t jobDelay[] = {
//Distance
-1,
//Destroy Building
1000,
//Dig
1000,
//DestroyConstruction
1000
};
*/

using namespace std;

/*
limitations
    ramps
    cave-ins
*/
cost_t getEdgeCost(color_ostream& out, df::coord pt1, df::coord pt2, DigAbilities& abilities) {
    int32_t dx = pt2.x - pt1.x;
    int32_t dy = pt2.y - pt1.y;
    int32_t dz = pt2.z - pt1.z;
    cost_t cost = abilities.costWeight[CostDimension::Walk];
    if ( cost < 0 )
        return -1;

    if ( Maps::getTileBlock(pt1) == NULL || Maps::getTileBlock(pt2) == NULL )
        return -1;

    df::tiletype* type2 = Maps::getTileType(pt2);
    df::tiletype_shape shape2 = ENUM_ATTR(tiletype, shape, *type2);

    if ( Maps::getTileBlock(pt1)->designation[pt1.x&0xF][pt1.y&0xF].bits.flow_size >= 4 )
        return -1;
    if ( Maps::getTileBlock(pt2)->designation[pt2.x&0xF][pt2.y&0xF].bits.flow_size >= 4 )
        return -1;

    if ( shape2 == df::enums::tiletype_shape::EMPTY ) {
        return -1;
    }

    if ( shape2 == df::enums::tiletype_shape::BRANCH ||
         shape2 == df::enums::tiletype_shape::TRUNK_BRANCH ||
         shape2 == df::enums::tiletype_shape::TWIG )
        return -1;

/*
    if () {
        df::map_block* temp = Maps::getTileBlock(df::coord(pt1.x,pt1.y,pt1.z-1));
        if ( temp && temp->designation[pt1.x&0xF][pt1.y&0xF]
    }
*/

    if ( Maps::canStepBetween(pt1, pt2) ) {
        return cost;
    }

    df::building* building2 = Buildings::findAtTile(pt2);
    if ( building2 ) {
        if ( abilities.costWeight[CostDimension::DestroyBuilding] < 0 )
            return -1;
        cost += abilities.costWeight[CostDimension::DestroyBuilding];
        if ( dx*dx + dy*dy > 1 )
            return -1;
    }

    bool construction2 = ENUM_ATTR(tiletype, material, *type2) == df::enums::tiletype_material::CONSTRUCTION;
    if ( construction2 ) {
        //smooth or not?
        df::construction* constr = df::construction::find(pt2);
        bool smooth = constr != NULL && constr->item_type != df::enums::item_type::BOULDER;
        if ( smooth ) {
            if ( abilities.costWeight[CostDimension::DestroySmoothConstruction] < 0 )
                return -1;
            cost += abilities.costWeight[CostDimension::DestroySmoothConstruction];
        } else {
            if ( abilities.costWeight[CostDimension::DestroyRoughConstruction] < 0 )
                return -1;
            cost += abilities.costWeight[CostDimension::DestroyRoughConstruction];
        }
    }

    if ( dz == 0 ) {
        if ( !building2 && !construction2 ) {
            //it has to be a wall
            if ( shape2 == df::enums::tiletype_shape::RAMP_TOP ) {
                return -1;
            } else if ( shape2 != df::enums::tiletype_shape::WALL ) {
                //out << "shape = " << (int32_t)shape2 << endl;
                //out << __FILE__ << ", line " << __LINE__ << ": WTF?" << endl;
                return cost;
            }
            if ( abilities.costWeight[CostDimension::Dig] < 0 ) {
                return -1;
            }
            cost += abilities.costWeight[CostDimension::Dig];
        }
    } else {
        if ( dx == 0 && dy == 0 ) {
            df::tiletype* type1 = Maps::getTileType(pt1);
            df::tiletype_shape shape1 = ENUM_ATTR(tiletype, shape, *type1);
            if ( dz > 0 ) {
                bool walkable_low2 = shape2 == df::tiletype_shape::STAIR_DOWN || shape2 == df::tiletype_shape::STAIR_UPDOWN;
                if ( !walkable_low2 ) {
                    if ( building2 || construction2 )
                        return -1;
                    if ( abilities.costWeight[CostDimension::Dig] < 0 ) {
                        return -1;
                    }
                    cost += abilities.costWeight[CostDimension::Dig];
                }

                bool walkable_high1 = shape1 == df::tiletype_shape::STAIR_UP || shape1 == df::tiletype_shape::STAIR_UPDOWN;
                if ( !walkable_high1 ) {
                    if ( shape1 != df::enums::tiletype_shape::WALL ) {
                        return -1;
                    }
                    if ( abilities.costWeight[CostDimension::Dig] < 0 ) {
                        return -1;
                    }
                    cost += abilities.costWeight[CostDimension::Dig];
                }

                if ( building2 ) {
                    //moving up through an open bridge or a usable hatch is fine. other buildings are not
                    bool unforbiddenHatch = false;
                    if ( building2->getType() == df::building_type::Hatch ) {
                        df::building_hatchst* hatch = (df::building_hatchst*)building2;
                        if ( !hatch->door_flags.bits.forbidden && !(hatch->door_flags.bits.operated_by_mechanisms&&hatch->door_flags.bits.closed) )
                            unforbiddenHatch = true;
                    }
                    bool inactiveBridge = false;
                    if ( building2->getType() == df::building_type::Bridge ) {
                        df::building_bridgest* bridge = (df::building_bridgest*)building2;
                        bool xMin = pt2.x == bridge->x1;
                        bool xMax = pt2.x == bridge->x2;
                        bool yMin = pt2.y == bridge->y1;
                        bool yMax = pt2.y == bridge->y2;
                        if ( !bridge->gate_flags.bits.closed ) {
                            //if it's open, we could still be in the busy part of it
                            if ( bridge->direction == df::building_bridgest::T_direction::Left && !xMin ) {
                                inactiveBridge = true;
                            } else if ( bridge->direction == df::building_bridgest::T_direction::Right && !xMax ) {
                                inactiveBridge = true;
                            } else if ( bridge->direction == df::building_bridgest::T_direction::Up && !yMax ) {
                                inactiveBridge = true;
                            } else if ( bridge->direction == df::building_bridgest::T_direction::Down && !yMin ) {
                                inactiveBridge = true;
                            } else if ( bridge->direction == df::building_bridgest::T_direction::Retracting ) {
                                inactiveBridge = true;
                            }
                        }
                    }
                    if ( !unforbiddenHatch && !inactiveBridge )
                        return -1;
                }

                /*bool forbidden = false;
                if ( building2 && building2->getType() == df::building_type::Hatch ) {
                    df::building_hatchst* hatch = (df::building_hatchst*)building2;
                    if ( hatch->door_flags.bits.forbidden )
                        forbidden = true;
                }
                if ( forbidden )
                    return -1;*/
            } else {
                bool walkable_high2 = shape2 == df::tiletype_shape::STAIR_UP || shape2 == df::tiletype_shape::STAIR_UPDOWN;
                if ( !walkable_high2 ) {
                    if ( building2 || construction2 )
                        return -1;

                    if ( shape2 != df::enums::tiletype_shape::WALL )
                        return -1;
                    if ( abilities.costWeight[CostDimension::Dig] < 0 ) {
                        return -1;
                    }
                    cost += abilities.costWeight[CostDimension::Dig];
                }
                bool walkable_low1 = shape1 == df::tiletype_shape::STAIR_DOWN || shape1 == df::tiletype_shape::STAIR_UPDOWN;
                if ( !walkable_low1 ) {
                    //if ( building1 || construction1 )
                        //return -1;
                    //TODO: consider ramps
                    if ( shape1 == df::tiletype_shape::RAMP )
                        return -1;
                    if ( abilities.costWeight[CostDimension::Dig] < 0 ) {
                        return -1;
                    }
                    cost += abilities.costWeight[CostDimension::Dig];
                }

                df::building* building1 = Buildings::findAtTile(pt1);
                //if you're moving down, and you're on a bridge, and that bridge is lowered, then you can't do it
                if ( building1 && building1->getType() == df::building_type::Bridge ) {
                    df::building_bridgest* bridge = (df::building_bridgest*)building2;
                    if ( bridge->gate_flags.bits.closed ) {
                        return -1;
                    }
                    //open bridges moving down, standing on bad spot
                    if ( bridge->direction == df::building_bridgest::T_direction::Left  && pt1.x == bridge->x1 )
                        return -1;
                    if ( bridge->direction == df::building_bridgest::T_direction::Right && pt1.x == bridge->x2 )
                        return -1;
                    if ( bridge->direction == df::building_bridgest::T_direction::Up    && pt1.y == bridge->y1 )
                        return -1;
                    if ( bridge->direction == df::building_bridgest::T_direction::Down  && pt1.y == bridge->y2 )
                        return -1;
                }

                bool forbidden = false;
                if ( building1 && building1->getType() == df::building_type::Hatch ) {
                    df::building_hatchst* hatch = (df::building_hatchst*)building1;
                    if ( hatch->door_flags.bits.forbidden || hatch->door_flags.bits.closed && hatch->door_flags.bits.operated_by_mechanisms )
                        forbidden = true;
                }

                //you can deconstruct a hatch from the side
                if ( building1 && forbidden /*&& building1->getType() == df::building_type::Hatch*/ ) {
/*
                    df::coord support[] = {df::coord(pt1.x-1, pt1.y, pt1.z), df::coord(pt1.x+1,pt1.y,pt1.z), df::coord(pt1.x,pt1.y-1,pt1.z), df::coord(pt1.x,pt1.y+1,pt1.z)};
                    if ( abilities.costWeight[CostDimension::DestroyBuilding] < 0 ) {
                        return -1;
                    }
                    cost_t minCost = -1;
                    for ( size_t a = 0; a < 4; a++ ) {
                        df::tiletype* supportType = Maps::getTileType(support[a]);
                        df::tiletype_shape shape = ENUM_ATTR(tiletype, shape, *supportType);
                        df::tiletype_shape_basic basic = ENUM_ATTR(tiletype_shape, basic_shape, shape);
                        cost_t cost2 = 2*abilities.costWeight[CostDimension::Walk] + abilities.costWeight[CostDimension::DestroyBuilding];
                        if ( !Maps::canStepBetween(pt1, support[a]) ) {
                            switch(basic) {
                                case tiletype_shape_basic::Open:
                                    //TODO: check for a hatch or a bridge: that makes it ok
                                    continue;
                                case tiletype_shape_basic::Wall:
                                    if ( ENUM_ATTR(tiletype, material, *supportType) == df::enums::tiletype_material::CONSTRUCTION ) {
                                        if ( abilities.costWeight[CostDimension::DestroyConstruction] < 0 ) {
                                            continue;
                                        }
                                        cost2 += abilities.costWeight[CostDimension::DestroyConstruction];
                                    } else {
                                        if ( abilities.costWeight[CostDimension::Dig] < 0 ) {
                                            continue;
                                        }
                                        cost2 += abilities.costWeight[CostDimension::Dig];
                                    }
                                case tiletype_shape_basic::Ramp:
                                    //TODO: check for a hatch or a bridge: that makes it ok
                                    if ( shape == df::enums::tiletype_shape::RAMP_TOP ) {
                                        continue;
                                    }
                                case tiletype_shape_basic::Stair:
                                case tiletype_shape_basic::Floor:
                                    break;
                            }
                            if ( Buildings::findAtTile(support[a]) ) {
                                if ( abilities.costWeight[CostDimension::DestroyBuilding] < 0 ) {
                                    continue;
                                }
                                cost2 += abilities.costWeight[CostDimension::DestroyBuilding];
                            }
                        }
                        if ( minCost == -1 || cost2 < minCost )
                            minCost = cost2;
                    }
                    if ( minCost == -1 )
                        return -1;
                    cost += minCost;

*/
                    //note: assignJob is not ready for this level of sophistication, so don't allow it
                    return -1;
                }
            }
        } else {
            //nonvertical
            //out.print("%s, line %d: (%d,%d,%d)->(%d,%d,%d)\n", __FILE__, __LINE__, pt1.x,pt1.y,pt1.z, pt2.x,pt2.y,pt2.z);
            return -1;
        }
    }

    return cost;
}

/*
cost_t getEdgeCostOld(color_ostream& out, df::coord pt1, df::coord pt2) {
    //first, list all the facts
    int32_t dx = pt2.x - pt1.x;
    int32_t dy = pt2.y - pt1.y;
    int32_t dz = pt2.z - pt1.z;
    cost_t cost = costWeight[CostDimension::Walk];

    if ( false ) {
        if ( Maps::canStepBetween(pt1,pt2) )
            return cost;
        return 100 + cost;
    }

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
*/

vector<Edge>* getEdgeSet(color_ostream &out, df::coord point, MapExtras::MapCache& cache, int32_t xMax, int32_t yMax, int32_t zMax, DigAbilities& abilities) {
    //vector<Edge>* result = new vector<Edge>(26);
    vector<Edge>* result = new vector<Edge>();
    result->reserve(26);

    //size_t count = 0;
    for ( int32_t dx = -1; dx <= 1; dx++ ) {
        for ( int32_t dy = -1; dy <= 1; dy++ ) {
            for ( int32_t dz = -1; dz <= 1; dz++ ) {
                df::coord neighbor(point.x+dx, point.y+dy, point.z+dz);
                if ( !Maps::isValidTilePos(neighbor) )
                    continue;
                if ( dz != 0 && /*(point.x == 0 || point.y == 0 || point.z == 0 || point.x == xMax-1 || point.y == yMax-1 || point.z == zMax-1) ||*/ (neighbor.x == 0 || neighbor.y == 0 || neighbor.z == 0 || neighbor.x == xMax-1 || neighbor.y == yMax-1 || neighbor.z == zMax-1) )
                    continue;
                if ( dx == 0 && dy == 0 && dz == 0 )
                    continue;
                cost_t cost = getEdgeCost(out, point, neighbor, abilities);
                if ( cost == -1 )
                    continue;
                Edge edge(point, neighbor, cost);
                //(*result)[count] = edge;
                result->push_back(edge);
                //count++;
            }
        }
    }

    return result;
}

