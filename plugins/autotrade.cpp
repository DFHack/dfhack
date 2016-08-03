#include "uicommon.h"

#include "modules/Gui.h"

#include "df/world.h"
#include "df/world_raws.h"
#include "df/building_def.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_tradegoodsst.h"
#include "df/building_stockpilest.h"
#include "modules/Buildings.h"
#include "modules/Items.h"
#include "df/building_tradedepotst.h"
#include "df/general_ref_building_holderst.h"
#include "df/job.h"
#include "df/job_item_ref.h"
#include "modules/Job.h"
#include "df/ui.h"
#include "df/mandate.h"
#include "modules/Maps.h"

using df::building_stockpilest;

DFHACK_PLUGIN("autotrade");
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(cursor);
REQUIRE_GLOBAL(ui);

static const string PERSISTENCE_KEY = "autotrade/stockpiles";

/*
 * Depot Access
 */

class TradeDepotInfo
{
public:
    TradeDepotInfo() : depot(0)
    {

    }

    bool findDepot()
    {
        if (isValid())
            return true;

        reset();
        for(auto bld_it = world->buildings.all.begin(); bld_it != world->buildings.all.end(); bld_it++)
        {
            auto bld = *bld_it;
            if (!isUsableDepot(bld))
                continue;

            depot = bld;
            id = depot->id;
            break;
        }

        return depot;
    }

    bool assignItem(df::item *item)
    {
        auto href = df::allocate<df::general_ref_building_holderst>();
        if (!href)
            return false;

        auto job = new df::job();

        df::coord tpos(depot->centerx, depot->centery, depot->z);
        job->pos = tpos;

        job->job_type = job_type::BringItemToDepot;

        // job <-> item link
        if (!Job::attachJobItem(job, item, df::job_item_ref::Hauled))
        {
            delete job;
            delete href;
            return false;
        }

        // job <-> building link
        href->building_id = id;
        depot->jobs.push_back(job);
        job->general_refs.push_back(href);

        // add to job list
        Job::linkIntoWorld(job);

        return true;
    }

    void reset()
    {
        depot = 0;
    }

private:
    int32_t id;
    df::building *depot;

    bool isUsableDepot(df::building* bld)
    {
        if (bld->getType() != building_type::TradeDepot)
            return false;

        if (bld->getBuildStage() < bld->getMaxBuildStage())
            return false;

        if (bld->jobs.size() == 1 && bld->jobs[0]->job_type == job_type::DestroyBuilding)
            return false;

        return true;
    }

    bool isValid()
    {
        if (!depot)
            return false;

        auto found = df::building::find(id);
        return found && found == depot && isUsableDepot(found);
    }

};

static TradeDepotInfo depot_info;


/*
 * Item Manipulation
 */

static bool check_mandates(df::item *item)
{
    for (auto it = world->mandates.begin(); it != world->mandates.end(); it++)
    {
        auto mandate = *it;

        if (mandate->mode != 0)
            continue;

        if (item->getType() != mandate->item_type ||
            (mandate->item_subtype != -1 && item->getSubtype() != mandate->item_subtype))
            continue;

        if (mandate->mat_type != -1 && item->getMaterial() != mandate->mat_type)
            continue;

        if (mandate->mat_index != -1 && item->getMaterialIndex() != mandate->mat_index)
            continue;

        return false;
    }

    return true;
}

static bool is_valid_item(df::item *item)
{
    for (size_t i = 0; i < item->general_refs.size(); i++)
    {
        df::general_ref *ref = item->general_refs[i];

        switch (ref->getType())
        {
        case general_ref_type::CONTAINED_IN_ITEM:
            return false;

        case general_ref_type::UNIT_HOLDER:
            return false;

        case general_ref_type::BUILDING_HOLDER:
            return false;

        default:
            break;
        }
    }

    for (size_t i = 0; i < item->specific_refs.size(); i++)
    {
        df::specific_ref *ref = item->specific_refs[i];

        if (ref->type == specific_ref_type::JOB)
        {
            // Ignore any items assigned to a job
            return false;
        }
    }

    if (!check_mandates(item))
        return false;

    return true;
}

static void mark_all_in_stockpiles(vector<PersistentStockpileInfo> &stockpiles)
{
    if (!depot_info.findDepot())
        return;


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
    size_t error_count = 0;
    for (auto it = stockpiles.begin(); it != stockpiles.end(); it++)
    {
        if (!it->isValid())
            continue;

        Buildings::StockpileIterator stored;
        for (stored.begin(it->getStockpile()); !stored.done(); ++stored)
        {
            df::item *item = *stored;
            if (item->flags.whole & bad_flags.whole)
                continue;

            if (!is_valid_item(item))
                continue;

            // In case of container, check contained items for mandates
            bool mandates_ok = true;
            vector<df::item*> contained_items;
            Items::getContainedItems(item, &contained_items);
            for (auto cit = contained_items.begin(); cit != contained_items.end(); cit++)
            {
                if (!check_mandates(*cit))
                {
                    mandates_ok = false;
                    break;
                }
            }

            if (!mandates_ok)
                continue;

            if (depot_info.assignItem(item))
            {
                ++marked_count;
            }
            else
            {
                if (++error_count < 5)
                {
                    Gui::showZoomAnnouncement(df::announcement_type::CANCEL_JOB, item->pos,
                        "Cannot trade item from stockpile " + int_to_string(it->getId()), COLOR_RED, true);
                }
            }
        }
    }

    if (marked_count)
        Gui::showAnnouncement("Marked " + int_to_string(marked_count) + " items for trade", COLOR_GREEN, false);

    if (error_count >= 5)
    {
        Gui::showAnnouncement(int_to_string(error_count) + " items were not marked", COLOR_RED, true);
    }
}


/*
 * Stockpile Monitoring
 */

class StockpileMonitor
{
public:
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
        auto pile = PersistentStockpileInfo(sp, PERSISTENCE_KEY);
        if (pile.isValid())
        {
            monitored_stockpiles.push_back(pile);
            monitored_stockpiles.back().save();
        }
    }

    void remove(df::building_stockpilest *sp)
    {
        for (auto it = monitored_stockpiles.begin(); it != monitored_stockpiles.end(); it++)
        {
            if (it->matches(sp))
            {
                it->remove();
                monitored_stockpiles.erase(it);
                break;
            }
        }
    }

    void doCycle()
    {
        if (!can_trade())
            return;

        for (auto it = monitored_stockpiles.begin(); it != monitored_stockpiles.end();)
        {
            if (!it->isValid())
            {
                it = monitored_stockpiles.erase(it);
                continue;
            }

            ++it;
        }

        mark_all_in_stockpiles(monitored_stockpiles);
    }

    void reset()
    {
        monitored_stockpiles.clear();
        std::vector<PersistentDataItem> items;
        DFHack::World::GetPersistentData(&items, PERSISTENCE_KEY);

        for (auto i = items.begin(); i != items.end(); i++)
        {
            auto pile = PersistentStockpileInfo(*i, PERSISTENCE_KEY);
            if (pile.load())
                monitored_stockpiles.push_back(pile);
            else
                pile.remove();
        }
    }


private:
    vector<PersistentStockpileInfo> monitored_stockpiles;
};

static StockpileMonitor monitor;

#define DELTA_TICKS 600

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

struct trade_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    bool handleInput(set<df::interface_key> *input)
    {
        building_stockpilest *sp = get_selected_stockpile();
        if (!sp)
            return false;

        if (input->count(interface_key::CUSTOM_SHIFT_T))
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
        int y = dims.y2 - 5;

        int links = 0;
        links += sp->links.give_to_pile.size();
        links += sp->links.take_from_pile.size();
        links += sp->links.give_to_workshop.size();
        links += sp->links.take_from_workshop.size();
        bool state = monitor.isMonitored(sp);

        if (links + 12 >= y) {
            y = dims.y2;
            OutputString(COLOR_WHITE, x, y, "Auto: ");
            x += 11;
            OutputString(COLOR_LIGHTRED, x, y, "T");
            OutputString(state? COLOR_LIGHTGREEN: COLOR_GREY, x, y, "rade ");
        } else {
            OutputToggleString(x, y, "Auto trade", "T", state, true, left_margin, COLOR_WHITE, COLOR_LIGHTRED);
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(trade_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(trade_hook, render);


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event)
    {
    case DFHack::SC_MAP_LOADED:
        depot_info.reset();
        monitor.reset();
        break;
    case DFHack::SC_MAP_UNLOADED:
        break;
    default:
        break;
    }
    return CR_OK;
}

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (enable != is_enabled)
    {
        depot_info.reset();
        monitor.reset();

        if (!INTERPOSE_HOOK(trade_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(trade_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
