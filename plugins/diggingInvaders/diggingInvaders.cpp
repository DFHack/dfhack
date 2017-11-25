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
void clearDijkstra();
void findAndAssignInvasionJob(color_ostream& out, void*);
//int32_t manageInvasion(color_ostream& out);

DFHACK_PLUGIN_IS_ENABLED(enabled);
DFHACK_PLUGIN("diggingInvaders");
REQUIRE_GLOBAL(world);

//TODO: when world unloads
static int32_t lastInvasionJob=-1;
static int32_t lastInvasionDigger = -1;
static int32_t edgesPerTick = 100;
//static EventManager::EventHandler jobCompleteHandler(watchForJobComplete, 5);
static bool activeDigging=false;
static unordered_set<string> diggingRaces;
static unordered_set<int32_t> invaderJobs;
static df::coord lastDebugEdgeCostPoint;
unordered_map<string, DigAbilities> digAbilities;

static cost_t costWeightDefault[] = {
//Distance
1,
//Destroy Building
2,
//Dig
10000,
//DestroyRoughConstruction
1000,
//DestroySmoothConstruction
100,
};

static int32_t jobDelayDefault[] = {
//Distance
-1,
//Destroy Building
1000,
//Dig
1000,
//DestroyRoughConstruction
1000,
//DestroySmoothConstruction
100,
};

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "diggingInvaders", "Makes invaders dig to your dwarves.",
        diggingInvadersCommand, false, /* true means that the command can't be used from non-interactive user interface */
        "  diggingInvaders 0\n    disables the plugin\n"
        "  diggingInvaders 1\n    enables the plugin\n"
        "  diggingInvaders enable\n    enables the plugin\n"
        "  diggingInvaders disable\n    disables the plugin\n"
        "  diggingInvaders add GOBLIN\n    registers the race GOBLIN as a digging invader. Case-sensitive.\n"
        "  diggingInvaders remove GOBLIN\n    unregisters the race GOBLIN as a digging invader. Case-sensitive.\n"
        "  diggingInvaders setCost GOBLIN walk n\n    sets the walk cost in the path algorithm for the race GOBLIN\n"
        "  diggingInvaders setCost GOBLIN destroyBuilding n\n"
        "  diggingInvaders setCost GOBLIN dig n\n"
        "  diggingInvaders setCost GOBLIN destroyRoughConstruction n\n  rough constructions are made from boulders\n"
        "  diggingInvaders setCost GOBLIN destroySmoothConstruction n\n  smooth constructions are made from blocks or bars instead of boulders\n"
        "  diggingInvaders setDelay GOBLIN destroyBuilding n\n    adds to the job_completion_timer of destroy building jobs that are assigned to invaders\n"
        "  diggingInvaders setDelay GOBLIN dig n\n"
        "  diggingInvaders setDelay GOBLIN destroyRoughConstruction n\n"
        "  diggingInvaders setDelay GOBLIN destroySmoothConstruction n\n"
        "  diggingInvaders now\n    makes invaders try to dig now, if plugin is enabled\n"
        "  diggingInvaders clear\n    clears all digging invader races\n"
        "  diggingInvaders edgesPerTick n\n    makes the pathfinding algorithm work on at most n edges per tick. Set to 0 or lower to make it unlimited."
//        "  diggingInvaders\n    Makes invaders try to dig now.\n"
    ));

    //*df::global::debug_showambush = true;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream& out, bool enable) {
    if ( enabled == enable )
        return CR_OK;

    enabled = enable;
    EventManager::unregisterAll(plugin_self);
    clearDijkstra();
    lastInvasionJob = lastInvasionDigger = -1;
    activeDigging = false;
    invaderJobs.clear();
    if ( enabled ) {
        EventManager::EventHandler handler(newInvasionHandler, 1000);
        EventManager::registerListener(EventManager::EventType::INVASION, handler, plugin_self);
        findAndAssignInvasionJob(out, (void*)0);
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case DFHack::SC_WORLD_LOADED:
        //TODO: check game mode
        //in case there are invaders when the game is loaded, we check if there's work to be done
        activeDigging = enabled;
        clearDijkstra();
        findAndAssignInvasionJob(out, (void*)0);
        break;
    case DFHack::SC_WORLD_UNLOADED:
        // cleanup
        plugin_enable(out, false);
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

    int32_t operator()(df::coord p1, df::coord p2) const {
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

void newInvasionHandler(color_ostream& out, void* ptr) {
    if ( activeDigging )
        return;
    activeDigging = true;
    findAndAssignInvasionJob(out, (void*)0);
}

command_result diggingInvadersCommand(color_ostream& out, std::vector<std::string>& parameters) {
    for ( size_t a = 0; a < parameters.size(); a++ ) {
        if ( parameters[a] == "1" || parameters[a] == "enable" ) {
            plugin_enable(out,true);
        } else if ( parameters[a] == "0" || parameters[a] == "disable" ) {
            plugin_enable(out,false);
        } else if ( parameters[a] == "add" || parameters[a] == "remove" ) {
            if ( a+1 >= parameters.size() )
                return CR_WRONG_USAGE;
            string race = parameters[a+1];
            if ( parameters[a] == "add" ) {
                diggingRaces.insert(race);
                DigAbilities& abilities = digAbilities[race];
                memcpy(abilities.costWeight, costWeightDefault, costDim*sizeof(cost_t));
                memcpy(abilities.jobDelay, jobDelayDefault, costDim*sizeof(int32_t));
            } else {
                diggingRaces.erase(race);
                digAbilities.erase(race);
            }
            a++;

        } else if ( parameters[a] == "setCost" || parameters[a] == "setDelay" ) {
            if ( a+3 >= parameters.size() )
                return CR_WRONG_USAGE;

            string raceString = parameters[a+1];
            if ( digAbilities.find(raceString) == digAbilities.end() ) {
                DigAbilities bob;
                memset(&bob, 0xFF, sizeof(bob));
                digAbilities[raceString] = bob;
            }
            DigAbilities& abilities = digAbilities[raceString];

            string costStr = parameters[a+2];
            int32_t costDim = -1;
            if ( costStr == "walk" ) {
                costDim = CostDimension::Walk;
                if ( parameters[a] == "setDelay" )
                    return CR_WRONG_USAGE;
            } else if ( costStr == "destroyBuilding" ) {
                costDim = CostDimension::DestroyBuilding;
            } else if ( costStr == "dig" ) {
                costDim = CostDimension::Dig;
            } else if ( costStr == "destroyRoughConstruction" ) {
                costDim = CostDimension::DestroyRoughConstruction;
            } else if ( costStr == "destroySmoothConstruction" ) {
                costDim = CostDimension::DestroySmoothConstruction;
            } else {
                return CR_WRONG_USAGE;
            }

            cost_t value;
            stringstream asdf(parameters[a+3]);
            asdf >> value;
            //if ( parameters[a] == "setCost" && value <= 0 )
            //    return CR_WRONG_USAGE;
            if ( parameters[a] == "setCost" ) {
                abilities.costWeight[costDim] = value;
            } else {
                abilities.jobDelay[costDim] = value;
            }
            a += 3;
        } else if ( parameters[a] == "edgeCost" ) {
            if ( a+1 >= parameters.size() )
                return CR_WRONG_USAGE;

            string raceString = parameters[a+1];

            if ( digAbilities.find(raceString) == digAbilities.end() ) {
                out.print("Race %s does not have dig abilities assigned.\n", raceString.c_str());
                return CR_WRONG_USAGE;
            }
            DigAbilities& abilities = digAbilities[raceString];

            df::coord bob = Gui::getCursorPos();
            out.print("(%d,%d,%d), (%d,%d,%d): cost = %lld\n", lastDebugEdgeCostPoint.x, lastDebugEdgeCostPoint.y, lastDebugEdgeCostPoint.z, bob.x, bob.y, bob.z, getEdgeCost(out, lastDebugEdgeCostPoint, bob, abilities));
            lastDebugEdgeCostPoint = bob;
            a++;
        } else if ( parameters[a] == "now" ) {
            activeDigging = true;
            findAndAssignInvasionJob(out, (void*)0);
        } else if ( parameters[a] == "clear" ) {
            diggingRaces.clear();
            digAbilities.clear();
        } else if ( parameters[a] == "edgesPerTick" ) {
            if ( a+1 >= parameters.size() )
                return CR_WRONG_USAGE;
            stringstream asdf(parameters[a+1]);
            int32_t edgeCount = 100;
            asdf >> edgeCount;
            edgesPerTick = edgeCount;
            a++;
        }
        else {
            return CR_WRONG_USAGE;
        }
    }
    activeDigging = enabled;
    out.print("diggingInvaders: enabled = %d, activeDigging = %d, edgesPerTick = %d\n", enabled, activeDigging, edgesPerTick);

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

    if ( !enabled || !activeDigging ) {
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
        for ( size_t a = 0; a < world->units.all.size(); a++ ) {
            df::unit* unit = world->units.all[a];
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
                /*
                if ( diggingRaces.find(raw->creature_id) == diggingRaces.end() )
                    continue;
                */
                if ( digAbilities.find(raw->creature_id) == digAbilities.end() )
                    continue;
                if ( invaderPts.find(unit->pos) != invaderPts.end() )
                    continue;
                //must be able to wield a pick: this is overly pessimistic
                if ( unit->status2.limbs_grasp_max <= 0 || unit->status2.limbs_grasp_count < unit->status2.limbs_grasp_max )
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

    df::creature_raw* creature_raw = df::creature_raw::find(firstInvader->race);
    if ( creature_raw == NULL || digAbilities.find(creature_raw->creature_id) == digAbilities.end() ) {
        //inappropriate digger: no dig abilities
        fringe.clear();
        return;
    }
    DigAbilities& abilities = digAbilities[creature_raw->creature_id];
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
        if ( edgesPerTick > 0 && edgesExpanded++ >= edgesPerTick ) {
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
        vector<Edge>* myEdges = getEdgeSet(out, pt, cache, xMax, yMax, zMax, abilities);
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
    //out.print("tickTime = %d, time = %d, totalEdgeTime = %d, total points = %d, total edges = %d, time per point = %.3f, time per edge = %.3f, clocks/sec = %d\n", (int32_t)tickTime, time, totalEdgeTime, closedSet.size(), edgeCount, (float)time / closedSet.size(), (float)time / edgeCount, CLOCKS_PER_SEC);
    fringe.clear();

    if ( !foundTarget )
        return;

    unordered_set<df::coord, PointHash> requiresZNeg;
    unordered_set<df::coord, PointHash> requiresZPos;

    //find important edges
    Edge firstImportantEdge(df::coord(), df::coord(), -1);
    //df::coord closest;
    //cost_t closestCostEstimate=0;
    //cost_t closestCostActual=0;
    for ( auto i = localPts.begin(); i != localPts.end(); i++ ) {
        df::coord pt = *i;
        if ( costMap.find(pt) == costMap.end() )
            continue;
        if ( parentMap.find(pt) == parentMap.end() )
            continue;
        //closest = pt;
        //closestCostEstimate = costMap[closest];
        //if ( workNeeded[pt] == 0 )
        //    continue;
        while ( parentMap.find(pt) != parentMap.end() ) {
            //out.print("(%d,%d,%d)\n", pt.x, pt.y, pt.z);
            df::coord parent = parentMap[pt];
            cost_t cost = getEdgeCost(out, parent, pt, abilities);
            if ( cost < 0 ) {
                //path invalidated
                return;
            }
            //closestCostActual += cost;
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

/*
    if ( closestCostActual != closestCostEstimate ) {
        out.print("%s,%d: closest = (%d,%d,%d), estimate = %lld != actual = %lld\n", __FILE__, __LINE__, closest.x,closest.y,closest.z, closestCostEstimate, closestCostActual);
        return;
    }
*/

    assignJob(out, firstImportantEdge, parentMap, costMap, invaders, requiresZNeg, requiresZPos, cache, abilities);
    lastInvasionDigger = firstInvader->id;
    lastInvasionJob = firstInvader->job.current_job ? firstInvader->job.current_job->id : -1;
    invaderJobs.erase(lastInvasionJob);
    for ( df::job_list_link* link = &world->jobs.list; link != NULL; link = link->next ) {
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


