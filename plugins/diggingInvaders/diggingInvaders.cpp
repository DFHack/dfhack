#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include "DataDefs.h"
#include "df/building.h"
#include "df/coord.h"
#include "df/map_block.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/world.h"

#include "Types.h"
#include "modules/Buildings.h"
	//crashes without Types.h
#include "modules/MapCache.h"
#include "modules/World.h"

#include <vector>
#include <algorithm>
#include <map>
#include <set>
using namespace std;

using namespace DFHack;
using namespace df::enums;

command_result diggingInvadersFunc(color_ostream &out, std::vector <std::string> & parameters);

DFHACK_PLUGIN("diggingInvaders");

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    //out.print("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA!\n\n\n\n\n");
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "diggingInvaders", "Makes invaders dig to your dwarves.",
        diggingInvadersFunc, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "EXTRA HELP STRINGGNGNGNGNGNNGG.\n"
    ));
    return CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    // You *MUST* kill all threads you created before this returns.
    // If everything fails, just return CR_FAILURE. Your plugin will be
    // in a zombie state, but things won't crash.
    return CR_OK;
}

// Called to notify the plugin about important state changes.
// Invoked with DF suspended, and always before the matching plugin_onupdate.
// More event codes may be added in the future.
/*
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_GAME_LOADED:
        // initialize from the world just loaded
        break;
    case SC_GAME_UNLOADED:
        // cleanup
        break;
    default:
        break;
    }
    return CR_OK;
}
*/

// Whatever you put here will be done in each game step. Don't abuse it.
// It's optional, so you can just comment it out like this if you don't need it.
/*
DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    // whetever. You don't need to suspend DF execution here.
    return CR_OK;
}
*/

/*class CompareEdge {
    bool operator()(edge e1, edge e2) {
        
    }
};*/

class Edge {
public:
    //static map<df::coord, int> pointCost;
    df::coord p1;
    df::coord p2;
    int cost;
    Edge(df::coord p1In, df::coord p2In, int costIn): p1(p1In), p2(p2In), cost(costIn) {
        
    }
    
    /*bool operator<(const Edge e) const {
        int pCost = max(pointCost[p1], pointCost[p2]) + cost;
        int e_pCost = max(pointCost[e.p1], pointCost[e.p2]) + e.cost;
        if ( pCost != e_pCost )
            return pCost < e_pCost;
        if ( p1 != e.p1 )
            return p1 < e.p1;
        return p2 < e.p2;
    }*/
};

vector<Edge>* getEdgeSet(color_ostream &out, df::coord point, MapExtras::MapCache& cache, int xMax, int yMax, int zMax);
df::coord getRoot(df::coord point, map<df::coord, df::coord>& rootMap);

class PointComp {
public:
    map<df::coord, int> *pointCost;
    PointComp(map<df::coord, int> *p): pointCost(p) {
        
    }
    
    int operator()(df::coord p1, df::coord p2) {
        map<df::coord, int>::iterator i1 = pointCost->find(p1);
        map<df::coord, int>::iterator i2 = pointCost->find(p2);
        if ( i1 == pointCost->end() && i2 == pointCost->end() )
            return p1 < p2;
        if ( i1 == pointCost->end() )
            return 1;
        if ( i2 == pointCost->end() )
            return -1;
        int c1 = (*i1).second;
        int c2 = (*i2).second;
        if ( c1 != c2 )
            return c1 < c2;
        return p1 < p2;
    }
};

// A command! It sits around and looks pretty. And it's nice and friendly.
command_result diggingInvadersFunc(color_ostream &out, std::vector <std::string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    CoreSuspender suspend;
    
    //eventually we're going to want a path from each surviving invader to each dwarf, but for now, let's just do from each dwarf to each dwarf
    int32_t race_id = df::global::ui->race_id;
    int32_t civ_id = df::global::ui->civ_id;
    
    uint32_t xMax, yMax, zMax;
    Maps::getSize(xMax,yMax,zMax);
    xMax *= 16;
    yMax *= 16;
    MapExtras::MapCache cache;
    
    //TODO: consider whether to pursue hidden dwarf diplomats and merchants
    vector<df::unit*> locals;
    vector<df::unit*> invaders;
    map<df::coord, int> dwarfCount;
    //map<df::coord, set<Edge>*> edgeSet;
    map<df::coord, df::coord> rootMap;
    map<df::coord, df::coord> parentMap;
    map<df::coord, int> pointCost;
    PointComp comp(&pointCost);
    set<df::coord, PointComp> fringe(comp);
    for ( size_t a = 0; a < df::global::world->units.all.size(); a++ ) {
        df::unit* unit = df::global::world->units.all[a];
        bool isInvader = false;
        if ( df::global::ui->invasions.next_id > 0 && unit->invasion_id+1 == df::global::ui->invasions.next_id ) {
            invaders.push_back(unit);
            //dwarfCount[unit->pos]++;
            isInvader = true;
        }
        
        if ( (!isInvader && (unit->race != race_id || unit->civ_id != civ_id)) || unit->flags1.bits.dead )
            continue;
        
        if ( !isInvader )
            locals.push_back(unit);
        dwarfCount[unit->pos]++;
        //edgeSet[unit->pos] = getEdgeSet(unit->pos, cache, xMax, yMax, zMax);
        rootMap[unit->pos] = unit->pos;
        parentMap[unit->pos] = unit->pos;
        pointCost[unit->pos] = 0;
        fringe.insert(unit->pos);
    }
    
    //TODO: if only one connectivity group, return
    if ( invaders.size() == 0 ) {
        return CR_OK; //no invaders, no problem!
    }
    set<df::coord> importantPoints;
    int a=0;
    int dwarvesFound = 1;
    while(dwarvesFound < invaders.size()+locals.size() && fringe.size() > 0) {
        df::coord point = *fringe.begin();
        //out.print("%d: (%d,%d,%d); dwarvesFound = %d\n", a++, (int)point.x, (int)point.y, (int)point.z, dwarvesFound);
        //if ( a > 10000 ) break;
        fringe.erase(fringe.begin());
        //dwarfCount[getRoot(point, rootMap)] += dwarfCount[point];
        
        if ( getRoot(point, rootMap) != point && dwarfCount[point] != 0 ) {
            dwarfCount[getRoot(point, rootMap)] += dwarfCount[point];
        }
        
        int costSoFar = pointCost[point];
        vector<Edge>* neighbors = getEdgeSet(out, point, cache, xMax, yMax, zMax);
        for ( size_t a = 0; a < neighbors->size(); a++ ) {
            df::coord neighbor = (*neighbors)[a].p2;
            int neighborCost;
            if ( pointCost.find(neighbor) == pointCost.end() )
                neighborCost = -1;
            else
                neighborCost = pointCost[neighbor];
            if ( neighborCost == -1 || neighborCost > costSoFar + (*neighbors)[a].cost ) {
                fringe.erase(neighbor);
                pointCost[neighbor] = costSoFar + (*neighbors)[a].cost;
                parentMap[neighbor] = point;
                //if ( getRoot(neighbor, rootMap) == neighbor )
                rootMap[neighbor] = rootMap[point];
                fringe.insert(neighbor);
            }
            df::coord pointRoot = getRoot(point, rootMap);
            df::coord neighborRoot = getRoot(neighbor, rootMap);
            //check for unified sections of the map
            if ( neighborRoot != neighbor && neighborRoot != pointRoot ) {
                //dwarvesFound++;
                dwarfCount[pointRoot] += dwarfCount[neighborRoot];
                dwarfCount[neighborRoot] = 0;
                dwarvesFound = max(dwarvesFound, dwarfCount[pointRoot]);
                rootMap[neighborRoot] = rootMap[pointRoot];
                
                df::coord temp = point;
                while(true) {
                    importantPoints.insert(temp);
                    if ( parentMap[temp] != temp )
                        temp = parentMap[temp];
                    else break;
                }
                temp = neighbor;
                while(true) {
                    importantPoints.insert(temp);
                    if ( parentMap[temp] != temp )
                        temp = parentMap[temp];
                    else break;
                }
            }
        }
        delete neighbors;
    }
    out.print("dwarves found: %d\n", dwarvesFound);
    
    out.print("Important points:\n");
    for ( set<df::coord>::iterator i = importantPoints.begin(); i != importantPoints.end(); i++ ) {
        df::coord point = *i;
        out.print("  (%d, %d, %d)\n", (int)point.x, (int)point.y, (int)point.z);
    }
    
    //dig out all the important points
    for ( set<df::coord>::iterator i = importantPoints.begin(); i != importantPoints.end(); i++ ) {
        df::coord point = *i;
        
        //deal with buildings, hatches, and doors
        {
            df::map_block* block = cache.BlockAt(df::coord((point.x)/16, (point.y)/16, point.z))->getRaw();
            /*if ( block == NULL ) {
                continue;
            }*/
            df::tiletype type = cache.tiletypeAt(point);
            df::tiletype_shape shape = tileShape(type);
            df::tiletype_shape_basic basic = ENUM_ATTR(tiletype_shape, basic_shape, shape);
            df::tile_building_occ building_occ = block->occupancy[point.x%16][point.y%16].bits.building;
            int z = point.z;
            if ( basic == df::tiletype_shape_basic::Ramp && building_occ == df::tile_building_occ::None ) {
                df::map_block* block2 = cache.BlockAt(df::coord(point.x/16, point.y/16, point.z+1))->getRaw();
                if ( block2 != NULL ) {
                    building_occ = block2->occupancy[point.x%16][point.y%16].bits.building;
                    z = z+1;
                    if ( building_occ != df::tile_building_occ::None ) {
                        //if it doesn't block pathing, don't destroy it
                        
                    }
                }
            }
            if ( building_occ != df::tile_building_occ::None ) {
                //find the building there
                bool foundIt = false;
                for( size_t a = 0; a < df::global::world->buildings.all.size(); a++ ) {
                    df::building* building = df::global::world->buildings.all[a];
                    if ( z != building->z )
                        continue;
                    if ( building->x1 < point.x || building->x2 > point.x )
                        continue;
                    if ( building->y1 < point.y || building->y2 > point.y )
                        continue;
                    //found it!
                    foundIt = true;
                    //destroy it
					out.print("deconstructImmediately...\n");
					DFHack::Buildings::deconstructImmediately(building);
					out.print("done\n");
					building = NULL;
                    //building->deconstructItems(false, false);
                    //building->removeUses(false, false);
                    break;
                }
                if ( !foundIt ) {
                    out.print("Error: could not find building at (%d,%d,%d).\n", point.x, point.y, point.z);
                }
            }
        }
        
        df::tiletype type = cache.tiletypeAt(point);
        df::tiletype_shape shape = tileShape(type);
        if ( shape == df::tiletype_shape::STAIR_UPDOWN )
            continue;
        df::tiletype_shape_basic basicShape = ENUM_ATTR(tiletype_shape, basic_shape, shape);
        bool uppyDowny;
        {
            //only needs to change if we need uppy-downiness
            df::coord up = df::coord(point.x, point.y, point.z+1);
            df::coord down = df::coord(point.x, point.y, point.z-1);
            uppyDowny = !(importantPoints.find(up) == importantPoints.end() && importantPoints.find(down) == importantPoints.end());
        }
        if ( (basicShape == df::tiletype_shape_basic::Floor || basicShape == df::tiletype_shape_basic::Ramp) && !uppyDowny ) {
            continue;
        }
        
        if ( uppyDowny ) {
            cache.setTiletypeAt(point, df::tiletype::StoneStairUD);
        } else {
            cache.setTiletypeAt(point, df::tiletype::StoneFloor1);
        }
    }
    
    cache.WriteAll();
    return CR_OK;
}

vector<Edge>* getEdgeSet(color_ostream &out, df::coord point, MapExtras::MapCache& cache, int xMax, int yMax, int zMax) {
    vector<df::coord> candidates;
    for ( int dx = -1; dx <= 1; dx++ ) {
        if ( point.x + dx < 0 ) continue;
        for ( int dy = -1; dy <= 1; dy++ ) {
            if ( point.y + dy < 0 ) continue;
            if ( dx == 0 && dy == 0)
                continue;
            candidates.push_back(df::coord(point.x+dx,point.y+dy,point.z));
        }
    }
    for ( int dz = -1; dz <= 1; dz++ ) {
        if ( point.z + dz < 0 ) continue;
        if ( dz == 0 ) continue;
        candidates.push_back(df::coord(point.x, point.y, point.z+dz));
    }
    int connectivityType;
    {
        df::map_block* block = cache.BlockAt(df::coord(point.x/16, point.y/16, point.z))->getRaw();
        if ( block == NULL ) {
            return new vector<Edge>;
        }
        connectivityType = block->walkable[point.x%16][point.y%16];
    }
    if ( connectivityType == 0 )
        return new vector<Edge>;
    for ( int dx = -1; dx <= 1; dx++ ) {
        if ( point.x + dx < 0 )
            continue;
        for ( int dy = -1; dy <= 1; dy++ ) {
            if ( point.y + dy < 0 )
                continue;
            for ( int dz = -1; dz <= 1; dz++ ) {
                if ( dz == 0 ) continue;
                if ( point.z + dz < 0 )
                    continue;
                if ( dx == 0 && dy == 0 )
                    continue;
                df::map_block* block = cache.BlockAt(df::coord((point.x+dx)/16, (point.y+dy)/16, point.z+dz))->getRaw();
                if ( block == NULL ) {
                    continue;
                }
                if ( block->walkable[(point.x+dx)%16][(point.y+dy)%16] != connectivityType ) {
                    continue;
                }
                candidates.push_back(df::coord(point.x+dx, point.y+dy, point.z+dz));
            }
        }
    }
    
    //TODO: ramps, buildings
    
    vector<Edge>* result = new vector<Edge>;
    df::tiletype_shape_basic basePointBasicShape;
    bool basePointIsWall;
    {
        df::tiletype type = cache.tiletypeAt(point);
        df::tiletype_shape shape = tileShape(type);
        if ( shape == df::tiletype_shape::EMPTY )
            return result;
        basePointBasicShape = ENUM_ATTR(tiletype_shape, basic_shape, shape);
        //TODO: worry about up stairs vs down stairs vs updown stairs
    }
    
    if ( basePointBasicShape == df::tiletype_shape_basic::Wall && cache.hasConstructionAt(point) )
        return result;
    
    /*if ( point.z < zMax-1 ) {
        //ramps part 1: going up
        //if I'm a ramp, and there's a wall in some direction, and there's nothing above me, and that tile is open, I can go there.
        df::tiletype_shape_basic upBasicShape;
        {
            df::tiletype type = cache.tiletypeAt(df::coord(point.x, point.y, point.z+1));
            df::tiletype_shape shape = tileShape(type);
            upBasicShape = ENUM_ATTR(tiletype_shape, basic_shape, shape);
        }
        if ( upBasicShape == df::tiletype_shape_basic::Ramp ) {
            for ( int dx = -1; dx <= 1; dx++ ) {
                for ( int dy = -1; dy <= 1; dy++ ) {
                    if ( dx == 0 && dy == 0 )
                        continue;
                    df::tiletype type = cache.tiletypeAt(df::coord(point.x+dx, point.y+dy, point.z+1));
                    df::tiletype_shape shape = tileShape(type);
                    df::tiletype_shape_basic basicShape = ENUM_ATTR(tiletype_shape, basic_shape, shape);
                    if ( basicShape == df::tiletype_shape_basic::Floor ||
                        basicShape == df::tiletype_shape_basic::Stair ||
                        basicShape == df::tiletype_shape_basic::Ramp ) {
                        candidates.push_back(df::coord(point.x+dx, point.y+dy, point.z+1));
                    }
                }
            }
        }
    }
    
    if ( point.z >= 1 ) {
        //ramps part 2: going down
        
    }*/
    
    for ( size_t a = 0; a < candidates.size(); a++ ) {
        if ( candidates[a].x <= 1 || candidates[a].x >= xMax-1
            || candidates[a].y <= 1 || candidates[a].y >= yMax-1
            || candidates[a].z <= 1 || candidates[a].z >= zMax-1
        ) {
            continue;
        }
        df::tiletype type = cache.tiletypeAt(candidates[a]);
        df::tiletype_shape shape = tileShape(type); //what namespace?
        if ( shape == df::tiletype_shape::EMPTY )
            continue;
        df::tiletype_shape_basic basicShape = ENUM_ATTR(tiletype_shape, basic_shape, shape);
        if ( basicShape == df::tiletype_shape_basic::Wall && cache.hasConstructionAt(candidates[a]) ) {
            continue;
        }
        
        //if it's a forbidden door, continue
        df::map_block* block = cache.BlockAt(df::coord(candidates[a].x/16, candidates[a].y/16, candidates[a].z))->getRaw();
        if ( block == NULL ) {
            continue;
        } else {
            df::tile_building_occ building_occ = block->occupancy[candidates[a].x%16][candidates[a].y%16].bits.building;
            if ( building_occ == df::tile_building_occ::Obstacle )
                continue;
            if ( building_occ == df::tile_building_occ::Impassable )
                continue;
            if ( building_occ == df::tile_building_occ::Well )
                continue;
            if ( building_occ == df::tile_building_occ::Dynamic ) {
                //continue; //TODO: check df.map.xml.walkable
            }
        }
        
        int cost = 1;
        if ( basePointIsWall || basicShape == df::tiletype_shape_basic::Wall ) {
            cost += 1000000; //TODO: fancy cost
        }
        //if ( candidates[a] < point ) {
        //    result->push_back(Edge(candidates[a], point, cost));
        //} else {
        result->push_back(Edge(point, candidates[a], cost));
        //}
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


