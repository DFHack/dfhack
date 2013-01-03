#include "assignJob.h"
#include "edgeCost.h"

#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "Types.h"

#include "modules/Buildings.h"
#include "modules/EventManager.h"
#include "modules/Job.h"
#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/body_part_raw_flags.h"
#include "df/building.h"
#include "df/building_type.h"
#include "df/caste_body_info.h"
#include "df/coord.h"
#include "df/general_ref.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_unit.h"
#include "df/general_ref_unit_holderst.h"
#include "df/general_ref_unit_workerst.h"
#include "df/item.h"
#include "df/itemdef_weaponst.h"
#include "df/item_quality.h"
#include "df/item_weaponst.h"
#include "df/inorganic_raw.h"
#include "df/job.h"
#include "df/job_skill.h"
#include "df/job_type.h"
#include "df/map_block.h"
#include "df/strain_type.h"
#include "df/tile_building_occ.h"
#include "df/tile_occupancy.h"
#include "df/tiletype.h"
#include "df/tiletype_material.h"
#include "df/tiletype_shape.h"
#include "df/tiletype_shape_basic.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/unit_inventory_item.h"
#include "df/world.h"

#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <unordered_set>
#include <unordered_map>

using namespace std;

using namespace DFHack;
using namespace df::enums;

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

df::coord getRoot(df::coord point, unordered_map<df::coord, df::coord>& rootMap);

class PointComp {
public:
    unordered_map<df::coord, int64_t, PointHash> *pointCost;
    PointComp(unordered_map<df::coord, int64_t, PointHash> *p): pointCost(p) {
        
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
        int64_t c1 = (*i1).second;
        int64_t c2 = (*i2).second;
        if ( c1 != c2 )
            return c1 < c2;
        return p1 < p2;
    }
};

//bool important(df::coord pos, map<df::coord, set<Edge> >& edges, df::coord prev, set<df::coord>& importantPoints, set<Edge>& importantEdges);
int32_t findAndAssignInvasionJob(color_ostream& out);

//TODO: when world unloads
static int32_t lastInvasionJob=-1;

void watchForComplete(color_ostream& out, void* ptr) {
    df::job* job = (df::job*)ptr;

    if ( job->id != lastInvasionJob )
        return;
    
    Plugin* me = Core::getInstance().getPluginManager()->getPluginByName("diggingInvaders");
    EventManager::unregisterAll(me);

    lastInvasionJob = -1;
    std::vector<string> parameters;
    diggingInvadersFunc(out, parameters);
}

command_result diggingInvadersFunc(color_ostream& out, std::vector<std::string>& parameters) {
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    if ( lastInvasionJob != -1 ) {
        out.print("Still working on the previous job.\n");
        return CR_OK;
    }
    int32_t jobId = findAndAssignInvasionJob(out);
    if ( jobId == -1 )
        return CR_OK;
    
    lastInvasionJob = jobId;

    EventManager::EventHandler completeHandler(watchForComplete);
    Plugin* me = Core::getInstance().getPluginManager()->getPluginByName("diggingInvaders");
    EventManager::unregisterAll(me);
    
    EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, completeHandler, 5, me);
    
    return CR_OK;
}

int32_t findAndAssignInvasionJob(color_ostream& out) {
    //returns the job id created
    
    CoreSuspender suspend;
    
    unordered_set<df::coord, PointHash> invaderPts;
    unordered_set<df::coord, PointHash> localPts;
    unordered_map<df::coord,df::coord,PointHash> parentMap;
    unordered_map<df::coord,int64_t,PointHash> costMap;

    PointComp comp(&costMap);
    set<df::coord, PointComp> fringe(comp);
    uint32_t xMax, yMax, zMax;
    Maps::getSize(xMax,yMax,zMax);
    xMax *= 16;
    yMax *= 16;
    MapExtras::MapCache cache;
    vector<df::unit*> invaders;

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
            invaders.push_back(unit);
        } else {
            continue;
        }
    }

    if ( invaders.empty() ) {
        return -1;
    }
    df::unit* firstInvader = invaders[0];
    out << firstInvader->id << endl;
    out << firstInvader->pos.x << ", " << firstInvader->pos.y << ", " << firstInvader->pos.z << endl;

    int32_t localPtsFound = 0;
    unordered_set<df::coord,PointHash> closedSet;
    unordered_map<df::coord,int32_t,PointHash> workNeeded; //non-walking work needed to get there

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
        closedSet.insert(pt);
        
        if ( localPts.find(pt) != localPts.end() ) {
            localPtsFound++;
            if ( localPtsFound >= localPts.size() )
                break;
            if ( workNeeded.find(pt) == workNeeded.end() || workNeeded[pt] == 0 ) {
                //there are still dwarves to kill that don't require digging to get to
                return -1;
            }
        }

        int64_t myCost = costMap[pt];
        clock_t edgeTime = clock();
        vector<Edge>* myEdges = getEdgeSet(out, pt, cache, xMax, yMax, zMax);
        totalEdgeTime += (clock() - edgeTime);
        for ( auto a = myEdges->begin(); a != myEdges->end(); a++ ) {
            Edge &e = *a;
            df::coord& other = e.p1;
            if ( other == pt )
                other = e.p2;
            //if ( closedSet.find(other) != closedSet.end() )
            //    continue;
            auto i = costMap.find(other);
            if ( i != costMap.end() ) {
                int64_t cost = (*i).second;
                if ( cost <= myCost + e.cost ) {
                    continue;
                }
                fringe.erase((*i).first);
            }
            costMap[other] = myCost + e.cost;
            fringe.insert(other);
            parentMap[other] = pt;
            workNeeded[other] = (e.cost > 1 ? 1 : 0) + workNeeded[pt];
        }
        delete myEdges;
    }
    clock_t time = clock() - t0;
    out.print("time = %d, totalEdgeTime = %d\n", time, totalEdgeTime);

    unordered_set<df::coord, PointHash> requiresZNeg;
    unordered_set<df::coord, PointHash> requiresZPos;

    //find important edges
    Edge firstImportantEdge(df::coord(), df::coord(), -1);
    for ( auto i = localPts.begin(); i != localPts.end(); i++ ) {
        df::coord pt = *i;
        if ( costMap.find(pt) == costMap.end() )
            continue;
        if ( parentMap.find(pt) == parentMap.end() )
            continue;
        //if ( workNeeded[pt] == 0 ) 
        //    continue;
        while ( parentMap.find(pt) != parentMap.end() ) {
            //out.print("(%d,%d,%d)\n", pt.x, pt.y, pt.z);
            df::coord parent = parentMap[pt];
            if ( Maps::canStepBetween(parent, pt) ) {

            } else {
                if ( pt.x == parent.x && pt.y == parent.y ) {
                    if ( pt.z < parent.z ) {
                        requiresZNeg.insert(parent);
                        requiresZPos.insert(pt);
                    } else if ( pt.z > parent.z ) {
                        requiresZNeg.insert(pt);
                        requiresZPos.insert(parent);
                    }
                }
                //if ( workNeeded[pt] > workNeeded[parent] ) {
                    //importantEdges.push_front(Edge(pt,parent,0));
                //}
                firstImportantEdge = Edge(pt,parent,0);
                out.print("(%d,%d,%d) -> (%d,%d,%d)\n", parent.x,parent.y,parent.z, pt.x,pt.y,pt.z);
            }
            pt = parent;
        }
        break;
    }

    unordered_set<df::coord,PointHash> toDelete;
    for ( auto a = requiresZNeg.begin(); a != requiresZNeg.end(); a++ ) {
        df::coord pos = *a;
        df::tiletype* type = Maps::getTileType(pos);
        df::tiletype_shape shape = ENUM_ATTR(tiletype, shape, *type);
        if ( ENUM_ATTR(tiletype_shape, passable_low, shape) ) {
            toDelete.insert(pos);
        }
    }
    requiresZNeg.erase(toDelete.begin(), toDelete.end());
    toDelete.clear();
    for ( auto a = requiresZPos.begin(); a != requiresZPos.end(); a++ ) {
        df::coord pos = *a;
        df::tiletype* type = Maps::getTileType(pos);
        df::tiletype_shape shape = ENUM_ATTR(tiletype, shape, *type);
        if ( ENUM_ATTR(tiletype_shape, passable_high, shape) ) {
            toDelete.insert(pos);
        }
    }
    requiresZPos.erase(toDelete.begin(), toDelete.end());
    toDelete.clear();
    
    return assignJob(out, firstImportantEdge, parentMap, costMap, invaders, requiresZNeg, requiresZPos, cache);
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


