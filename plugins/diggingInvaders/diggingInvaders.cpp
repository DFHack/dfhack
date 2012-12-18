#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "Types.h"

#include "modules/Buildings.h"
#include "modules/EventManager.h"
#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/building.h"
#include "df/coord.h"
#include "df/inorganic_raw.h"
#include "df/map_block.h"
#include "df/strain_type.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/tiletype.h"
#include "df/tiletype_material.h"
#include "df/tiletype_shape.h"
#include "df/tiletype_shape_basic.h"
#include "df/world.h"

#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using namespace std;

using namespace DFHack;
using namespace df::enums;

int32_t digSpeed = 100;
int32_t destroySpeed = 100;
int32_t deconstructSpeed = 100;
int32_t constructSpeed = 100;

command_result diggingInvadersFunc(color_ostream &out, std::vector <std::string> & parameters);

DFHACK_PLUGIN("diggingInvaders");

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "diggingInvaders", "Makes invaders dig to your dwarves.",
        diggingInvadersFunc, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "EXTRA HELP STRINGGNGNGNGNGNNGG.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

//cost is [path cost, building destruction cost, dig cost, construct cost]. Minimize constructions, then minimize dig cost, then minimize path cost.
const size_t costDim = 4;

struct Cost {
    int32_t cost[costDim];
    Cost() {
        memset(cost, 0, costDim*sizeof(int32_t));
    }
    Cost( int32_t costIn[costDim] ) {
        memcpy(cost, costIn, costDim*sizeof(int32_t));
    }
    Cost(const Cost& c) {
        memcpy(cost, c.cost, costDim*sizeof(int32_t));
    }
    Cost( int32_t i ) {
        memset(cost, 0, costDim*sizeof(int32_t));
        cost[0] = i;
    }

    bool operator>(const Cost& c) const {
        for ( size_t a = 0; a < costDim; a++ ) {
            if ( cost[costDim-1-a] != c.cost[costDim-1-a] )
                return cost[costDim-1-a] > c.cost[costDim-1-a];
        }
        return false;
    }

    bool operator<(const Cost& c) const {
        for ( size_t a = 0; a < costDim; a++ ) {
            if ( cost[costDim-1-a] != c.cost[costDim-1-a] )
                return cost[costDim-1-a] < c.cost[costDim-1-a];
        }
        return false;
    }

    bool operator==(const Cost& c) const {
        for ( size_t a = 0; a < costDim; a++ ) {
            if ( cost[a] != c.cost[a] )
                return false;
        }
        return true;
    }

    bool operator!=(const Cost& c) const {
        return !( *this == c);
    }

    Cost operator+(const Cost& c) const {
        Cost result(*this);
        for ( size_t a = 0; a < costDim; a++ ) {
            result.cost[a] += c.cost[a];
        }
        return result;
    }
};

class Edge {
public:
    //static map<df::coord, int32_t> pointCost;
    df::coord p1;
    df::coord p2;
    Cost cost;
    Edge(df::coord p1In, df::coord p2In, Cost costIn): cost(costIn) {
        if ( p2In < p1In ) {
            p1 = p2In;
            p2 = p1In;
        } else {
            p1 = p1In;
            p2 = p2In;
        }
    }

    bool operator==(const Edge& e) const {
        return (cost == e.cost && p1 == e.p1 && p2 == e.p2);
    }

    bool operator<(const Edge& e) const {
        if ( cost != e.cost )
            return cost < e.cost;
        if ( p1.z != e.p1.z )
            return p1.z < e.p1.z;
        if ( p1 != e.p1 )
            return p1 < e.p1;
        if ( p2.z != e.p2.z )
            return p2.z < e.p2.z;
        if ( p2 != e.p2 )
            return p2 < e.p2;
        return false;
    }
};

vector<Edge>* getEdgeSet(color_ostream &out, df::coord point, MapExtras::MapCache& cache, int32_t xMax, int32_t yMax, int32_t zMax);
df::coord getRoot(df::coord point, map<df::coord, df::coord>& rootMap);

class PointComp {
public:
    map<df::coord, Cost> *pointCost;
    PointComp(map<df::coord, Cost> *p): pointCost(p) {
        
    }
    
    int32_t operator()(df::coord p1, df::coord p2) {
        if ( p1 == p2 ) return 0;
        auto i1 = pointCost->find(p1);
        auto i2 = pointCost->find(p2);
        if ( i1 == pointCost->end() && i2 == pointCost->end() )
            return p1 < p2;
        if ( i1 == pointCost->end() )
            return true;
        if ( i2 == pointCost->end() )
            return false;
        Cost c1 = (*i1).second;
        Cost c2 = (*i2).second;
        if ( c1 != c2 )
            return c1 < c2;
        return p1 < p2;
    }
};

bool important(df::coord pos, map<df::coord, set<Edge> >& edges, df::coord prev, set<df::coord>& importantPoints, set<Edge>& importantEdges);
void doDiggingInvaders(color_ostream& out, void* ptr);

command_result diggingInvadersFunc(color_ostream& out, std::vector<std::string>& parameters) {
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    doDiggingInvaders(out, NULL);
    return CR_OK;
}

void doDiggingInvaders(color_ostream& out, void* ptr) {
    CoreSuspender suspend;
    
    set<df::coord> invaderPts;
    set<df::coord> localPts;
    map<df::coord, df::coord> parentMap;
    map<df::coord, Cost> costMap;
    PointComp comp(&costMap);
    set<df::coord, PointComp> fringe(comp);
    uint32_t xMax, yMax, zMax;
    Maps::getSize(xMax,yMax,zMax);
    xMax *= 16;
    yMax *= 16;
    MapExtras::MapCache cache;

    //TODO: look for invaders with buildingdestroyer:3

    //find all locals and invaders
    for ( size_t a = 0; a < df::global::world->units.active.size(); a++ ) {
        df::unit* unit = df::global::world->units.active[a];
        if ( unit->flags1.bits.dead )
            continue;
        if ( Units::isCitizen(unit) ) {
            if ( localPts.find(unit->pos) != localPts.end() )
                continue;
            localPts.insert(unit->pos);
        } else if ( unit->flags1.bits.active_invader ) {
            if ( invaderPts.find(unit->pos) != invaderPts.end() )
                continue;
            invaderPts.insert(unit->pos);
            costMap[unit->pos] = 0;
            fringe.insert(unit->pos);
        } else {
            continue;
        }
    }

    int32_t localPtsFound = 0;
    set<df::coord> closedSet;

    clock_t t0 = clock();
    clock_t totalEdgeTime = 0;
    while(!fringe.empty()) {
        df::coord pt = *(fringe.begin());
        fringe.erase(fringe.begin());
        //out.print("line %d: fringe size = %d, localPtsFound = %d / %d, closedSetSize = %d\n", __LINE__, fringe.size(), localPtsFound, localPts.size(), closedSet.size());
        if ( closedSet.find(pt) != closedSet.end() ) {
            out.print("Double closure! Bad!\n");
            break;
        }
        //closedSet.insert(pt);
        
        if ( localPts.find(pt) != localPts.end() ) {
            localPtsFound++;
            if ( localPtsFound >= localPts.size() )
                break;
            if ( costMap[pt].cost[1] > 0 || costMap[pt].cost[2] > 0 || costMap[pt].cost[3] > 0 )
                break;
        }

        Cost& myCost = costMap[pt];
        clock_t edgeTime = clock();
        vector<Edge>* myEdges = getEdgeSet(out, pt, cache, xMax, yMax, zMax);
        totalEdgeTime += (clock() - edgeTime);
        for ( auto a = myEdges->begin(); a != myEdges->end(); a++ ) {
            Edge &e = *a;
            df::coord& other = e.p1;
            if ( other == pt )
                other = e.p2;
            if ( costMap.find(other) == costMap.end() || costMap[other] > myCost + e.cost ) {
                fringe.erase(other);
                costMap[other] = myCost + e.cost;
                fringe.insert(other);
                parentMap[other] = pt;
            }
        }
        delete myEdges;
    }
    clock_t time = clock() - t0;
    out.print("time = %d, totalEdgeTime = %d\n", time, totalEdgeTime);

    //find important edges
    list<Edge> importantEdges;
    for ( auto i = localPts.begin(); i != localPts.end(); i++ ) {
        df::coord pt = *i;
        if ( costMap.find(pt) == costMap.end() )
            continue;
        if ( parentMap.find(pt) == parentMap.end() )
            continue;
        if ( costMap[pt].cost[1] == 0 && costMap[pt].cost[2] == 0 )
            continue;
        while ( parentMap.find(pt) != parentMap.end() ) {
            //out.print("(%d,%d,%d)\n", pt.x, pt.y, pt.z);
            df::coord parent = parentMap[pt];
            if ( !Maps::canWalkBetween(pt, parent) ) {
                importantEdges.push_front(Edge(pt,parent,0));
            }
            pt = parent;
        }
        break;
    }

    bool didSomething = false;
    df::coord where;
    for ( auto i = importantEdges.begin(); i != importantEdges.end(); i++ ) {
        Edge e = *i;
        df::coord pt1 = e.p1;
        df::coord pt2 = e.p2;
        if ( costMap[e.p2] < costMap[e.p1] ) {
            pt1 = e.p2;
            pt2 = e.p1;
        }
        df::building* building = Buildings::findAtTile(pt2);
        if ( building != NULL ) {
            building->flags.bits.almost_deleted = true;
            didSomething = true;
            where = pt2;
            break;
        } else {
            df::map_block* block1 = Maps::getTileBlock(pt1);
            df::map_block* block2 = Maps::getTileBlock(pt2);
            df::tiletype* type1 = Maps::getTileType(pt1);
            df::tiletype* type2 = Maps::getTileType(pt2);
            df::tiletype_shape shape1 = ENUM_ATTR(tiletype, shape, *type1);
            df::tiletype_shape shape2 = ENUM_ATTR(tiletype, shape, *type2);

            if ( pt1.z != pt2.z && shape1 != df::enums::tiletype_shape::STAIR_DOWN && shape1 != df::enums::tiletype_shape::STAIR_UPDOWN ) {
                block1->tiletype[pt2.x&0x0F][pt2.y&0x0F] = df::enums::tiletype::ConstructedStairUD;
                where = pt2;
                didSomething = true;
                break;
            }

            if ( ENUM_ATTR(tiletype_shape, basic_shape, shape2) == df::enums::tiletype_shape_basic::Wall ) {
                block2->tiletype[pt2.x&0x0F][pt2.y&0x0F] = df::enums::tiletype::ConstructedStairUD;
                didSomething = true;
                where = pt2;
                break;
            }
        }
    }

    if ( !didSomething )
        return;
    
    Cost cost = costMap[where];
    int32_t cost_tick = 0;
    for ( size_t a = 0; a < costDim; a++ ) {
        cost_tick += cost.cost[a];
    }
    
    EventManager::EventHandler handle(doDiggingInvaders);
    Plugin* me = Core::getInstance().getPluginManager()->getPluginByName("diggingInvaders");
    EventManager::registerTick(handle, cost_tick, me);
    df::global::world->reindex_pathfinding = true;
}

bool important(df::coord pos, map<df::coord, set<Edge> >& edges, df::coord prev, set<df::coord>& importantPoints, set<Edge>& importantEdges) {
    //glob_out->print("oh my glob; (%d,%d,%d)\n", pos.x,pos.y,pos.z);
    set<Edge>& myEdges = edges[pos];
    bool result = importantPoints.find(pos) != importantPoints.end();
    for ( auto i = myEdges.begin(); i != myEdges.end(); i++ ) {
        Edge e = *i;
        df::coord other = e.p1;
        if ( other == pos )
            other = e.p2;
        if ( other == prev )
            continue;
        if ( important(other, edges, pos, importantPoints, importantEdges) ) {
            result = true;
            importantEdges.insert(e);
        }
    }
    return result;
}

int32_t getDestroyCost(df::building* building) {
    return 1000 / destroySpeed;
#if 0 
    if ( building->mat_type != 0 ) {
        cerr << "Error " << __FILE__ << ", " << __LINE__ << endl;
        exit(1);
    }
    df::inorganic_raw* mat = df::global::world->raws.inorganics[building->mat_index];
    int32_t str = mat->material.strength.fracture[df::enums::strain_type::IMPACT];
    return str / destroySpeed;
#endif
}

int32_t getDigCost(MapExtras::MapCache& cache, df::coord point) {
    //TODO: check for constructions
    return 10000 / digSpeed;
#if 0
    df::tiletype* type = Maps::getTileType(point);
    df::enums::tiletype_material::tiletype_material ttmat = ENUM_ATTR(tiletype, material, *type);
    if ( ttmat == df::enums::tiletype_material::CONSTRUCTION ) {
        //construction stuff
        return 1;
    } else if ( ttmat == df::enums::tiletype_material::SOIL ) {
        
    }
    
    return 1;
#endif
}

int32_t getDeconstructCost(df::coord point) {
    return 10000 / deconstructSpeed;
}

int32_t getConstructCost(df::coord point) {
    return 100000 / constructSpeed;
}

vector<Edge>* getEdgeSet(color_ostream &out, df::coord point, MapExtras::MapCache& cache, int32_t xMax, int32_t yMax, int32_t zMax) {
    vector<Edge>* result = new vector<Edge>;
    
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
                Cost cost = 1;
                //if ( dz != 0 ) cost++;
                if ( Maps::canWalkBetween(point, neighbor) ) {
                    Edge edge(point, neighbor, cost);
                    result->push_back(edge);
                } else {
                    //cost.cost[1] = 1;
                    //find out WHY we can't walk there
                    //make it simple: don't deal with unallocated blocks
                    Maps::ensureTileBlock(point);
                    Maps::ensureTileBlock(neighbor);
                    df::tiletype* type1 = Maps::getTileType(point);
                    df::tiletype* type2 = Maps::getTileType(neighbor);
                    df::map_block* block1 = Maps::getTileBlock(point);
                    df::map_block* block2 = Maps::getTileBlock(neighbor);
                    
                    df::tiletype_shape shape1 = ENUM_ATTR(tiletype, shape, *type1);
                    df::tiletype_shape shape2 = ENUM_ATTR(tiletype, shape, *type2);

                    {
                        df::building* building1 = Buildings::findAtTile(point);
                        df::building* building2 = Buildings::findAtTile(neighbor);
                        if ( building2 != NULL && building2 != building1 ) {
                            cost.cost[1] += getDestroyCost(building2);
                            if ( dz != 0 )
                                continue;
                        }
                    }

                    if ( shape2 == df::enums::tiletype_shape::EMPTY ) {
                        cost.cost[3] += getConstructCost(neighbor);
                    } else {
                        if ( point.z == neighbor.z ) {
                            if ( ENUM_ATTR(tiletype_shape, walkable, shape2) ) {
                                if ( ENUM_ATTR(tiletype_shape, walkable, shape1 ) ) {
                                    //exit(1);
                                    //must be building impassible tile or something
                                    //TODO: check
                                    df::building* building = Buildings::findAtTile(neighbor);
                                    if ( building != NULL )
                                        cost.cost[1]+=getDestroyCost(building);
                                    else {
                                        building = Buildings::findAtTile(point);
                                        if ( building == NULL ) {
                                            //out.print("%s, %d: (%d,%d,%d), (%d,%d,%d)\n", __FILE__, __LINE__, point.x,point.y,point.z, neighbor.x,neighbor.y,neighbor.z);
                                            //exit(1);
                                            //TODO: deal with the silly RAMP_TOP condition
                                            continue;
                                        }
                                    }
                                }
                                //this is fine: only charge once for digging through a wall
                            } else {
                                cost.cost[2] += getDigCost(cache, neighbor);
                            }
                        } else {
                            bool ascending = neighbor.z > point.z;
                            /*df::tiletype_shape temp;
                            if ( neighbor.z > point.z ) {
                                temp = shape1;
                                shape1 = shape2;
                                shape2 = temp;
                            }*/
                            if ( point.x == neighbor.x && point.y == neighbor.y ) {
                                if ( ENUM_ATTR(tiletype_shape, basic_shape, shape2) == df::enums::tiletype_shape_basic::Stair ) {
                                    if ( (ascending && ENUM_ATTR(tiletype_shape, passable_low, shape2)) || (!ascending && ENUM_ATTR(tiletype_shape, walkable_up, shape2)) ) {
                                        //must be a forbidden hatch: TODO: check
                                        df::building* building = Buildings::findAtTile(dz < 0 ? point : neighbor);
                                        if ( building != NULL ) {
                                            cost.cost[1] += getDestroyCost(building);
                                        } else {
                                            out.print("%s, line %d: Weirdness (%d,%d,%d), (%d,%d,%d).\n", __FILE__, __LINE__, point.x,point.y,point.z, neighbor.x,neighbor.y,neighbor.z);
                                        }
                                    } else {
                                        //too complicated
                                        continue;
                                    }
                                } else {
                                    //bad!
                                    if ( ENUM_ATTR(tiletype_shape, basic_shape, shape2) == df::enums::tiletype_shape_basic::Wall ) {
                                        cost.cost[2] += getDigCost(cache, neighbor);
                                    } else if ( ENUM_ATTR(tiletype_shape, basic_shape, shape2) == df::enums::tiletype_shape_basic::Open ) {
                                        cost.cost[3] += getConstructCost(neighbor);
                                    } else {
                                        continue;
                                    }
                                }
                            } else {
                                //too complicated
                                continue;
                            }
                        }
                    }
                    
                    Edge edge(point, neighbor, cost);
                    result->push_back(edge);
                }
            }
        }
    }

    return result;
}

df::coord getRoot(df::coord point, map<df::coord, df::coord>& rootMap) {
    map<df::coord, df::coord>::iterator i = rootMap.find(point);
    if ( i == rootMap.end() ) {
        rootMap[point] = point;
        return point;
    }
    df::coord parent = (*i).second;
    if ( parent == point )
        return parent;
    df::coord root = getRoot(parent, rootMap);
    rootMap[point] = root;
    return root;
}


