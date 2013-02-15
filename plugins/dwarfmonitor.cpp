#include "uicommon.h"

#include "DataDefs.h"
#include "MiscUtils.h"

#include "df/job.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"
#include "df/misc_trait_type.h"
#include "df/unit_misc_trait.h"

#include "modules/Gui.h"
#include "modules/Units.h"
#include "modules/Translation.h"
#include "modules/World.h"
#include "modules/Maps.h"

using std::deque;

using df::global::gps;
using df::global::world;
using df::global::ui;

typedef int16_t activity_type;

#define PLUGIN_VERSION 0.2
#define DAY_TICKS 1200
#define DELTA_TICKS 100

int max_history_days = 7;

template <typename T1, typename T2>
struct less_second {
    typedef pair<T1, T2> type;
    bool operator ()(type const& a, type const& b) const {
        return a.second > b.second;
    }
};

static bool monitor_jobs = false;
static bool monitor_misery = false;
static map<df::unit *, deque<activity_type>> work_history;

static int misery[] = { 0, 0, 0, 0, 0, 0, 0 };
static bool misery_upto_date = false;

static int get_max_history()
{
    return (DAY_TICKS / DELTA_TICKS) * max_history_days;
}

static int getPercentage(const int n, const int d)
{
    return static_cast<int>(
        static_cast<float>(n) / static_cast<float>(d) * 100.0);
}

static string getUnitName(df::unit * unit)
{
    string label = "";
    auto name = Units::getVisibleName(unit);
    if (name->has_name)
        label = Translation::TranslateName(name, false);

    return label;
}

static void send_key(const df::interface_key &key)
{
    set< df::interface_key > keys;
    keys.insert(key);
    Gui::getCurViewscreen(true)->feed(&keys);
}

static void move_cursor(df::coord &pos)
{
    Gui::setCursorCoords(pos.x, pos.y, pos.z);
    send_key(interface_key::CURSOR_DOWN_Z);
    send_key(interface_key::CURSOR_UP_Z);
}

static void open_stats_srceen();

#define JOB_UNKNOWN -2
#define JOB_IDLE -1
#define JOB_MILITARY -3
#define JOB_LEISURE -4
#define JOB_OTHER -5
#define JOB_DESIG_SLOPE -6
#define JOB_STORE_ITEM -7
#define JOB_CONSTRUCT_FURNITURE -8
#define JOB_DETAILING -9
#define JOB_HUNTING -10
#define JOB_CLOTING -11
#define JOB_ARMOUR -12
#define JOB_WEAPONS -13
#define JOB_MEDICAL -14

static string getActivityLabel(const activity_type & activity)
{
    string label;
    switch (activity)
    {
    case JOB_IDLE:
        label = "Idle";
        break;
    case JOB_MILITARY:
        label = "Military Duty";
        break;
    case JOB_LEISURE:
        label = "Leisure";
        break;
    case JOB_OTHER:
        label = "Other";
        break;
    case JOB_DESIG_SLOPE:
        label = "Designate Stair/Slope";
        break;
    case JOB_STORE_ITEM:
        label = "Store Item";
        break;
    case JOB_CONSTRUCT_FURNITURE:
        label = "Construct Furniture";
        break;
    case JOB_DETAILING:
        label = "Detailing";
        break;
    case JOB_HUNTING:
        label = "Hunting";
        break;
    case JOB_CLOTING:
        label = "Make Clothing";
        break;
    case JOB_ARMOUR:
        label = "Make Armor";
        break;
    case JOB_WEAPONS:
        label = "Make Weapons";
        break;
    case JOB_MEDICAL:
        label = "Medical";
        break;

    default:
        string raw_label = enum_item_key_str(static_cast<df::job_type>(activity));
        for (auto c = raw_label.begin(); c != raw_label.end(); c++)
        {
            if (label.length() > 0 && *c >= 'A' && *c <= 'Z')
                label += ' ';

            label += *c;
        }

        break;
    }

    return label;
}

class ViewscreenDwarfStats : public dfhack_viewscreen
{
public:
    ViewscreenDwarfStats(df::unit *starting_selection) : selected_column(0)
    {
        dwarves_column.multiselect = false;
        dwarves_column.auto_select = true;
        dwarves_column.setTitle("Dwarves");

        dwarf_activity_column.multiselect = false;
        dwarf_activity_column.auto_select = true;
        dwarf_activity_column.setTitle("Dwarf Activity");

        for (auto it = work_history.begin(); it != work_history.end();)
        {
            auto unit = it->first;
            if (Units::isDead(unit))
            {
                work_history.erase(it++);
                continue;
            }

            deque<activity_type> *work_list = &it->second;
            ++it;

            size_t dwarf_total = 0;
            dwarf_activity_values[unit] =  map<activity_type, size_t>();
            for (auto entry = work_list->begin(); entry != work_list->end(); entry++)
            {
                if (*entry == JOB_UNKNOWN || *entry == job_type::DrinkBlood)
                    continue;

                ++dwarf_total;
                addDwarfActivity(unit, *entry);
            }

            for_each_(dwarf_activity_values[unit],
                [&] (const pair<activity_type, size_t> &x) 
                { dwarf_activity_values[unit][x.first] = getPercentage(x.second,  dwarf_total); } );

            dwarves_column.add(getUnitName(unit), *unit);
        }

        dwarf_activity_column.left_margin = dwarves_column.fixWidth() + 2;
        dwarves_column.filterDisplay();
        dwarves_column.selectItem(starting_selection);
        populateActivityColumn();
    }

    void addDwarfActivity(df::unit *unit, const activity_type &activity)
    {
        if (dwarf_activity_values[unit].find(activity) == dwarf_activity_values[unit].end())
            dwarf_activity_values[unit][activity] = 0;

        dwarf_activity_values[unit][activity]++;
    }

    void populateActivityColumn()
    {
        dwarf_activity_column.clear();
        if (dwarves_column.getDisplayedListSize() == 0)
            return;

        auto unit = dwarves_column.getFirstSelectedElem();
        if (dwarf_activity_values.find(unit) == dwarf_activity_values.end())
            return;

        auto dwarf_activities = &dwarf_activity_values[unit];
        if (dwarf_activities)
        {
            vector<pair<activity_type, size_t>> rev_vec(dwarf_activities->begin(), dwarf_activities->end());
            sort(rev_vec.begin(), rev_vec.end(), less_second<activity_type, size_t>());

            for_each_(rev_vec,
                [&] (pair<activity_type, size_t> x)
            { dwarf_activity_column.add(getActivityItem(x.first, x.second), x.first); });
        }

        dwarf_activity_column.fixWidth();
        dwarf_activity_column.clearSearch();
        dwarf_activity_column.setHighlight(0);
    }

    string getActivityItem(activity_type activity, size_t value)
    {
        return pad_string(int_to_string(value), 3) + " " + getActivityLabel(activity);
    }

    void feed(set<df::interface_key> *input)
    {
        bool key_processed = false;
        switch (selected_column)
        {
        case 0:
            key_processed = dwarves_column.feed(input);
            break;
        case 1:
            key_processed = dwarf_activity_column.feed(input);
            break;
        }

        if (key_processed)
        {
            if (selected_column == 0 && dwarves_column.feed_changed_highlight)
                populateActivityColumn();

            return;
        }

        if (input->count(interface_key::LEAVESCREEN))
        {
            input->clear();
            Screen::dismiss(this);
            return;
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_D))
        {
            Screen::dismiss(this);
            open_stats_srceen();
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_Z))
        {
            df::unit *selected_unit = (selected_column == 0) ? dwarves_column.getFirstSelectedElem() : nullptr;
            if (selected_unit)
            {
                input->clear();
                Screen::dismiss(this);
                Gui::resetDwarfmodeView(true);
                send_key(interface_key::D_VIEWUNIT);
                move_cursor(selected_unit->pos);
            }
        }
        else if  (input->count(interface_key::CURSOR_LEFT))
        {
            --selected_column;
            validateColumn();
        }
        else if  (input->count(interface_key::CURSOR_RIGHT))
        {
            ++selected_column;
            validateColumn();
        }
        else if (enabler->tracking_on && enabler->mouse_lbut)
        {
            if (dwarves_column.setHighlightByMouse())
            {
                selected_column = 0;
                populateActivityColumn();
            }
            else if (dwarf_activity_column.setHighlightByMouse())
                selected_column = 1;

            enabler->mouse_lbut = enabler->mouse_rbut = 0;
        }
    }

    void render()
    {
        if (Screen::isDismissed(this))
            return;

        dfhack_viewscreen::render();

        Screen::clear();
        Screen::drawBorder("  Dwarf Activity  ");

        dwarves_column.display(selected_column == 0);
        dwarf_activity_column.display(selected_column == 1);

        int32_t y = gps->dimy - 3;
        int32_t x = 2;
        OutputHotkeyString(x, y, "Leave", "Esc");
        x += 3;
        OutputHotkeyString(x, y, "Fort Stats", "Shift-D");
        x += 3;
        OutputHotkeyString(x, y, "Zoom Unit", "Shift-Z");
    }

    std::string getFocusString() { return "dwarfmonitor_dwarfstats"; }

private:
    ListColumn<df::unit> dwarves_column;
    ListColumn<activity_type> dwarf_activity_column;
    int selected_column;

    map<df::unit *, map<activity_type, size_t>> dwarf_activity_values;

    void validateColumn()
    {
        set_to_limit(selected_column, 1);
    }

    void resize(int32_t x, int32_t y)
    {
        dfhack_viewscreen::resize(x, y);
        dwarves_column.resize();
        dwarf_activity_column.resize();
    }
};


class ViewscreenFortStats : public dfhack_viewscreen
{
public:
    ViewscreenFortStats() : selected_column(0), fort_activity_count(0)
    {
        fort_activity_column.multiselect = false;
        fort_activity_column.auto_select = true;
        fort_activity_column.setTitle("Fort Activities");

        dwarf_activity_column.multiselect = false;
        dwarf_activity_column.auto_select = true;
        dwarf_activity_column.setTitle("Units on Activity");

        for (auto it = work_history.begin(); it != work_history.end();)
        {
            auto unit = it->first;
            if (Units::isDead(unit))
            {
                work_history.erase(it++);
                continue;
            }

            deque<activity_type> *work_list = &it->second;
            ++it;

            for (auto entry = work_list->begin(); entry != work_list->end(); entry++)
            {
                if (*entry == JOB_UNKNOWN)
                    continue;

                ++fort_activity_count;

                auto real_activity = *entry;
                if (real_activity < 0)
                {
                    addFortActivity(real_activity);
                }
                else
                {
                    auto activity = static_cast<df::job_type>(real_activity);

                    switch (activity)
                    {
                    case job_type::Eat:
                    case job_type::Drink:
                    case job_type::Drink2:
                    case job_type::Sleep:
                    case job_type::AttendParty:
                    case job_type::Rest:
                    case job_type::CleanSelf:
                    case job_type::DrinkBlood:
                        real_activity = JOB_LEISURE;
                        break;

                    case job_type::Kidnap:
                    case job_type::StartingFistFight:
                    case job_type::SeekInfant:
                    case job_type::SeekArtifact:
                    case job_type::GoShopping:
                    case job_type::GoShopping2:
                    case job_type::RecoverPet:
                    case job_type::CauseTrouble:
                    case job_type::ReportCrime:
                    case job_type::BeatCriminal:
                        real_activity = JOB_OTHER;
                        break;

                    case job_type::CarveUpwardStaircase:
                    case job_type::CarveDownwardStaircase:
                    case job_type::CarveUpDownStaircase:
                    case job_type::CarveRamp:
                    case job_type::DigChannel:
                        real_activity = JOB_DESIG_SLOPE;
                        break;

                    case job_type::StoreOwnedItem:
                    case job_type::PlaceItemInTomb:
                    case job_type::StoreItemInStockpile:
                    case job_type::StoreItemInBag:
                    case job_type::StoreItemInHospital:
                    case job_type::StoreItemInChest:
                    case job_type::StoreItemInCabinet:
                    case job_type::StoreWeapon:
                    case job_type::StoreArmor:
                    case job_type::StoreItemInBarrel:
                    case job_type::StoreItemInBin:
                        real_activity = JOB_STORE_ITEM;
                        break;

                    case job_type::ConstructDoor:
                    case job_type::ConstructFloodgate:
                    case job_type::ConstructBed:
                    case job_type::ConstructThrone:
                    case job_type::ConstructCoffin:
                    case job_type::ConstructTable:
                    case job_type::ConstructChest:
                    case job_type::ConstructBin:
                    case job_type::ConstructArmorStand:
                    case job_type::ConstructWeaponRack:
                    case job_type::ConstructCabinet:
                    case job_type::ConstructStatue:
                        real_activity = JOB_CONSTRUCT_FURNITURE;
                        break;

                    case job_type::DetailFloor:
                    case job_type::DetailWall:
                        real_activity = JOB_DETAILING;
                        break;

                    case job_type::Hunt:
                    case job_type::ReturnKill:
                    case job_type::HuntVermin:
                        real_activity = JOB_HUNTING;
                        break;

                    default:
                        break;
                    }

                    addFortActivity(real_activity);
                }

                if (dwarf_activity_values.find(real_activity) == dwarf_activity_values.end())
                    dwarf_activity_values[real_activity] = map<df::unit *, size_t>();

                map<df::unit *, size_t> &activity_for_dwarf = dwarf_activity_values[real_activity];
                if (activity_for_dwarf.find(unit) == activity_for_dwarf.end())
                    activity_for_dwarf[unit] = 0;

                ++activity_for_dwarf[unit];
            }
        }

        vector<pair<activity_type, size_t>> rev_vec(fort_activity_totals.begin(), fort_activity_totals.end());
        sort(rev_vec.begin(), rev_vec.end(), less_second<activity_type, size_t>());
        
        for (auto rev_it = rev_vec.begin(); rev_it != rev_vec.end(); rev_it++)
        {
            auto activity = rev_it->first;
            addToFortAverageColumn(activity);

            for (auto it = dwarf_activity_values[activity].begin(); it != dwarf_activity_values[activity].end(); it++)
            {
                auto avg = getPercentage(it->second, getFortActivityCount(activity));
                dwarf_activity_values[activity][it->first] = avg;
            }
        }

        dwarf_activity_column.left_margin = fort_activity_column.fixWidth() + 2;
        fort_activity_column.filterDisplay();
        populateDwarfColumn();
    }

    void addToFortAverageColumn(activity_type type)
    {
        if (getFortActivityCount(type))
            fort_activity_column.add(getFortAverage(type), dwarf_activity_values[type]);
    }

    string getFortAverage(const activity_type &activity)
    {
        auto average = getPercentage(getFortActivityCount(activity), fort_activity_count);
        auto label = getActivityLabel(activity);
        auto result = pad_string(int_to_string(average), 3) + " " + label;

        return result;
    }

    string getDwarfAverage(df::unit *unit, const size_t value)
    {
        auto label = getUnitName(unit);
        auto result = pad_string(int_to_string(value), 3) + " " + label;

        return result;
    }

    size_t getFortActivityCount(const activity_type &activity)
    {
        if (fort_activity_totals.find(activity) == fort_activity_totals.end())
            return 0;

        return fort_activity_totals[activity];
    }

    void addFortActivity(const activity_type &activity)
    {
        if (fort_activity_totals.find(activity) == fort_activity_totals.end())
            fort_activity_totals[activity] = 0;

        fort_activity_totals[activity]++;
    }

    void populateDwarfColumn()
    {
        dwarf_activity_column.clear();
        if (fort_activity_column.getDisplayListSize() == 0)
            return;

        auto dwarf_activities = fort_activity_column.getFirstSelectedElem();
        if (dwarf_activities)
        {
            vector<pair<df::unit *, size_t>> rev_vec(dwarf_activities->begin(), dwarf_activities->end());
            sort(rev_vec.begin(), rev_vec.end(), less_second<df::unit *, size_t>());

            for_each_(rev_vec,
                [&] (pair<df::unit *, size_t> x)
            { dwarf_activity_column.add(getDwarfAverage(x.first, x.second), *x.first); });
        }

        dwarf_activity_column.fixWidth();
        dwarf_activity_column.clearSearch();
        dwarf_activity_column.setHighlight(0);
    }

    void feed(set<df::interface_key> *input)
    {
        bool key_processed = false;
        switch (selected_column)
        {
        case 0:
            key_processed = fort_activity_column.feed(input);
            break;
        case 1:
            key_processed = dwarf_activity_column.feed(input);
            break;
        }

        if (key_processed)
        {
            if (selected_column == 0 && fort_activity_column.feed_changed_highlight)
                populateDwarfColumn();
            
            return;
        }

        if (input->count(interface_key::LEAVESCREEN))
        {
            input->clear();
            Screen::dismiss(this);
            return;
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_D))
        {
            df::unit *selected_unit = (selected_column == 1) ? dwarf_activity_column.getFirstSelectedElem() : nullptr;
            Screen::dismiss(this);
            Screen::show(new ViewscreenDwarfStats(selected_unit));
        }
        else if  (input->count(interface_key::CUSTOM_SHIFT_Z))
        {
            df::unit *selected_unit = (selected_column == 1) ? dwarf_activity_column.getFirstSelectedElem() : nullptr;
            if (selected_unit)
            {
                input->clear();
                Screen::dismiss(this);
                Gui::resetDwarfmodeView(true);
                send_key(interface_key::D_VIEWUNIT);
                move_cursor(selected_unit->pos);
            }
        }
        else if  (input->count(interface_key::CURSOR_LEFT))
        {
            --selected_column;
            validateColumn();
        }
        else if  (input->count(interface_key::CURSOR_RIGHT))
        {
            ++selected_column;
            validateColumn();
        }
        else if (enabler->tracking_on && enabler->mouse_lbut)
        {
            if (fort_activity_column.setHighlightByMouse())
            {
                selected_column = 0;
                populateDwarfColumn();
            }
            else if (dwarf_activity_column.setHighlightByMouse())
                selected_column = 1;

            enabler->mouse_lbut = enabler->mouse_rbut = 0;
        }
    }

    void render()
    {
        if (Screen::isDismissed(this))
            return;

        dfhack_viewscreen::render();

        Screen::clear();
        Screen::drawBorder("  Fortress Efficiency  ");

        fort_activity_column.display(selected_column == 0);
        dwarf_activity_column.display(selected_column == 1);

        int32_t y = gps->dimy - 3;
        int32_t x = 2;
        OutputHotkeyString(x, y, "Leave", "Esc");
        x += 3;
        OutputHotkeyString(x, y, "Dwarf Stats", "Shift-D");
        x += 3;
        OutputHotkeyString(x, y, "Zoom Unit", "Shift-Z");
    }

    std::string getFocusString() { return "dwarfmonitor_fortstats"; }

private:
    ListColumn<map<df::unit *, size_t>> fort_activity_column;
    ListColumn<df::unit> dwarf_activity_column;
    int selected_column;

    map<activity_type, size_t> fort_activity_totals;
    map<activity_type, map<df::unit *, size_t>> dwarf_activity_values;
    size_t fort_activity_count;

    void validateColumn()
    {
        set_to_limit(selected_column, 1);
    }

    void resize(int32_t x, int32_t y)
    {
        dfhack_viewscreen::resize(x, y);
        fort_activity_column.resize();
        dwarf_activity_column.resize();
    }
};

static void open_stats_srceen()
{
    Screen::show(new ViewscreenFortStats());
}

static void add_work_history(df::unit *unit, activity_type type)
{
    if (work_history.find(unit) == work_history.end())
    {
        auto max_history = get_max_history();
        for (int i = 0; i < max_history; i++)
            work_history[unit].push_back(JOB_UNKNOWN);
    }

    work_history[unit].push_back(type);
    work_history[unit].pop_front();
}

static bool is_at_leisure(df::unit *unit)
{
    for (auto p = unit->status.misc_traits.begin(); p < unit->status.misc_traits.end(); p++)
    {
        if ((*p)->id == misc_trait_type::Migrant || (*p)->id == misc_trait_type::OnBreak)
            return true;
    }

    return false;
}

static void reset()
{
    work_history.clear();

    for (int i = 0; i < 7; i++)
        misery[i] = 0;

    misery_upto_date = false;
}

static void update_dwarf_stats(bool is_paused)
{
    if (monitor_misery)
    {
        for (int i = 0; i < 7; i++)
            misery[i] = 0;
    }

    for (auto iter = world->units.active.begin(); iter != world->units.active.end(); iter++)
    {
        df::unit* unit = *iter;
        if (!Units::isCitizen(unit))
            continue;

        if (unit->profession == profession::BABY ||
            unit->profession == profession::CHILD ||
            unit->profession == profession::DRUNK)
        {
            continue;
        }

        using namespace DFHack::Units;
        if (!isSane(unit) || isDead(unit))
        {
            auto it = work_history.find(unit);
            if (it != work_history.end())
                work_history.erase(it);

            continue;
        }

        if (monitor_misery)
        {
            int happy = unit->status.happiness;
            if (happy == 0)         // miserable
                misery[0]++;
            else if (happy <= 25)   // very unhappy
                misery[1]++;
            else if (happy <= 50)   // unhappy
                misery[2]++;
            else if (happy <= 75)    // fine
                misery[3]++;
            else if (happy <= 125)   // quite content
                misery[4]++;
            else if (happy <= 150)   // happy
                misery[5]++;
            else                    // ecstatic
                misery[6]++;
        }

        if (!monitor_jobs || is_paused)
            continue;

        if (ENUM_ATTR(profession, military, unit->profession))
            add_work_history(unit, JOB_MILITARY);

        if (!unit->job.current_job)
        {
            add_work_history(unit, JOB_IDLE);
            continue;
        }

        if (is_at_leisure(unit))
        {
            add_work_history(unit, JOB_LEISURE);
            continue;
        }
         
        add_work_history(unit, unit->job.current_job->job_type);
    }
}

DFhackCExport command_result plugin_onupdate (color_ostream &out)
{
    if (!monitor_jobs && !monitor_misery)
        return CR_OK;

    if(!Maps::IsValid())
        return CR_OK;

    static decltype(world->frame_counter) last_frame_count = 0;
    
    bool is_paused = Core::getInstance().getWorld()->ReadPauseState();
    if (is_paused)
    {
        if (monitor_misery && !misery_upto_date)
            misery_upto_date = true;
        else
            return CR_OK;
    }
    else
    {
        if (world->frame_counter - last_frame_count < DELTA_TICKS)
            return CR_OK;

        last_frame_count = world->frame_counter;
    }

    update_dwarf_stats(is_paused);

    return CR_OK;
}

struct dwarf_monitor_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        if (monitor_misery && Maps::IsValid())
        {
            int x = 1;
            int y = gps->dimy - 1;
            OutputString(COLOR_WHITE, x, y, "H:");
            OutputString(COLOR_LIGHTRED, x, y, int_to_string(misery[0]));
            OutputString(COLOR_WHITE, x, y, "/");
            OutputString(COLOR_RED, x, y, int_to_string(misery[1]));
            OutputString(COLOR_WHITE, x, y, "/");
            OutputString(COLOR_YELLOW, x, y, int_to_string(misery[2]));
            OutputString(COLOR_WHITE, x, y, "/");
            OutputString(COLOR_WHITE, x, y, int_to_string(misery[3]));
            OutputString(COLOR_WHITE, x, y, "/");
            OutputString(COLOR_CYAN, x, y, int_to_string(misery[4]));
            OutputString(COLOR_WHITE, x, y, "/");
            OutputString(COLOR_LIGHTBLUE, x, y, int_to_string(misery[5]));
            OutputString(COLOR_WHITE, x, y, "/");
            OutputString(COLOR_LIGHTGREEN, x, y, int_to_string(misery[6]));
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(dwarf_monitor_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(dwarf_monitor_hook, render);

DFHACK_PLUGIN("dwarfmonitor");

static bool set_monitoring_mode(const string &mode, const bool &state)
{
    bool mode_recognized = false;

    if (mode == "work" || mode == "all")
    {
        mode_recognized = true;
        monitor_jobs = state;
        if (!monitor_jobs)
            reset();
    }

    if (mode == "misery" || mode == "all")
    {
        mode_recognized = true;
        monitor_misery = state;
    }

    return mode_recognized;
}

static command_result dwarfmonitor_cmd(color_ostream &out, vector <string> & parameters)
{
    bool show_help = false;
    if (parameters.empty())
    {
        show_help = true;
    }
    else
    {
        auto cmd = parameters[0][0];
        string mode;

        if (parameters.size() > 1)
            mode = toLower(parameters[1]);

        if (cmd == 'v' || cmd == 'V')
        {
            out << "DwarfMonitor" << endl << "Version: " << PLUGIN_VERSION << endl;
        }
        else if ((cmd == 'e' || cmd == 'E') && !mode.empty())
        {
            if (set_monitoring_mode(mode, true))
            {
                out << "Monitoring enabled: " << mode << endl;
                if (parameters.size() > 2)
                {
                    int new_window = atoi(parameters[2].c_str());
                    if (new_window > 0)
                        max_history_days = new_window;
                }
            }
            else
            {
                show_help = true;
            }
        }
        else if ((cmd == 'd' || cmd == 'D') && !mode.empty())
        {
            if (set_monitoring_mode(mode, false))
                out << "Monitoring disabled: " << mode << endl;
            else
                show_help = true;
        }
        else if (cmd == 's' || cmd == 'S')
        {
            if(Maps::IsValid())
                Screen::show(new ViewscreenFortStats());
        }
        else
        {
            show_help = true;
        }
    }

    if (show_help)
        return CR_WRONG_USAGE;

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (!gps || !INTERPOSE_HOOK(dwarf_monitor_hook, feed).apply() || !INTERPOSE_HOOK(dwarf_monitor_hook, render).apply())
        out.printerr("Could not insert dwarfmonitor hooks!\n");

    commands.push_back(
        PluginCommand(
        "dwarfmonitor", "Records dwarf activity to measure fort efficiency",
        dwarfmonitor_cmd, false, 
        "dwarfmonitor enable <mode>\n"
        "  Start monitoring <mode> [days]\n"
        "    <mode> can be \"work\", \"misery\", or \"all\"\n"
        "    [days] number of days to track (default is 7)\n\n"
        "dwarfmonitor disable <mode>\n"
        "    <mode> as above\n\n"
        "dwarfmonitor stats\n"
        "  Show statistics summary\n\n"
        ));

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        reset();
        break;
    default:
        break;
    }
    return CR_OK;
}
