// automatically chop trees

#include "uicommon.h"
#include "listcolumn.h"

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "DataDefs.h"
#include "TileTypes.h"

#include "df/world.h"
#include "df/map_block.h"
#include "df/tile_dig_designation.h"
#include "df/plant_raw.h"
#include "df/plant.h"
#include "df/ui.h"
#include "df/burrow.h"
#include "df/item_flags.h"
#include "df/item.h"
#include "df/items_other_id.h"
#include "df/viewscreen_dwarfmodest.h"

#include "modules/Screen.h"
#include "modules/Maps.h"
#include "modules/Burrows.h"
#include "modules/World.h"
#include "modules/MapCache.h"
#include "modules/Gui.h"

#include <set>

using std::string;
using std::vector;
using std::set;
using namespace DFHack;
using namespace df::enums;

#define PLUGIN_VERSION 0.3
DFHACK_PLUGIN("autochop");
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);

static bool autochop_enabled = false;
static int min_logs, max_logs;
static bool wait_for_threshold;

static PersistentDataItem config_autochop;

struct WatchedBurrow
{
    int32_t id;
    df::burrow *burrow;

    WatchedBurrow(df::burrow *burrow) : burrow(burrow)
    {
        id = burrow->id;
    }
};

class WatchedBurrows
{
public:
    string getSerialisedIds()
    {
        validate();
        stringstream burrow_ids;
        bool append_started = false;
        for (auto it = burrows.begin(); it != burrows.end(); it++)
        {
            if (append_started)
                burrow_ids << " ";
            burrow_ids << it->id;
            append_started = true;
        }

        return burrow_ids.str();
    }

    void clear()
    {
        burrows.clear();
    }

    void add(const int32_t id)
    {
        if (!isValidBurrow(id))
            return;

        WatchedBurrow wb(getBurrow(id));
        burrows.push_back(wb);
    }

    void add(const string burrow_ids)
    {
        istringstream iss(burrow_ids);
        int id;
        while (iss >> id)
        {
            add(id);
        }
    }

    bool isValidPos(const df::coord &plant_pos)
    {
        validate();
        if (!burrows.size())
            return true;

        for (auto it = burrows.begin(); it != burrows.end(); it++)
        {
            df::burrow *burrow = it->burrow;
            if (Burrows::isAssignedTile(burrow, plant_pos))
                return true;
        }

        return false;
    }

    bool isBurrowWatched(const df::burrow *burrow)
    {
        validate();
        for (auto it = burrows.begin(); it != burrows.end(); it++)
        {
            if (it->burrow == burrow)
                return true;
        }

        return false;
    }

private:
    static bool isValidBurrow(const int32_t id)
    {
        return getBurrow(id);
    }

    static df::burrow *getBurrow(const int32_t id)
    {
        return df::burrow::find(id);
    }

    void validate()
    {
        for (auto it = burrows.begin(); it != burrows.end();)
        {
            if (!isValidBurrow(it->id))
                it = burrows.erase(it);
            else
                ++it;
        }
    }

    vector<WatchedBurrow> burrows;
};

static WatchedBurrows watchedBurrows;

static void save_config()
{
    config_autochop.val() = watchedBurrows.getSerialisedIds();
    config_autochop.ival(0) = autochop_enabled;
    config_autochop.ival(1) = min_logs;
    config_autochop.ival(2) = max_logs;
    config_autochop.ival(3) = wait_for_threshold;
}

static void initialize()
{
    watchedBurrows.clear();
    autochop_enabled = false;
    min_logs = 80;
    max_logs = 100;
    wait_for_threshold = false;

    config_autochop = World::GetPersistentData("autochop/config");
    if (config_autochop.isValid())
    {
        watchedBurrows.add(config_autochop.val());
        autochop_enabled = config_autochop.ival(0);
        min_logs = config_autochop.ival(1);
        max_logs = config_autochop.ival(2);
        wait_for_threshold = config_autochop.ival(3);
    }
    else
    {
        config_autochop = World::AddPersistentData("autochop/config");
        if (config_autochop.isValid())
            save_config();
    }
}

static int do_chop_designation(bool chop, bool count_only)
{
    int count = 0;
    for (size_t i = 0; i < world->plants.all.size(); i++)
    {
        const df::plant *plant = world->plants.all[i];
        df::map_block *cur = Maps::getTileBlock(plant->pos);
        if (!cur)
            continue;
        int x = plant->pos.x % 16;
        int y = plant->pos.y % 16;

        if (plant->flags.bits.is_shrub)
            continue;
        if (cur->designation[x][y].bits.hidden)
            continue;

        df::tiletype_material material = tileMaterial(cur->tiletype[x][y]);
        if (material != tiletype_material::TREE)
            continue;

        if (!count_only && !watchedBurrows.isValidPos(plant->pos))
            continue;

        bool dirty = false;
        if (chop && cur->designation[x][y].bits.dig == tile_dig_designation::No)
        {
            if (count_only)
            {
                ++count;
            }
            else
            {
                cur->designation[x][y].bits.dig = tile_dig_designation::Default;
                dirty = true;
            }
        }

        if (!chop && cur->designation[x][y].bits.dig == tile_dig_designation::Default)
        {
            if (count_only)
            {
                ++count;
            }
            else
            {
                cur->designation[x][y].bits.dig = tile_dig_designation::No;
                dirty = true;
            }
        }

        if (dirty)
        {
            cur->flags.bits.designated = true;
            ++count;
        }
    }

    return count;
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

    return true;
}

static int get_log_count()
{
    std::vector<df::item*> &items = world->items.other[items_other_id::IN_PLAY];

    // Pre-compute a bitmask with the bad flags
    df::item_flags bad_flags;
    bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
    F(dump); F(forbid); F(garbage_collect);
    F(hostile); F(on_fire); F(rotten); F(trader);
    F(in_building); F(construction); F(artifact);
    F(spider_web); F(owned); F(in_job);
#undef F

    size_t valid_count = 0;
    for (size_t i = 0; i < items.size(); i++)
    {
        df::item *item = items[i];

        if (item->getType() != item_type::WOOD)
            continue;

        if (item->flags.whole & bad_flags.whole)
            continue;

        if (!is_valid_item(item))
            continue;

        ++valid_count;
    }

    return valid_count;
}

static void set_threshold_check(bool state)
{
    wait_for_threshold = state;
    save_config();
}

static void do_autochop()
{
    int log_count = get_log_count();
    if (wait_for_threshold)
    {
        if (log_count < min_logs)
        {
            set_threshold_check(false);
            do_chop_designation(true, false);
        }
    }
    else
    {
        if (log_count >= max_logs)
        {
            set_threshold_check(true);
            do_chop_designation(false, false);
        }
        else
        {
            do_chop_designation(true, false);
        }
    }
}

class ViewscreenAutochop : public dfhack_viewscreen
{
public:
    ViewscreenAutochop()
    {
        burrows_column.multiselect = true;
        burrows_column.setTitle("Burrows");
        burrows_column.bottom_margin = 3;
        burrows_column.allow_search = false;
        burrows_column.text_clip_at = 30;

        populateBurrowsColumn();
        message.clear();
    }

    void populateBurrowsColumn()
    {
        selected_column = 0;

        auto last_selected_index = burrows_column.highlighted_index;
        burrows_column.clear();

        for (auto iter = ui->burrows.list.begin(); iter != ui->burrows.list.end(); iter++)
        {
            df::burrow* burrow = *iter;
            auto elem = ListEntry<df::burrow *>(burrow->name, burrow);
            elem.selected = watchedBurrows.isBurrowWatched(burrow);
            burrows_column.add(elem);
        }

        burrows_column.fixWidth();
        burrows_column.filterDisplay();

        current_log_count = get_log_count();
        marked_tree_count = do_chop_designation(false, true);
    }

    void change_min_logs(int delta)
    {
        if (!autochop_enabled)
            return;

        min_logs += delta;
        if (min_logs < 0)
            min_logs = 0;
        if (min_logs > max_logs)
            max_logs = min_logs;
    }

    void change_max_logs(int delta)
    {
        if (!autochop_enabled)
            return;

        max_logs += delta;
        if (max_logs < min_logs)
            min_logs = max_logs;
    }

    void feed(set<df::interface_key> *input)
    {
        bool key_processed = false;
        message.clear();
        switch (selected_column)
        {
        case 0:
            key_processed = burrows_column.feed(input);
            break;
        }

        if (key_processed)
        {
            if (input->count(interface_key::SELECT))
                updateAutochopBurrows();
            return;
        }

        if (input->count(interface_key::LEAVESCREEN))
        {
            save_config();
            input->clear();
            Screen::dismiss(this);
            if (autochop_enabled)
                do_autochop();
            return;
        }
        else if  (input->count(interface_key::CUSTOM_A))
        {
            autochop_enabled = !autochop_enabled;
        }
        else if  (input->count(interface_key::CUSTOM_D))
        {
            int count = do_chop_designation(true, false);
            message = "Trees marked for chop: " + int_to_string(count);
            marked_tree_count = do_chop_designation(false, true);
        }
        else if  (input->count(interface_key::CUSTOM_U))
        {
            int count = do_chop_designation(false, false);
            message = "Trees unmarked: " + int_to_string(count);
            marked_tree_count = do_chop_designation(false, true);
        }
        else if  (input->count(interface_key::CUSTOM_H))
        {
            change_min_logs(-1);
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_H))
        {
            change_min_logs(-10);
        }
        else if  (input->count(interface_key::CUSTOM_J))
        {
            change_min_logs(1);
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_J))
        {
            change_min_logs(10);
        }
        else if  (input->count(interface_key::CUSTOM_K))
        {
            change_max_logs(-1);
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_K))
        {
            change_max_logs(-10);
        }
        else if  (input->count(interface_key::CUSTOM_L))
        {
            change_max_logs(1);
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_L))
        {
            change_max_logs(10);
        }
        else if (enabler->tracking_on && enabler->mouse_lbut)
        {
            if (burrows_column.setHighlightByMouse())
            {
                selected_column = 0;
            }

            enabler->mouse_lbut = enabler->mouse_rbut = 0;
        }
    }

    void render()
    {
        if (Screen::isDismissed(this))
            return;

        dfhack_viewscreen::render();

        Screen::clear();
        Screen::drawBorder("  Autochop  ");

        burrows_column.display(selected_column == 0);

        int32_t y = gps->dimy - 3;
        int32_t x = 2;
        OutputHotkeyString(x, y, "Leave", "Esc");
        x += 3;
        OutputString(COLOR_YELLOW, x, y, message);

        y = 3;
        int32_t left_margin = burrows_column.getMaxItemWidth() + 3;
        x = left_margin;
        if (burrows_column.getSelectedElems().size() > 0)
        {
            OutputString(COLOR_GREEN, x, y, "Will chop in selected burrows", true, left_margin);
        }
        else
        {
            OutputString(COLOR_YELLOW, x, y, "Will chop from whole map", true, left_margin);
            OutputString(COLOR_YELLOW, x, y, "Select from left to chop in specific burrows", true, left_margin);
        }

        ++y;
        OutputToggleString(x, y, "Autochop", "a", autochop_enabled, true, left_margin);
        OutputHotkeyString(x, y, "Designate Now", "d", true, left_margin);
        OutputHotkeyString(x, y, "Undesignate Now", "u", true, left_margin);
        OutputHotkeyString(x, y, "Toggle Burrow", "Enter", true, left_margin);
        if (autochop_enabled)
        {
            OutputLabelString(x, y, "Min Logs", "hjHJ", int_to_string(min_logs), true, left_margin);
            OutputLabelString(x, y, "Max Logs", "klKL", int_to_string(max_logs), true, left_margin);
        }

        ++y;
        OutputString(COLOR_BROWN, x, y, "Current Counts", true, left_margin);
        OutputString(COLOR_WHITE, x, y, "Current Logs: ");
        OutputString(COLOR_GREEN, x, y, int_to_string(current_log_count), true, left_margin);
        OutputString(COLOR_WHITE, x, y, "Marked Trees: ");
        OutputString(COLOR_GREEN, x, y, int_to_string(marked_tree_count), true, left_margin);
    }

    std::string getFocusString() { return "autochop"; }

    void updateAutochopBurrows()
    {
        watchedBurrows.clear();
        vector<df::burrow *> v = burrows_column.getSelectedElems();
        for_each_<df::burrow *>(v, [] (df::burrow *b) { watchedBurrows.add(b->id); });
    }

private:
    ListColumn<df::burrow *> burrows_column;
    int selected_column;
    int current_log_count;
    int marked_tree_count;
    MapExtras::MapCache mcache;
    string message;

    void validateColumn()
    {
        set_to_limit(selected_column, 0);
    }

    void resize(int32_t x, int32_t y)
    {
        dfhack_viewscreen::resize(x, y);
        burrows_column.resize();
    }
};

struct autochop_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    bool isInDesignationMenu()
    {
        using namespace df::enums::ui_sidebar_mode;
        return (ui->main.mode == DesignateChopTrees);
    }

    void sendKey(const df::interface_key &key)
    {
        set<df::interface_key> tmp;
        tmp.insert(key);
        INTERPOSE_NEXT(feed)(&tmp);
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (isInDesignationMenu() && input->count(interface_key::CUSTOM_C))
        {
            sendKey(interface_key::LEAVESCREEN);
            Screen::show(new ViewscreenAutochop());
        }
        else
        {
            INTERPOSE_NEXT(feed)(input);
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        auto dims = Gui::getDwarfmodeViewDims();
        if (dims.menu_x1 <= 0)
            return;

        df::ui_sidebar_mode d = ui->main.mode;
        if (!isInDesignationMenu())
            return;

        int left_margin = dims.menu_x1 + 1;
        int x = left_margin;
        int y = 26;
        OutputHotkeyString(x, y, "Autochop Dashboard", "c");
    }
};

IMPLEMENT_VMETHOD_INTERPOSE_PRIO(autochop_hook, feed, 100);
IMPLEMENT_VMETHOD_INTERPOSE_PRIO(autochop_hook, render, 100);


command_result df_autochop (color_ostream &out, vector <string> & parameters)
{
    for (size_t i = 0; i < parameters.size(); i++)
    {
        if (parameters[i] == "help" || parameters[i] == "?")
            return CR_WRONG_USAGE;
        if (parameters[i] == "debug")
            save_config();
        else
            return CR_WRONG_USAGE;
    }
    if (Maps::IsValid())
        Screen::show(new ViewscreenAutochop());
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate (color_ostream &out)
{
    if (!autochop_enabled)
        return CR_OK;

    if(!Maps::IsValid())
        return CR_OK;

    static decltype(world->frame_counter) last_frame_count = 0;

    if (DFHack::World::ReadPauseState())
        return CR_OK;

    if (world->frame_counter - last_frame_count < 1200) // Check every day
        return CR_OK;

    last_frame_count = world->frame_counter;

    do_autochop();

    return CR_OK;
}

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        if (!INTERPOSE_HOOK(autochop_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(autochop_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
        initialize();
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "autochop", "Allows automatic harvesting of trees based on the number of stockpiled logs",
        df_autochop, false,
        "Opens the automated chopping control screen. Specify 'debug' to forcibly save settings.\n"
    ));

    initialize();
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        initialize();
        break;
    default:
        break;
    }

    return CR_OK;
}
