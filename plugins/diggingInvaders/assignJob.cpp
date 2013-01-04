#include "assignJob.h"

#include "modules/Buildings.h"
#include "modules/Items.h"
#include "modules/Job.h"

#include "df/building.h"
#include "df/coord.h"
#include "df/general_ref.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_unit.h"
//#include "df/general_ref_unit_holderst.h"
#include "df/general_ref_unit_workerst.h"
#include "df/item.h"
#include "df/itemdef_weaponst.h"
#include "df/item_quality.h"
#include "df/item_weaponst.h"
#include "df/job.h"
#include "df/job_type.h"
#include "df/unit.h"
#include "df/unit_inventory_item.h"

void getRidOfOldJob(df::unit* unit) {
    if ( unit->job.current_job == NULL ) {
        return;
    }

    df::job* job = unit->job.current_job;
    unit->job.current_job = NULL;
    if ( job->list_link->prev != NULL ) {
        job->list_link->prev->next = job->list_link->next;
    }
    if ( job->list_link->next != NULL ) {
        job->list_link->next->prev = job->list_link->prev;
    }
    //TODO: consider building pointers?
    //for now, just let the memory leak TODO: fix
    //delete job->list_link;
    //delete job;
}

int32_t assignJob(color_ostream& out, Edge firstImportantEdge, unordered_map<df::coord,df::coord,PointHash> parentMap, unordered_map<df::coord,int64_t,PointHash>& costMap, vector<df::unit*>& invaders, unordered_set<df::coord,PointHash>& requiresZNeg, unordered_set<df::coord,PointHash>& requiresZPos, MapExtras::MapCache& cache) {
    df::unit* firstInvader = invaders[0];
    
    //do whatever you need to do at the first important edge
    df::coord pt1 = firstImportantEdge.p1;
    df::coord pt2 = firstImportantEdge.p2;
    if ( costMap[pt1] > costMap[pt2] ) {
        df::coord temp = pt1;
        pt1 = pt2;
        pt2 = temp;
    }
    out.print("first important edge: (%d,%d,%d) -> (%d,%d,%d)\n", pt1.x,pt1.y,pt1.z, pt2.x,pt2.y,pt2.z);

    int32_t jobId = -1;

    df::map_block* block1 = Maps::getTileBlock(pt1);
    df::map_block* block2 = Maps::getTileBlock(pt2);
    bool passable1 = block1->walkable[pt1.x&0x0F][pt1.y&0x0F];
    bool passable2 = block2->walkable[pt2.x&0x0F][pt2.y&0x0F];

    df::building* building = Buildings::findAtTile(pt2);
    df::coord buildingPos = pt2;
    if ( pt1.z > pt2.z ) {
        building = Buildings::findAtTile(df::coord(pt2.x,pt2.y,pt2.z+1));
        buildingPos = df::coord(pt2.x,pt2.y,pt2.z+1);
    }
    if ( building != NULL ) {
        df::coord destroyFrom = parentMap[buildingPos];
        out.print("%s, line %d: Destroying building %d at (%d,%d,%d) from (%d,%d,%d).\n", __FILE__, __LINE__, building->id, buildingPos.x,buildingPos.y,buildingPos.z, destroyFrom.x,destroyFrom.y,destroyFrom.z);

        df::job* job = new df::job;
        job->job_type = df::enums::job_type::DestroyBuilding;
        job->flags.bits.special = 1;
        df::general_ref_building_holderst* buildingRef = new df::general_ref_building_holderst;
        buildingRef->building_id = building->id;
        job->general_refs.push_back(buildingRef);
        df::general_ref_unit_workerst* workerRef = new df::general_ref_unit_workerst;
        workerRef->unit_id = firstInvader->id;
        job->general_refs.push_back(workerRef);
        getRidOfOldJob(firstInvader);
        firstInvader->job.current_job = job;
        firstInvader->path.path.x.clear();
        firstInvader->path.path.y.clear();
        firstInvader->path.path.z.clear();
        firstInvader->path.dest = destroyFrom;
        firstInvader->job.hunt_target = NULL;
        firstInvader->job.destroy_target = NULL;

        building->jobs.clear();
        building->jobs.push_back(job);
        Job::linkIntoWorld(job);
        jobId = job->id;
    } else {
        df::tiletype* type1 = Maps::getTileType(pt1);
        df::tiletype* type2 = Maps::getTileType(pt2);
        df::tiletype_shape shape1 = ENUM_ATTR(tiletype, shape, *type1);
        df::tiletype_shape shape2 = ENUM_ATTR(tiletype, shape, *type2);
        bool construction2 = ENUM_ATTR(tiletype, material, *type2) == df::enums::tiletype_material::CONSTRUCTION;
        if ( construction2 ) {
            df::job* job = new df::job;
            job->job_type = df::enums::job_type::RemoveConstruction;
            df::general_ref_unit_workerst* workerRef = new df::general_ref_unit_workerst;
            workerRef->unit_id = firstInvader->id;
            job->general_refs.push_back(workerRef);
            job->pos = pt2;
            getRidOfOldJob(firstInvader);
            firstInvader->job.current_job = job;
            firstInvader->path.path.x.clear();
            firstInvader->path.path.y.clear();
            firstInvader->path.path.z.clear();
            firstInvader->path.dest = pt1;
            firstInvader->job.hunt_target = NULL;
            firstInvader->job.destroy_target = NULL;
            Job::linkIntoWorld(job);
            jobId = job->id;
        } else {
            //must be a dig job
            bool up = requiresZPos.find(pt2) != requiresZPos.end();
            bool down = requiresZNeg.find(pt2) != requiresZNeg.end();
            df::job* job = new df::job;
            if ( up && down ) {
                job->job_type = df::enums::job_type::CarveUpDownStaircase;
                job->pos = pt2;
                firstInvader->path.dest = pt2;
            } else if ( up && !down ) {
                job->job_type = df::enums::job_type::CarveUpwardStaircase;
                job->pos = pt2;
                firstInvader->path.dest = pt2;
            } else if ( !up && down ) {
                job->job_type = df::enums::job_type::CarveDownwardStaircase;
                job->pos = pt2;
                firstInvader->path.dest = pt2;
            } else {
                job->job_type = df::enums::job_type::Dig;
                job->pos = pt2;
                firstInvader->path.dest = pt1;
            }
            df::general_ref_unit_workerst* ref = new df::general_ref_unit_workerst;
            ref->unit_id = firstInvader->id;
            job->general_refs.push_back(ref);
            firstInvader->job.hunt_target = NULL;
            firstInvader->job.destroy_target = NULL;
            getRidOfOldJob(firstInvader);
            firstInvader->job.current_job = job;
            firstInvader->path.path.x.clear();
            firstInvader->path.path.y.clear();
            firstInvader->path.path.z.clear();
            Job::linkIntoWorld(job);
            jobId = job->id;

            //TODO: test if he already has a pick
            
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
            pick->temperature.whole = 10059;
            pick->temperature.fraction = 0;
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
                return -1;
            }
            pick->subtype = itemdef;
            pick->sharpness = 5000;

            int32_t part = -1;
            part = firstInvader->body.weapon_bp; //weapon_bp
            if ( part == -1 ) {
                out.print("%s, %d: no grasp part.\n", __FILE__, __LINE__);
                return -1;
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
        }
    }
    return jobId;
}
