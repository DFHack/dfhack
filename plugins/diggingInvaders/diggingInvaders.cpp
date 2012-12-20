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

#define HASHMAP 1
#if HASHMAP
#include <unordered_set>
#include <unordered_map>

#endif

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
enum CostDimension {
    Distance,
    DestroyBuilding,
    Dig,
    DestroyConstruction,
    //Construct,
    costDim
};

const int32_t costBucket[] = {
//Distance
0,
//DestroyBuilding
1,
//Dig
2,
//DestroyConstruction
3,
};

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
        cost[CostDimension::Distance] = i;
    }

    /*
    int32_t workNeeded() const {
        int32_t result = 0;
        for ( int32_t a = 1; a < costDim; a++ ) {
            result += cost[a];
        }
        return result;
    }
    */

    int32_t compare(const Cost& c) const {
        int32_t table[costDim];
        memset(table, 0, costDim*sizeof(int32_t));
        for ( size_t a = 0; a < costDim; a++ ) {
            table[costBucket[a]] += cost[a] - c.cost[a];
        }
        for ( size_t a = 0; a < costDim; a++ ) {
            if ( table[costDim-1-a] > 0 )
                return 1;
            if ( table[costDim-1-a] < 0 )
                return -1;
        }
        return 0;
    }

    bool operator>(const Cost& c) const {
        return compare(c) > 0;
        /*
        for ( size_t a = 0; a < costDim; a++ ) {
            if ( cost[costDim-1-a] != c.cost[costDim-1-a] )
                return cost[costDim-1-a] > c.cost[costDim-1-a];
        }
        return false;
        */
    }

    bool operator<(const Cost& c) const {
        return compare(c) < 0;
        /*
        for ( size_t a = 0; a < costDim; a++ ) {
            if ( cost[costDim-1-a] != c.cost[costDim-1-a] )
                return cost[costDim-1-a] < c.cost[costDim-1-a];
        }
        return false;
        */
    }

    bool operator==(const Cost& c) const {
        return compare(c) == 0;
        /*
        for ( size_t a = 0; a < costDim; a++ ) {
            if ( cost[a] != c.cost[a] )
                return false;
        }
        return true;
        */
    }

    bool operator<=(const Cost& c) const {
        return compare(c) <= 0;
    }

    bool operator!=(const Cost& c) const {
        return compare(c) != 0;
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

#if HASHMAP
df::coord getRoot(df::coord point, unordered_map<df::coord, df::coord>& rootMap);
#else
df::coord getRoot(df::coord point, map<df::coord, df::coord>& rootMap);
#endif

struct PointHash {
    size_t operator()(const df::coord c) const {
        return c.x * 65537 + c.y * 17 + c.z;
    }
};

class PointComp {
public:
#if HASHMAP
    unordered_map<df::coord, Cost, PointHash> *pointCost;
    PointComp(unordered_map<df::coord, Cost, PointHash> *p): pointCost(p) {
        
    }
#else
    map<df::coord, Cost> *pointCost;
    PointComp(map<df::coord, Cost> *p): pointCost(p) {
        
    }
#endif
    
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

//bool important(df::coord pos, map<df::coord, set<Edge> >& edges, df::coord prev, set<df::coord>& importantPoints, set<Edge>& importantEdges);
void doDiggingInvaders(color_ostream& out, void* ptr);

command_result diggingInvadersFunc(color_ostream& out, std::vector<std::string>& parameters) {
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    doDiggingInvaders(out, NULL);
    return CR_OK;
}

void doDiggingInvaders(color_ostream& out, void* ptr) {
    CoreSuspender suspend;
    
#if HASHMAP
    unordered_set<df::coord, PointHash> invaderPts;
    unordered_set<df::coord, PointHash> localPts;
    unordered_map<df::coord,df::coord,PointHash> parentMap;
    unordered_map<df::coord,Cost,PointHash> costMap;
#else
    set<df::coord> invaderPts;
    set<df::coord> localPts;
    map<df::coord, df::coord> parentMap;
    map<df::coord, Cost> costMap;
#endif

    PointComp comp(&costMap);
    set<df::coord, PointComp> fringe(comp);
    uint32_t xMax, yMax, zMax;
    Maps::getSize(xMax,yMax,zMax);
    xMax *= 16;
    yMax *= 16;
    MapExtras::MapCache cache;
    df::unit* firstInvader = NULL;

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
            if ( firstInvader == NULL )
                firstInvader = unit;
        } else {
            continue;
        }
    }
    out << firstInvader->id << endl;
    out << firstInvader->pos.x << ", " << firstInvader->pos.y << ", " << firstInvader->pos.z << endl;

    int32_t localPtsFound = 0;
#if HASHMAP
    unordered_set<df::coord,PointHash> closedSet;
#else
    set<df::coord> closedSet;
#endif

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
            bool doBreak = false;
            for ( int32_t a = 1; a < costDim; a++ ) {
                if ( costMap[pt].cost[a] > 0 ) {
                    doBreak = true;
                    break;
                }
            }
            if ( doBreak )
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
            //if ( closedSet.find(other) != closedSet.end() )
            //    continue;
            auto i = costMap.find(other);
            if ( i != costMap.end() ) {
                Cost& cost = (*i).second;
                if ( cost <= myCost + e.cost ) {
                    continue;
                }
                fringe.erase((*i).first);
            }
            costMap[other] = myCost + e.cost;
            fringe.insert(other);
            parentMap[other] = pt;
        }
        delete myEdges;
    }
    clock_t time = clock() - t0;
    out.print("time = %d, totalEdgeTime = %d\n", time, totalEdgeTime);

    unordered_set<df::coord, PointHash> requiresZNeg;
    unordered_set<df::coord, PointHash> requiresZPos;

    //find important edges
    list<Edge> importantEdges;
    for ( auto i = localPts.begin(); i != localPts.end(); i++ ) {
        df::coord pt = *i;
        if ( costMap.find(pt) == costMap.end() )
            continue;
        if ( parentMap.find(pt) == parentMap.end() )
            continue;
        bool requireAction = false;
        for ( int32_t a = 1; a < costDim; a++ ) {
            if ( costMap[pt].cost[a] != 0 ) {
                requireAction = true;
                break;
            }
        }
        if ( !requireAction )
            continue;
        while ( parentMap.find(pt) != parentMap.end() ) {
            //out.print("(%d,%d,%d)\n", pt.x, pt.y, pt.z);
            df::coord parent = parentMap[pt];
            if ( pt.z < parent.z ) {
                requiresZNeg.insert(parent);
                requiresZPos.insert(pt);
            } else if ( pt.z > parent.z ) {
                requiresZNeg.insert(pt);
                requiresZPos.insert(parent);
            }
            Cost cost1 = costMap[pt];
            Cost cost2 = costMap[parent];
            cost1.cost[0] = 0;
            cost2.cost[0] = 0;
            if ( true || cost1 != cost2 ) {
                importantEdges.push_front(Edge(pt,parent,0));
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
        bool important = //requireZNeg.find(pt1) != requireZNeg.end() ||
            //requireZPos.find(pt1) != requireZPos.end() ||
            requiresZNeg.find(pt2) != requiresZNeg.end() ||
            requiresZPos.find(pt2) != requiresZPos.end();
        if ( !important ) {
            Cost c1 = costMap[pt1];
            Cost c2 = costMap[pt2];
            c1.cost[0] = 0;
            c2.cost[0] = 0;
            if ( c1 == c2 ) {
                //definitely not important
                continue;
            }
        }

        df::map_block* block1 = Maps::getTileBlock(pt1);
        df::map_block* block2 = Maps::getTileBlock(pt2);
        bool passable1 = block1->walkable[pt1.x&0x0F][pt1.y&0x0F];
        bool passable2 = block2->walkable[pt2.x&0x0F][pt2.y&0x0F];

        //TODO: if actions required > 1, continue
        df::building* building = Buildings::findAtTile(pt2);
        if ( building != NULL && passable2 ) {
            building = NULL;
        }
        if ( building != NULL ) {
            out.print("%s, line %d: Destroying building %d\n", __FILE__, __LINE__, building->id);
            //building->flags.bits.almost_deleted = true;
            
            df::job* job = new df::job;
            job->job_type = df::enums::job_type::DestroyBuilding;
            job->flags.bits.special = 1;
            df::general_ref_building_holderst* buildingRef = new df::general_ref_building_holderst;
            buildingRef->building_id = building->id;
            job->references.push_back(buildingRef);
            df::general_ref_unit_workerst* workerRef = new df::general_ref_unit_workerst;
            workerRef->unit_id = firstInvader->id;
            job->references.push_back(workerRef);
            firstInvader->job.current_job = job;
            firstInvader->path.path.x.clear();
            firstInvader->path.path.y.clear();
            firstInvader->path.path.z.clear();
            firstInvader->path.dest = pt1;
            firstInvader->job.hunt_target = NULL;
            firstInvader->job.destroy_target = NULL;

            building->jobs.clear();
            building->jobs.push_back(job);
            Job::linkIntoWorld(job);
            
            didSomething = true;
            where = pt2;
            continue;
        } else {
            df::tiletype* type1 = Maps::getTileType(pt1);
            df::tiletype* type2 = Maps::getTileType(pt2);
            df::tiletype_shape shape1 = ENUM_ATTR(tiletype, shape, *type1);
            df::tiletype_shape shape2 = ENUM_ATTR(tiletype, shape, *type2);
            bool construction2 = ENUM_ATTR(tiletype, material, *type1) == df::enums::tiletype_material::CONSTRUCTION;
            if ( construction2 ) {
                out.print("%s, line %d. Removing construction (%d,%d,%d)\n", __FILE__, __LINE__, pt2.x,pt2.y,pt2.z);
                df::job* job = new df::job();
                job->job_type = df::enums::job_type::RemoveConstruction;
                df::general_ref_unit_workerst* workerRef = new df::general_ref_unit_workerst;
                workerRef->unit_id = firstInvader->id;
                job->references.push_back(workerRef);
                job->pos = pt2;
                firstInvader->job.current_job = job;
                firstInvader->path.path.x.clear();
                firstInvader->path.path.y.clear();
                firstInvader->path.path.z.clear();
                firstInvader->path.dest = pt1;
                firstInvader->job.hunt_target = NULL;
                firstInvader->job.destroy_target = NULL;
                Job::linkIntoWorld(job);
                didSomething = true;
                where = pt2;
                break;
            }

            /*if ( pt1.z != pt2.z && shape1 != df::enums::tiletype_shape::STAIR_DOWN && shape1 != df::enums::tiletype_shape::STAIR_UPDOWN ) {
                block1->tiletype[pt2.x&0x0F][pt2.y&0x0F] = df::enums::tiletype::ConstructedStairUD;
                where = pt2;
                didSomething = true;
                break;
            }*/

            bool up = requiresZPos.find(pt2) != requiresZPos.end();
            bool down = requiresZNeg.find(pt2) != requiresZNeg.end();
            df::job* job = new df::job;
            if ( up && down ) {
                job->job_type = df::enums::job_type::CarveUpDownStaircase;
                job->pos = pt2;
            } else if ( up && !down ) {
                job->job_type = df::enums::job_type::CarveUpwardStaircase;
                job->pos = pt2;
            } else if ( !up && down ) {
                job->job_type = df::enums::job_type::CarveDownwardStaircase;
                job->pos = pt2;
            } else {
                job->job_type = df::enums::job_type::Dig;
                job->pos = pt1;
            }
            df::general_ref_unit_workerst* ref = new df::general_ref_unit_workerst;
            ref->unit_id = firstInvader->id;
            job->references.push_back(ref);
            firstInvader->job.hunt_target = NULL;
            firstInvader->job.destroy_target = NULL;
            firstInvader->job.current_job = job;
            firstInvader->path.path.x.clear();
            firstInvader->path.path.y.clear();
            firstInvader->path.path.z.clear();
            firstInvader->path.dest = pt2;
            Job::linkIntoWorld(job);
            
            //create and give a pick
            df::item_weaponst* pick = new df::item_weaponst;
            pick->pos = firstInvader->pos;
            pick->flags.bits.forbid = 1;
            pick->flags.bits.on_ground = 1;
            pick->id = (*df::global::item_next_id)++;
            pick->ignite_point = -1;
            pick->heatdam_point = -1;
            pick->colddam_point = -1;
            pick->boiling_point = 11000;
            pick->melting_point = 10500;
            pick->fixed_temp = -1;
            pick->weight = 0;
            pick->weight_fraction = 0;
            pick->stack_size = 1;
            pick->temperature = 10059;
            pick->temperature_fraction = 0;
            pick->mat_type = 0;
            pick->mat_index = 5;
            pick->maker_race = 0; //hehe
            pick->quality = (df::enums::item_quality::item_quality)0;
            pick->skill_used = (df::enums::job_skill::job_skill)0;
            pick->maker = -1;
            df::itemdef_weaponst* itemdef = NULL;
            for ( size_t a = 0; a < df::global::world->raws.itemdefs.weapons.size(); a++ ) {
                df::itemdef_weaponst* candidate = df::global::world->raws.itemdefs.weapons[a];
                if ( candidate->id == "ITEM_WEAPON_PICK" ) {
                    itemdef = candidate;
                    break;
                }
            }
            if ( itemdef == NULL ) {
                out.print("%s, %d: null itemdef.\n", __FILE__, __LINE__);
                return;
            }
            pick->subtype = itemdef;
            pick->sharpness = 5000;

            int32_t part = -1;
            part = firstInvader->body.unk_3c8; //weapon_bp
            if ( part == -1 ) {
                out.print("%s, %d: no grasp part.\n", __FILE__, __LINE__);
                return;
            }
            //check for existing item there
            for ( size_t a = 0; a < firstInvader->inventory.size(); a++ ) {
                df::unit_inventory_item* inv_item = firstInvader->inventory[a];
                if ( false || inv_item->body_part_id == part ) {
                    //throw it on the GROUND
                    Items::moveToGround(cache, inv_item->item, firstInvader->pos);
                }
            }
            Items::moveToInventory(cache, pick, firstInvader, df::unit_inventory_item::T_mode::Weapon, part);
            didSomething = true;
            where = pt2;
            break;
        }
    }
    out << "didSomething = " << didSomething << endl;

    if ( !didSomething )
        return;
    
    Cost cost = costMap[where];
    float cost_tick = 0;
    cost_tick += cost.cost[CostDimension::Distance];
    cost_tick += cost.cost[CostDimension::DestroyBuilding] / (float)destroySpeed;
    cost_tick += cost.cost[CostDimension::Dig] / (float)digSpeed;
    
    EventManager::EventHandler handle(doDiggingInvaders);
    Plugin* me = Core::getInstance().getPluginManager()->getPluginByName("diggingInvaders");
    //EventManager::registerTick(handle, (int32_t)cost_tick, me);
    df::global::world->reindex_pathfinding = true;
}


int32_t getDestroyCost(df::building* building) {
    return 1;
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
    return 1;
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
    return 1;
}

int32_t getConstructCost(df::coord point) {
    return 1;
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
                Cost cost = 1;
                //if ( dz != 0 ) cost++;
                if ( Maps::canStepBetween(point, neighbor) ) {
                    df::map_block* block2 = Maps::getTileBlock(neighbor);
                    bool building2 = block2->occupancy[point.x&0x0F][point.y&0x0F].bits.building == df::enums::tile_building_occ::Obstacle || block2->occupancy[point.x&0x0F][point.y&0x0F].bits.building == df::enums::tile_building_occ::Impassable;
                    if ( building2 ) {
                        df::building* building = Buildings::findAtTile(neighbor);
                        if ( building->getType() == df::enums::building_type::Hatch ) {
                            if ( building->isForbidden() ) {
                                //TODO: worry about destroying hatches with nowhere to stand
                                cost.cost[CostDimension::DestroyBuilding] += getDestroyCost(building);
                            }
                        }
                    }
                    
                    Edge edge(point, neighbor, cost);
                    result->push_back(edge);
                } else {
#if 0
                    {
                        cost.cost[1] = 1;
                        result->push_back(Edge(point,neighbor,cost));
                        continue;
                    }
#endif
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

                    bool construction1 = ENUM_ATTR(tiletype, material, *type1) == df::enums::tiletype_material::CONSTRUCTION;
                    bool construction2 = ENUM_ATTR(tiletype, material, *type2) == df::enums::tiletype_material::CONSTRUCTION;
                    bool passable1 = block1->walkable[point.x&0xF][point.y&0xF] != 0;
                    bool passable2 = block2->walkable[neighbor.x&0xF][neighbor.y&0xF] != 0;

                    bool building1, building2;
                    bool sameBuilding = false;
                    {
                        df::enums::tile_building_occ::tile_building_occ awk = block1->occupancy[point.x&0x0F][point.y&0x0F].bits.building;
                        building1 = awk == df::enums::tile_building_occ::Obstacle || awk == df::enums::tile_building_occ::Impassable;
                        awk = block2->occupancy[neighbor.x&0x0F][neighbor.y&0x0F].bits.building;
                        building2 = awk == df::enums::tile_building_occ::Obstacle || awk == df::enums::tile_building_occ::Impassable;
                        if ( building1 && building2 ) {
                            df::building* b1 = Buildings::findAtTile(point);
                            df::building* b2 = Buildings::findAtTile(neighbor);
                            sameBuilding = b1 == b2;
                        }
                    }

                    if ( !passable2 && building2 && !sameBuilding ) {
                        df::building* b2 = Buildings::findAtTile(neighbor);
                        cost.cost[CostDimension::DestroyBuilding] += getDestroyCost(b2);
                        if ( dz != 0 )
                            continue;
                    } else {

                        if ( shape2 == df::enums::tiletype_shape::EMPTY ) {
                            //cost.cost[CostDimension::Construct] += getConstructCost(neighbor);
                            continue;
                        } else {
                            if ( dz == 0 ) {
                                if ( passable2 ) {
                                    if ( passable1 ) {
                                        out.print("%s, line %d: weirdness. (%d,%d,%d), (%d,%d,%d)\n", __FILE__, __LINE__, point.x,point.y,point.z, neighbor.x,neighbor.y,neighbor.z);
                                        //exit(1);
                                        //TODO: check
                                    } else {
                                        //this is fine: only charge once for digging through a wall, etc
                                    }
                                } else {
                                    //passable2 is false
                                    if ( construction2 ) {
                                        //must deconstruct orthogonally
                                        if ( dx*dy != 0 )
                                            continue;
                                        cost.cost[CostDimension::DestroyConstruction] += getDeconstructCost(neighbor);
                                    } else {
                                        cost.cost[CostDimension::Dig] += getDigCost(cache, neighbor);
                                    }
                                }
                            } else {
                                //dz is nonzero
                                bool ascending = dz > 0;
                                if ( dx == 0 && dy == 0 ) {
                                    if ( ENUM_ATTR(tiletype_shape, basic_shape, shape2) == df::enums::tiletype_shape_basic::Stair ) {
                                        if ( (ascending && ENUM_ATTR(tiletype_shape, passable_low, shape2)) || (!ascending && ENUM_ATTR(tiletype_shape, walkable_up, shape2)) ) {
                                            out.print("%s, line %d: weirdness. (%d,%d,%d), (%d,%d,%d)\n", __FILE__, __LINE__, point.x,point.y,point.z, neighbor.x,neighbor.y,neighbor.z);
                                            //TODO: check for forbidden hatch
                                            continue;
                                        } else {
                                            //figure out if we can dig something in there
                                            df::enums::tiletype_shape_basic::tiletype_shape_basic basic = ENUM_ATTR(tiletype_shape, basic_shape, shape2);
                                            if ( ascending ) {
                                                if ( construction2 )
                                                    continue; //technically possible: moving up, construction is up stair, hiding a floor with no down stair
                                                if ( basic == df::enums::tiletype_shape_basic::Wall || basic == df::enums::tiletype_shape_basic::Stair || df::enums::tiletype_shape_basic::Floor ) {
                                                    cost.cost[CostDimension::Dig] += 1;
                                                } else {
                                                    continue;
                                                }
                                            } else {
                                                //descending
                                                if ( construction2 )
                                                    continue;
                                                if ( basic == df::enums::tiletype_shape_basic::Wall ) {
                                                    cost.cost[CostDimension::Dig] += 1;
                                                } else {
                                                    continue;
                                                }
                                            }
                                        }
                                    } else {
                                        //shape2 is not a stair
                                        if ( ENUM_ATTR(tiletype_shape, basic_shape, shape2) == df::enums::tiletype_shape_basic::Wall ) {
                                            if ( construction2 )
                                                continue;
                                            cost.cost[CostDimension::Dig] += getDigCost(cache, neighbor);
                                        } else if ( ENUM_ATTR(tiletype_shape, basic_shape, shape2) == df::enums::tiletype_shape_basic::Open ) {
                                            continue;
                                            //cost.cost[CostDimension::Construct] += getConstructCost(neighbor);
                                        } else {
                                            if ( ascending && ENUM_ATTR(tiletype_shape, basic_shape, shape2) == df::enums::tiletype_shape_basic::Floor && !construction2 ) {
                                                cost.cost[CostDimension::Dig] += 1;
                                            }
                                            continue;
                                        }
                                    }
                                } else {
                                    //now we're talking about moving up and down nonvertically, ie with ramps
                                    continue;
                                }
                            }
                        }
                    }
                    
                    if ( cost.cost[CostDimension::DestroyBuilding] != 0 ) {
                        //out.print("Line %d: Destroy building from (%d,%d,%d), (%d,%d,%d)\n", __LINE__, point.x,point.y,point.z, neighbor.x,neighbor.y,neighbor.z);
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


