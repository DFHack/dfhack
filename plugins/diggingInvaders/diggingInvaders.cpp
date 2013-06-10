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
#include "modules/Gui.h"
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
#include "df/creature_raw.h"
#include "df/general_ref.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_unit.h"
#include "df/general_ref_unit_holderst.h"
#include "df/general_ref_unit_workerst.h"
#include "df/global_objects.h"
#include "df/invasion_info.h"
#include "df/item.h"
#include "df/itemdef_weaponst.h"
#include "df/item_quality.h"
#include "df/item_weaponst.h"
#include "df/inorganic_raw.h"
#include "df/job.h"
#include "df/job_list_link.h"
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
#include <unordered_map>
#include <unordered_set>

using namespace std;

using namespace DFHack;
using namespace df::enums;

command_result diggingInvadersCommand(color_ostream &out, std::vector <std::string> & parameters);
void watchForJobComplete(color_ostream& out, void* ptr);
void newInvasionHandler(color_ostream& out, void* ptr);
//int32_t manageInvasion(color_ostream& out);

DFHACK_PLUGIN("diggingInvaders");

//TODO: when world unloads
static int32_t lastInvasionJob=-1;
static int32_t lastInvasionDigger = -1;
//static EventManager::EventHandler jobCompleteHandler(watchForJobComplete, 5);
static bool enabled=false;
static bool activeDigging=false;
static unordered_set<string> diggingRaces;
static unordered_set<int32_t> invaderJobs;
static df::coord lastDebugEdgeCostPoint;

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    EventManager::EventHandler handler(newInvasionHandler, 1000);
    EventManager::registerListener(EventManager::EventType::INVASION, handler, plugin_self);
    
    commands.push_back(PluginCommand(
        "diggingInvaders", "Makes invaders dig to your dwarves.",
        diggingInvadersCommand, false, /* true means that the command can't be used from non-interactive user interface */
        "  diggingInvaders enable\n    enables the plugin\n"
        "  diggingInvaders disable\n    disables the plugin\n"
        "  diggingInvaders add GOBLIN\n    registers the race GOBLIN as a digging invader\n"
        "  diggingInvaders remove GOBLIN\n    unregisters the race GOBLIN as a digging invader\n"
        "  diggingInvaders\n    Makes invaders try to dig now.\n"
    ));
    
    *df::global::debug_showambush = true;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    EventManager::EventHandler invasionHandler(newInvasionHandler, 1000);
    switch (event) {
    case DFHack::SC_WORLD_LOADED:
        //TODO: check game mode
        lastInvasionJob = -1;
        //in case there are invaders when the game is loaded, we should check 
        EventManager::registerTick(invasionHandler, 10, plugin_self);
        break;
    case DFHack::SC_WORLD_UNLOADED:
        // cleanup
        break;
    default:
        break;
    }
    return CR_OK;
}

df::coord getRoot(df::coord point, unordered_map<df::coord, df::coord>& rootMap);

class PointComp {
public:
    unordered_map<df::coord, cost_t, PointHash> *pointCost;
    PointComp(unordered_map<df::coord, cost_t, PointHash> *p): pointCost(p) {
        
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
        cost_t c1 = (*i1).second;
        cost_t c2 = (*i2).second;
        if ( c1 != c2 )
            return c1 < c2;
        return p1 < p2;
    }
};

//bool important(df::coord pos, map<df::coord, set<Edge> >& edges, df::coord prev, set<df::coord>& importantPoints, set<Edge>& importantEdges);
void findAndAssignInvasionJob(color_ostream& out, void*);

void newInvasionHandler(color_ostream& out, void* ptr) {
    if ( activeDigging )
        return;
    activeDigging = true;
    EventManager::EventHandler handler(findAndAssignInvasionJob, 1);
    EventManager::registerTick(handler, 1, plugin_self);
#if 0
    //called when there's a new invasion
    //TODO: check if invaders can dig
    if ( manageInvasion(out) == -2 )
        return;
    
    //schedule the next thing
    uint32_t tick = World::ReadCurrentTick();
    tick = tick % 1000;
    tick = 1000 - tick;

    EventManager::EventHandler handle(newInvasionHandler, 1000);
    EventManager::registerTick(handle, tick, plugin_self);
#endif
}

#if 0
void watchForJobComplete(color_ostream& out, void* ptr) {
/*
    df::job* job = (df::job*)ptr;

    if ( job->id != lastInvasionJob )
        return;
    
    EventManager::unregister(EventManager::EventType::JOB_COMPLETED, jobCompleteHandler, plugin_self);
*/

    manageInvasion(out);
}
#endif

#if 0
int32_t manageInvasion(color_ostream& out) {
    //EventManager::unregisterAll(plugin_self);
    if ( !enabled ) {
        return -1;
    }
    int32_t lastInvasion = df::global::ui->invasions.next_id-1;
    if ( lastInvasion < 0 || df::global::ui->invasions.list[lastInvasion]->flags.bits.active == 0 ) {
        //if the invasion is over, we're done
        //out.print("Invasion is over. Stopping diggingInvaders.\n");
        return -2;
    }
    EventManager::registerTick(jobCompleteHandler, 1, plugin_self);
    if ( lastInvasionJob != -1 ) {
        //check if he's still doing it
        df::unit* worker = df::unit::find(lastInvasionDigger);
        //int32_t index = df::unit::binsearch_index(df::global::world->units.all, lastInvasionDigger);
        if ( !worker ) {
            out.print("Error %s line %d.\n", __FILE__, __LINE__);
            return -1;
        }
        df::job* job = worker->job.current_job;
        //out.print("job id: old = %d, new = %d\n", lastInvasionJob, job == NULL ? -1 : job->id);
        if ( job != NULL && lastInvasionJob == job->id ) {
            //out.print("Still working on the previous job.\n");
            return -1;
        }

        //return 1; //still invading, but nothing new done
    }
    
    int32_t unitId = findAndAssignInvasionJob(out);
    if ( unitId == -1 ) {
        //might need to do more digging later, after we've killed off a few locals
        //out.print("DiggingInvaders is waiting.\n");
        return -1;
    }
    
    lastInvasionDigger = unitId;
    {
        df::unit* unit = df::unit::find(unitId);
        if ( !unit ) {
            //out.print("Error %s line %d: unitId = %d, index = %d.\n", __FILE__, __LINE__, unitId, index);
            return -1;
        }
        lastInvasionJob = unit->job.current_job->id;
    }

    //EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, jobCompleteHandler, plugin_self);
    //out.print("DiggingInvaders: job assigned.\n");
    *df::global::pause_state = true;
    return 0; //did something
}
#endif

command_result diggingInvadersCommand(color_ostream& out, std::vector<std::string>& parameters) {
    for ( size_t a = 0; a < parameters.size(); a++ ) {
        if ( parameters[a] == "enable" ) {
            enabled = true;
        } else if ( parameters[a] == "disable" ) {
            enabled = false;
        } else if ( parameters[a] == "add" || parameters[a] == "remove" ) {
            if ( a+1 >= parameters.size() )
                return CR_WRONG_USAGE;
            string race = parameters[a+1];
            if ( parameters[a] == "add" ) {
                diggingRaces.insert(race);
            } else {
                diggingRaces.erase(race);
            }
            /*bool foundIt = false;
            for ( size_t b = 0; b < df::global::world->raws.creatures.all.size(); b++ ) {
                df::creature_raw* raw = df::global::world->raws.creatures.all[b];
                if ( race == raw->creature_id ) {
                    //out.print("%s = %s\n", race.c_str(), raw->creature_id.c_str());
                    if ( parameters[a] == "add" ) {
                        diggingRaces.insert(b);
                    } else {
                        diggingRaces.erase(b);
                    }
                    foundIt = true;
                    break;
                }
            }
            if ( !foundIt ) {
                out.print("Couldn't find \"%s\"\n", race.c_str());
                return CR_WRONG_USAGE;
            }*/
            a++;
        } else if ( parameters[a] == "setCost" ) {
            if ( a+2 >= parameters.size() )
                return CR_WRONG_USAGE;
            string costStr = parameters[a+1];
            int32_t costDim = -1;
            if ( costStr == "walk" ) {
                costDim = CostDimension::Walk;
            } else if ( costStr == "destroyBuilding" ) {
                costDim = CostDimension::DestroyBuilding;
            } else if ( costStr == "dig" ) {
                costDim = CostDimension::Dig;
            } else if ( costStr == "destroyConstruction" ) {
                costDim = CostDimension::DestroyConstruction;
            } else {
                return CR_WRONG_USAGE;
            }
            cost_t value;
            stringstream asdf(parameters[a+2]);
            asdf >> value;
            if ( value <= 0 )
                return CR_WRONG_USAGE;
            costWeight[costDim] = value;
            a += 2;
        } else if ( parameters[a] == "edgeCost" ) {
            df::coord bob = Gui::getCursorPos();
            out.print("(%d,%d,%d), (%d,%d,%d): cost = %lld\n", lastDebugEdgeCostPoint.x, lastDebugEdgeCostPoint.y, lastDebugEdgeCostPoint.z, bob.x, bob.y, bob.z, getEdgeCost(out, lastDebugEdgeCostPoint, bob));
            lastDebugEdgeCostPoint = bob;
        }
        else {
            return CR_WRONG_USAGE;
        }
    }
    
    if ( parameters.size() == 0 ) {
        //manageInvasion(out);
        newInvasionHandler(out, (void*)0);
    }
    return CR_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
//dijkstra globals
vector<int32_t> invaders;
unordered_set<df::coord, PointHash> invaderPts;
unordered_set<df::coord, PointHash> localPts;
unordered_map<df::coord,df::coord,PointHash> parentMap;
unordered_map<df::coord,cost_t,PointHash> costMap;

PointComp comp(&costMap);
set<df::coord, PointComp> fringe(comp);
EventManager::EventHandler findJobTickHandler(findAndAssignInvasionJob, 1);
const int32_t edgesPerFrame = 10000000;

int32_t localPtsFound = 0;
unordered_set<df::coord,PointHash> closedSet;
unordered_map<df::coord,int32_t,PointHash> workNeeded; //non-walking work needed to get there
bool foundTarget = false;
int32_t edgeCount = 0;

void clearDijkstra() {
    invaders.clear();
    invaderPts.clear();
    localPts.clear();
    parentMap.clear();
    costMap.clear();
    comp = PointComp(&costMap);
    fringe = set<df::coord,PointComp>(comp);
    localPtsFound = edgeCount = 0;
    foundTarget = false;
    closedSet.clear();
    workNeeded.clear();
}
/////////////////////////////////////////////////////////////////////////////////////////

void findAndAssignInvasionJob(color_ostream& out, void* tickTime) {
    CoreSuspender suspend;
    //returns the worker id of the job created //used to
    //out.print("%s, %d: %d\n", __FILE__, __LINE__, (int32_t)tickTime);
    
    if ( !activeDigging ) {
        clearDijkstra();
        return;
    }
    EventManager::unregister(EventManager::EventType::TICK, findJobTickHandler, plugin_self);
    EventManager::registerTick(findJobTickHandler, 1, plugin_self);
    
    if ( fringe.empty() ) {
        df::unit* lastDigger = df::unit::find(lastInvasionDigger);
        if ( lastDigger && lastDigger->job.current_job && lastDigger->job.current_job->id == lastInvasionJob ) {
            return;
        }
        //out.print("%s,%d: lastDigger = %d, last job = %d, last digger's job = %d\n", __FILE__, __LINE__, lastInvasionDigger, lastInvasionJob, !lastDigger ? -1 : (!lastDigger->job.current_job ? -1 : lastDigger->job.current_job->id));
        lastInvasionDigger = lastInvasionJob = -1;
        
        clearDijkstra();
        unordered_set<uint16_t> invaderConnectivity;
        unordered_set<uint16_t> localConnectivity;

        //find all locals and invaders
        for ( size_t a = 0; a < df::global::world->units.all.size(); a++ ) {
            df::unit* unit = df::global::world->units.all[a];
            if ( unit->flags1.bits.dead )
                continue;
            if ( Units::isCitizen(unit) ) {
                if ( localPts.find(unit->pos) != localPts.end() )
                    continue;
                localPts.insert(unit->pos);
                df::map_block* block = Maps::getTileBlock(unit->pos);
                localConnectivity.insert(block->walkable[unit->pos.x&0xF][unit->pos.y&0xF]);
            } else if ( unit->flags1.bits.active_invader ) {
                df::creature_raw* raw = df::creature_raw::find(unit->race);
                if ( raw == NULL ) {
                    out.print("%s,%d: WTF? Couldn't find creature raw.\n", __FILE__, __LINE__);
                    continue;
                }
                if ( diggingRaces.find(raw->creature_id) == diggingRaces.end() )
                    continue;
                if ( invaderPts.find(unit->pos) != invaderPts.end() )
                    continue;
                //must be able to wield a pick: this is overly pessimistic
                if ( unit->status2.limbs_grasp_count < unit->status2.limbs_grasp_max )
                    continue;
                df::map_block* block = Maps::getTileBlock(unit->pos);
                invaderConnectivity.insert(block->walkable[unit->pos.x&0xF][unit->pos.y&0xF]);
                if ( invaderPts.size() > 0 )
                    continue;
                invaderPts.insert(unit->pos);
                costMap[unit->pos] = 0;
                fringe.insert(unit->pos);
                invaders.push_back(unit->id);
            } else {
                continue;
            }
        }

        if ( invaders.empty() || localPts.empty() ) {
            activeDigging = false;
            return;
        }

        //if local connectivity is not disjoint from invader connectivity, no digging required
        bool overlap = false;
        for ( auto a = localConnectivity.begin(); a != localConnectivity.end(); a++ ) {
            uint16_t conn = *a;
            if ( invaderConnectivity.find(conn) == invaderConnectivity.end() )
                continue;
            overlap = true;
            break;
        }
        if ( overlap ) {
            //still keep checking next frame: might kill a few outsiders then dig down
            return;
        }
    }
    
    df::unit* firstInvader = df::unit::find(invaders[0]);
    if ( firstInvader == NULL ) {
        fringe.clear();
        return;
    }
    //TODO: check that firstInvader is an appropriate digger
    //out << firstInvader->id << endl;
    //out << firstInvader->pos.x << ", " << firstInvader->pos.y << ", " << firstInvader->pos.z << endl;
    //out << __LINE__ << endl;
    
    uint32_t xMax, yMax, zMax;
    Maps::getSize(xMax,yMax,zMax);
    xMax *= 16;
    yMax *= 16;
    MapExtras::MapCache cache;

    clock_t t0 = clock();
    clock_t totalEdgeTime = 0;
    int32_t edgesExpanded = 0;
    while(!fringe.empty()) {
        if ( edgesExpanded++ >= edgesPerFrame ) {
            return;
        }
        df::coord pt = *(fringe.begin());
        fringe.erase(fringe.begin());
        //out.print("line %d: fringe size = %d, localPtsFound = %d / %d, closedSetSize = %d, pt = %d,%d,%d\n", __LINE__, fringe.size(), localPtsFound, localPts.size(), closedSet.size(), pt.x,pt.y,pt.z);
        if ( closedSet.find(pt) != closedSet.end() ) {
            out.print("%s, line %d: Double closure! Bad!\n", __FILE__, __LINE__);
            break;
        }
        closedSet.insert(pt);
        
        if ( localPts.find(pt) != localPts.end() ) {
            localPtsFound++;
            if ( true || localPtsFound >= localPts.size() ) {
                foundTarget = true;
                break;
            }
            if ( workNeeded.find(pt) == workNeeded.end() || workNeeded[pt] == 0 ) {
                //there are still dwarves to kill that don't require digging to get to
                return;
            }
        }

        cost_t myCost = costMap[pt];
        clock_t edgeTime = clock();
        vector<Edge>* myEdges = getEdgeSet(out, pt, cache, xMax, yMax, zMax);
        totalEdgeTime += (clock() - edgeTime);
        for ( auto a = myEdges->begin(); a != myEdges->end(); a++ ) {
            Edge &e = *a;
            if ( e.p1 == df::coord() )
                break;
            edgeCount++;
            df::coord& other = e.p1;
            if ( other == pt )
                other = e.p2;
            //if ( closedSet.find(other) != closedSet.end() )
            //    continue;
            auto i = costMap.find(other);
            if ( i != costMap.end() ) {
                cost_t cost = (*i).second;
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
    out.print("tickTime = %d, time = %d, totalEdgeTime = %d, total points = %d, total edges = %d, time per point = %.3f, time per edge = %.3f, clocks/sec = %d\n", (int32_t)tickTime, time, totalEdgeTime, closedSet.size(), edgeCount, (float)time / closedSet.size(), (float)time / edgeCount, CLOCKS_PER_SEC);
    fringe.clear();

    if ( !foundTarget )
        return;

    unordered_set<df::coord, PointHash> requiresZNeg;
    unordered_set<df::coord, PointHash> requiresZPos;
    
    //find important edges
    Edge firstImportantEdge(df::coord(), df::coord(), -1);
    df::coord closest;
    cost_t closestCostEstimate=0;
    cost_t closestCostActual=0;
    for ( auto i = localPts.begin(); i != localPts.end(); i++ ) {
        df::coord pt = *i;
        if ( costMap.find(pt) == costMap.end() )
            continue;
        if ( parentMap.find(pt) == parentMap.end() )
            continue;
        closest = pt;
        closestCostEstimate = costMap[closest];
        //if ( workNeeded[pt] == 0 ) 
        //    continue;
        while ( parentMap.find(pt) != parentMap.end() ) {
            //out.print("(%d,%d,%d)\n", pt.x, pt.y, pt.z);
            df::coord parent = parentMap[pt];
            closestCostActual += getEdgeCost(out, parent, pt);
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
                //out.print("(%d,%d,%d) -> (%d,%d,%d)\n", parent.x,parent.y,parent.z, pt.x,pt.y,pt.z);
            }
            pt = parent;
        }
        break;
    }
    if ( firstImportantEdge.p1 == df::coord() )
        return;
    
    if ( closestCostActual != closestCostEstimate ) {
        out.print("%s,%d: closest = (%d,%d,%d), estimate = %lld != actual = %lld\n", __FILE__, __LINE__, closest.x,closest.y,closest.z, closestCostEstimate, closestCostActual);
        return;
    }
#if 0
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
#endif
    
    assignJob(out, firstImportantEdge, parentMap, costMap, invaders, requiresZNeg, requiresZPos, cache);
    lastInvasionDigger = firstInvader->id;
    lastInvasionJob = firstInvader->job.current_job ? firstInvader->job.current_job->id : -1;
    invaderJobs.erase(lastInvasionJob);
    for ( df::job_list_link* link = &df::global::world->job_list; link != NULL; link = link->next ) {
        if ( link->item == NULL )
            continue;
        df::job* job = link->item;
        if ( invaderJobs.find(job->id) == invaderJobs.end() ) {
            continue;
        }
        
        //cancel it
        job->flags.bits.item_lost = 1;
        out.print("%s,%d: cancelling job %d.\n", __FILE__,__LINE__, job->id);
        //invaderJobs.remove(job->id);
    }
    invaderJobs.erase(lastInvasionJob);
    return;
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


