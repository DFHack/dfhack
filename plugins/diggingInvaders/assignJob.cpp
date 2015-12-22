#include "assignJob.h"

#include "modules/Buildings.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/Materials.h"

#include "df/building.h"
#include "df/construction.h"
#include "df/coord.h"
#include "df/general_ref.h"
#include "df/general_ref_building_holderst.h"
#include "df/general_ref_unit.h"
//#include "df/general_ref_unit_holderst.h"
#include "df/general_ref_unit_workerst.h"
#include "df/historical_entity.h"
#include "df/item.h"
#include "df/itemdef_weaponst.h"
#include "df/item_quality.h"
#include "df/item_type.h"
#include "df/item_weaponst.h"
#include "df/job.h"
#include "df/job_skill.h"
#include "df/job_type.h"
#include "df/reaction_product_itemst.h"
#include "df/reaction_reagent.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/unit_inventory_item.h"
#include "df/world_site.h"

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

int32_t assignJob(color_ostream& out, Edge firstImportantEdge, unordered_map<df::coord,df::coord,PointHash> parentMap, unordered_map<df::coord,cost_t,PointHash>& costMap, vector<int32_t>& invaders, unordered_set<df::coord,PointHash>& requiresZNeg, unordered_set<df::coord,PointHash>& requiresZPos, MapExtras::MapCache& cache, DigAbilities& abilities ) {
    df::unit* firstInvader = df::unit::find(invaders[0]);
    if ( !firstInvader ) {
        return -1;
    }

    //do whatever you need to do at the first important edge
    df::coord pt1 = firstImportantEdge.p1;
    df::coord pt2 = firstImportantEdge.p2;
    if ( costMap[pt1] > costMap[pt2] ) {
        df::coord temp = pt1;
        pt1 = pt2;
        pt2 = temp;
    }
    //out.print("first important edge: (%d,%d,%d) -> (%d,%d,%d)\n", pt1.x,pt1.y,pt1.z, pt2.x,pt2.y,pt2.z);

    int32_t jobId = -1;

    df::map_block* block1 = Maps::getTileBlock(pt1);
    df::map_block* block2 = Maps::getTileBlock(pt2);
    bool passable1 = block1->walkable[pt1.x&0xF][pt1.y&0xF];
    bool passable2 = block2->walkable[pt2.x&0xF][pt2.y&0xF];

    df::coord location;
    df::building* building = Buildings::findAtTile(pt2);
    df::coord buildingPos = pt2;
    if ( pt1.z > pt2.z ) {
        building = Buildings::findAtTile(df::coord(pt2.x,pt2.y,pt2.z+1));
        buildingPos = df::coord(pt2.x,pt2.y,pt2.z+1);
    }
    if ( building != NULL ) {
        df::coord destroyFrom = parentMap[buildingPos];
        if ( destroyFrom.z != buildingPos.z ) {
            //TODO: deal with this
        }
        //out.print("%s, line %d: Destroying building %d at (%d,%d,%d) from (%d,%d,%d).\n", __FILE__, __LINE__, building->id, buildingPos.x,buildingPos.y,buildingPos.z, destroyFrom.x,destroyFrom.y,destroyFrom.z);

        df::job* job = new df::job;
        job->job_type = df::enums::job_type::DestroyBuilding;
        //job->flags.bits.special = 1;
        df::general_ref_building_holderst* buildingRef = df::allocate<df::general_ref_building_holderst>();
        buildingRef->building_id = building->id;
        job->general_refs.push_back(buildingRef);
        df::general_ref_unit_workerst* workerRef = df::allocate<df::general_ref_unit_workerst>();
        workerRef->unit_id = firstInvader->id;
        job->general_refs.push_back(workerRef);
        getRidOfOldJob(firstInvader);
        firstInvader->job.current_job = job;
        firstInvader->path.path.x.clear();
        firstInvader->path.path.y.clear();
        firstInvader->path.path.z.clear();
        firstInvader->path.dest = destroyFrom;
        location = destroyFrom;
        firstInvader->job.hunt_target = NULL;
        firstInvader->job.destroy_target = NULL;

        building->jobs.clear();
        building->jobs.push_back(job);
        Job::linkIntoWorld(job);
        jobId = job->id;
        job->completion_timer = abilities.jobDelay[CostDimension::DestroyBuilding];
    } else {
        df::tiletype* type1 = Maps::getTileType(pt1);
        df::tiletype* type2 = Maps::getTileType(pt2);
        df::tiletype_shape shape1 = ENUM_ATTR(tiletype, shape, *type1);
        df::tiletype_shape shape2 = ENUM_ATTR(tiletype, shape, *type2);
        bool construction2 = ENUM_ATTR(tiletype, material, *type2) == df::enums::tiletype_material::CONSTRUCTION;
        if ( construction2 ) {
            df::job* job = new df::job;
            job->job_type = df::enums::job_type::RemoveConstruction;
            df::general_ref_unit_workerst* workerRef = df::allocate<df::general_ref_unit_workerst>();
            workerRef->unit_id = firstInvader->id;
            job->general_refs.push_back(workerRef);
            job->pos = pt2;
            getRidOfOldJob(firstInvader);
            firstInvader->job.current_job = job;
            firstInvader->path.path.x.clear();
            firstInvader->path.path.y.clear();
            firstInvader->path.path.z.clear();
            firstInvader->path.dest = pt1;
            location = pt1;
            firstInvader->job.hunt_target = NULL;
            firstInvader->job.destroy_target = NULL;
            Job::linkIntoWorld(job);
            jobId = job->id;
            df::construction* constr = df::construction::find(pt2);
            bool smooth = constr != NULL && constr->item_type != df::enums::item_type::BOULDER;
            if ( smooth )
                job->completion_timer = abilities.jobDelay[CostDimension::DestroySmoothConstruction];
            else
                job->completion_timer = abilities.jobDelay[CostDimension::DestroyRoughConstruction];
        } else {
            bool walkable_low1 = shape1 == df::tiletype_shape::STAIR_DOWN || shape1 == df::tiletype_shape::STAIR_UPDOWN;
            bool walkable_low2 = shape2 == df::tiletype_shape::STAIR_DOWN || shape2 == df::tiletype_shape::STAIR_UPDOWN;
            bool walkable_high1 = shape1 == df::tiletype_shape::STAIR_UP || shape1 == df::tiletype_shape::STAIR_UPDOWN;
            bool walkable_high2 = shape2 == df::tiletype_shape::STAIR_UP || shape2 == df::tiletype_shape::STAIR_UPDOWN;
            //must be a dig job
            bool up1 = !walkable_high1 && requiresZPos.find(pt1) != requiresZPos.end();
            bool up2 = !walkable_high2 && requiresZPos.find(pt2) != requiresZPos.end();
            bool down1 = !walkable_low1 && requiresZNeg.find(pt1) != requiresZNeg.end();
            bool down2 = !walkable_low2 && requiresZNeg.find(pt2) != requiresZNeg.end();
            bool up;
            bool down;
            df::coord goHere;
            df::coord workHere;
            if ( pt1.z == pt2.z ) {
                up = up2;
                down = down2;
                goHere = pt1;
                workHere = pt2;
            } else {
                if ( up1 || down1 ) {
                    up = up1;
                    down = down1;
                    goHere = pt1;
                    workHere = pt1;
                } else {
                    up = up2;
                    down = down2;
                    goHere = pt1;
                    workHere = pt2;
                }
            }
            df::job* job = new df::job;
            if ( up && down ) {
                job->job_type = df::enums::job_type::CarveUpDownStaircase;
                //out.print("%s, line %d: type = up/down\n", __FILE__, __LINE__);
            } else if ( up && !down ) {
                job->job_type = df::enums::job_type::CarveUpwardStaircase;
                //out.print("%s, line %d: type = up\n", __FILE__, __LINE__);
            } else if ( !up && down ) {
                job->job_type = df::enums::job_type::CarveDownwardStaircase;
                //out.print("%s, line %d: type = down\n", __FILE__, __LINE__);
            } else {
                job->job_type = df::enums::job_type::Dig;
                //out.print("%s, line %d: type = dig\n", __FILE__, __LINE__);
            }
            //out.print("%s, line %d: up=%d,up1=%d,up2=%d, down=%d,down1=%d,down2=%d\n", __FILE__, __LINE__, up,up1,up2, down,down1,down2);
            job->pos = workHere;
            firstInvader->path.dest = goHere;
            location = goHere;
            df::general_ref_unit_workerst* ref = df::allocate<df::general_ref_unit_workerst>();
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
            job->completion_timer = abilities.jobDelay[CostDimension::Dig];

            //TODO: test if he already has a pick
            bool hasPick = false;
            for ( size_t a = 0; a < firstInvader->inventory.size(); a++ ) {
                df::unit_inventory_item* inv_item = firstInvader->inventory[a];
                if ( inv_item->mode != df::unit_inventory_item::Weapon || inv_item->body_part_id != firstInvader->body.weapon_bp )
                    continue;
                df::item* oldItem = inv_item->item;
                if ( oldItem->getType() != df::enums::item_type::WEAPON )
                    continue;
                df::item_weaponst* oldWeapon = (df::item_weaponst*)oldItem;
                df::itemdef_weaponst* oldType = oldWeapon->subtype;
                if ( oldType->skill_melee != df::enums::job_skill::MINING )
                    continue;
                hasPick = true;
                break;
            }

            if ( hasPick )
                return firstInvader->id;

            //create and give a pick
            //based on createitem.cpp
            df::reaction_product_itemst *prod = NULL;
            //TODO: consider filtering based on entity/civ stuff
            for ( size_t a = 0; a < df::global::world->raws.itemdefs.weapons.size(); a++ ) {
                df::itemdef_weaponst* oldType = df::global::world->raws.itemdefs.weapons[a];
                if ( oldType->skill_melee != df::enums::job_skill::MINING )
                    continue;
                prod = df::allocate<df::reaction_product_itemst>();
                prod->item_type = df::item_type::WEAPON;
                prod->item_subtype = a;
                break;
            }
            if ( prod == NULL ) {
                out.print("%s, %d: no valid item.\n", __FILE__, __LINE__);
                return -1;
            }

            DFHack::MaterialInfo material;
            if ( !material.find("OBSIDIAN") ) {
                out.print("%s, %d: no water.\n", __FILE__, __LINE__);
                return -1;
            }
            prod->mat_type = material.type;
            prod->mat_index = material.index;
            prod->probability = 100;
            prod->count = 1;
            prod->product_dimension = 1;

            vector<df::reaction_product*> out_products;
            vector<df::item*> out_items;
            vector<df::reaction_reagent*> in_reag;
            vector<df::item*> in_items;
            prod->produce(firstInvader, &out_products, &out_items, &in_reag, &in_items, 1, df::job_skill::NONE,
                df::historical_entity::find(firstInvader->civ_id),
                df::world_site::find(df::global::ui->site_id));

            if ( out_items.size() != 1 ) {
                out.print("%s, %d: wrong size: %d.\n", __FILE__, __LINE__, out_items.size());
                return -1;
            }
            out_items[0]->moveToGround(firstInvader->pos.x, firstInvader->pos.y, firstInvader->pos.z);

#if 0
            //check for existing item there
            for ( size_t a = 0; a < firstInvader->inventory.size(); a++ ) {
                df::unit_inventory_item* inv_item = firstInvader->inventory[a];
                if ( false || inv_item->body_part_id == part ) {
                    //throw it on the ground
                    Items::moveToGround(cache, inv_item->item, firstInvader->pos);
                }
            }
#endif
            Items::moveToInventory(cache, out_items[0], firstInvader, df::unit_inventory_item::T_mode::Weapon, firstInvader->body.weapon_bp);

            delete prod;
        }
    }

#if 0
    //tell EVERYONE to move there
    for ( size_t a = 0; a < invaders.size(); a++ ) {
        df::unit* invader = invaders[a];
        invader->path.path.x.clear();
        invader->path.path.y.clear();
        invader->path.path.z.clear();
        invader->path.dest = location;
        //invader->flags1.bits.invades = true;
        //invader->flags1.bits.marauder = true;
        //invader->flags2.bits.visitor_uninvited = true;
        invader->relations.group_leader_id = invader->id;
    }
#endif

    return firstInvader->id;
}
