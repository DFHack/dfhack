/*a
 * This file contains the logic to attempt to intuit the labor required for
 * a given job. This is way more complicated than it should be, but I have
 * not figured out how to make it simpler.
 *
 * Usage:
 *  Instantiate an instance of the JobLaborMapper class
 *  Call the find_job_labor method of that class instance,
 *   passing the job as the only argument, to determine the labor for that job
 *  When done, destroy the instance
 *
 * The class should allow you to create multiple instances, although there is
 * little benefit to doing so. jlfuncs are not reused across instances.
 *
 */


#include "DataDefs.h"
#include "MiscUtils.h"
#include "modules/Materials.h"

#include <df/building.h>
#include <df/building_actual.h>
#include <df/building_def.h>
#include <df/building_design.h>
#include <df/building_furnacest.h>
#include <df/building_type.h>
#include <df/building_workshopst.h>

#include <df/furnace_type.h>

#include <df/general_ref.h>
#include <df/general_ref_building_holderst.h>
#include <df/general_ref_contains_itemst.h>

#include <df/item.h>
#include <df/item_type.h>

#include <df/job.h>
#include <df/job_item.h>
#include <df/job_item_ref.h>

#include <df/material_flags.h>

#include <df/reaction.h>

#include <df/unit_labor.h>

#include <df/world.h>

#include <vector>
#include <set>

using namespace std;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;
using df::global::ui;
using df::global::world;

#include "labormanager.h"
#include "joblabormapper.h"

static df::unit_labor hauling_labor_map[] =
{
    df::unit_labor::HAUL_ITEM,    /* BAR */
    df::unit_labor::HAUL_STONE,    /* SMALLGEM */
    df::unit_labor::HAUL_ITEM,    /* BLOCKS */
    df::unit_labor::HAUL_STONE,    /* ROUGH */
    df::unit_labor::HAUL_STONE,    /* BOULDER */
    df::unit_labor::HAUL_WOOD,    /* WOOD */
    df::unit_labor::HAUL_FURNITURE,    /* DOOR */
    df::unit_labor::HAUL_FURNITURE,    /* FLOODGATE */
    df::unit_labor::HAUL_FURNITURE,    /* BED */
    df::unit_labor::HAUL_FURNITURE,    /* CHAIR */
    df::unit_labor::HAUL_ITEM,    /* CHAIN */
    df::unit_labor::HAUL_ITEM,    /* FLASK */
    df::unit_labor::HAUL_ITEM,    /* GOBLET */
    df::unit_labor::HAUL_ITEM,    /* INSTRUMENT */
    df::unit_labor::HAUL_ITEM,    /* TOY */
    df::unit_labor::HAUL_FURNITURE,    /* WINDOW */
    df::unit_labor::HAUL_ANIMALS,    /* CAGE */
    df::unit_labor::HAUL_ITEM,    /* BARREL */
    df::unit_labor::HAUL_ITEM,    /* BUCKET */
    df::unit_labor::HAUL_ANIMALS,    /* ANIMALTRAP */
    df::unit_labor::HAUL_FURNITURE,    /* TABLE */
    df::unit_labor::HAUL_FURNITURE,    /* COFFIN */
    df::unit_labor::HAUL_FURNITURE,    /* STATUE */
    df::unit_labor::HAUL_REFUSE,    /* CORPSE */
    df::unit_labor::HAUL_ITEM,    /* WEAPON */
    df::unit_labor::HAUL_ITEM,    /* ARMOR */
    df::unit_labor::HAUL_ITEM,    /* SHOES */
    df::unit_labor::HAUL_ITEM,    /* SHIELD */
    df::unit_labor::HAUL_ITEM,    /* HELM */
    df::unit_labor::HAUL_ITEM,    /* GLOVES */
    df::unit_labor::HAUL_FURNITURE,    /* BOX */
    df::unit_labor::HAUL_ITEM,    /* BIN */
    df::unit_labor::HAUL_FURNITURE,    /* ARMORSTAND */
    df::unit_labor::HAUL_FURNITURE,    /* WEAPONRACK */
    df::unit_labor::HAUL_FURNITURE,    /* CABINET */
    df::unit_labor::HAUL_ITEM,    /* FIGURINE */
    df::unit_labor::HAUL_ITEM,    /* AMULET */
    df::unit_labor::HAUL_ITEM,    /* SCEPTER */
    df::unit_labor::HAUL_ITEM,    /* AMMO */
    df::unit_labor::HAUL_ITEM,    /* CROWN */
    df::unit_labor::HAUL_ITEM,    /* RING */
    df::unit_labor::HAUL_ITEM,    /* EARRING */
    df::unit_labor::HAUL_ITEM,    /* BRACELET */
    df::unit_labor::HAUL_ITEM,    /* GEM */
    df::unit_labor::HAUL_FURNITURE,    /* ANVIL */
    df::unit_labor::HAUL_REFUSE,    /* CORPSEPIECE */
    df::unit_labor::HAUL_REFUSE,    /* REMAINS */
    df::unit_labor::HAUL_FOOD,    /* MEAT */
    df::unit_labor::HAUL_FOOD,    /* FISH */
    df::unit_labor::HAUL_FOOD,    /* FISH_RAW */
    df::unit_labor::HAUL_REFUSE,    /* VERMIN */
    df::unit_labor::HAUL_ITEM,    /* PET */
    df::unit_labor::HAUL_ITEM,    /* SEEDS */
    df::unit_labor::HAUL_FOOD,    /* PLANT */
    df::unit_labor::HAUL_ITEM,    /* SKIN_TANNED */
    df::unit_labor::HAUL_FOOD,    /* LEAVES */
    df::unit_labor::HAUL_ITEM,    /* THREAD */
    df::unit_labor::HAUL_ITEM,    /* CLOTH */
    df::unit_labor::HAUL_ITEM,    /* TOTEM */
    df::unit_labor::HAUL_ITEM,    /* PANTS */
    df::unit_labor::HAUL_ITEM,    /* BACKPACK */
    df::unit_labor::HAUL_ITEM,    /* QUIVER */
    df::unit_labor::HAUL_FURNITURE,    /* CATAPULTPARTS */
    df::unit_labor::HAUL_FURNITURE,    /* BALLISTAPARTS */
    df::unit_labor::HAUL_FURNITURE,    /* SIEGEAMMO */
    df::unit_labor::HAUL_FURNITURE,    /* BALLISTAARROWHEAD */
    df::unit_labor::HAUL_FURNITURE,    /* TRAPPARTS */
    df::unit_labor::HAUL_FURNITURE,    /* TRAPCOMP */
    df::unit_labor::HAUL_FOOD,    /* DRINK */
    df::unit_labor::HAUL_FOOD,    /* POWDER_MISC */
    df::unit_labor::HAUL_FOOD,    /* CHEESE */
    df::unit_labor::HAUL_FOOD,    /* FOOD */
    df::unit_labor::HAUL_FOOD,    /* LIQUID_MISC */
    df::unit_labor::HAUL_ITEM,    /* COIN */
    df::unit_labor::HAUL_FOOD,    /* GLOB */
    df::unit_labor::HAUL_STONE,    /* ROCK */
    df::unit_labor::HAUL_FURNITURE,    /* PIPE_SECTION */
    df::unit_labor::HAUL_FURNITURE,    /* HATCH_COVER */
    df::unit_labor::HAUL_FURNITURE,    /* GRATE */
    df::unit_labor::HAUL_FURNITURE,    /* QUERN */
    df::unit_labor::HAUL_FURNITURE,    /* MILLSTONE */
    df::unit_labor::HAUL_ITEM,    /* SPLINT */
    df::unit_labor::HAUL_ITEM,    /* CRUTCH */
    df::unit_labor::HAUL_FURNITURE,    /* TRACTION_BENCH */
    df::unit_labor::HAUL_ITEM,    /* ORTHOPEDIC_CAST */
    df::unit_labor::HAUL_ITEM,    /* TOOL */
    df::unit_labor::HAUL_FURNITURE,    /* SLAB */
    df::unit_labor::HAUL_FOOD,    /* EGG */
    df::unit_labor::HAUL_ITEM,    /* BOOK */
};

static df::unit_labor workshop_build_labor[] =
{
    /* Carpenters */        df::unit_labor::CARPENTER,
    /* Farmers */            df::unit_labor::PROCESS_PLANT,
    /* Masons */            df::unit_labor::MASON,
    /* Craftsdwarfs */        df::unit_labor::STONE_CRAFT,
    /* Jewelers */            df::unit_labor::CUT_GEM,
    /* MetalsmithsForge */    df::unit_labor::METAL_CRAFT,
    /* MagmaForge */        df::unit_labor::METAL_CRAFT,
    /* Bowyers */            df::unit_labor::BOWYER,
    /* Mechanics */            df::unit_labor::MECHANIC,
    /* Siege */                df::unit_labor::SIEGECRAFT,
    /* Butchers */            df::unit_labor::BUTCHER,
    /* Leatherworks */        df::unit_labor::LEATHER,
    /* Tanners */            df::unit_labor::TANNER,
    /* Clothiers */            df::unit_labor::CLOTHESMAKER,
    /* Fishery */            df::unit_labor::CLEAN_FISH,
    /* Still */                df::unit_labor::BREWER,
    /* Loom */                df::unit_labor::WEAVER,
    /* Quern */                df::unit_labor::MILLER,
    /* Kennels */            df::unit_labor::ANIMALTRAIN,
    /* Kitchen */            df::unit_labor::COOK,
    /* Ashery */            df::unit_labor::LYE_MAKING,
    /* Dyers */                df::unit_labor::DYER,
    /* Millstone */            df::unit_labor::MILLER,
    /* Custom */            df::unit_labor::NONE,
    /* Tool */                df::unit_labor::NONE
};

static df::building* get_building_from_job(df::job* j)
{
    for (auto r = j->general_refs.begin(); r != j->general_refs.end(); r++)
    {
        if ((*r)->getType() == df::general_ref_type::BUILDING_HOLDER)
        {
            int32_t id = ((df::general_ref_building_holderst*)(*r))->building_id;
            df::building* bld = binsearch_in_vector(world->buildings.all, id);
            return bld;
        }
    }
    return 0;
}

static df::unit_labor construction_build_labor(df::building_actual* b)
{
    if (b->getType() == df::building_type::RoadPaved)
        return df::unit_labor::BUILD_ROAD;
    // Find last item in building with use mode appropriate to the building's constructions state
    // For screw pumps contained_items[0] = pipe, 1 corkscrew, 2 block
    // For wells 0 mechanism, 1 rope, 2 bucket, 3 block
    // Trade depots and bridges use the last one too
    // Must check use mode b/c buildings may have items in them that are not part of the building

    df::item* i = 0;
    for (auto p = b->contained_items.begin(); p != b->contained_items.end(); p++)
        if (b->construction_stage > 0 && (*p)->use_mode == 2 ||
            b->construction_stage == 0 && (*p)->use_mode == 0)
            i = (*p)->item;

    MaterialInfo matinfo;
    if (i && matinfo.decode(i))
    {
        if (matinfo.material->flags.is_set(df::material_flags::IS_METAL))
            return df::unit_labor::METAL_CRAFT;
        if (matinfo.material->flags.is_set(df::material_flags::WOOD))
            return df::unit_labor::CARPENTER;
    }
    return df::unit_labor::MASON;
}


class jlfunc
{
public:
    virtual df::unit_labor get_labor(df::job* j) = 0;
};

class jlfunc_const : public jlfunc
{
private:
    df::unit_labor labor;
public:
    df::unit_labor get_labor(df::job* j)
    {
        return labor;
    }
    jlfunc_const(df::unit_labor l) : labor(l) {};
};

class jlfunc_hauling : public jlfunc
{
public:
    df::unit_labor get_labor(df::job* j)
    {
        df::item* item = 0;
        if (j->job_type == df::job_type::StoreItemInStockpile && j->item_subtype != -1)
            return (df::unit_labor) j->item_subtype;

        for (auto i = j->items.begin(); i != j->items.end(); i++)
        {
            if ((*i)->role == 7)
            {
                item = (*i)->item;
                break;
            }
        }

        if (item && item->flags.bits.container)
        {
            for (auto a = item->general_refs.begin(); a != item->general_refs.end(); a++)
            {
                if ((*a)->getType() == df::general_ref_type::CONTAINS_ITEM)
                {
                    int item_id = ((df::general_ref_contains_itemst *) (*a))->item_id;
                    item = binsearch_in_vector(world->items.all, item_id);
                    break;
                }
            }
        }

        df::unit_labor l = item ? hauling_labor_map[item->getType()] : df::unit_labor::HAUL_ITEM;
        if (item && l == df::unit_labor::HAUL_REFUSE && item->flags.bits.dead_dwarf)
            l = df::unit_labor::HAUL_BODY;
        return l;
    }
    jlfunc_hauling() {};
};

class jlfunc_construct_bld : public jlfunc
{
public:
    df::unit_labor get_labor(df::job* j)
    {
        if (j->flags.bits.item_lost)
            return df::unit_labor::NONE;

        df::building* bld = get_building_from_job(j);
        switch (bld->getType())
        {
        case df::building_type::Hive:
            return df::unit_labor::BEEKEEPING;
        case df::building_type::Workshop:
        {
            df::building_workshopst* ws = (df::building_workshopst*) bld;
            if (ws->design && !ws->design->flags.bits.designed)
                return df::unit_labor::ARCHITECT;
            if (ws->type == df::workshop_type::Custom)
            {
                df::building_def* def = df::building_def::find(ws->custom_type);
                return def->build_labors[0];
            }
            else
                return workshop_build_labor[ws->type];
        }
        break;
        case df::building_type::Construction:
            return df::unit_labor::BUILD_CONSTRUCTION;
        case df::building_type::Furnace:
        case df::building_type::TradeDepot:
        case df::building_type::Bridge:
        case df::building_type::ArcheryTarget:
        case df::building_type::WaterWheel:
        case df::building_type::RoadPaved:
        case df::building_type::Well:
        case df::building_type::ScrewPump:
        case df::building_type::Wagon:
        case df::building_type::Shop:
        case df::building_type::Support:
        case df::building_type::Windmill:
        {
            df::building_actual* b = (df::building_actual*) bld;
            if (b->design && !b->design->flags.bits.designed)
                return df::unit_labor::ARCHITECT;
            return construction_build_labor(b);
        }
        break;
        case df::building_type::FarmPlot:
            return df::unit_labor::PLANT;
        case df::building_type::Chair:
        case df::building_type::Bed:
        case df::building_type::Table:
        case df::building_type::Coffin:
        case df::building_type::Door:
        case df::building_type::Floodgate:
        case df::building_type::Box:
        case df::building_type::Weaponrack:
        case df::building_type::Armorstand:
        case df::building_type::Cabinet:
        case df::building_type::Statue:
        case df::building_type::WindowGlass:
        case df::building_type::WindowGem:
        case df::building_type::Cage:
        case df::building_type::NestBox:
        case df::building_type::TractionBench:
        case df::building_type::Slab:
        case df::building_type::Chain:
        case df::building_type::GrateFloor:
        case df::building_type::Hatch:
        case df::building_type::BarsFloor:
        case df::building_type::BarsVertical:
        case df::building_type::GrateWall:
        case df::building_type::Bookcase:
        case df::building_type::Instrument:
        case df::building_type::DisplayFurniture:
            return df::unit_labor::HAUL_FURNITURE;
        case df::building_type::Trap:
        case df::building_type::GearAssembly:
        case df::building_type::AxleHorizontal:
        case df::building_type::AxleVertical:
        case df::building_type::Rollers:
            return df::unit_labor::MECHANIC;
        case df::building_type::AnimalTrap:
            return df::unit_labor::TRAPPER;
        case df::building_type::Civzone:
        case df::building_type::Nest:
        case df::building_type::Stockpile:
        case df::building_type::Weapon:
            return df::unit_labor::NONE;
        case df::building_type::SiegeEngine:
            return df::unit_labor::SIEGECRAFT;
        case df::building_type::RoadDirt:
            return df::unit_labor::BUILD_ROAD;
        }

        debug("LABORMANAGER: Cannot deduce labor for construct building job of type %s\n",
            ENUM_KEY_STR(building_type, bld->getType()).c_str());
        debug_pause();

        return df::unit_labor::NONE;
    }
    jlfunc_construct_bld() {}
};

class jlfunc_destroy_bld : public jlfunc
{
public:
    df::unit_labor get_labor(df::job* j)
    {
        df::building* bld = get_building_from_job(j);
        df::building_type type = bld->getType();

        switch (bld->getType())
        {
        case df::building_type::Hive:
            return df::unit_labor::BEEKEEPING;
        case df::building_type::Workshop:
        {
            df::building_workshopst* ws = (df::building_workshopst*) bld;
            if (ws->type == df::workshop_type::Custom)
            {
                df::building_def* def = df::building_def::find(ws->custom_type);
                return def->build_labors[0];
            }
            else
                return workshop_build_labor[ws->type];
        }
        break;
        case df::building_type::Construction:
            return df::unit_labor::REMOVE_CONSTRUCTION;
        case df::building_type::Furnace:
        case df::building_type::TradeDepot:
        case df::building_type::Wagon:
        case df::building_type::Bridge:
        case df::building_type::ScrewPump:
        case df::building_type::ArcheryTarget:
        case df::building_type::RoadPaved:
        case df::building_type::Shop:
        case df::building_type::Support:
        case df::building_type::WaterWheel:
        case df::building_type::Well:
        case df::building_type::Windmill:
        {
            auto b = (df::building_actual*) bld;
            return construction_build_labor(b);
        }
        break;
        case df::building_type::FarmPlot:
            return df::unit_labor::PLANT;
        case df::building_type::Trap:
        case df::building_type::AxleHorizontal:
        case df::building_type::AxleVertical:
        case df::building_type::GearAssembly:
        case df::building_type::Rollers:
            return df::unit_labor::MECHANIC;
        case df::building_type::Chair:
        case df::building_type::Bed:
        case df::building_type::Table:
        case df::building_type::Coffin:
        case df::building_type::Door:
        case df::building_type::Floodgate:
        case df::building_type::Box:
        case df::building_type::Weaponrack:
        case df::building_type::Armorstand:
        case df::building_type::Cabinet:
        case df::building_type::Statue:
        case df::building_type::WindowGlass:
        case df::building_type::WindowGem:
        case df::building_type::Cage:
        case df::building_type::NestBox:
        case df::building_type::TractionBench:
        case df::building_type::Slab:
        case df::building_type::Chain:
        case df::building_type::Hatch:
        case df::building_type::BarsFloor:
        case df::building_type::BarsVertical:
        case df::building_type::GrateFloor:
        case df::building_type::GrateWall:
        case df::building_type::Bookcase:
        case df::building_type::Instrument:
        case df::building_type::DisplayFurniture:
            return df::unit_labor::HAUL_FURNITURE;
        case df::building_type::AnimalTrap:
            return df::unit_labor::TRAPPER;
        case df::building_type::Civzone:
        case df::building_type::Nest:
        case df::building_type::RoadDirt:
        case df::building_type::Stockpile:
        case df::building_type::Weapon:
            return df::unit_labor::NONE;
        case df::building_type::SiegeEngine:
            return df::unit_labor::SIEGECRAFT;
        }

        debug("LABORMANAGER: Cannot deduce labor for destroy building job of type %s\n",
            ENUM_KEY_STR(building_type, bld->getType()).c_str());
        debug_pause();

        return df::unit_labor::NONE;
    }
    jlfunc_destroy_bld() {}
};

class jlfunc_make : public jlfunc
{
private:
    df::unit_labor metaltype;
public:
    df::unit_labor get_labor(df::job* j)
    {
        df::building* bld = get_building_from_job(j);
        if (bld->getType() == df::building_type::Workshop)
        {
            df::workshop_type type = ((df::building_workshopst*)(bld))->type;
            switch (type)
            {
            case df::workshop_type::Craftsdwarfs:
            {
                df::item_type jobitem = j->job_items[0]->item_type;
                switch (jobitem)
                {
                case df::item_type::BOULDER:
                    return df::unit_labor::STONE_CRAFT;
                case df::item_type::NONE:
                    if (j->material_category.bits.bone ||
                        j->material_category.bits.horn ||
                        j->material_category.bits.tooth ||
                        j->material_category.bits.shell)
                        return df::unit_labor::BONE_CARVE;
                    else
                    {
                        debug("LABORMANAGER: Cannot deduce labor for make crafts job (not bone)\n");
                        debug_pause();
                        return df::unit_labor::NONE;
                    }
                case df::item_type::WOOD:
                    return df::unit_labor::WOOD_CRAFT;
                case df::item_type::CLOTH:
                    return df::unit_labor::CLOTHESMAKER;
                case df::item_type::SKIN_TANNED:
                    return df::unit_labor::LEATHER;
                default:
                    debug("LABORMANAGER: Cannot deduce labor for make crafts job, item type %s\n",
                        ENUM_KEY_STR(item_type, jobitem).c_str());
                    debug_pause();
                    return df::unit_labor::NONE;
                }
            }
            case df::workshop_type::Masons:
                return df::unit_labor::MASON;
            case df::workshop_type::Carpenters:
                return df::unit_labor::CARPENTER;
            case df::workshop_type::Leatherworks:
                return df::unit_labor::LEATHER;
            case df::workshop_type::Clothiers:
                return df::unit_labor::CLOTHESMAKER;
            case df::workshop_type::Bowyers:
                return df::unit_labor::BOWYER;
            case df::workshop_type::MagmaForge:
            case df::workshop_type::MetalsmithsForge:
                return metaltype;
            default:
                debug("LABORMANAGER: Cannot deduce labor for make job, workshop type %s\n",
                    ENUM_KEY_STR(workshop_type, type).c_str());
                debug_pause();
                return df::unit_labor::NONE;
            }
        }
        else if (bld->getType() == df::building_type::Furnace)
        {
            df::furnace_type type = ((df::building_furnacest*)(bld))->type;
            switch (type)
            {
            case df::furnace_type::MagmaGlassFurnace:
            case df::furnace_type::GlassFurnace:
                return df::unit_labor::GLASSMAKER;
            default:
                debug("LABORMANAGER: Cannot deduce labor for make job, furnace type %s\n",
                    ENUM_KEY_STR(furnace_type, type).c_str());
                debug_pause();
                return df::unit_labor::NONE;
            }
        }

        debug("LABORMANAGER: Cannot deduce labor for make job, building type %s\n",
            ENUM_KEY_STR(building_type, bld->getType()).c_str());
        debug_pause();

        return df::unit_labor::NONE;
    }

    jlfunc_make(df::unit_labor mt) : metaltype(mt) {}
};

class jlfunc_custom : public jlfunc
{
public:
    df::unit_labor get_labor(df::job* j)
    {
        for (auto r = world->raws.reactions.begin(); r != world->raws.reactions.end(); r++)
        {
            if ((*r)->code == j->reaction_name)
            {
                df::job_skill skill = (*r)->skill;
                df::unit_labor labor = ENUM_ATTR(job_skill, labor, skill);
                return labor;
            }
        }
        return df::unit_labor::NONE;
    }
    jlfunc_custom() {}
};

jlfunc* JobLaborMapper::jlf_const(df::unit_labor l) {
    jlfunc* jlf;
    if (jlf_cache.count(l) == 0)
    {
        jlf = new jlfunc_const(l);
        jlf_cache[l] = jlf;
    }
    else
        jlf = jlf_cache[l];

    return jlf;
}

JobLaborMapper::~JobLaborMapper()
{
    std::set<jlfunc*> log;

    for (auto i = jlf_cache.begin(); i != jlf_cache.end(); i++)
    {
        if (!log.count(i->second))
        {
            log.insert(i->second);
            delete i->second;
        }
        i->second = 0;
    }

    FOR_ENUM_ITEMS(job_type, j)
    {
        if (j < 0)
            continue;

        jlfunc* p = job_to_labor_table[j];
        if (!log.count(p))
        {
            log.insert(p);
            delete p;
        }
        job_to_labor_table[j] = 0;
    }

}

JobLaborMapper::JobLaborMapper()
{
    jlfunc* jlf_hauling = new jlfunc_hauling();
    jlfunc* jlf_make_furniture = new jlfunc_make(df::unit_labor::FORGE_FURNITURE);
    jlfunc* jlf_make_object = new jlfunc_make(df::unit_labor::METAL_CRAFT);
    jlfunc* jlf_make_armor = new jlfunc_make(df::unit_labor::FORGE_ARMOR);
    jlfunc* jlf_make_weapon = new jlfunc_make(df::unit_labor::FORGE_WEAPON);

    jlfunc* jlf_no_labor = jlf_const(df::unit_labor::NONE);

    job_to_labor_table[df::job_type::CarveFortification] = jlf_const(df::unit_labor::DETAIL);
    job_to_labor_table[df::job_type::DetailWall] = jlf_const(df::unit_labor::DETAIL);
    job_to_labor_table[df::job_type::DetailFloor] = jlf_const(df::unit_labor::DETAIL);
    job_to_labor_table[df::job_type::Dig] = jlf_const(df::unit_labor::MINE);
    job_to_labor_table[df::job_type::CarveUpwardStaircase] = jlf_const(df::unit_labor::MINE);
    job_to_labor_table[df::job_type::CarveDownwardStaircase] = jlf_const(df::unit_labor::MINE);
    job_to_labor_table[df::job_type::CarveUpDownStaircase] = jlf_const(df::unit_labor::MINE);
    job_to_labor_table[df::job_type::CarveRamp] = jlf_const(df::unit_labor::MINE);
    job_to_labor_table[df::job_type::DigChannel] = jlf_const(df::unit_labor::MINE);
    job_to_labor_table[df::job_type::FellTree] = jlf_const(df::unit_labor::CUTWOOD);
    job_to_labor_table[df::job_type::GatherPlants] = jlf_const(df::unit_labor::HERBALIST);
    job_to_labor_table[df::job_type::RemoveConstruction] = jlf_const(df::unit_labor::REMOVE_CONSTRUCTION);
    job_to_labor_table[df::job_type::CollectWebs] = jlf_const(df::unit_labor::WEAVER);
    job_to_labor_table[df::job_type::BringItemToDepot] = jlf_const(df::unit_labor::HAUL_TRADE);
    job_to_labor_table[df::job_type::BringItemToShop] = jlf_no_labor;
    job_to_labor_table[df::job_type::Eat] = jlf_no_labor;
    job_to_labor_table[df::job_type::GetProvisions] = jlf_no_labor;
    job_to_labor_table[df::job_type::Drink] = jlf_no_labor;
    job_to_labor_table[df::job_type::Drink2] = jlf_no_labor;
    job_to_labor_table[df::job_type::FillWaterskin] = jlf_no_labor;
    job_to_labor_table[df::job_type::FillWaterskin2] = jlf_no_labor;
    job_to_labor_table[df::job_type::Sleep] = jlf_no_labor;
    job_to_labor_table[df::job_type::CollectSand] = jlf_const(df::unit_labor::HAUL_ITEM);
    job_to_labor_table[df::job_type::Fish] = jlf_const(df::unit_labor::FISH);
    job_to_labor_table[df::job_type::Hunt] = jlf_const(df::unit_labor::HUNT);
    job_to_labor_table[df::job_type::HuntVermin] = jlf_no_labor;
    job_to_labor_table[df::job_type::Kidnap] = jlf_no_labor;
    job_to_labor_table[df::job_type::BeatCriminal] = jlf_no_labor;
    job_to_labor_table[df::job_type::StartingFistFight] = jlf_no_labor;
    job_to_labor_table[df::job_type::CollectTaxes] = jlf_no_labor;
    job_to_labor_table[df::job_type::GuardTaxCollector] = jlf_no_labor;
    job_to_labor_table[df::job_type::CatchLiveLandAnimal] = jlf_const(df::unit_labor::HUNT);
    job_to_labor_table[df::job_type::CatchLiveFish] = jlf_const(df::unit_labor::FISH);
    job_to_labor_table[df::job_type::ReturnKill] = jlf_no_labor;
    job_to_labor_table[df::job_type::CheckChest] = jlf_no_labor;
    job_to_labor_table[df::job_type::StoreOwnedItem] = jlf_no_labor;
    job_to_labor_table[df::job_type::PlaceItemInTomb] = jlf_const(df::unit_labor::HAUL_BODY);
    job_to_labor_table[df::job_type::StoreItemInStockpile] = jlf_hauling;
    job_to_labor_table[df::job_type::StoreItemInBag] = jlf_hauling;
    job_to_labor_table[df::job_type::StoreItemInHospital] = jlf_hauling;
    job_to_labor_table[df::job_type::StoreWeapon] = jlf_hauling;
    job_to_labor_table[df::job_type::StoreArmor] = jlf_hauling;
    job_to_labor_table[df::job_type::StoreItemInBarrel] = jlf_hauling;
    job_to_labor_table[df::job_type::StoreItemInBin] = jlf_hauling;
    job_to_labor_table[df::job_type::SeekArtifact] = jlf_no_labor;
    job_to_labor_table[df::job_type::SeekInfant] = jlf_no_labor;
    job_to_labor_table[df::job_type::AttendParty] = jlf_no_labor;
    job_to_labor_table[df::job_type::GoShopping] = jlf_no_labor;
    job_to_labor_table[df::job_type::GoShopping2] = jlf_no_labor;
    job_to_labor_table[df::job_type::Clean] = jlf_const(df::unit_labor::CLEAN);
    job_to_labor_table[df::job_type::Rest] = jlf_no_labor;
    job_to_labor_table[df::job_type::PickupEquipment] = jlf_no_labor;
    job_to_labor_table[df::job_type::DumpItem] = jlf_const(df::unit_labor::HAUL_REFUSE);
    job_to_labor_table[df::job_type::StrangeMoodCrafter] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodJeweller] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodForge] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodMagmaForge] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodBrooding] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodFell] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodCarpenter] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodMason] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodBowyer] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodTanner] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodWeaver] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodGlassmaker] = jlf_no_labor;
    job_to_labor_table[df::job_type::StrangeMoodMechanics] = jlf_no_labor;
    job_to_labor_table[df::job_type::ConstructBuilding] = new jlfunc_construct_bld();
    job_to_labor_table[df::job_type::ConstructDoor] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructFloodgate] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructBed] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructThrone] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructCoffin] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructTable] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructChest] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructBin] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructArmorStand] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructWeaponRack] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructCabinet] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructStatue] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructBlocks] = jlf_make_furniture;
    job_to_labor_table[df::job_type::MakeRawGlass] = jlf_const(df::unit_labor::GLASSMAKER);
    job_to_labor_table[df::job_type::MakeCrafts] = jlf_make_object;
    job_to_labor_table[df::job_type::MintCoins] = jlf_const(df::unit_labor::METAL_CRAFT);
    job_to_labor_table[df::job_type::CutGems] = jlf_const(df::unit_labor::CUT_GEM);
    job_to_labor_table[df::job_type::CutGlass] = jlf_const(df::unit_labor::CUT_GEM);
    job_to_labor_table[df::job_type::EncrustWithGems] = jlf_const(df::unit_labor::ENCRUST_GEM);
    job_to_labor_table[df::job_type::EncrustWithGlass] = jlf_const(df::unit_labor::ENCRUST_GEM);
    job_to_labor_table[df::job_type::DestroyBuilding] = new jlfunc_destroy_bld();
    job_to_labor_table[df::job_type::SmeltOre] = jlf_const(df::unit_labor::SMELT);
    job_to_labor_table[df::job_type::MeltMetalObject] = jlf_const(df::unit_labor::SMELT);
    job_to_labor_table[df::job_type::ExtractMetalStrands] = jlf_const(df::unit_labor::EXTRACT_STRAND);
    job_to_labor_table[df::job_type::PlantSeeds] = jlf_const(df::unit_labor::PLANT);
    job_to_labor_table[df::job_type::HarvestPlants] = jlf_const(df::unit_labor::PLANT);
    job_to_labor_table[df::job_type::TrainHuntingAnimal] = jlf_const(df::unit_labor::ANIMALTRAIN);
    job_to_labor_table[df::job_type::TrainWarAnimal] = jlf_const(df::unit_labor::ANIMALTRAIN);
    job_to_labor_table[df::job_type::MakeWeapon] = jlf_make_weapon;
    job_to_labor_table[df::job_type::ForgeAnvil] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructCatapultParts] = jlf_const(df::unit_labor::SIEGECRAFT);
    job_to_labor_table[df::job_type::ConstructBallistaParts] = jlf_const(df::unit_labor::SIEGECRAFT);
    job_to_labor_table[df::job_type::MakeArmor] = jlf_make_armor;
    job_to_labor_table[df::job_type::MakeHelm] = jlf_make_armor;
    job_to_labor_table[df::job_type::MakePants] = jlf_make_armor;
    job_to_labor_table[df::job_type::StudWith] = jlf_make_object;
    job_to_labor_table[df::job_type::ButcherAnimal] = jlf_const(df::unit_labor::BUTCHER);
    job_to_labor_table[df::job_type::PrepareRawFish] = jlf_const(df::unit_labor::CLEAN_FISH);
    job_to_labor_table[df::job_type::MillPlants] = jlf_const(df::unit_labor::MILLER);
    job_to_labor_table[df::job_type::BaitTrap] = jlf_const(df::unit_labor::TRAPPER);
    job_to_labor_table[df::job_type::MilkCreature] = jlf_const(df::unit_labor::MILK);
    job_to_labor_table[df::job_type::MakeCheese] = jlf_const(df::unit_labor::MAKE_CHEESE);
    job_to_labor_table[df::job_type::ProcessPlants] = jlf_const(df::unit_labor::PROCESS_PLANT);
    job_to_labor_table[df::job_type::ProcessPlantsVial] = jlf_const(df::unit_labor::PROCESS_PLANT);
    job_to_labor_table[df::job_type::ProcessPlantsBarrel] = jlf_const(df::unit_labor::PROCESS_PLANT);
    job_to_labor_table[df::job_type::PrepareMeal] = jlf_const(df::unit_labor::COOK);
    job_to_labor_table[df::job_type::WeaveCloth] = jlf_const(df::unit_labor::WEAVER);
    job_to_labor_table[df::job_type::MakeGloves] = jlf_make_armor;
    job_to_labor_table[df::job_type::MakeShoes] = jlf_make_armor;
    job_to_labor_table[df::job_type::MakeShield] = jlf_make_armor;
    job_to_labor_table[df::job_type::MakeCage] = jlf_make_furniture;
    job_to_labor_table[df::job_type::MakeChain] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeFlask] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeGoblet] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeToy] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeAnimalTrap] = jlf_const(df::unit_labor::TRAPPER);
    job_to_labor_table[df::job_type::MakeBarrel] = jlf_make_furniture;
    job_to_labor_table[df::job_type::MakeBucket] = jlf_make_furniture;
    job_to_labor_table[df::job_type::MakeWindow] = jlf_make_furniture;
    job_to_labor_table[df::job_type::MakeTotem] = jlf_const(df::unit_labor::BONE_CARVE);
    job_to_labor_table[df::job_type::MakeAmmo] = jlf_make_weapon;
    job_to_labor_table[df::job_type::DecorateWith] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeBackpack] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeQuiver] = jlf_make_armor;
    job_to_labor_table[df::job_type::MakeBallistaArrowHead] = jlf_make_weapon;
    job_to_labor_table[df::job_type::AssembleSiegeAmmo] = jlf_const(df::unit_labor::SIEGECRAFT);
    job_to_labor_table[df::job_type::LoadCatapult] = jlf_const(df::unit_labor::SIEGEOPERATE);
    job_to_labor_table[df::job_type::LoadBallista] = jlf_const(df::unit_labor::SIEGEOPERATE);
    job_to_labor_table[df::job_type::FireCatapult] = jlf_const(df::unit_labor::SIEGEOPERATE);
    job_to_labor_table[df::job_type::FireBallista] = jlf_const(df::unit_labor::SIEGEOPERATE);
    job_to_labor_table[df::job_type::ConstructMechanisms] = jlf_const(df::unit_labor::MECHANIC);
    job_to_labor_table[df::job_type::MakeTrapComponent] = jlf_make_weapon;
    job_to_labor_table[df::job_type::LoadCageTrap] = jlf_const(df::unit_labor::MECHANIC);
    job_to_labor_table[df::job_type::LoadStoneTrap] = jlf_const(df::unit_labor::MECHANIC);
    job_to_labor_table[df::job_type::LoadWeaponTrap] = jlf_const(df::unit_labor::MECHANIC);
    job_to_labor_table[df::job_type::CleanTrap] = jlf_const(df::unit_labor::MECHANIC);
    job_to_labor_table[df::job_type::CastSpell] = jlf_no_labor;
    job_to_labor_table[df::job_type::LinkBuildingToTrigger] = jlf_const(df::unit_labor::MECHANIC);
    job_to_labor_table[df::job_type::PullLever] = jlf_const(df::unit_labor::PULL_LEVER);
    job_to_labor_table[df::job_type::ExtractFromPlants] = jlf_const(df::unit_labor::HERBALIST);
    job_to_labor_table[df::job_type::ExtractFromRawFish] = jlf_const(df::unit_labor::DISSECT_FISH);
    job_to_labor_table[df::job_type::ExtractFromLandAnimal] = jlf_const(df::unit_labor::DISSECT_VERMIN);
    job_to_labor_table[df::job_type::TameVermin] = jlf_const(df::unit_labor::ANIMALTRAIN);
    job_to_labor_table[df::job_type::TameAnimal] = jlf_const(df::unit_labor::ANIMALTRAIN);
    job_to_labor_table[df::job_type::ChainAnimal] = jlf_const(df::unit_labor::HAUL_ANIMALS);
    job_to_labor_table[df::job_type::UnchainAnimal] = jlf_const(df::unit_labor::HAUL_ANIMALS);
    job_to_labor_table[df::job_type::UnchainPet] = jlf_no_labor;
    job_to_labor_table[df::job_type::ReleaseLargeCreature] = jlf_const(df::unit_labor::HAUL_ANIMALS);
    job_to_labor_table[df::job_type::ReleasePet] = jlf_no_labor;
    job_to_labor_table[df::job_type::ReleaseSmallCreature] = jlf_no_labor;
    job_to_labor_table[df::job_type::HandleSmallCreature] = jlf_no_labor;
    job_to_labor_table[df::job_type::HandleLargeCreature] = jlf_const(df::unit_labor::HAUL_ANIMALS);
    job_to_labor_table[df::job_type::CageLargeCreature] = jlf_const(df::unit_labor::HAUL_ANIMALS);
    job_to_labor_table[df::job_type::CageSmallCreature] = jlf_no_labor;
    job_to_labor_table[df::job_type::RecoverWounded] = jlf_const(df::unit_labor::RECOVER_WOUNDED);
    job_to_labor_table[df::job_type::DiagnosePatient] = jlf_const(df::unit_labor::DIAGNOSE);
    job_to_labor_table[df::job_type::ImmobilizeBreak] = jlf_const(df::unit_labor::BONE_SETTING);
    job_to_labor_table[df::job_type::DressWound] = jlf_const(df::unit_labor::DRESSING_WOUNDS);
    job_to_labor_table[df::job_type::CleanPatient] = jlf_const(df::unit_labor::DRESSING_WOUNDS);
    job_to_labor_table[df::job_type::Surgery] = jlf_const(df::unit_labor::SURGERY);
    job_to_labor_table[df::job_type::Suture] = jlf_const(df::unit_labor::SUTURING);
    job_to_labor_table[df::job_type::SetBone] = jlf_const(df::unit_labor::BONE_SETTING);
    job_to_labor_table[df::job_type::PlaceInTraction] = jlf_const(df::unit_labor::BONE_SETTING);
    job_to_labor_table[df::job_type::DrainAquarium] = jlf_const(df::unit_labor::HAUL_WATER);
    job_to_labor_table[df::job_type::FillAquarium] = jlf_const(df::unit_labor::HAUL_WATER);
    job_to_labor_table[df::job_type::FillPond] = jlf_const(df::unit_labor::HAUL_WATER);
    job_to_labor_table[df::job_type::GiveWater] = jlf_const(df::unit_labor::FEED_WATER_CIVILIANS);
    job_to_labor_table[df::job_type::GiveFood] = jlf_const(df::unit_labor::FEED_WATER_CIVILIANS);
    job_to_labor_table[df::job_type::GiveWater2] = jlf_const(df::unit_labor::FEED_WATER_CIVILIANS);
    job_to_labor_table[df::job_type::GiveFood2] = jlf_const(df::unit_labor::FEED_WATER_CIVILIANS);
    job_to_labor_table[df::job_type::RecoverPet] = jlf_no_labor;
    job_to_labor_table[df::job_type::PitLargeAnimal] = jlf_const(df::unit_labor::HAUL_ANIMALS);
    job_to_labor_table[df::job_type::PitSmallAnimal] = jlf_no_labor;
    job_to_labor_table[df::job_type::SlaughterAnimal] = jlf_const(df::unit_labor::BUTCHER);
    job_to_labor_table[df::job_type::MakeCharcoal] = jlf_const(df::unit_labor::BURN_WOOD);
    job_to_labor_table[df::job_type::MakeAsh] = jlf_const(df::unit_labor::BURN_WOOD);
    job_to_labor_table[df::job_type::MakeLye] = jlf_const(df::unit_labor::LYE_MAKING);
    job_to_labor_table[df::job_type::MakePotashFromLye] = jlf_const(df::unit_labor::POTASH_MAKING);
    job_to_labor_table[df::job_type::FertilizeField] = jlf_const(df::unit_labor::PLANT);
    job_to_labor_table[df::job_type::MakePotashFromAsh] = jlf_const(df::unit_labor::POTASH_MAKING);
    job_to_labor_table[df::job_type::DyeThread] = jlf_const(df::unit_labor::DYER);
    job_to_labor_table[df::job_type::DyeCloth] = jlf_const(df::unit_labor::DYER);
    job_to_labor_table[df::job_type::SewImage] = jlf_make_object;
    job_to_labor_table[df::job_type::MakePipeSection] = jlf_make_furniture;
    job_to_labor_table[df::job_type::OperatePump] = jlf_const(df::unit_labor::OPERATE_PUMP);
    job_to_labor_table[df::job_type::ManageWorkOrders] = jlf_no_labor;
    job_to_labor_table[df::job_type::UpdateStockpileRecords] = jlf_no_labor;
    job_to_labor_table[df::job_type::TradeAtDepot] = jlf_no_labor;
    job_to_labor_table[df::job_type::ConstructHatchCover] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructGrate] = jlf_make_furniture;
    job_to_labor_table[df::job_type::RemoveStairs] = jlf_const(df::unit_labor::MINE);
    job_to_labor_table[df::job_type::ConstructQuern] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructMillstone] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructSplint] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructCrutch] = jlf_make_furniture;
    job_to_labor_table[df::job_type::ConstructTractionBench] = jlf_const(df::unit_labor::MECHANIC);
    job_to_labor_table[df::job_type::CleanSelf] = jlf_no_labor;
    job_to_labor_table[df::job_type::BringCrutch] = jlf_const(df::unit_labor::BONE_SETTING);
    job_to_labor_table[df::job_type::ApplyCast] = jlf_const(df::unit_labor::BONE_SETTING);
    job_to_labor_table[df::job_type::CustomReaction] = new jlfunc_custom();
    job_to_labor_table[df::job_type::ConstructSlab] = jlf_make_furniture;
    job_to_labor_table[df::job_type::EngraveSlab] = jlf_const(df::unit_labor::DETAIL);
    job_to_labor_table[df::job_type::ShearCreature] = jlf_const(df::unit_labor::SHEARER);
    job_to_labor_table[df::job_type::SpinThread] = jlf_const(df::unit_labor::SPINNER);
    job_to_labor_table[df::job_type::PenLargeAnimal] = jlf_const(df::unit_labor::HAUL_ANIMALS);
    job_to_labor_table[df::job_type::PenSmallAnimal] = jlf_no_labor;
    job_to_labor_table[df::job_type::MakeTool] = jlf_make_object;
    job_to_labor_table[df::job_type::CollectClay] = jlf_const(df::unit_labor::POTTERY);
    job_to_labor_table[df::job_type::InstallColonyInHive] = jlf_const(df::unit_labor::BEEKEEPING);
    job_to_labor_table[df::job_type::CollectHiveProducts] = jlf_const(df::unit_labor::BEEKEEPING);
    job_to_labor_table[df::job_type::CauseTrouble] = jlf_no_labor;
    job_to_labor_table[df::job_type::DrinkBlood] = jlf_no_labor;
    job_to_labor_table[df::job_type::ReportCrime] = jlf_no_labor;
    job_to_labor_table[df::job_type::ExecuteCriminal] = jlf_no_labor;
    job_to_labor_table[df::job_type::TrainAnimal] = jlf_const(df::unit_labor::ANIMALTRAIN);
    job_to_labor_table[df::job_type::CarveTrack] = jlf_const(df::unit_labor::DETAIL);
    job_to_labor_table[df::job_type::PushTrackVehicle] = jlf_const(df::unit_labor::HANDLE_VEHICLES);
    job_to_labor_table[df::job_type::PlaceTrackVehicle] = jlf_const(df::unit_labor::HANDLE_VEHICLES);
    job_to_labor_table[df::job_type::StoreItemInVehicle] = jlf_hauling;
    job_to_labor_table[df::job_type::GeldAnimal] = jlf_const(df::unit_labor::GELD);
    job_to_labor_table[df::job_type::MakeFigurine] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeAmulet] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeScepter] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeCrown] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeRing] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeEarring] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeBracelet] = jlf_make_object;
    job_to_labor_table[df::job_type::MakeGem] = jlf_make_object;
    job_to_labor_table[df::job_type::PutItemOnDisplay] = jlf_const(df::unit_labor::HAUL_ITEM);

    job_to_labor_table[df::job_type::StoreItemInLocation] = jlf_no_labor; // StoreItemInLocation
};

df::unit_labor JobLaborMapper::find_job_labor(df::job* j)
{
    if (j->job_type == df::job_type::CustomReaction)
    {
        for (auto r = world->raws.reactions.begin(); r != world->raws.reactions.end(); r++)
        {
            if ((*r)->code == j->reaction_name)
            {
                df::job_skill skill = (*r)->skill;
                return ENUM_ATTR(job_skill, labor, skill);
            }
        }
        return df::unit_labor::NONE;
    }


    df::unit_labor labor;
    if (job_to_labor_table.count(j->job_type) == 0)
    {
        debug("LABORMANAGER: job has no job to labor table entry: %s (%d)\n", ENUM_KEY_STR(job_type, j->job_type).c_str(), j->job_type);
        debug_pause();
        labor = df::unit_labor::NONE;
    }
    else {

        labor = job_to_labor_table[j->job_type]->get_labor(j);
    }

    return labor;
}

/* End of labor deducer */
