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
#include "df/creature_raw.h"
#include "df/general_ref.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_unit.h"
#include "df/general_ref_unit_holderst.h"
#include "df/general_ref_unit_workerst.h"
#include "df/invasion_info.h"
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
void watchForJobComplete(color_ostream& out, void* ptr);
void initiateDigging(color_ostream& out, void* ptr);
int32_t manageInvasion(color_ostream& out);

DFHACK_PLUGIN("diggingInvaders");

//TODO: when world unloads
static int32_t lastInvasionJob=-1;
static int32_t lastInvasionDigger = -1;
static EventManager::EventHandler jobCompleteHandler(watchForJobComplete, 5);
static Plugin* diggingInvadersPlugin;
static bool enabled=false;
static unordered_set<int32_t> diggingRaces;

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    diggingInvadersPlugin = Core::getInstance().getPluginManager()->getPluginByName("diggingInvaders");
    EventManager::EventHandler invasionHandler(initiateDigging, 1000);
    EventManager::registerListener(EventManager::EventType::INVASION, invasionHandler, diggingInvadersPlugin);
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "diggingInvaders", "Makes invaders dig to your dwarves.",
        diggingInvadersFunc, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  diggingInvaders enable\n    enables the plugin\n"
        "  diggingInvaders disable\n    disables the plugin\n"
        "  diggingInvaders add GOBLIN\n    registers the race GOBLIN as a digging invader\n"
        "  diggingInvaders remove GOBLIN\n    unregisters the race GOBLIN as a digging invader\n"
        "  diggingInvaders\n    Makes invaders try to dig now.\n"
    ));
    
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    EventManager::EventHandler invasionHandler(initiateDigging, 1000);
    switch (event) {
    case DFHack::SC_WORLD_LOADED:
        //TODO: check game mode
        lastInvasionJob = -1;
        //in case there are invaders when the game is loaded, we should check 
        EventManager::registerTick(invasionHandler, 10, diggingInvadersPlugin);
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

void initiateDigging(color_ostream& out, void* ptr) {
    //called when there's a new invasion
    //TODO: check if invaders can dig
    if ( manageInvasion(out) == -2 )
        return;
    
    //schedule the next thing
    uint32_t tick = World::ReadCurrentTick();
    tick = tick % 1000;
    tick = 1000 - tick;

    EventManager::EventHandler handle(initiateDigging, 1000);
    EventManager::registerTick(handle, tick, diggingInvadersPlugin);
}

void watchForJobComplete(color_ostream& out, void* ptr) {
    df::job* job = (df::job*)ptr;

    if ( job->id != lastInvasionJob )
        return;
    
    EventManager::unregister(EventManager::EventType::JOB_COMPLETED, jobCompleteHandler, diggingInvadersPlugin);

    manageInvasion(out);
}

int32_t manageInvasion(color_ostream& out) {
    if ( !enabled ) {
        return -1;
    }
    int32_t lastInvasion = df::global::ui->invasions.next_id-1;
    if ( lastInvasion < 0 || df::global::ui->invasions.list[lastInvasion]->flags.bits.active == 0 ) {
        //if the invasion is over, we're done
        //out.print("Invasion is over. Stopping diggingInvaders.\n");
        return -2;
    }
    if ( lastInvasionJob != -1 ) {
        //check if he's still doing it
        int32_t index = df::unit::binsearch_index(df::global::world->units.all, lastInvasionDigger);
        if ( index == -1 ) {
            out.print("Error %s line %d.\n", __FILE__, __LINE__);
            return -1;
        }
        df::job* job = df::global::world->units.all[index]->job.current_job;
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
        int32_t index = df::unit::binsearch_index(df::global::world->units.all, unitId);
        if ( index == -1 ) {
            out.print("Error %s line %d: unitId = %d, index = %d.\n", __FILE__, __LINE__, unitId, index);
            return -1;
        }
        lastInvasionJob = df::global::world->units.all[index]->job.current_job->id;
    }

    EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, jobCompleteHandler, diggingInvadersPlugin);
    //out.print("DiggingInvaders: job assigned.\n");
    return 0; //did something
}

command_result diggingInvadersFunc(color_ostream& out, std::vector<std::string>& parameters) {
    for ( size_t a = 0; a < parameters.size(); a++ ) {
        if ( parameters[a] == "enable" ) {
            enabled = true;
        } else if ( parameters[a] == "disable" ) {
            enabled = false;
        } else if ( parameters[a] == "add" || parameters[a] == "remove" ) {
            if ( a+1 >= parameters.size() )
                return CR_WRONG_USAGE;
            string race = parameters[a+1];
            bool foundIt = false;
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
            }
            a++;
        }
    }

    if ( parameters.size() == 0 )
        manageInvasion(out);
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


    unordered_set<uint16_t> invaderConnectivity;
    unordered_set<uint16_t> localConnectivity;
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
            df::map_block* block = Maps::getTileBlock(unit->pos);
            localConnectivity.insert(block->walkable[unit->pos.x&0xF][unit->pos.y&0xF]);
        } else if ( unit->flags1.bits.active_invader ) {
            if ( diggingRaces.find(unit->race) == diggingRaces.end() )
                continue;
            if ( invaderPts.find(unit->pos) != invaderPts.end() )
                continue;
            invaderPts.insert(unit->pos);
            costMap[unit->pos] = 0;
            fringe.insert(unit->pos);
            invaders.push_back(unit);
            df::map_block* block = Maps::getTileBlock(unit->pos);
            invaderConnectivity.insert(block->walkable[unit->pos.x&0xF][unit->pos.y&0xF]);
        } else {
            continue;
        }
    }

    if ( invaders.empty() ) {
        return -1;
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
        return -1;
    }
    df::unit* firstInvader = invaders[0];
    //out << firstInvader->id << endl;
    //out << firstInvader->pos.x << ", " << firstInvader->pos.y << ", " << firstInvader->pos.z << endl;

    int32_t localPtsFound = 0;
    unordered_set<df::coord,PointHash> closedSet;
    unordered_map<df::coord,int32_t,PointHash> workNeeded; //non-walking work needed to get there
    bool foundTarget = false;

    clock_t t0 = clock();
    clock_t totalEdgeTime = 0;
    while(!fringe.empty()) {
        df::coord pt = *(fringe.begin());
        fringe.erase(fringe.begin());
        //out.print("line %d: fringe size = %d, localPtsFound = %d / %d, closedSetSize = %d\n", __LINE__, fringe.size(), localPtsFound, localPts.size(), closedSet.size());
        if ( closedSet.find(pt) != closedSet.end() ) {
            out.print("%s, line %d: Double closure! Bad!\n", __FILE__, __LINE__);
            break;
        }
        closedSet.insert(pt);
        
        if ( localPts.find(pt) != localPts.end() ) {
            localPtsFound++;
            if ( localPtsFound >= localPts.size() ) {
                foundTarget = true;
                break;
            }
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
    //out.print("time = %d, totalEdgeTime = %d\n", time, totalEdgeTime);

    if ( !foundTarget )
        return -1;

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
                //out.print("(%d,%d,%d) -> (%d,%d,%d)\n", parent.x,parent.y,parent.z, pt.x,pt.y,pt.z);
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
    
    return assignJob(out, firstImportantEdge, parentMap, costMap, invaders, requiresZNeg, requiresZPos, cache, diggingRaces);
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


