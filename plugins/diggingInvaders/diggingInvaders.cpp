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
        return (p1 == e.p1 && p2 == e.p2);
    }

    bool operator<(const Edge& e) const {
        if ( p1 != e.p1 )
            return p1 < e.p1;
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
    set<df::coord> roots;
    set<df::coord> importantPoints;
    map<df::coord, df::coord> rootMap;
    uint32_t xMax, yMax, zMax;
    Maps::getSize(xMax,yMax,zMax);
    xMax *= 16;
    yMax *= 16;

    //find all locals and invaders
    for ( size_t a = 0; a < df::global::world->units.active.size(); a++ ) {
        df::unit* unit = df::global::world->units.active[a];
        if ( unit->flags1.bits.dead )
            continue;
        if ( !Units::isCitizen(unit) && !unit->flags1.bits.active_invader )
            continue;
        if ( unit->flags2.bits.resident ) {
            out.print("resident\n");
        }
        
        if ( roots.find(unit->pos) != roots.end() )
            continue;

        roots.insert(unit->pos);
        importantPoints.insert(unit->pos);
        vector<Edge>* neighbors = getEdgeSet(out, unit->pos, xMax, yMax, zMax);
        set<Edge>& rootEdges = edgeSet[unit->pos];
        for ( auto i = neighbors->begin(); i != neighbors->end(); i++ ) {
            Edge edge = *i;
            rootEdges.insert(edge);
        }
        delete neighbors;
    }

    set<Edge> importantEdges;

    int32_t dumb = 0;
    while(roots.size() > 1) {
        if ( dumb >= 1000 )
            break;
        dumb++;
        set<df::coord> toDelete;
        int32_t firstSize = edgeSet[*roots.begin()].size();
        //out.print("%s, %d: root size = %d, first size = %d\n", __FILE__, __LINE__, roots.size(), firstSize);
        for ( auto i = roots.begin(); i != roots.end(); i++ ) {
            df::coord root = *i;
            //out.print("  (%d,%d,%d)\n", root.x, root.y, root.z);
            if ( toDelete.find(root) != toDelete.end() )
                continue;
            if ( edgeSet[root].empty() ) {
                out.print("%s, %d: Error: no edges: %d, %d, %d\n", __FILE__, __LINE__, root.x, root.y, root.z);
                return CR_FAILURE;
            }
            set<Edge>& myEdges = edgeSet[root];
            Edge edge = *myEdges.begin();
            myEdges.erase(myEdges.begin());
            if ( edgeSet[root].size() != myEdges.size() ) {
                out.print("DOOOOOM! %s, %d\n", __FILE__, __LINE__);
                return CR_FAILURE;
            }
            if ( getRoot(edge.p1, rootMap) != root && getRoot(edge.p2, rootMap) != root ) {
                out.print("%s, %d: Invalid edge.\n", __FILE__, __LINE__);
                return CR_FAILURE;
            }
            
            df::coord other = edge.p1;
            if ( getRoot(other, rootMap) == root )
                other = edge.p2;
            if ( getRoot(other, rootMap) == root ) {
                //out.print("%s, %d: Error: self edge: %d, %d, %d\n", __FILE__, __LINE__, root.x, root.y, root.z);
                /*vector<Edge> badEdges;
                for ( auto j = myEdges.begin(); j != myEdges.end(); j++ ) {
                    Edge e = *j;
                    if ( getRoot(e.p1, rootMap) == getRoot(e.p2, rootMap) )
                        badEdges.push_back(e);
                }
                for ( size_t j = 0; j < badEdges.size(); j++ ) {
                    myEdges.erase(badEdges[j]);
                }*/
                continue;
            }

            importantEdges.insert(edge);

            df::coord otherRoot = getRoot(other,rootMap);
            rootMap[otherRoot] = root;
            
            //merge his stuff with my stuff
            if ( edgeSet.find(other) == edgeSet.end() ) {
                set<Edge>& hisEdges = edgeSet[other];
                vector<Edge>* neighbors = getEdgeSet(out, other, xMax, yMax, zMax);
                for ( auto i = neighbors->begin(); i != neighbors->end(); i++ ) {
                    Edge edge = *i;
                    hisEdges.insert(edge);
                }
                delete neighbors;
            }
            set<Edge>& hisEdges = edgeSet[other];

            for ( auto j = hisEdges.begin(); j != hisEdges.end(); j++ ) {
                Edge e = *j;
                if ( getRoot(e.p1, rootMap) == getRoot(e.p2, rootMap) )
                    continue;
                df::coord farPt = e.p1;
                if ( farPt == other )
                    farPt = e.p2;
                myEdges.insert(e);
                //myEdges.insert(Edge(root, farPt, e.cost));
            }
            //hisEdges.clear();
            edgeSet.erase(otherRoot);
            toDelete.insert(otherRoot);
        }
        for ( auto j = toDelete.begin(); j != toDelete.end(); j++ ) {
            df::coord bob = *j;
            roots.erase(bob);
        }
    }

    edgeSet.clear();
    for ( auto i = importantEdges.begin(); i != importantEdges.end(); i++ ) {
        Edge e = *i;
        edgeSet[e.p1].insert(e);
        edgeSet[e.p2].insert(e);
    }

    //now we find the edges used along the paths between any two roots
    importantEdges.clear();
    glob_out = &out;
    {
        important(*importantPoints.begin(), edgeSet, df::coord(-1,-1,-1), importantPoints, importantEdges);
    }

    //NOW we filter to see edges that require digging/constructing
    map<df::coord, int32_t> actionable;
    for ( auto a = importantEdges.begin(); a != importantEdges.end(); a++ ) {
        Edge e = *a;
        if ( Maps::canWalkBetween(e.p1, e.p2) )
            continue;
        actionable[e.p1]++;
        if( actionable[e.p1] == 0 ) {
            out.print("fuck\n");
            return CR_FAILURE;
        }
        actionable[e.p2]++;
    }

    for ( auto a = actionable.begin(); a != actionable.end(); a++ ) {
        df::coord pos = (*a).first;
        if ( (*a).second < 2 )
            continue;
        out.print("Requires action: (%d,%d,%d): %d\n", pos.x,pos.y,pos.z, (*a).second);
    }

    /*for ( auto a = importantPoints.begin(); a != importantPoints.end(); a++ ) {
        df::coord pos = (*a);
        out.print("Important point: (%d,%d,%d)\n", pos.x,pos.y,pos.z);
    }*/

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
                if ( dz != 0 && (point.x == 0 || point.y == 0 || point.z == 0 || point.x == xMax-1 || point.y == yMax-1 || point.z == zMax-1 || neighbor.x == 0 || neighbor.y == 0 || neighbor.z == 0 || neighbor.x == xMax-1 || neighbor.y == yMax-1 || neighbor.z == zMax-1) )
                    continue;
                if ( dx == 0 && dy == 0 && dz == 0 )
                    continue;
                int32_t cost = 0;
                if ( dz != 0 ) cost++;
                if ( Maps::canWalkBetween(point, neighbor) ) {
                    Edge edge(point, neighbor, cost+1);
                    result->push_back(edge);
                } else {
                    Edge edge(point, neighbor, cost+100);
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


