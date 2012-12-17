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
#include "df/map_block.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/world.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>
using namespace std;

using namespace DFHack;
using namespace df::enums;

///////////////////////
color_ostream* glob_out;

#if 0
#define DEBUG_PRINT(str) \
out.print("%s, line %d" STR, __FILE__, __LINE__);
#endif

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
    //static map<df::coord, int32_t> pointCost;
    df::coord p1;
    df::coord p2;
    int32_t cost;
    Edge(df::coord p1In, df::coord p2In, int32_t costIn): cost(costIn) {
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
    
    /*bool operator<(const Edge e) const {
        int32_t pCost = max(pointCost[p1], pointCost[p2]) + cost;
        int32_t e_pCost = max(pointCost[e.p1], pointCost[e.p2]) + e.cost;
        if ( pCost != e_pCost )
            return pCost < e_pCost;
        if ( p1 != e.p1 )
            return p1 < e.p1;
        return p2 < e.p2;
    }*/
};

vector<Edge>* getEdgeSet(color_ostream &out, df::coord point, int32_t xMax, int32_t yMax, int32_t zMax);
df::coord getRoot(df::coord point, map<df::coord, df::coord>& rootMap);

class PointComp {
public:
    map<df::coord, int32_t> *pointCost;
    PointComp(map<df::coord, int32_t> *p): pointCost(p) {
        
    }
    
    int32_t operator()(df::coord p1, df::coord p2) {
        if ( p1 == p2 ) return 0;
        map<df::coord, int32_t>::iterator i1 = pointCost->find(p1);
        map<df::coord, int32_t>::iterator i2 = pointCost->find(p2);
        if ( i1 == pointCost->end() && i2 == pointCost->end() )
            return p1 < p2;
        if ( i1 == pointCost->end() )
            return 1;
        if ( i2 == pointCost->end() )
            return -1;
        int32_t c1 = (*i1).second;
        int32_t c2 = (*i2).second;
        if ( c1 != c2 )
            return c1 < c2;
        return p1 < p2;
    }
};

bool important(df::coord pos, map<df::coord, set<Edge> >& edges, df::coord prev, set<df::coord>& importantPoints, set<Edge>& importantEdges);

command_result diggingInvadersFunc(color_ostream& out, std::vector<std::string>& parameters) {
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    CoreSuspender suspend;
    
    map<df::coord, set<Edge> > edgeSet;
    set<df::coord> invaderPts;
    set<df::coord> localPts;
    map<df::coord, df::coord> parentMap;
    map<df::coord, int32_t> costMap;
    PointComp comp(&costMap);
    set<df::coord, PointComp> fringe(comp);
    uint32_t xMax, yMax, zMax;
    Maps::getSize(xMax,yMax,zMax);
    xMax *= 16;
    yMax *= 16;

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
        
        vector<Edge>* neighbors = getEdgeSet(out, unit->pos, xMax, yMax, zMax);
        set<Edge>& rootEdges = edgeSet[unit->pos];
        for ( auto i = neighbors->begin(); i != neighbors->end(); i++ ) {
            Edge edge = *i;
            rootEdges.insert(edge);
        }
        delete neighbors;
    }

    int32_t localPtsFound = 0;
    set<df::coord> closedSet;

    while(!fringe.empty()) {
        df::coord pt = *(fringe.begin());
        fringe.erase(fringe.begin());
        out.print("line %d: fringe size = %d, localPtsFound = %d / %d, closedSetSize = %d\n", __LINE__, fringe.size(), localPtsFound, localPts.size(), closedSet.size());
        if ( closedSet.find(pt) != closedSet.end() ) {
            out.print("Double closure! Bad!\n");
            break;
        }
        closedSet.insert(pt);
        
        if ( localPts.find(pt) != localPts.end() ) {
            localPtsFound++;
            if ( localPtsFound >= localPts.size() )
                break;
            if ( costMap[pt] > 0 )
                break;
        }

        if ( edgeSet.find(pt) == edgeSet.end() ) {
            set<Edge>& temp = edgeSet[pt];
            vector<Edge>* edges = getEdgeSet(out, pt, xMax, yMax, zMax);
            for ( auto a = edges->begin(); a != edges->end(); a++ ) {
                Edge e = *a;
                temp.insert(e);
            }
            delete edges;
        }
        int32_t myCost = costMap[pt];
        set<Edge>& myEdges = edgeSet[pt];
        for ( auto a = myEdges.begin(); a != myEdges.end(); a++ ) {
            Edge e = *a;
            df::coord other = e.p1;
            if ( other == pt )
                other = e.p2;
            if ( costMap.find(other) == costMap.end() || costMap[other] > myCost + e.cost ) {
                fringe.erase(other);
                costMap[other] = myCost + e.cost;
                fringe.insert(other);
                parentMap[other] = pt;
            }
        }
        edgeSet.erase(pt);
    }

    //find important edges
    set<Edge> importantEdges;
    map<df::coord, int32_t> importance;
    for ( auto i = localPts.begin(); i != localPts.end(); i++ ) {
        df::coord pt = *i;
        if ( costMap.find(pt) == costMap.end() )
            continue;
        if ( parentMap.find(pt) == parentMap.end() )
            continue;
        while ( parentMap.find(pt) != parentMap.end() ) {
            df::coord parent = parentMap[pt];
            if ( !Maps::canWalkBetween(pt, parent) ) {
                importantEdges.insert(Edge(pt,parent,1));
            }
            pt = parent;
        }
    }

    for ( auto i = importantEdges.begin(); i != importantEdges.end(); i++ ) {
        Edge e = *i;
        /*if ( e.p1.z == e.p2.z ) {
            
        }*/
        importance[e.p1]++;
        importance[e.p2]++;
    }

    //dig important points
    for ( auto a = importance.begin(); a != importance.end(); a++ ) {
        df::coord pos = (*a).first;
        int32_t cost = (*a).second;
        if ( cost < 1 )
            continue;

        out.print("Requires action: (%d,%d,%d): %d\n", pos.x,pos.y,pos.z, cost);
        df::map_block* block = Maps::getTileBlock(pos);
        block->tiletype[pos.x&0x0F][pos.y&0x0F] = df::enums::tiletype::ConstructedStairUD;
    }

#if 0
    /*for ( auto a = importantPoints.begin(); a != importantPoints.end(); a++ ) {
        df::coord pos = (*a);
        out.print("Important point: (%d,%d,%d)\n", pos.x,pos.y,pos.z);
    }*/
#endif

    return CR_OK;
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

vector<Edge>* getEdgeSet(color_ostream &out, df::coord point, int32_t xMax, int32_t yMax, int32_t zMax) {
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
                int32_t cost = 0;
                //if ( dz != 0 ) cost++;
                if ( Maps::canWalkBetween(point, neighbor) ) {
                    Edge edge(point, neighbor, cost+0);
                    result->push_back(edge);
                } else {
                    Edge edge(point, neighbor, cost+1);
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


