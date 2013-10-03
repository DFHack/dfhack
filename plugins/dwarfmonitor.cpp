#include "uicommon.h"

#include "DataDefs.h"

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
#include "df/activity_event.h"
#include "df/activity_entry.h"

using std::deque;

using df::global::world;
using df::global::ui;

typedef int16_t activity_type;

#define PLUGIN_VERSION 0.5
#define DAY_TICKS 1200
#define DELTA_TICKS 100

const int min_window = 28;
const int max_history_days = 3 * min_window;
const int ticks_per_day = DAY_TICKS / DELTA_TICKS;

template <typename T1, typename T2>
struct less_second {
    typedef pair<T1, T2> type;
    bool operator ()(type const& a, type const& b) const {
        return a.second > b.second;
    }
};

static bool monitor_jobs = false;
static bool monitor_misery = true;
static map<df::unit *, deque<activity_type>> work_history;

static int misery[] = { 0, 0, 0, 0, 0, 0, 0 };
static bool misery_upto_date = false;

static int get_max_history()
{
    return ticks_per_day * max_history_days;
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

#define JOB_IDLE -1
#define JOB_UNKNOWN -2
#define JOB_MILITARY -3
#define JOB_LEISURE -4
#define JOB_UNPRODUCTIVE -5
#define JOB_DESIGNATE -6
#define JOB_STORE_ITEM -7
#define JOB_MANUFACTURE -8
#define JOB_DETAILING -9
#define JOB_HUNTING -10
#define JOB_MEDICAL -14
#define JOB_COLLECT -15
#define JOB_CONSTRUCTION -16
#define JOB_AGRICULTURE -17
#define JOB_FOOD_PROD -18
#define JOB_MECHANICAL -19
#define JOB_ANIMALS -20
#define JOB_PRODUCTIVE -21

static map<activity_type, string> activity_labels;

static string getActivityLabel(const activity_type activity)
{
    string label;

    if (activity_labels.find(activity) != activity_labels.end())
    {
        label = activity_labels[activity];
    }
    else
    {
        string raw_label = enum_item_key_str(static_cast<df::job_type>(activity));
        for (auto c = raw_label.begin(); c != raw_label.end(); c++)
        {
            if (label.length() > 0 && *c >= 'A' && *c <= 'Z')
                label += ' ';

            label += *c;
        }
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

        window_days = min_window;

        populateDwarfColumn(starting_selection);
    }

    void populateDwarfColumn(df::unit *starting_selection = NULL)
    {
        selected_column = 0;

        auto last_selected_index = dwarf_activity_column.highlighted_index;
        dwarves_column.clear();
        dwarf_activity_values.clear();

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
            size_t count = window_days * ticks_per_day;
            for (auto entry = work_list->rbegin(); entry != work_list->rend() && count > 0; entry++, count--)
            {
                if (*entry == JOB_UNKNOWN || *entry == job_type::DrinkBlood)
                    continue;

                ++dwarf_total;
                addDwarfActivity(unit, *entry);
            }

            auto &values = dwarf_activity_values[unit];
            for (auto it = values.begin(); it != values.end(); ++it)
                it->second = getPercentage(it->second, dwarf_total);

            dwarves_column.add(getUnitName(unit), unit);
        }

        dwarf_activity_column.left_margin = dwarves_column.fixWidth() + 2;
        dwarves_column.filterDisplay();
        if (starting_selection)
            dwarves_column.selectItem(starting_selection);
        else
            dwarves_column.setHighlight(last_selected_index);

        populateActivityColumn();
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

            for (auto it = rev_vec.begin(); it != rev_vec.end(); ++it)
                dwarf_activity_column.add(getActivityItem(it->first, it->second), it->first);
        }

        dwarf_activity_column.fixWidth();
        dwarf_activity_column.clearSearch();
        dwarf_activity_column.setHighlight(0);
    }

    void addDwarfActivity(df::unit *unit, const activity_type &activity)
    {
        if (dwarf_activity_values[unit].find(activity) == dwarf_activity_values[unit].end())
            dwarf_activity_values[unit][activity] = 0;

        dwarf_activity_values[unit][activity]++;
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
        else if  (input->count(interface_key::SECONDSCROLL_PAGEDOWN))
        {
            window_days += min_window;
            if (window_days > max_history_days)
                window_days = min_window;

            populateDwarfColumn();
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

        int32_t y = gps->dimy - 4;
        int32_t x = 2;
        OutputHotkeyString(x, y, "Leave", "Esc");

        x += 13;
        string window_label = "Window Months: " + int_to_string(window_days / min_window);
        OutputHotkeyString(x, y, window_label.c_str(), "*");

        ++y;
        x = 2;
        OutputHotkeyString(x, y, "Fort Stats", "Shift-D");

        x += 3;
        OutputHotkeyString(x, y, "Zoom Unit", "Shift-Z");
    }

    std::string getFocusString() { return "dwarfmonitor_dwarfstats"; }

private:
    ListColumn<df::unit *> dwarves_column;
    ListColumn<activity_type> dwarf_activity_column;
    int selected_column;
    size_t window_days;

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
    ViewscreenFortStats()
    {
        fort_activity_column.multiselect = false;
        fort_activity_column.auto_select = true;
        fort_activity_column.setTitle("Fort Activities");
        fort_activity_column.bottom_margin = 4;

        dwarf_activity_column.multiselect = false;
        dwarf_activity_column.auto_select = true;
        dwarf_activity_column.setTitle("Units on Activity");
        dwarf_activity_column.bottom_margin = 4;
        dwarf_activity_column.text_clip_at = 25;

        category_breakdown_column.setTitle("Category Breakdown");
        category_breakdown_column.bottom_margin = 4;

        window_days = min_window;

        populateFortColumn();
    }

    void populateFortColumn()
    {
        selected_column = 0;
        fort_activity_count = 0;

        auto last_selected_index = fort_activity_column.highlighted_index;
        fort_activity_column.clear();
        fort_activity_totals.clear();
        dwarf_activity_values.clear();
        category_breakdown.clear();

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

            size_t count = window_days * ticks_per_day;
            for (auto entry = work_list->rbegin(); entry != work_list->rend() && count > 0; entry++, count--)
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
                    case job_type::ExecuteCriminal:
                        real_activity = JOB_UNPRODUCTIVE;
                        break;

                    case job_type::CarveUpwardStaircase:
                    case job_type::CarveDownwardStaircase:
                    case job_type::CarveUpDownStaircase:
                    case job_type::CarveRamp:
                    case job_type::DigChannel:
                    case job_type::Dig:
                    case job_type::CarveTrack:
                    case job_type::CarveFortification:
                        real_activity = JOB_DESIGNATE;
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
                    case job_type::BringItemToDepot:
                    case job_type::BringItemToShop:
                    case job_type::GetProvisions:
                    case job_type::FillWaterskin:
                    case job_type::FillWaterskin2:
                    case job_type::CheckChest:
                    case job_type::PickupEquipment:
                    case job_type::DumpItem:
                    case job_type::PushTrackVehicle:
                    case job_type::PlaceTrackVehicle:
                    case job_type::StoreItemInVehicle:
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
                    case job_type::ConstructBlocks:
                    case job_type::MakeRawGlass:
                    case job_type::MakeCrafts:
                    case job_type::MintCoins:
                    case job_type::CutGems:
                    case job_type::CutGlass:
                    case job_type::EncrustWithGems:
                    case job_type::EncrustWithGlass:
                    case job_type::SmeltOre:
                    case job_type::MeltMetalObject:
                    case job_type::ExtractMetalStrands:
                    case job_type::MakeWeapon:
                    case job_type::ForgeAnvil:
                    case job_type::ConstructCatapultParts:
                    case job_type::ConstructBallistaParts:
                    case job_type::MakeArmor:
                    case job_type::MakeHelm:
                    case job_type::MakePants:
                    case job_type::StudWith:
                    case job_type::ProcessPlantsBag:
                    case job_type::ProcessPlantsVial:
                    case job_type::ProcessPlantsBarrel:
                    case job_type::WeaveCloth:
                    case job_type::MakeGloves:
                    case job_type::MakeShoes:
                    case job_type::MakeShield:
                    case job_type::MakeCage:
                    case job_type::MakeChain:
                    case job_type::MakeFlask:
                    case job_type::MakeGoblet:
                    case job_type::MakeInstrument:
                    case job_type::MakeToy:
                    case job_type::MakeAnimalTrap:
                    case job_type::MakeBarrel:
                    case job_type::MakeBucket:
                    case job_type::MakeWindow:
                    case job_type::MakeTotem:
                    case job_type::MakeAmmo:
                    case job_type::DecorateWith:
                    case job_type::MakeBackpack:
                    case job_type::MakeQuiver:
                    case job_type::MakeBallistaArrowHead:
                    case job_type::AssembleSiegeAmmo:
                    case job_type::ConstructMechanisms:
                    case job_type::MakeTrapComponent:
                    case job_type::ExtractFromPlants:
                    case job_type::ExtractFromRawFish:
                    case job_type::ExtractFromLandAnimal:
                    case job_type::MakeCharcoal:
                    case job_type::MakeAsh:
                    case job_type::MakeLye:
                    case job_type::MakePotashFromLye:
                    case job_type::MakePotashFromAsh:
                    case job_type::DyeThread:
                    case job_type::DyeCloth:
                    case job_type::SewImage:
                    case job_type::MakePipeSection:
                    case job_type::ConstructHatchCover:
                    case job_type::ConstructGrate:
                    case job_type::ConstructQuern:
                    case job_type::ConstructMillstone:
                    case job_type::ConstructSplint:
                    case job_type::ConstructCrutch:
                    case job_type::ConstructTractionBench:
                    case job_type::CustomReaction:
                    case job_type::ConstructSlab:
                    case job_type::EngraveSlab:
                    case job_type::SpinThread:
                    case job_type::MakeTool:
                        real_activity = JOB_MANUFACTURE;
                        break;

                    case job_type::DetailFloor:
                    case job_type::DetailWall:
                        real_activity = JOB_DETAILING;
                        break;

                    case job_type::Hunt:
                    case job_type::ReturnKill:
                    case job_type::HuntVermin:
                    case job_type::GatherPlants:
                    case job_type::Fish:
                    case job_type::CatchLiveFish:
                    case job_type::BaitTrap:
                    case job_type::InstallColonyInHive:
                        real_activity = JOB_HUNTING;
                        break;

                    case job_type::RemoveConstruction:
                    case job_type::DestroyBuilding:
                    case job_type::RemoveStairs:
                    case job_type::ConstructBuilding:
                        real_activity = JOB_CONSTRUCTION;
                        break;

                    case job_type::FellTree:
                    case job_type::CollectWebs:
                    case job_type::CollectSand:
                    case job_type::DrainAquarium:
                    case job_type::FillAquarium:
                    case job_type::FillPond:
                    case job_type::CollectClay:
                        real_activity = JOB_COLLECT;
                        break;

                    case job_type::TrainHuntingAnimal:
                    case job_type::TrainWarAnimal:
                    case job_type::CatchLiveLandAnimal:
                    case job_type::TameVermin:
                    case job_type::TameAnimal:
                    case job_type::ChainAnimal:
                    case job_type::UnchainAnimal:
                    case job_type::UnchainPet:
                    case job_type::ReleaseLargeCreature:
                    case job_type::ReleasePet:
                    case job_type::ReleaseSmallCreature:
                    case job_type::HandleSmallCreature:
                    case job_type::HandleLargeCreature:
                    case job_type::CageLargeCreature:
                    case job_type::CageSmallCreature:
                    case job_type::PitLargeAnimal:
                    case job_type::PitSmallAnimal:
                    case job_type::SlaughterAnimal:
                    case job_type::ShearCreature:
                    case job_type::PenLargeAnimal:
                    case job_type::PenSmallAnimal:
                    case job_type::TrainAnimal:
                        real_activity = JOB_ANIMALS;
                        break;

                    case job_type::PlantSeeds:
                    case job_type::HarvestPlants:
                    case job_type::FertilizeField:
                        real_activity = JOB_AGRICULTURE;
                        break;
                        
                    case job_type::ButcherAnimal:
                    case job_type::PrepareRawFish:
                    case job_type::MillPlants:
                    case job_type::MilkCreature:
                    case job_type::MakeCheese:
                    case job_type::PrepareMeal:
                    case job_type::ProcessPlants:
                    case job_type::BrewDrink:
                    case job_type::CollectHiveProducts:
                        real_activity = JOB_FOOD_PROD;
                        break;

                    case job_type::LoadCatapult:
                    case job_type::LoadBallista:
                    case job_type::FireCatapult:
                    case job_type::FireBallista:
                        real_activity = JOB_MILITARY;
                        break;

                    case job_type::LoadCageTrap:
                    case job_type::LoadStoneTrap:
                    case job_type::LoadWeaponTrap:
                    case job_type::CleanTrap:
                    case job_type::LinkBuildingToTrigger:
                    case job_type::PullLever:
                        real_activity = JOB_MECHANICAL;
                        break;

                    case job_type::RecoverWounded:
                    case job_type::DiagnosePatient:
                    case job_type::ImmobilizeBreak:
                    case job_type::DressWound:
                    case job_type::CleanPatient:
                    case job_type::Surgery:
                    case job_type::Suture:
                    case job_type::SetBone:
                    case job_type::PlaceInTraction:
                    case job_type::GiveWater:
                    case job_type::GiveFood:
                    case job_type::GiveWater2:
                    case job_type::GiveFood2:
                    case job_type::BringCrutch:
                    case job_type::ApplyCast:
                        real_activity = JOB_MEDICAL;
                        break;

                    case job_type::OperatePump:
                    case job_type::ManageWorkOrders:
                    case job_type::UpdateStockpileRecords:
                    case job_type::TradeAtDepot:
                        real_activity = JOB_PRODUCTIVE;
                        break;

                    default:
                        break;
                    }

                    addFortActivity(real_activity);
                    addCategoryActivity(real_activity, *entry);
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

        for (auto cat_it = category_breakdown.begin(); cat_it != category_breakdown.end(); cat_it++)
        {
            auto cat_total = fort_activity_totals[cat_it->first];
            for (auto val_it = cat_it->second.begin(); val_it != cat_it->second.end(); val_it++)
            {
                category_breakdown[cat_it->first][val_it->first] = getPercentage(val_it->second, cat_total);
            }
        }

        dwarf_activity_column.left_margin = fort_activity_column.fixWidth() + 2;
        fort_activity_column.filterDisplay();
        fort_activity_column.setHighlight(last_selected_index);
        populateDwarfColumn();
        populateCategoryBreakdownColumn();
    }

    void populateDwarfColumn()
    {
        dwarf_activity_column.clear();
        if (fort_activity_column.getDisplayListSize() > 0)
        {
            activity_type selected_activity = fort_activity_column.getFirstSelectedElem();
            auto dwarf_activities = &dwarf_activity_values[selected_activity];
            if (dwarf_activities)
            {
                vector<pair<df::unit *, size_t>> rev_vec(dwarf_activities->begin(), dwarf_activities->end());
                sort(rev_vec.begin(), rev_vec.end(), less_second<df::unit *, size_t>());

                for (auto it = rev_vec.begin(); it != rev_vec.end(); ++it)
                    dwarf_activity_column.add(getDwarfAverage(it->first, it->second), it->first);
            }
        }

        category_breakdown_column.left_margin = dwarf_activity_column.fixWidth() + 2;
        dwarf_activity_column.clearSearch();
        dwarf_activity_column.setHighlight(0);
    }

    void populateCategoryBreakdownColumn()
    {
        category_breakdown_column.clear();
        if (fort_activity_column.getDisplayListSize() == 0)
            return;

        auto selected_activity = fort_activity_column.getFirstSelectedElem();
        auto category_activities = &category_breakdown[selected_activity];
        if (category_activities)
        {
            vector<pair<activity_type, size_t>> rev_vec(category_activities->begin(), category_activities->end());
            sort(rev_vec.begin(), rev_vec.end(), less_second<activity_type, size_t>());

            for (auto it = rev_vec.begin(); it != rev_vec.end(); ++it)
                category_breakdown_column.add(getBreakdownAverage(it->first, it->second), it->first);
        }

        category_breakdown_column.fixWidth();
        category_breakdown_column.clearSearch();
        category_breakdown_column.setHighlight(0);
    }

    void addToFortAverageColumn(activity_type &type)
    {
        if (getFortActivityCount(type))
            fort_activity_column.add(getFortAverage(type), type);
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

    string getBreakdownAverage(activity_type activity, const size_t value)
    {
        auto label = getActivityLabel(activity);
        auto result = pad_string(int_to_string(value), 3) + " " + label;

        return result;
    }

    size_t getFortActivityCount(const activity_type activity)
    {
        if (fort_activity_totals.find(activity) == fort_activity_totals.end())
            return 0;

        return fort_activity_totals[activity];
    }

    void addFortActivity(const activity_type activity)
    {
        if (fort_activity_totals.find(activity) == fort_activity_totals.end())
            fort_activity_totals[activity] = 0;

        fort_activity_totals[activity]++;
    }

    void addCategoryActivity(const int category, const activity_type activity)
    {
        if (category_breakdown.find(category) == category_breakdown.end())
            category_breakdown[category] = map<activity_type, size_t>();

        if (category_breakdown[category].find(activity) == category_breakdown[category].end())
            category_breakdown[category][activity] = 0;

        category_breakdown[category][activity]++;
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
            {
                populateDwarfColumn();
                populateCategoryBreakdownColumn();
            }
            
            return;
        }

        if (input->count(interface_key::LEAVESCREEN))
        {
            input->clear();
            Screen::dismiss(this);
            return;
        }
        else if  (input->count(interface_key::SECONDSCROLL_PAGEDOWN))
        {
            window_days += min_window;
            if (window_days > max_history_days)
                window_days = min_window;

            populateFortColumn();
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
        category_breakdown_column.display(false);

        int32_t y = gps->dimy - 4;
        int32_t x = 2;
        OutputHotkeyString(x, y, "Leave", "Esc");

        x += 13;
        string window_label = "Window Months: " + int_to_string(window_days / min_window);
        OutputHotkeyString(x, y, window_label.c_str(), "*");

        ++y;
        x = 2;
        OutputHotkeyString(x, y, "Dwarf Stats", "Shift-D");

        x += 3;
        OutputHotkeyString(x, y, "Zoom Unit", "Shift-Z");
    }

    std::string getFocusString() { return "dwarfmonitor_fortstats"; }

private:
    ListColumn<activity_type> fort_activity_column, category_breakdown_column;
    ListColumn<df::unit *> dwarf_activity_column;
    int selected_column;

    map<activity_type, size_t> fort_activity_totals;
    map<int, map<activity_type, size_t>> category_breakdown;
    map<activity_type, map<df::unit *, size_t>> dwarf_activity_values;
    size_t fort_activity_count;
    size_t window_days;
    
    vector<activity_type> listed_activities;

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

        if (DFHack::Units::isDead(unit))
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
            else if (happy <= 75)   // fine
                misery[3]++;
            else if (happy <= 125)  // quite content
                misery[4]++;
            else if (happy <= 150)  // happy
                misery[5]++;
            else                    // ecstatic
                misery[6]++;
        }

        if (!monitor_jobs || is_paused)
            continue;

        if (unit->profession == profession::BABY ||
            unit->profession == profession::CHILD ||
            unit->profession == profession::DRUNK)
        {
            continue;
        }

        if (ENUM_ATTR(profession, military, unit->profession))
        {
            add_work_history(unit, JOB_MILITARY);
            continue;
        }

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
    
    bool is_paused = DFHack::World::ReadPauseState();
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

static color_value monitor_colors[] = 
{
    COLOR_LIGHTRED,
    COLOR_RED,
    COLOR_YELLOW,
    COLOR_WHITE,
    COLOR_CYAN,
    COLOR_LIGHTBLUE,
    COLOR_LIGHTGREEN
};

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
            string entries[7];
            size_t length = 9;
            for (int i = 0; i < 7; i++)
            {
                entries[i] = int_to_string(misery[i]);
                length += entries[i].length();
            }

            int x = gps->dimx - length;
            int y = gps->dimy - 1;
            OutputString(COLOR_WHITE, x, y, "H:");
            for (int i = 0; i < 7; i++)
            {
                OutputString(monitor_colors[i], x, y, entries[i]);
                if (i < 6)
                    OutputString(COLOR_WHITE, x, y, "/");
            }
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(dwarf_monitor_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(dwarf_monitor_hook, render);

DFHACK_PLUGIN("dwarfmonitor");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

static bool set_monitoring_mode(const string &mode, const bool &state)
{
    bool mode_recognized = false;

    if (!is_enabled)
        return false;

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

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (is_enabled != enable)
    {
        if (!INTERPOSE_HOOK(dwarf_monitor_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(dwarf_monitor_hook, render).apply(enable))
            return CR_FAILURE;

        reset();
        is_enabled = enable;
    }

    return CR_OK;
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
            if (!is_enabled)
                plugin_enable(out, true);

            if (set_monitoring_mode(mode, true))
            {
                out << "Monitoring enabled: " << mode << endl;
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
    activity_labels[JOB_IDLE]               = "Idle";
    activity_labels[JOB_MILITARY]           = "Military Duty";
    activity_labels[JOB_LEISURE]            = "Leisure";
    activity_labels[JOB_UNPRODUCTIVE]       = "Unproductive";
    activity_labels[JOB_DESIGNATE]          = "Mining";
    activity_labels[JOB_STORE_ITEM]         = "Store/Fetch Item";
    activity_labels[JOB_MANUFACTURE]        = "Manufacturing";
    activity_labels[JOB_DETAILING]          = "Detailing";
    activity_labels[JOB_HUNTING]            = "Hunting/Gathering";
    activity_labels[JOB_MEDICAL]            = "Medical";
    activity_labels[JOB_COLLECT]            = "Collect Materials";
    activity_labels[JOB_CONSTRUCTION]       = "Construction";
    activity_labels[JOB_AGRICULTURE]        = "Agriculture";
    activity_labels[JOB_FOOD_PROD]          = "Food/Drink Production";
    activity_labels[JOB_MECHANICAL]         = "Mechanics";
    activity_labels[JOB_ANIMALS]            = "Animal Handling";
    activity_labels[JOB_PRODUCTIVE]         = "Other Productive";
    
    commands.push_back(
        PluginCommand(
        "dwarfmonitor", "Records dwarf activity to measure fort efficiency",
        dwarfmonitor_cmd, false, 
        "dwarfmonitor enable <mode>\n"
        "  Start monitoring <mode>\n"
        "    <mode> can be \"work\", \"misery\", or \"all\"\n"
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
