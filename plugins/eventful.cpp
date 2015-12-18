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

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("eventful");
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);

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

static ReactionInfo *find_reaction(const std::string &name)
{
    auto it = reactions.find(name);
    return (it != reactions.end()) ? &it->second : NULL;
}

static bool is_lua_hook(const std::string &name)
{
    return name.size() > 9 && memcmp(name.data(), "LUA_HOOK_", 9) == 0;
}

/*
 * Hooks
 */
static void handle_fillsidebar(color_ostream &out,df::building_actual*,bool *call_native){};
static void handle_postfillsidebar(color_ostream &out,df::building_actual*){};

static void handle_reaction_done(color_ostream &out,df::reaction*, df::reaction_product_itemst*, df::unit *unit, std::vector<df::item*> *in_items,std::vector<df::reaction_reagent*> *in_reag
    , std::vector<df::item*> *out_items,bool *call_native){};
static void handle_contaminate_wound(color_ostream &out,df::item_actual*,df::unit* unit, df::unit_wound* wound, uint8_t a1, int16_t a2){};
static void handle_projitem_ci(color_ostream &out,df::proj_itemst*,bool){};
static void handle_projitem_cm(color_ostream &out,df::proj_itemst*){};
static void handle_projunit_ci(color_ostream &out,df::proj_unitst*,bool){};
static void handle_projunit_cm(color_ostream &out,df::proj_unitst*){};

DEFINE_LUA_EVENT_2(onWorkshopFillSidebarMenu, handle_fillsidebar, df::building_actual*,bool* );
DEFINE_LUA_EVENT_1(postWorkshopFillSidebarMenu, handle_postfillsidebar, df::building_actual*);

DEFINE_LUA_EVENT_7(onReactionComplete, handle_reaction_done,df::reaction*, df::reaction_product_itemst*, df::unit *, std::vector<df::item*> *,std::vector<df::reaction_reagent*> *,std::vector<df::item*> *,bool *);
DEFINE_LUA_EVENT_5(onItemContaminateWound, handle_contaminate_wound, df::item_actual*,df::unit* , df::unit_wound* , uint8_t , int16_t );
//projectiles
DEFINE_LUA_EVENT_2(onProjItemCheckImpact, handle_projitem_ci, df::proj_itemst*,bool );
DEFINE_LUA_EVENT_1(onProjItemCheckMovement, handle_projitem_cm, df::proj_itemst*);
DEFINE_LUA_EVENT_2(onProjUnitCheckImpact, handle_projunit_ci, df::proj_unitst*,bool );
DEFINE_LUA_EVENT_1(onProjUnitCheckMovement, handle_projunit_cm, df::proj_unitst* );
//event manager
static void handle_int32t(color_ostream &out,int32_t){}; //we don't use this so why not use it everywhere
static void handle_job_init(color_ostream &out,df::job*){};
static void handle_job_complete(color_ostream &out,df::job*){};
static void handle_constructions(color_ostream &out,df::construction*){};
static void handle_syndrome(color_ostream &out,int32_t,int32_t){};
static void handle_inventory_change(color_ostream& out,int32_t,int32_t,df::unit_inventory_item*,df::unit_inventory_item*){};
static void handle_report(color_ostream& out,int32_t){};
static void handle_unitAttack(color_ostream& out,int32_t,int32_t,int32_t){};
static void handle_unload(color_ostream& out){};
static void handle_interaction(color_ostream& out, std::string, std::string, int32_t, int32_t, int32_t, int32_t){};
DEFINE_LUA_EVENT_1(onBuildingCreatedDestroyed, handle_int32t, int32_t);
DEFINE_LUA_EVENT_1(onJobInitiated,handle_job_init,df::job*);
DEFINE_LUA_EVENT_1(onJobCompleted,handle_job_complete,df::job*);
DEFINE_LUA_EVENT_1(onUnitDeath,handle_int32t,int32_t);
DEFINE_LUA_EVENT_1(onItemCreated,handle_int32t,int32_t);
DEFINE_LUA_EVENT_1(onConstructionCreatedDestroyed, handle_constructions, df::construction*);
DEFINE_LUA_EVENT_2(onSyndrome, handle_syndrome, int32_t,int32_t);
DEFINE_LUA_EVENT_1(onInvasion,handle_int32t,int32_t);
DEFINE_LUA_EVENT_4(onInventoryChange,handle_inventory_change,int32_t,int32_t,df::unit_inventory_item*,df::unit_inventory_item*);
DEFINE_LUA_EVENT_1(onReport,handle_report,int32_t);
DEFINE_LUA_EVENT_3(onUnitAttack,handle_unitAttack,int32_t,int32_t,int32_t);
DEFINE_LUA_EVENT_0(onUnload,handle_unload);
DEFINE_LUA_EVENT_6(onInteraction,handle_interaction, std::string, std::string, int32_t, int32_t, int32_t, int32_t);
DFHACK_PLUGIN_LUA_EVENTS {
    DFHACK_LUA_EVENT(onWorkshopFillSidebarMenu),
    DFHACK_LUA_EVENT(postWorkshopFillSidebarMenu),
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
    DFHACK_LUA_EVENT(onJobCompleted),
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
void ev_mng_jobCompleted(color_ostream& out, void* job)
{
    df::job* ptr=reinterpret_cast<df::job*>(job);
    onJobCompleted(out,ptr);
}
void ev_mng_unitDeath(color_ostream& out, void* ptr)
{
    int32_t myId=int32_t(ptr);
    onUnitDeath(out,myId);
}
void ev_mng_itemCreate(color_ostream& out, void* ptr)
{
    int32_t myId=int32_t(ptr);
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
    int32_t myId=int32_t(ptr);
    onInvasion(out,myId);
}
static void ev_mng_building(color_ostream& out, void* ptr)
{
    int32_t myId=int32_t(ptr);
    onBuildingCreatedDestroyed(out,myId);
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
    onReport(out,(int32_t)ptr);
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
static const handler_t eventHandlers[] = {
 NULL,
 ev_mng_jobInitiated,
 ev_mng_jobCompleted,
 ev_mng_unitDeath,
 ev_mng_itemCreate,
 ev_mng_building,
 ev_mng_construction,
 ev_mng_syndrome,
 ev_mng_invasion,
 ev_mng_inventory,
 ev_mng_report,
 ev_mng_unitAttack,
 ev_mng_unload,
 ev_mng_interaction,
};
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
        EventManager::unregister(typeToEnable,EventManager::EventHandler(fun_ptr,oldFreq),plugin_self);
    }
    EventManager::registerListener(typeToEnable,EventManager::EventHandler(fun_ptr,freq),plugin_self);
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
         std::vector<void*> *unk,
         std::vector<df::item*> *out_items,
         std::vector<df::reaction_reagent*> *in_reag,
         std::vector<df::item*> *in_items,
         int32_t quantity, df::job_skill skill,
         df::historical_entity *entity, df::world_site *site)
    ) {
        color_ostream_proxy out(Core::getInstance().getConsole());
        auto product = products[this];
        if ( !product ) {
            INTERPOSE_NEXT(produce)(unit, unk, out_items, in_reag, in_items, quantity, skill, entity, site);
            return;
        }
        df::reaction* this_reaction=product->react;
        CoreSuspendClaimer suspend;
        bool call_native=true;
        onReactionComplete(out,this_reaction,(df::reaction_product_itemst*)this,unit,in_items,in_reag,out_items,&call_native);
        if(!call_native)
            return;

        size_t out_item_count = out_items->size();
        INTERPOSE_NEXT(produce)(unit, unk, out_items, in_reag, in_items, quantity, skill, entity, site);
        if ( out_items->size() == out_item_count )
            return;
        //if it produced something, call the scripts
        onReactionComplete(out,this_reaction,(df::reaction_product_itemst*)this,unit,in_items,in_reag,out_items,NULL);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(product_hook, produce);


struct item_hooks :df::item_actual {
        typedef df::item_actual interpose_base;

        DEFINE_VMETHOD_INTERPOSE(void, contaminateWound,(df::unit* unit, df::unit_wound* wound, uint8_t a1, int16_t a2))
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

    auto &rlist = world->raws.reactions;

    for (size_t i = 0; i < rlist.size(); i++)
    {
        //if (!is_lua_hook(rlist[i]->code))
        //    continue;
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
        //out.print("Detected reaction hooks - enabling plugin.\n");
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
