#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/Gui.h>
#include <modules/Screen.h>
#include <modules/Maps.h>
#include <modules/Job.h>
#include <modules/Items.h>
#include <modules/Units.h>
#include <TileTypes.h>
#include <vector>
#include <cstdio>
#include <stack>
#include <string>
#include <cmath>
#include <string.h>

#include <VTableInterpose.h>
#include "df/item_liquid_miscst.h"
#include "df/item_constructed.h"
#include "df/builtin_mats.h"
#include "df/world.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/job_item_ref.h"
#include "df/ui.h"
#include "df/report.h"
#include "df/reaction.h"
#include "df/reaction_reagent_itemst.h"
#include "df/reaction_product_itemst.h"
#include "df/matter_state.h"
#include "df/contaminant.h"

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

DFHACK_PLUGIN("reactionhooks");

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

static void find_material(int *type, int *index, df::item *input, MaterialSource &mat)
{
    if (input && mat.reagent)
    {
        MaterialInfo info(input);

        if (mat.product)
        {
            if (!info.findProduct(info, mat.product_name))
            {
                color_ostream_proxy out(Core::getInstance().getConsole());
                out.printerr("Cannot find product '%s'\n", mat.product_name.c_str());
            }
        }

        *type = info.type;
        *index = info.index;
    }
    else
    {
        *type = mat.mat_type;
        *index = mat.mat_index;
    }
}


/*
 * Hooks
 */

typedef std::map<int, std::vector<df::item*> > item_table;

static void index_items(item_table &table, df::job *job, ReactionInfo *info)
{
    for (int i = job->items.size()-1; i >= 0; i--)
    {
        auto iref = job->items[i];
        if (iref->job_item_idx < 0) continue;
        auto iitem = job->job_items[iref->job_item_idx];

        if (iitem->contains.empty())
        {
            table[iitem->reagent_index].push_back(iref->item);
        }
        else
        {
            std::vector<df::item*> contents;
            Items::getContainedItems(iref->item, &contents);

            for (int j = contents.size()-1; j >= 0; j--)
            {
                for (int k = iitem->contains.size()-1; k >= 0; k--)
                {
                    int ridx = iitem->contains[k];
                    auto reag = info->react->reagents[ridx];

                    if (reag->matchesChild(contents[j], info->react, iitem->reaction_id))
                        table[ridx].push_back(contents[j]);
                }
            }
        }
    }
}

df::item* find_item(ReagentSource &info, item_table &table)
{
    if (!info.reagent)
        return NULL;
    if (table[info.idx].empty())
        return NULL;
    return table[info.idx].back();
}



df::item* find_item(
    ReagentSource &info,
    std::vector<df::reaction_reagent*> *in_reag,
    std::vector<df::item*> *in_items
) {
    if (!info.reagent)
        return NULL;
    for (int i = in_items->size(); i >= 0; i--)
        if ((*in_reag)[i] == info.reagent)
            return (*in_items)[i];
    return NULL;
}

static void handle_reaction_done(color_ostream &out,df::reaction*, df::unit *unit, std::vector<df::item*> *in_items,std::vector<df::reaction_reagent*> *in_reag
	, std::vector<df::item*> *out_items,bool *call_native){};

DEFINE_LUA_EVENT_6(onReactionComplete, handle_reaction_done,df::reaction*, df::unit *, std::vector<df::item*> *,std::vector<df::reaction_reagent*> *,std::vector<df::item*> *,bool *);


DFHACK_PLUGIN_LUA_EVENTS {
    DFHACK_LUA_EVENT(onReactionComplete),
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
    INTERPOSE_HOOK(product_hook, produce).apply(enable);
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_WORLD_LOADED:
        if (find_reactions(out))
        {
            out.print("Detected reaction hooks - enabling plugin.\n");
            enable_hooks(true);
        }
        else
            enable_hooks(false);
        break;
    case SC_WORLD_UNLOADED:
        enable_hooks(false);
        reactions.clear();
        products.clear();
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

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    enable_hooks(false);
    return CR_OK;
}
