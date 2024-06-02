#include "Core.h"
#include "Error.h"
#include "Console.h"
#include "Export.h"
#include "LuaTools.h"
#include "MiscUtils.h"
#include "PluginManager.h"
#include "VTableInterpose.h"

#include "df/building.h"
#include "df/building_furnacest.h"
#include "df/building_workshopst.h"
#include "df/construction.h"
#include "df/item.h"
#include "df/item_actual.h"
#include "df/job.h"
#include "df/proj_itemst.h"
#include "df/proj_unitst.h"
#include "df/reaction.h"
#include "df/reaction_reagent_itemst.h"
#include "df/reaction_product_itemst.h"
#include "df/unit.h"
#include "df/unit_inventory_item.h"
#include "df/unit_wound.h"
#include "df/world.h"

#include "modules/EventManager.h"

#include <string.h>
#include <stdexcept>
#include <array>

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("eventful");
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(plotinfo);

typedef df::reaction_product_itemst item_product;

struct ReagentSource {
    int idx;
    df::reaction_reagent *reagent;

    ReagentSource() : idx(-1), reagent(NULL) {}
};

struct MaterialSource : ReagentSource {
    bool product;
    std::string product_name;

    int mat_type, mat_index;

    MaterialSource() : product(false), mat_type(-1), mat_index(-1) {}
};

struct ProductInfo {
    df::reaction *react;
    item_product *product;

    MaterialSource material;

    bool isValid() {
        return true;//due to mat_type being -1 = any
    }
};

struct ReactionInfo {
    df::reaction *react;

    std::vector<ProductInfo> products;
};

static std::map<std::string, ReactionInfo> reactions;
static std::map<df::reaction_product*, ProductInfo*> products;

/*
 * Hooks
 */

DEFINE_LUA_EVENT_NH_2(onWorkshopFillSidebarMenu, df::building_actual*, bool*);
DEFINE_LUA_EVENT_NH_1(postWorkshopFillSidebarMenu, df::building_actual*);

DEFINE_LUA_EVENT_NH_7(onReactionCompleting, df::reaction*, df::reaction_product_itemst*, df::unit *, std::vector<df::item*> *, std::vector<df::reaction_reagent*> *, std::vector<df::item*> *, bool *);
DEFINE_LUA_EVENT_NH_6(onReactionComplete, df::reaction*, df::reaction_product_itemst*, df::unit *, std::vector<df::item*> *, std::vector<df::reaction_reagent*> *, std::vector<df::item*> *);
DEFINE_LUA_EVENT_NH_5(onItemContaminateWound, df::item_actual*, df::unit*, df::unit_wound*, uint8_t, int16_t);
//projectiles
DEFINE_LUA_EVENT_NH_2(onProjItemCheckImpact, df::proj_itemst*, bool);
DEFINE_LUA_EVENT_NH_1(onProjItemCheckMovement, df::proj_itemst*);
DEFINE_LUA_EVENT_NH_2(onProjUnitCheckImpact, df::proj_unitst*, bool);
DEFINE_LUA_EVENT_NH_1(onProjUnitCheckMovement, df::proj_unitst*);
//event manager
DEFINE_LUA_EVENT_NH_1(onBuildingCreatedDestroyed, int32_t);
DEFINE_LUA_EVENT_NH_1(onJobInitiated, df::job*);
DEFINE_LUA_EVENT_NH_1(onJobStarted, df::job*);
DEFINE_LUA_EVENT_NH_1(onJobCompleted, df::job*);
DEFINE_LUA_EVENT_NH_1(onUnitNewActive, int32_t);
DEFINE_LUA_EVENT_NH_1(onUnitDeath, int32_t);
DEFINE_LUA_EVENT_NH_1(onItemCreated, int32_t);
DEFINE_LUA_EVENT_NH_1(onConstructionCreatedDestroyed, df::construction*);
DEFINE_LUA_EVENT_NH_2(onSyndrome, int32_t, int32_t);
DEFINE_LUA_EVENT_NH_1(onInvasion, int32_t);
DEFINE_LUA_EVENT_NH_4(onInventoryChange, int32_t, int32_t, df::unit_inventory_item*, df::unit_inventory_item*);
DEFINE_LUA_EVENT_NH_1(onReport, int32_t);
DEFINE_LUA_EVENT_NH_3(onUnitAttack, int32_t, int32_t, int32_t);
DEFINE_LUA_EVENT_NH_0(onUnload);
DEFINE_LUA_EVENT_NH_6(onInteraction, std::string, std::string, int32_t, int32_t, int32_t, int32_t);

DFHACK_PLUGIN_LUA_EVENTS {
    DFHACK_LUA_EVENT(onWorkshopFillSidebarMenu),
    DFHACK_LUA_EVENT(postWorkshopFillSidebarMenu),
    DFHACK_LUA_EVENT(onReactionCompleting),
    DFHACK_LUA_EVENT(onReactionComplete),
    DFHACK_LUA_EVENT(onItemContaminateWound),
    DFHACK_LUA_EVENT(onProjItemCheckImpact),
    DFHACK_LUA_EVENT(onProjItemCheckMovement),
    DFHACK_LUA_EVENT(onProjUnitCheckImpact),
    DFHACK_LUA_EVENT(onProjUnitCheckMovement),
    /*  event manager events */
    DFHACK_LUA_EVENT(onBuildingCreatedDestroyed),
    DFHACK_LUA_EVENT(onConstructionCreatedDestroyed),
    DFHACK_LUA_EVENT(onJobInitiated),
    DFHACK_LUA_EVENT(onJobStarted),
    DFHACK_LUA_EVENT(onJobCompleted),
    DFHACK_LUA_EVENT(onUnitNewActive),
    DFHACK_LUA_EVENT(onUnitDeath),
    DFHACK_LUA_EVENT(onItemCreated),
    DFHACK_LUA_EVENT(onSyndrome),
    DFHACK_LUA_EVENT(onInvasion),
    DFHACK_LUA_EVENT(onInventoryChange),
    DFHACK_LUA_EVENT(onReport),
    DFHACK_LUA_EVENT(onUnitAttack),
    DFHACK_LUA_EVENT(onUnload),
    DFHACK_LUA_EVENT(onInteraction),
    DFHACK_LUA_END
};

static void ev_mng_jobInitiated(color_ostream& out, void* job)
{
    df::job* ptr=reinterpret_cast<df::job*>(job);
    onJobInitiated(out,ptr);
}
static void ev_mng_jobStarted(color_ostream& out, void* job)
{
    df::job* ptr=reinterpret_cast<df::job*>(job);
    onJobStarted(out,ptr);
}
void ev_mng_jobCompleted(color_ostream& out, void* job)
{
    df::job* ptr=reinterpret_cast<df::job*>(job);
    onJobCompleted(out,ptr);
}
void ev_mng_unitNewActive(color_ostream& out, void* ptr)
{
    int32_t myId=(int32_t)(intptr_t)ptr;
    onUnitNewActive(out,myId);
}
void ev_mng_unitDeath(color_ostream& out, void* ptr)
{
    int32_t myId=(int32_t)(intptr_t)ptr;
    onUnitDeath(out,myId);
}
void ev_mng_itemCreate(color_ostream& out, void* ptr)
{
    int32_t myId=(int32_t)(intptr_t)ptr;
    onItemCreated(out,myId);
}
void ev_mng_construction(color_ostream& out, void* ptr)
{
    df::construction* cons=reinterpret_cast<df::construction*>(ptr);
    onConstructionCreatedDestroyed(out,cons);
}
void ev_mng_syndrome(color_ostream& out, void* ptr)
{
    EventManager::SyndromeData* data=reinterpret_cast<EventManager::SyndromeData*>(ptr);
    onSyndrome(out,data->unitId,data->syndromeIndex);
}
void ev_mng_invasion(color_ostream& out, void* ptr)
{
    int32_t myId=(int32_t)(intptr_t)ptr;
    onInvasion(out,myId);
}
static void ev_mng_building(color_ostream& out, void* ptr)
{
    int32_t id=(int32_t)(intptr_t)ptr;
    onBuildingCreatedDestroyed(out, id);
}
static void ev_mng_inventory(color_ostream& out, void* ptr)
{
    EventManager::InventoryChangeData* data = reinterpret_cast<EventManager::InventoryChangeData*>(ptr);
    int32_t unitId = data->unitId;
    int32_t itemId = -1;
    df::unit_inventory_item* item_old = NULL;
    df::unit_inventory_item* item_new = NULL;
    if ( data->item_old ) {
        itemId = data->item_old->itemId;
        item_old = &data->item_old->item;
    }
    if ( data->item_new ) {
        itemId = data->item_new->itemId;
        item_new = &data->item_new->item;
    }
    onInventoryChange(out,unitId,itemId,item_old,item_new);
}
static void ev_mng_report(color_ostream& out, void* ptr) {
    onReport(out,(int32_t)(intptr_t)ptr);
}
static void ev_mng_unitAttack(color_ostream& out, void* ptr) {
    EventManager::UnitAttackData* data = (EventManager::UnitAttackData*)ptr;
    onUnitAttack(out,data->attacker,data->defender,data->wound);
}
static void ev_mng_unload(color_ostream& out, void* ptr) {
    onUnload(out);
}
static void ev_mng_interaction(color_ostream& out, void* ptr) {
    EventManager::InteractionData* data = (EventManager::InteractionData*)ptr;
    onInteraction(out, data->attackVerb, data->defendVerb, data->attacker, data->defender, data->attackReport, data->defendReport);
}
std::vector<int> enabledEventManagerEvents(EventManager::EventType::EVENT_MAX,-1);
typedef void (*handler_t) (color_ostream&,void*);

using namespace EventManager::EventType;
// integrate new events into this function, and no longer worry about syncing with the enum list
handler_t getManager(EventType t) {
    switch (t) {
        case TICK:
            return nullptr;
        case JOB_INITIATED:
            return ev_mng_jobInitiated;
        case JOB_STARTED:
            return ev_mng_jobStarted;
        case JOB_COMPLETED:
            return ev_mng_jobCompleted;
        case UNIT_NEW_ACTIVE:
            return ev_mng_unitNewActive;
        case UNIT_DEATH:
            return ev_mng_unitDeath;
        case ITEM_CREATED:
            return ev_mng_itemCreate;
        case BUILDING:
            return ev_mng_building;
        case CONSTRUCTION:
            return ev_mng_construction;
        case SYNDROME:
            return ev_mng_syndrome;
        case INVASION:
            return ev_mng_invasion;
        case INVENTORY_CHANGE:
            return ev_mng_inventory;
        case REPORT:
            return ev_mng_report;
        case UNIT_ATTACK:
            return ev_mng_unitAttack;
        case UNLOAD:
            return ev_mng_unload;
        case INTERACTION:
            return ev_mng_interaction;
        case EVENT_MAX:
            return nullptr;
            //default:
            //we don't do this... because then the compiler wouldn't error for missing cases in the enum
    }
    return nullptr;
}

std::array<handler_t,EventManager::EventType::EVENT_MAX> compileEventHandlerArray() {
    std::array<handler_t, EventManager::EventType::EVENT_MAX> managers{};
    auto t = (EventManager::EventType::EventType) 0;
    while (t < EventManager::EventType::EVENT_MAX) {
        managers[t] = getManager(t);
        t = (EventManager::EventType::EventType) int(t + 1);
    }
    return managers;
}
static std::array<handler_t,EventManager::EventType::EVENT_MAX> eventHandlers;

static void enableEvent(int evType,int freq)
{
    if (freq < 0)
        return;
    CHECK_INVALID_ARGUMENT(evType >= 0 && evType < EventManager::EventType::EVENT_MAX &&
                           evType != EventManager::EventType::TICK);
    EventManager::EventHandler::callback_t fun_ptr = eventHandlers[evType];
    EventManager::EventType::EventType typeToEnable=static_cast<EventManager::EventType::EventType>(evType);

    int oldFreq = enabledEventManagerEvents[typeToEnable];
    if (oldFreq != -1) {
        if (freq >= oldFreq)
            return;
        EventManager::unregister(typeToEnable,EventManager::EventHandler(plugin_self,fun_ptr,oldFreq));
    }
    EventManager::registerListener(typeToEnable,EventManager::EventHandler(plugin_self,fun_ptr,freq));
    enabledEventManagerEvents[typeToEnable] = freq;
}
DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(enableEvent),
    DFHACK_LUA_END
};
struct workshop_hook : df::building_workshopst{
    typedef df::building_workshopst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void,fillSidebarMenu,())
    {
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        bool call_native=true;
        onWorkshopFillSidebarMenu(out,this,&call_native);
        if(call_native)
            INTERPOSE_NEXT(fillSidebarMenu)();
        postWorkshopFillSidebarMenu(out,this);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, fillSidebarMenu);

struct furnace_hook : df::building_furnacest{
    typedef df::building_furnacest interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void,fillSidebarMenu,())
    {
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        bool call_native=true;
        onWorkshopFillSidebarMenu(out,this,&call_native);
        if(call_native)
            INTERPOSE_NEXT(fillSidebarMenu)();
        postWorkshopFillSidebarMenu(out,this);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(furnace_hook, fillSidebarMenu);

struct product_hook : item_product {
    typedef item_product interpose_base;

    DEFINE_VMETHOD_INTERPOSE(
        void, produce,
        (df::unit *unit,
         std::vector<df::reaction_product*> *out_products,
         std::vector<df::item*> *out_items,
         std::vector<df::reaction_reagent*> *in_reag,
         std::vector<df::item*> *in_items,
         int32_t quantity, df::job_skill skill,
         int32_t quality, df::historical_entity *entity,  df::world_site *site, std::vector<void *> *unk2)
    ) {
        color_ostream_proxy out(Core::getInstance().getConsole());
        auto product = products[this];
        if ( !product ) {
            INTERPOSE_NEXT(produce)(unit, out_products, out_items, in_reag, in_items, quantity, skill, quality, entity, site, unk2);
            return;
        }
        df::reaction* this_reaction=product->react;
        CoreSuspendClaimer suspend;
        bool call_native=true;
        onReactionCompleting(out,this_reaction,(df::reaction_product_itemst*)this,unit,in_items,in_reag,out_items,&call_native);
        if(!call_native)
            return;

        size_t out_item_count = out_items->size();

        INTERPOSE_NEXT(produce)(unit, out_products, out_items, in_reag, in_items, quantity, skill, quality, entity, site, unk2);
        if ( out_items->size() == out_item_count )
            return;
        //if it produced something, call the scripts
        onReactionComplete(out,this_reaction,(df::reaction_product_itemst*)this,unit,in_items,in_reag,out_items);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(product_hook, produce);


struct item_hooks :df::item_actual {
        typedef df::item_actual interpose_base;

        DEFINE_VMETHOD_INTERPOSE(void, contaminateWound,(df::unit* unit, df::unit_wound* wound, int32_t a1, int16_t a2))
        {
            CoreSuspendClaimer suspend;
            color_ostream_proxy out(Core::getInstance().getConsole());
            onItemContaminateWound(out,this,unit,wound,a1,a2);
            INTERPOSE_NEXT(contaminateWound)(unit,wound,a1,a2);
        }

};
IMPLEMENT_VMETHOD_INTERPOSE(item_hooks, contaminateWound);

struct proj_item_hook: df::proj_itemst{
    typedef df::proj_itemst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(bool,checkImpact,(bool mode))
    {
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        onProjItemCheckImpact(out,this,mode);
        return INTERPOSE_NEXT(checkImpact)(mode); //returns destroy item or not?
    }
    DEFINE_VMETHOD_INTERPOSE(bool,checkMovement,())
    {
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        onProjItemCheckMovement(out,this);
        return INTERPOSE_NEXT(checkMovement)();
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(proj_item_hook,checkImpact);
IMPLEMENT_VMETHOD_INTERPOSE(proj_item_hook,checkMovement);

struct proj_unit_hook: df::proj_unitst{
    typedef df::proj_unitst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(bool,checkImpact,(bool mode))
    {
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        onProjUnitCheckImpact(out,this,mode);
        return INTERPOSE_NEXT(checkImpact)(mode); //returns destroy item or not?
    }
    DEFINE_VMETHOD_INTERPOSE(bool,checkMovement,())
    {
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        onProjUnitCheckMovement(out,this);
        return INTERPOSE_NEXT(checkMovement)();
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(proj_unit_hook,checkImpact);
IMPLEMENT_VMETHOD_INTERPOSE(proj_unit_hook,checkMovement);
/*
 * Scan raws for matching reactions.
 */


static void parse_product(
    color_ostream &out, ProductInfo &info, df::reaction *react, item_product *prod
    ) {
        info.react = react;
        info.product = prod;
        info.material.mat_type = prod->mat_type;
        info.material.mat_index = prod->mat_index;
}

static bool find_reactions(color_ostream &out)
{
    reactions.clear();

    auto &rlist = df::reaction::get_vector();

    for (size_t i = 0; i < rlist.size(); i++)
    {
        reactions[rlist[i]->code].react = rlist[i];
    }

    for (auto it = reactions.begin(); it != reactions.end(); ++it)
    {
        auto &prod = it->second.react->products;
        auto &out_prod = it->second.products;

        for (size_t i = 0; i < prod.size(); i++)
        {
            auto itprod = strict_virtual_cast<item_product>(prod[i]);
            if (!itprod) continue;

            out_prod.push_back(ProductInfo());
            parse_product(out, out_prod.back(), it->second.react, itprod);
        }

        for (size_t i = 0; i < out_prod.size(); i++)
        {
            if (out_prod[i].isValid())
                products[out_prod[i].product] = &out_prod[i];
        }
    }

    return !products.empty();
}

static void enable_hooks(bool enable)
{
    INTERPOSE_HOOK(workshop_hook,fillSidebarMenu).apply(enable);
    INTERPOSE_HOOK(furnace_hook,fillSidebarMenu).apply(enable);
    INTERPOSE_HOOK(item_hooks,contaminateWound).apply(enable);
    INTERPOSE_HOOK(proj_unit_hook,checkImpact).apply(enable);
    INTERPOSE_HOOK(proj_unit_hook,checkMovement).apply(enable);
    INTERPOSE_HOOK(proj_item_hook,checkImpact).apply(enable);
    INTERPOSE_HOOK(proj_item_hook,checkMovement).apply(enable);
}
static void world_specific_hooks(color_ostream &out,bool enable)
{
    if(enable && find_reactions(out))
    {
        INTERPOSE_HOOK(product_hook, produce).apply(true);
    }
    else
    {
        INTERPOSE_HOOK(product_hook, produce).apply(false);
        reactions.clear();
        products.clear();
    }
}
void disable_all_hooks(color_ostream &out)
{
    world_specific_hooks(out,false);
    enable_hooks(false);
}
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_WORLD_LOADED:
        world_specific_hooks(out,true);
        break;
    case SC_WORLD_UNLOADED:
        world_specific_hooks(out,false);

        break;
    default:
        break;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    eventHandlers = compileEventHandlerArray();
    if (Core::getInstance().isWorldLoaded())
        plugin_onstatechange(out, SC_WORLD_LOADED);
    enable_hooks(true);
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    disable_all_hooks(out);
    return CR_OK;
}
