// (un)designate matching plants for gathering/cutting

#include "uicommon.h"

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

#include "modules/Screen.h"
#include "modules/Maps.h"
#include "modules/Burrows.h"

#include <set>
#include "df/item_flags.h"
#include "df/item.h"

using std::string;
using std::vector;
using std::set;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;

static bool autochop_enabled = false;
static vector<df::burrow *> burrows;
static int min_logs, max_logs;
static bool wait_for_threshold;

static void initialize()
{
    burrows.clear();
    autochop_enabled = false;
    min_logs = 80;
    max_logs = 100;
    wait_for_threshold = false;
}

static bool is_in_burrow(const df::coord &plant_pos)
{
    if (!burrows.size())
        return true;

    for (auto it = burrows.begin(); it != burrows.end(); it++)
    {
        df::burrow *burrow = *it;
        if (Burrows::isAssignedTile(burrow, plant_pos))
            return true;
    }

    return false;
}

static int do_chop_designation()
{
    int count = 0;
    for (size_t i = 0; i < world->map.map_blocks.size(); i++)
    {
        df::map_block *cur = world->map.map_blocks[i];
        for (size_t j = 0; j < cur->plants.size(); j++)
        {
            const df::plant *plant = cur->plants[j];
            int x = plant->pos.x % 16;
            int y = plant->pos.y % 16;

            if (plant->flags.bits.is_shrub)
                continue;
            if (cur->designation[x][y].bits.hidden)
                continue;

            df::tiletype_shape shape = tileShape(cur->tiletype[x][y]);
            if (shape != tiletype_shape::TREE)
                continue;

            if (!is_in_burrow(plant->pos))
                continue;

            if (cur->designation[x][y].bits.dig == tile_dig_designation::No)
            {
                cur->designation[x][y].bits.dig = tile_dig_designation::Default;
                cur->flags.bits.designated = true;
                ++count;
            }
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

static bool should_chop_more()
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
        if (wait_for_threshold)
        {
            if (valid_count >= min_logs)
                return false;
        }
        else
        {
            if (valid_count >= max_logs)
                return false;
        }
    }

    return true;
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
            elem.selected = (find(burrows.begin(), burrows.end(), burrow) != burrows.end());
            burrows_column.add(elem);
        }

        burrows_column.fixWidth();
        burrows_column.filterDisplay();
    }

    void feed(set<df::interface_key> *input)
    {
        bool key_processed = false;
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
            input->clear();
            Screen::dismiss(this);
            return;
        }
        else if  (input->count(interface_key::CUSTOM_A))
        {
            autochop_enabled = !autochop_enabled;
        }
        else if  (input->count(interface_key::CUSTOM_D))
        {
            int count = do_chop_designation();
            color_ostream_proxy out(Core::getInstance().getConsole());
            out << "Trees marked for chop: " << count << endl;
        }
        else if  (input->count(interface_key::CUSTOM_H))
        {
            --min_logs;
            if (min_logs < 0)
                min_logs = 0;
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_H))
        {
            min_logs -= 10;
            if (min_logs < 0)
                min_logs = 0;
        }
        else if  (input->count(interface_key::CUSTOM_J))
        {
            ++min_logs;
            if (min_logs > max_logs)
                max_logs = min_logs;
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_J))
        {
            min_logs += 10;
            if (min_logs > max_logs)
                max_logs = min_logs;
        }
        else if  (input->count(interface_key::CUSTOM_K))
        {
            --max_logs;
            if (max_logs < min_logs)
                min_logs = max_logs;
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_K))
        {
            max_logs -= 10;
            if (max_logs < min_logs)
                min_logs = max_logs;
        }
        else if  (input->count(interface_key::CUSTOM_L))
        {
            ++max_logs;
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_L))
        {
            max_logs += 10;
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

        y++;
        OutputToggleString(x, y, "Autochop", "a", autochop_enabled, true, left_margin);
        OutputHotkeyString(x, y, "Designate Now", "d", true, left_margin);
        if (!autochop_enabled)
            return;
        OutputLabelString(x, y, "Min Logs", "hjHJ", int_to_string(min_logs), true, left_margin);
        OutputLabelString(x, y, "Max Logs", "klKL", int_to_string(max_logs), true, left_margin);
    }

    std::string getFocusString() { return "autochop"; }


    void updateAutochopBurrows()
    {
        burrows.clear();
        for_each_(burrows_column.getSelectedElems(), 
            [] (df::burrow *b) { burrows.push_back(b); });
    }

private:
    ListColumn<df::burrow *> burrows_column;
    int selected_column;

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


command_result df_getplants (color_ostream &out, vector <string> & parameters)
{
    string plantMatStr = "";
    set<int> plantIDs;
    set<string> plantNames;
    bool deselect = false, exclude = false, treesonly = false, shrubsonly = false, all = false;

    int count = 0;
    for (size_t i = 0; i < parameters.size(); i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
            return CR_WRONG_USAGE;
        else if(parameters[i] == "-t")
            treesonly = true;
        else if(parameters[i] == "-s")
            shrubsonly = true;
        else if(parameters[i] == "-c")
            deselect = true;
        else if(parameters[i] == "-x")
            exclude = true;
        else if(parameters[i] == "-a")
            all = true;
        else if(parameters[i] == "autochop")
        {
            if(Maps::IsValid())
                Screen::show(new ViewscreenAutochop());

            return CR_OK;
        }
        else
            plantNames.insert(parameters[i]);
    }
    if (treesonly && shrubsonly)
    {
        out.printerr("Cannot specify both -t and -s at the same time!\n");
        return CR_WRONG_USAGE;
    }
    if (all && exclude)
    {
        out.printerr("Cannot specify both -a and -x at the same time!\n");
        return CR_WRONG_USAGE;
    }
    if (all && plantNames.size())
    {
        out.printerr("Cannot specify -a along with plant IDs!\n");
        return CR_WRONG_USAGE;
    }

    CoreSuspender suspend;

    for (size_t i = 0; i < world->raws.plants.all.size(); i++)
    {
        df::plant_raw *plant = world->raws.plants.all[i];
        if (all)
            plantIDs.insert(i);
        else if (plantNames.find(plant->id) != plantNames.end())
        {
            plantNames.erase(plant->id);
            plantIDs.insert(i);
        }
    }
    if (plantNames.size() > 0)
    {
        out.printerr("Invalid plant ID(s):");
        for (set<string>::const_iterator it = plantNames.begin(); it != plantNames.end(); it++)
            out.printerr(" %s", it->c_str());
        out.printerr("\n");
        return CR_FAILURE;
    }

    if (plantIDs.size() == 0)
    {
        out.print("Valid plant IDs:\n");
        for (size_t i = 0; i < world->raws.plants.all.size(); i++)
        {
            df::plant_raw *plant = world->raws.plants.all[i];
            if (plant->flags.is_set(plant_raw_flags::GRASS))
                continue;
            out.print("* (%s) %s - %s\n", plant->flags.is_set(plant_raw_flags::TREE) ? "tree" : "shrub", plant->id.c_str(), plant->name.c_str());
        }
        return CR_OK;
    }

    count = 0;
    for (size_t i = 0; i < world->map.map_blocks.size(); i++)
    {
        df::map_block *cur = world->map.map_blocks[i];
        bool dirty = false;
        for (size_t j = 0; j < cur->plants.size(); j++)
        {
            const df::plant *plant = cur->plants[j];
            int x = plant->pos.x % 16;
            int y = plant->pos.y % 16;
            if (plantIDs.find(plant->material) != plantIDs.end())
            {
                if (exclude)
                    continue;
            }
            else
            {
                if (!exclude)
                    continue;
            }
            df::tiletype_shape shape = tileShape(cur->tiletype[x][y]);
            df::tiletype_special special = tileSpecial(cur->tiletype[x][y]);
            if (plant->flags.bits.is_shrub && (treesonly || !(shape == tiletype_shape::SHRUB && special != tiletype_special::DEAD)))
                continue;
            if (!plant->flags.bits.is_shrub && (shrubsonly || !(shape == tiletype_shape::TREE)))
                continue;
            if (cur->designation[x][y].bits.hidden)
                continue;
            if (deselect && cur->designation[x][y].bits.dig == tile_dig_designation::Default)
            {
                cur->designation[x][y].bits.dig = tile_dig_designation::No;
                dirty = true;
                ++count;
            }
            if (!deselect && cur->designation[x][y].bits.dig == tile_dig_designation::No)
            {
                cur->designation[x][y].bits.dig = tile_dig_designation::Default;
                dirty = true;
                ++count;
            }
        }
        if (dirty)
            cur->flags.bits.designated = true;
    }
    if (count)
        out.print("Updated %d plant designations.\n", count);
    return CR_OK;
}

DFHACK_PLUGIN("getplants");

DFhackCExport command_result plugin_onupdate (color_ostream &out)
{
    if (!autochop_enabled)
        return CR_OK;

    if(!Maps::IsValid())
        return CR_OK;

    static decltype(world->frame_counter) last_frame_count = 0;

    if (DFHack::World::ReadPauseState())
        return CR_OK;

    if (world->frame_counter - last_frame_count < 6000) // Check ever 5 days
        return CR_OK;

    last_frame_count = world->frame_counter;

    do_chop_designation();

    return CR_OK;
}


DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "getplants", "Cut down all of the specified trees or gather specified shrubs",
        df_getplants, false,
        "  Specify the types of trees to cut down and/or shrubs to gather by their\n"
        "  plant IDs, separated by spaces.\n"
        "Options:\n"
        "  -t - Select trees only (exclude shrubs)\n"
        "  -s - Select shrubs only (exclude trees)\n"
        "  -c - Clear designations instead of setting them\n"
        "  -x - Apply selected action to all plants except those specified\n"
        "  -a - Select every type of plant (obeys -t/-s)\n"
        "Specifying both -t and -s will have no effect.\n"
        "If no plant IDs are specified, all valid plant IDs will be listed.\n\n"
        "  autochop - Opens the automated tree cutting control screen\n"
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
