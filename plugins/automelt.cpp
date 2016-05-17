
#include "modules/Buildings.h"
#include "modules/EventManager.h"
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Maps.h"
#include "modules/Persistent.h"
#include "modules/World.h"

#include "df/building_def.h"
#include "df/building_stockpilest.h"
#include "df/item_quality.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"
#include "df/world_raws.h"
#include "df/ui.h"

#include "jsoncpp.h"
#include "uicommon.h"

#include <string>

using df::building_stockpilest;

DFHACK_PLUGIN("automelt");
#define PLUGIN_VERSION 0.3
REQUIRE_GLOBAL(cursor);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(world);

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

static const string PERSISTENCE_KEY = "automelt/stockpiles";
static const int32_t persist_version=1;
static void save_config(color_ostream& out);
static void on_presave_callback(color_ostream& out, void* nothing) {
    save_config(out);
}

static int mark_item(df::item *item, df::item_flags bad_flags, int32_t stockpile_id)
{
    if (item->flags.whole & bad_flags.whole)
        return 0;

    if (item->isAssignedToThisStockpile(stockpile_id)) {
        size_t marked_count = 0;
        std::vector<df::item*> contents;
        Items::getContainedItems(item, &contents);
        for (auto child = contents.begin(); child != contents.end(); child++)
        {
            marked_count += mark_item(*child, bad_flags, stockpile_id);
        }

        return marked_count;
    }

    if (!can_melt(item))
        return 0;

    if (is_set_to_melt(item))
        return 0;

    insert_into_vector(world->items.other[items_other_id::ANY_MELT_DESIGNATED], &df::item::id, item);
    item->flags.bits.melt = true;
    return 1;
}

static void mark_all_in_stockpiles(vector<StockpileInfo> &stockpiles)
{
    // Precompute a bitmask with the bad flags
    df::item_flags bad_flags;
    bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
    F(dump); F(forbid); F(garbage_collect);
    F(hostile); F(on_fire); F(rotten); F(trader);
    F(in_building); F(construction); F(artifact);
    F(spider_web); F(owned); F(in_job);
#undef F

    size_t marked_count = 0;
    for (auto it = stockpiles.begin(); it != stockpiles.end(); it++)
    {
        if (!it->isValid())
            continue;

        auto spid = it->getId();
        Buildings::StockpileIterator stored;
        for (stored.begin(it->getStockpile()); !stored.done(); ++stored)
        {
            marked_count += mark_item(*stored, bad_flags, spid);
        }
    }

    if (marked_count)
        Gui::showAnnouncement("Marked " + int_to_string(marked_count) + " items to melt", COLOR_GREEN, false);
}

/*
 * Stockpile Monitoring
 */

class StockpileMonitor
{
public:
    vector<StockpileInfo> monitored_stockpiles;
    bool isMonitored(df::building_stockpilest *sp)
    {
        for (auto it = monitored_stockpiles.begin(); it != monitored_stockpiles.end(); it++)
        {
            if (it->matches(sp))
                return true;
        }

        return false;
    }

    void add(df::building_stockpilest *sp)
    {
        StockpileInfo info(sp);
        if (!info.isValid())
            return;
        monitored_stockpiles.push_back(info);
    }

    void remove(df::building_stockpilest *sp)
    {
        for (auto it = monitored_stockpiles.begin(); it != monitored_stockpiles.end(); it++)
        {
            if (it->matches(sp))
            {
                //it->remove();
                monitored_stockpiles.erase(it);
                break;
            }
        }
    }

    void doCycle()
    {
        for (auto it = monitored_stockpiles.begin(); it != monitored_stockpiles.end();)
        {
            if (!it->isValid())
                it = monitored_stockpiles.erase(it);
            else
                ++it;
        }

        mark_all_in_stockpiles(monitored_stockpiles);
    }
};

static StockpileMonitor monitor;

static void load_config(color_ostream& out) {
    vector<StockpileInfo>& monitored_stockpiles = monitor.monitored_stockpiles;
    monitored_stockpiles.clear();

    plugin_self->plugin_enable(out,false);
    Json::Value& p = Persistent::get("automelt");
    int32_t version = p["version"].isInt() ? p["version"].asInt() : 0;
    if ( version == 0 ) {
        //read the old persistent histfigs
        vector<PersistentDataItem> old_items;
        DFHack::World::GetPersistentData(&old_items,PERSISTENCE_KEY);
        bool didSomething = false;
        for ( auto i = old_items.begin(); i != old_items.end(); ++i ) {
            int32_t id = i->ival(1);
            if ( !DFHack::World::DeletePersistentData(*i) ) {
                cerr << __FILE__ << ":" << __LINE__ << ": could not delete persistent data (id=" << id << ")" << endl;
            }
            df::building_stockpilest* stock = dynamic_cast<df::building_stockpilest*>(df::building::find(id));
            if ( !stock )
                continue;
            monitored_stockpiles.push_back(StockpileInfo(stock));
            didSomething = true;
        }
        if ( didSomething ) {
            plugin_self->plugin_enable(out,true);
        }
    } else if ( version == 1 ) {
        bool is_enabled = p["is_enabled"].isBool() ? p["is_enabled"].asBool() : false;
        plugin_self->plugin_enable(out,is_enabled);
        Json::Value& c = p["melt_stockpiles"];
        if ( c.type() == Json::arrayValue ) {
            for ( int32_t i = 0; i < c.size(); i++ ) {
                int32_t id = c[i].asInt();
                df::building_stockpilest* stock = dynamic_cast<df::building_stockpilest*>(df::building::find(id));
                if ( !stock )
                    return;
                monitored_stockpiles.push_back(StockpileInfo(stock));
            }
        }
        Persistent::erase("automelt"); //delete the property_tree stuff so we don't have duplicated data
    } else {
        cerr << __FILE__ << ":" << __LINE__ << ": Unrecognized version: " << version << endl;
        exit(1);
    }
}

static void save_config(color_ostream& out) {
    Persistent::erase("automelt"); //make certain there's no old data lying around
    Json::Value& p = Persistent::get("automelt");
    p["version"] = persist_version;
    p["is_enabled"] = is_enabled;
    Json::Value& s = p["melt_stockpiles"];
    std::for_each(monitor.monitored_stockpiles.begin(),monitor.monitored_stockpiles.end(),[&s](StockpileInfo& i) {
        s.append(i.getId());
    });
}

#define DELTA_TICKS 610

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    if(!Maps::IsValid())
        return CR_OK;

    static decltype(world->frame_counter) last_frame_count = 0;

    if (DFHack::World::ReadPauseState())
        return CR_OK;

    if (world->frame_counter - last_frame_count < DELTA_TICKS)
        return CR_OK;

    last_frame_count = world->frame_counter;

    monitor.doCycle();

    return CR_OK;
}


/*
 * Interface
 */

struct melt_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    bool handleInput(set<df::interface_key> *input)
    {
        building_stockpilest *sp = get_selected_stockpile();
        if (!sp)
            return false;

        if (input->count(interface_key::CUSTOM_SHIFT_M))
        {
            if (monitor.isMonitored(sp))
                monitor.remove(sp);
            else
                monitor.add(sp);
        }

        return false;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (!handleInput(input))
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        building_stockpilest *sp = get_selected_stockpile();
        if (!sp)
            return;

        auto dims = Gui::getDwarfmodeViewDims();
        int left_margin = dims.menu_x1 + 1;
        int x = left_margin;
        int y = dims.y2 - 6;

        int links = 0;
        links += sp->links.give_to_pile.size();
        links += sp->links.take_from_pile.size();
        links += sp->links.give_to_workshop.size();
        links += sp->links.take_from_workshop.size();
        bool state = monitor.isMonitored(sp);

        if (links + 12 >= y) {
            y = dims.y2;
            OutputString(COLOR_WHITE, x, y, "Auto: ");
            x += 5;
            OutputString(COLOR_LIGHTRED, x, y, "M");
            OutputString(state? COLOR_LIGHTGREEN: COLOR_GREY, x, y, "elt ");
        } else {
            OutputToggleString(x, y, "Auto melt", "M", state, true, left_margin, COLOR_WHITE, COLOR_LIGHTRED);
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(melt_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(melt_hook, render);


static command_result automelt_cmd(color_ostream &out, vector <string> & parameters)
{
    if (!parameters.empty())
    {
        if (parameters.size() == 1 && toLower(parameters[0])[0] == 'v')
        {
            out << "Automelt" << endl << "Version: " << PLUGIN_VERSION << endl;
        }
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event)
    {
    case DFHack::SC_MAP_LOADED:
        load_config(out);
        break;
    case DFHack::SC_MAP_UNLOADED:
        break;
    default:
        break;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (enable != is_enabled)
    {
        if (!INTERPOSE_HOOK(melt_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(melt_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(
        PluginCommand(
        "automelt", "Automatically melt metal items in marked stockpiles.",
        automelt_cmd, false, ""));

    EventManager::EventHandler handler(on_presave_callback, 1);
    EventManager::registerListener(EventManager::EventType::PRESAVE, handler, plugin_self);
    if ( DFHack::Core::getInstance().isWorldLoaded() )
        load_config(out);
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if ( DFHack::Core::getInstance().isWorldLoaded() )
        save_config(out);
    return CR_OK;
}
