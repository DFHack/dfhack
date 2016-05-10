// Quick Dumper : Moves items marked as "dump" to cursor
// FIXME: local item cache in map blocks needs to be fixed after teleporting items
#include <algorithm>
#include <climits>
#include <functional>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/EventManager.h"
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
#include "modules/Persistent.h"

#include "df/building.h"
#include "df/building_stockpilest.h"
#include "df/general_ref.h"
#include "df/item.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"

#include "uicommon.h"

#include "jsoncpp.h"

using namespace DFHack;
using namespace df::enums;

using MapExtras::Block;
using MapExtras::MapCache;

using df::building_stockpilest;

DFHACK_PLUGIN("autodump");

DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(world);

static const int32_t persist_version=1;
static void save_config(color_ostream& out);
static void load_config(color_ostream& out);
static void on_presave_callback(color_ostream& out, void* nothing) {
    save_config(out);
}

// Stockpile interface START
static const string PERSISTENCE_KEY = "autodump/stockpiles";

//static void mark_all_in_stockpiles(vector<PersistentStockpileInfo> &stockpiles)
static void mark_all_in_stockpiles(vector<StockpileInfo>& stockpiles)
{
    std::vector<df::item*> &items = world->items.other[items_other_id::IN_PLAY];

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
    for (size_t i = 0; i < items.size(); i++)
    {
        df::item *item = items[i];
        if (item->flags.whole & bad_flags.whole)
            continue;

        for (auto it = stockpiles.begin(); it != stockpiles.end(); it++)
        {
            if (!it->inStockpile(item))
                continue;

            ++marked_count;
            item->flags.bits.dump = true;
        }
    }

    if (marked_count)
        Gui::showAnnouncement("Marked " + int_to_string(marked_count) + " items to dump", COLOR_GREEN, false);
}

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
    Json::Value& p = Persistent::get("autodump");
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
        Json::Value& c = p["dump_stockpiles"];
        if ( c.type() == Json::arrayValue ) {
            for ( int32_t i = 0; i < c.size(); i++ ) {
                int32_t id = c[i].asInt();
                df::building_stockpilest* stock = dynamic_cast<df::building_stockpilest*>(df::building::find(id));
                if ( !stock )
                    return;
                monitored_stockpiles.push_back(StockpileInfo(stock));
            }
        }
        Persistent::erase("autodump"); //delete the property_tree stuff so we don't have duplicated data
    } else {
        cerr << __FILE__ << ":" << __LINE__ << ": Unrecognized version: " << version << endl;
        exit(1);
    }
}

static void save_config(color_ostream& out) {
    Persistent::erase("autodump"); //make certain there's no old data lying around
    Json::Value& p = Persistent::get("autodump");
    p["version"] = persist_version;
    p["is_enabled"] = is_enabled;
    Json::Value& s = p["dump_stockpiles"];
    std::for_each(monitor.monitored_stockpiles.begin(),monitor.monitored_stockpiles.end(),[&s](StockpileInfo& i) {
        s.append(i.getId());
    });
}

#define DELTA_TICKS 620


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

struct dump_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    bool handleInput(set<df::interface_key> *input)
    {
        building_stockpilest *sp = get_selected_stockpile();
        if (!sp)
            return false;

        if (input->count(interface_key::CUSTOM_SHIFT_D))
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
        int y = dims.y2 - 7;

        int links = 0;
        links += sp->links.give_to_pile.size();
        links += sp->links.take_from_pile.size();
        links += sp->links.give_to_workshop.size();
        links += sp->links.take_from_workshop.size();
        bool state = monitor.isMonitored(sp);

        if (links + 12 >= y) {
            y = dims.y2;
            OutputString(COLOR_WHITE, x, y, "Auto: ");
            OutputString(COLOR_LIGHTRED, x, y, "D");
            OutputString(state? COLOR_LIGHTGREEN: COLOR_GREY, x, y, "ump ");
        } else {
            OutputToggleString(x, y, "Auto dump", "D", state, true, left_margin, COLOR_WHITE, COLOR_LIGHTRED);
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(dump_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(dump_hook, render);

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
        if (!INTERPOSE_HOOK(dump_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(dump_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

// Stockpile interface END

command_result df_autodump(color_ostream &out, vector <string> & parameters);
command_result df_autodump_destroy_here(color_ostream &out, vector <string> & parameters);
command_result df_autodump_destroy_item(color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "autodump", "Teleport items marked for dumping to the cursor.",
        df_autodump, false,
        "  This utility lets you quickly move all items designated to be dumped.\n"
        "  Items are instantly moved to the cursor position, the dump flag is unset,\n"
        "  and the forbid flag is set, as if it had been dumped normally.\n"
        "  Be aware that any active dump item tasks still point at the item.\n"
        "Options:\n"
        "  destroy       - instead of dumping, destroy the items instantly.\n"
        "  destroy-here  - only affect the tile under cursor.\n"
        "  visible       - only process items that are not hidden.\n"
        "  hidden        - only process hidden items.\n"
        "  forbidden     - only process forbidden items (default: only unforbidden).\n"
    ));
    commands.push_back(PluginCommand(
        "autodump-destroy-here", "Destroy items marked for dumping under cursor.",
        df_autodump_destroy_here, Gui::cursor_hotkey,
        "  Identical to autodump destroy-here, but intended for use as keybinding.\n"
    ));
    commands.push_back(PluginCommand(
        "autodump-destroy-item", "Destroy the selected item.",
        df_autodump_destroy_item, Gui::any_item_hotkey,
        "  Destroy the selected item. The item may be selected\n"
        "  in the 'k' list, or inside a container. If called\n"
        "  again before the game is resumed, cancels destroy.\n"
    ));

    EventManager::EventHandler handler(on_presave_callback, 1);
    EventManager::registerListener(EventManager::EventType::PRESAVE, handler, plugin_self);

    if ( DFHack::Core::getInstance().isWorldLoaded() ) {
        load_config(out);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if ( DFHack::Core::getInstance().isWorldLoaded() ) {
        save_config(out);
    }
    return CR_OK;
}

typedef map <DFCoord, uint32_t> coordmap;

static command_result autodump_main(color_ostream &out, vector <string> & parameters)
{
    // Command line options
    bool destroy = false;
    bool here = false;
    bool need_visible = false;
    bool need_hidden = false;
    bool need_forbidden = false;
    for (size_t i = 0; i < parameters.size(); i++)
    {
        string & p = parameters[i];
        if(p == "destroy")
            destroy = true;
        else if (p == "destroy-here")
            destroy = here = true;
        else if (p == "visible")
            need_visible = true;
        else if (p == "hidden")
            need_hidden = true;
        else if (p == "forbidden")
            need_forbidden = true;
        else
            return CR_WRONG_USAGE;
    }

    if (need_visible && need_hidden)
    {
        out.printerr("An item can't be both hidden and visible.\n");
        return CR_WRONG_USAGE;
    }

    //DFHack::VersionInfo *mem = Core::getInstance().vinfo;
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    size_t numItems = world->items.all.size();

    MapCache MC;
    int i = 0;
    int dumped_total = 0;

    int cx, cy, cz;
    DFCoord pos_cursor;
    if(!destroy || here)
    {
        if (!Gui::getCursorCoords(cx,cy,cz))
        {
            out.printerr("Cursor position not found. Please enable the cursor.\n");
            return CR_FAILURE;
        }
        pos_cursor = DFCoord(cx,cy,cz);
    }
    if (!destroy)
    {
        {
            Block * b = MC.BlockAt(pos_cursor / 16);
            if(!b)
            {
                out.printerr("Cursor is in an invalid/uninitialized area. Place it over a floor.\n");
                return CR_FAILURE;
            }
            df::tiletype ttype = MC.tiletypeAt(pos_cursor);
            if(!DFHack::isWalkable(ttype) || DFHack::isOpenTerrain(ttype))
            {
                out.printerr("Cursor should be placed over a floor.\n");
                return CR_FAILURE;
            }
        }
    }

    // proceed with the dumpification operation
    for(size_t i=0; i< numItems; i++)
    {
        df::item * itm = world->items.all[i];
        DFCoord pos_item(itm->pos.x, itm->pos.y, itm->pos.z);

        // only dump the stuff marked for dumping and laying on the ground
        if (   !itm->flags.bits.dump
//          || !itm->flags.bits.on_ground
            ||  itm->flags.bits.construction
            ||  itm->flags.bits.in_building
            ||  itm->flags.bits.in_chest
//          ||  itm->flags.bits.in_inventory
            ||  itm->flags.bits.artifact
        )
            continue;

        if (need_visible && itm->flags.bits.hidden)
            continue;
        if (need_hidden && !itm->flags.bits.hidden)
            continue;
        if (need_forbidden && !itm->flags.bits.forbid)
            continue;
        if (!need_forbidden && itm->flags.bits.forbid)
            continue;

        if(!destroy) // move to cursor
        {
            // Change flags to indicate the dump was completed, as if by super-dwarfs
            itm->flags.bits.dump = false;
            itm->flags.bits.forbid = true;

            // Don't move items if they're already at the cursor
            if (pos_cursor != pos_item)
            {
                if (!Items::moveToGround(MC, itm, pos_cursor))
                    out.print("Could not move item: %s\n",
                              Items::getDescription(itm, 0, true).c_str());
            }
        }
        else // destroy
        {
            if (here && pos_item != pos_cursor)
                continue;

            itm->flags.bits.garbage_collect = true;

            // Cosmetic changes: make them disappear from view instantly
            itm->flags.bits.forbid = true;
            itm->flags.bits.hidden = true;
        }

        dumped_total++;
    }

    // write map changes back to DF.
    if(!destroy)
        MC.WriteAll();

    out.print("Done. %d items %s.\n", dumped_total, destroy ? "marked for destruction" : "quickdumped");
    return CR_OK;
}

command_result df_autodump(color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    return autodump_main(out, parameters);
}

command_result df_autodump_destroy_here(color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND; CORE ALREADY SUSPENDED
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    vector<string> args;
    args.push_back("destroy-here");

    return autodump_main(out, args);
}

static map<int, df::item_flags> pending_destroy;
static int last_frame = 0;

command_result df_autodump_destroy_item(color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND; CORE ALREADY SUSPENDED
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    df::item *item = Gui::getSelectedItem(out);
    if (!item)
        return CR_FAILURE;

    // Allow undoing the destroy
    if (world->frame_counter != last_frame)
    {
        last_frame = world->frame_counter;
        pending_destroy.clear();
    }

    if (pending_destroy.count(item->id))
    {
        df::item_flags old_flags = pending_destroy[item->id];
        pending_destroy.erase(item->id);

        item->flags.bits.garbage_collect = false;
        item->flags.bits.hidden = old_flags.bits.hidden;
        item->flags.bits.dump = old_flags.bits.dump;
        item->flags.bits.forbid = old_flags.bits.forbid;
        return CR_OK;
    }

    // Check the item is good to destroy
    if (item->flags.bits.garbage_collect)
    {
        out.printerr("Item is already marked for destroy.\n");
        return CR_FAILURE;
    }

    if (item->flags.bits.construction ||
        item->flags.bits.in_building ||
        item->flags.bits.artifact)
    {
        out.printerr("Choosing not to destroy buildings, constructions and artifacts.\n");
        return CR_FAILURE;
    }

    for (size_t i = 0; i < item->general_refs.size(); i++)
    {
        df::general_ref *ref = item->general_refs[i];
        if (ref->getType() == general_ref_type::UNIT_HOLDER)
        {
            out.printerr("Choosing not to destroy items in unit inventory.\n");
            return CR_FAILURE;
        }
    }

    // Set the flags
    pending_destroy[item->id] = item->flags;

    item->flags.bits.garbage_collect = true;
    item->flags.bits.hidden = true;
    item->flags.bits.dump = true;
    item->flags.bits.forbid = true;
    return CR_OK;
}
