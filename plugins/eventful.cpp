#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <string.h>

#include <VTableInterpose.h>
#include "df/unit.h"
#include "df/item.h"
#include "df/item_actual.h"
#include "df/unit_wound.h"
#include "df/world.h"
#include "df/reaction.h"
#include "df/reaction_reagent_itemst.h"
#include "df/reaction_product_itemst.h"

#include "MiscUtils.h"
#include "LuaTools.h"

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;
using namespace df::enums;

using df::global::gps;
using df::global::world;
using df::global::ui;

typedef df::reaction_product_itemst item_product;

DFHACK_PLUGIN("eventful");

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
        return (material.mat_type >= 0 || material.reagent);
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

static void handle_reaction_done(color_ostream &out,df::reaction*, df::unit *unit, std::vector<df::item*> *in_items,std::vector<df::reaction_reagent*> *in_reag
    , std::vector<df::item*> *out_items,bool *call_native){};
static void handle_contaminate_wound(color_ostream &out,df::item_actual*,df::unit* unit, df::unit_wound* wound, uint8_t a1, int16_t a2){};

DEFINE_LUA_EVENT_6(onReactionComplete, handle_reaction_done,df::reaction*, df::unit *, std::vector<df::item*> *,std::vector<df::reaction_reagent*> *,std::vector<df::item*> *,bool *);
DEFINE_LUA_EVENT_5(onItemContaminateWound, handle_contaminate_wound, df::item_actual*,df::unit* , df::unit_wound* , uint8_t , int16_t );

DFHACK_PLUGIN_LUA_EVENTS {
    DFHACK_LUA_EVENT(onReactionComplete),
    DFHACK_LUA_EVENT(onItemContaminateWound),
    DFHACK_LUA_END
};

struct product_hook : item_product {
    typedef item_product interpose_base;

    DEFINE_VMETHOD_INTERPOSE(
        void, produce,
        (df::unit *unit, std::vector<df::item*> *out_items,
         std::vector<df::reaction_reagent*> *in_reag,
         std::vector<df::item*> *in_items,
         int32_t quantity, df::job_skill skill,
         df::historical_entity *entity, df::world_site *site)
    ) {
        if (auto product = products[this])
        {
            df::reaction* this_reaction=product->react;
            CoreSuspendClaimer suspend;
            color_ostream_proxy out(Core::getInstance().getConsole());
            bool call_native=true;
            onReactionComplete(out,this_reaction,unit,in_items,in_reag,out_items,&call_native);
            if(!call_native)
                return;
        }

        INTERPOSE_NEXT(produce)(unit, out_items, in_reag, in_items, quantity, skill, entity, site);
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
        if (!is_lua_hook(rlist[i]->code))
            continue;
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

        for (size_t i = 0; i < prod.size(); i++)
        {
            if (out_prod[i].isValid())
                products[out_prod[i].product] = &out_prod[i];
        }
    }

    return !products.empty();
}

static void enable_hooks(bool enable)
{
    INTERPOSE_HOOK(item_hooks,contaminateWound).apply(enable);
}
static void world_specific_hooks(color_ostream &out,bool enable)
{
    if(enable && find_reactions(out))
    {
        out.print("Detected reaction hooks - enabling plugin.\n");
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
