#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <vector>
#include <algorithm>

#include "modules/Units.h"
#include "modules/World.h"

// DF data structure definition headers
#include "DataDefs.h"
#include <df/ui.h>
#include <df/world.h>
#include <df/unit.h>
#include <df/unit_soul.h>
#include <df/unit_labor.h>
#include <df/unit_skill.h>
#include <df/job.h>
#include <df/building.h>
#include <df/workshop_type.h>
#include <df/unit_misc_trait.h>
#include <df/entity_position_responsibility.h>
#include <df/historical_figure.h>
#include <df/historical_entity.h>
#include <df/histfig_entity_link.h>
#include <df/histfig_entity_link_positionst.h>
#include <df/entity_position_assignment.h>
#include <df/entity_position.h>
#include <df/building_tradedepotst.h>
#include <df/building_stockpilest.h>
#include <df/items_other_id.h>
#include <df/ui.h>
#include <df/activity_info.h>

#include <MiscUtils.h>

#include "modules/MapCache.h"
#include "modules/Items.h"
#include "modules/Units.h"

#include "laborstatemap.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("autohauler");
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(world);

#define ARRAY_COUNT(array) (sizeof(array)/sizeof((array)[0]))

/*
 * Autohauler module for dfhack
 * Fork of autolabor, DFHack version 0.40.24-r2
 *
 * Rather than the all-of-the-above means of autolabor, autohauler will instead
 * only manage hauling labors and leave skilled labors entirely to the user, who
 * will probably use Dwarf Therapist to do so.
 * Idle dwarves will be assigned the hauling labors; everyone else (including
 * those currently hauling) will have the hauling labors removed. This is to
 * encourage every dwarf to do their assigned skilled labors whenever possible,
 * but resort to hauling when those jobs are not available. This also implies
 * that the user will have a very tight skill assignment, with most skilled
 * labors only being assigned to just one dwarf, no dwarf having more than two
 * active skilled labors, and almost every non-military dwarf having at least
 * one skilled labor assigned.
 * Autohauler allows skills to be flagged as to prevent hauling labors from
 * being assigned when the skill is present. By default this is the unused
 * ALCHEMIST labor but can be changed by the user.
 * It is noteworthy that, as stated in autolabor.cpp, "for almost all labors,
 * once a dwarf begins a job it will finish that job even if the associated
 * labor is removed." This is why we can remove hauling labors by default to try
 * to force dwarves to do "real" jobs whenever they can.
 * This is a standalone plugin. However, it would be wise to delete
 * autolabor.plug.dll as this plugin is mutually exclusive with it.
 */

DFHACK_PLUGIN_IS_ENABLED(enable_autohauler);

static bool print_debug = false;

static std::vector<int> state_count(NUM_STATE);

const static int DEFAULT_FRAME_SKIP = 30;

static PersistentDataItem config;

command_result autohauler (color_ostream &out, std::vector <std::string> & parameters);

static int frame_skip;

static bool isOptionEnabled(unsigned flag)
{
    return config.isValid() && (config.ival(0) & flag) != 0;
}

enum ConfigFlags {
    CF_ENABLED = 1,
};

static void setOptionEnabled(ConfigFlags flag, bool on)
{
    if (!config.isValid())
        return;

    if (on)
        config.ival(0) |= flag;
    else
        config.ival(0) &= ~flag;
}

enum labor_mode {
    ALLOW,
    HAULERS,
    FORBID
};

struct labor_info
{
    PersistentDataItem config;

    int active_dwarfs;

    labor_mode mode() { return (labor_mode) config.ival(0); }

    void set_mode(labor_mode mode) { config.ival(0) = mode; }

    void set_config(PersistentDataItem a) { config = a; }

    };

struct labor_default
{
    labor_mode mode;
    int active_dwarfs;
};

static std::vector<struct labor_info> labor_infos;

static const struct labor_default default_labor_infos[] = {
    /* MINE */                  {ALLOW,   0},
    /* HAUL_STONE */            {HAULERS, 0},
    /* HAUL_WOOD */             {HAULERS, 0},
    /* HAUL_BODY */             {HAULERS, 0},
    /* HAUL_FOOD */             {HAULERS, 0},
    /* HAUL_REFUSE */           {HAULERS, 0},
    /* HAUL_ITEM */             {HAULERS, 0},
    /* HAUL_FURNITURE */        {HAULERS, 0},
    /* HAUL_ANIMAL */           {HAULERS, 0},
    /* CLEAN */                 {HAULERS, 0},
    /* CUTWOOD */               {ALLOW,   0},
    /* CARPENTER */             {ALLOW,   0},
    /* DETAIL */                {ALLOW,   0},
    /* MASON */                 {ALLOW,   0},
    /* ARCHITECT */             {ALLOW,   0},
    /* ANIMALTRAIN */           {ALLOW,   0},
    /* ANIMALCARE */            {ALLOW,   0},
    /* DIAGNOSE */              {ALLOW,   0},
    /* SURGERY */               {ALLOW,   0},
    /* BONE_SETTING */          {ALLOW,   0},
    /* SUTURING */              {ALLOW,   0},
    /* DRESSING_WOUNDS */       {ALLOW,   0},
    /* FEED_WATER_CIVILIANS */  {HAULERS, 0}, // This could also be ALLOW
    /* RECOVER_WOUNDED */       {HAULERS, 0},
    /* BUTCHER */               {ALLOW,   0},
    /* TRAPPER */               {ALLOW,   0},
    /* DISSECT_VERMIN */        {ALLOW,   0},
    /* LEATHER */               {ALLOW,   0},
    /* TANNER */                {ALLOW,   0},
    /* BREWER */                {ALLOW,   0},
    /* ALCHEMIST */             {FORBID,  0},
    /* SOAP_MAKER */            {ALLOW,   0},
    /* WEAVER */                {ALLOW,   0},
    /* CLOTHESMAKER */          {ALLOW,   0},
    /* MILLER */                {ALLOW,   0},
    /* PROCESS_PLANT */         {ALLOW,   0},
    /* MAKE_CHEESE */           {ALLOW,   0},
    /* MILK */                  {ALLOW,   0},
    /* COOK */                  {ALLOW,   0},
    /* PLANT */                 {ALLOW,   0},
    /* HERBALIST */             {ALLOW,   0},
    /* FISH */                  {ALLOW,   0},
    /* CLEAN_FISH */            {ALLOW,   0},
    /* DISSECT_FISH */          {ALLOW,   0},
    /* HUNT */                  {ALLOW,   0},
    /* SMELT */                 {ALLOW,   0},
    /* FORGE_WEAPON */          {ALLOW,   0},
    /* FORGE_ARMOR */           {ALLOW,   0},
    /* FORGE_FURNITURE */       {ALLOW,   0},
    /* METAL_CRAFT */           {ALLOW,   0},
    /* CUT_GEM */               {ALLOW,   0},
    /* ENCRUST_GEM */           {ALLOW,   0},
    /* WOOD_CRAFT */            {ALLOW,   0},
    /* STONE_CRAFT */           {ALLOW,   0},
    /* BONE_CARVE */            {ALLOW,   0},
    /* GLASSMAKER */            {ALLOW,   0},
    /* EXTRACT_STRAND */        {ALLOW,   0},
    /* SIEGECRAFT */            {ALLOW,   0},
    /* SIEGEOPERATE */          {ALLOW,   0},
    /* BOWYER */                {ALLOW,   0},
    /* MECHANIC */              {ALLOW,   0},
    /* POTASH_MAKING */         {ALLOW,   0},
    /* LYE_MAKING */            {ALLOW,   0},
    /* DYER */                  {ALLOW,   0},
    /* BURN_WOOD */             {ALLOW,   0},
    /* OPERATE_PUMP */          {ALLOW,   0},
    /* SHEARER */               {ALLOW,   0},
    /* SPINNER */               {ALLOW,   0},
    /* POTTERY */               {ALLOW,   0},
    /* GLAZING */               {ALLOW,   0},
    /* PRESSING */              {ALLOW,   0},
    /* BEEKEEPING */            {ALLOW,   0},
    /* WAX_WORKING */           {ALLOW,   0},
    /* HANDLE_VEHICLES */       {HAULERS, 0},
    /* HAUL_TRADE */            {HAULERS, 0},
    /* PULL_LEVER */            {HAULERS, 0},
    /* REMOVE_CONSTRUCTION */   {HAULERS, 0},
    /* HAUL_WATER */            {HAULERS, 0},
    /* GELD */                  {ALLOW,   0},
    /* BUILD_ROAD */            {HAULERS, 0},
    /* BUILD_CONSTRUCTION */    {HAULERS, 0},
    /* PAPERMAKING */           {ALLOW,   0},
    /* BOOKBINDING */           {ALLOW,   0}
};

struct dwarf_info_t
{
    dwarf_state state;

    bool haul_exempt;
};

static void cleanup_state()
{
    enable_autohauler = false;
    labor_infos.clear();
}

static void reset_labor(df::unit_labor labor)
{
    labor_infos[labor].set_mode(default_labor_infos[labor].mode);
}

static void enable_alchemist(color_ostream &out)
{
    if (!Units::setLaborValidity(unit_labor::ALCHEMIST, true))
    {
        // informational only; this is a non-fatal error
        out.printerr("%s: Could not flag Alchemist as a valid skill; Alchemist will not"
                     " be settable from DF or DFHack labor management screens.\n", plugin_name);
    }
}

static void init_state(color_ostream &out)
{
    config = World::GetPersistentData("autohauler/config");

    if (config.isValid() && config.ival(0) == -1)
        config.ival(0) = 0;

    enable_autohauler = isOptionEnabled(CF_ENABLED);

    if (!enable_autohauler)
        return;

    auto cfg_frameskip = World::GetPersistentData("autohauler/frameskip");
    if (cfg_frameskip.isValid())
    {
        frame_skip = cfg_frameskip.ival(0);
    }
    else
    {
        cfg_frameskip = World::AddPersistentData("autohauler/frameskip");
        cfg_frameskip.ival(0) = DEFAULT_FRAME_SKIP;
        frame_skip = cfg_frameskip.ival(0);
    }
    labor_infos.resize(ARRAY_COUNT(default_labor_infos));

    std::vector<PersistentDataItem> items;
    World::GetPersistentData(&items, "autohauler/labors/", true);


    for (auto& p : items)
    {
        std::string key = p.key();
        df::unit_labor labor = (df::unit_labor) atoi(key.substr(strlen("autohauler/labors/")).c_str());
        if (labor >= 0 && size_t(labor) < labor_infos.size())
        {
            labor_infos[labor].set_config(p);
            labor_infos[labor].active_dwarfs = 0;
        }
    }

    // Add default labors for those not in save
    for (size_t i = 0; i < ARRAY_COUNT(default_labor_infos); i++) {
        if (labor_infos[i].config.isValid())
            continue;

        std::stringstream name;
        name << "autohauler/labors/" << i;

        labor_infos[i].set_config(World::AddPersistentData(name.str()));

        labor_infos[i].active_dwarfs = 0;
        reset_labor((df::unit_labor) i);
    }

    enable_alchemist(out);
}

static void enable_plugin(color_ostream &out)
{
    if (!config.isValid())
    {
        config = World::AddPersistentData("autohauler/config");
        config.ival(0) = 0;
    }

    setOptionEnabled(CF_ENABLED, true);
    enable_autohauler = true;
    out << "Enabling the plugin." << std::endl;

    cleanup_state();
    init_state(out);
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    if(ARRAY_COUNT(default_labor_infos) != ENUM_LAST_ITEM(unit_labor) + 1)
    {
        out.printerr("autohauler: labor size mismatch\n");
        return CR_FAILURE;
    }

    commands.push_back(PluginCommand(
        "autohauler",
        "Automatically manage hauling labors.",
        autohauler));

    init_state(out);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    cleanup_state();

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        cleanup_state();
        init_state(out);
        break;
    case SC_MAP_UNLOADED:
        cleanup_state();
        break;
    default:
        break;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    static int step_count = 0;
    if(!world || !world->map.block_index || !enable_autohauler)
    {
        return CR_OK;
    }

    if (++step_count < frame_skip)
        return CR_OK;
    step_count = 0;

    std::vector<df::unit *> dwarfs;

    for (auto& cre : world->units.active)
    {
        if (Units::isCitizen(cre))
        {
            dwarfs.push_back(cre);
        }
    }

    int n_dwarfs = dwarfs.size();

    if (n_dwarfs == 0)
        return CR_OK;

    std::vector<dwarf_info_t> dwarf_info(n_dwarfs);

    state_count.clear();
    state_count.resize(NUM_STATE);

    for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
    {
        /* Before determining how to handle employment status, handle
         * hauling exemptions first */

        // Default deny condition of on break for later else-if series
        bool is_migrant = false;

        // Scan every labor. If a labor that disallows hauling is present
        // for the dwarf, the dwarf is hauling exempt
        FOR_ENUM_ITEMS(unit_labor, labor)
        {
            if (!(labor == unit_labor::NONE))
            {
                bool test1 = labor_infos[labor].mode() == FORBID;
                bool test2 = dwarfs[dwarf]->status.labors[labor];

                if(test1 && test2) dwarf_info[dwarf].haul_exempt = true;
            }
        }

        // Scan a dwarf's miscellaneous traits for on break or migrant status.
        // If either of these are present, disable hauling because we want them
        // to try to find real jobs first
        auto v = dwarfs[dwarf]->status.misc_traits;
        auto test_migrant = [](df::unit_misc_trait* t) { return t->id == misc_trait_type::Migrant; };
        is_migrant = std::find_if(v.begin(), v.end(), test_migrant ) != v.end();

        /* Now determine a dwarf's employment status and decide whether
         * to assign hauling */

        // I don't think you can set the labors for babies and children, but let's
        // ignore them anyway
        if (Units::isBaby(dwarfs[dwarf]) || Units::isChild(dwarfs[dwarf]))
        {
            dwarf_info[dwarf].state = CHILD;
        }
        // Account for any hauling exemptions here
        else if (dwarf_info[dwarf].haul_exempt)
        {
            dwarf_info[dwarf].state = BUSY;
        }
        // Account for the military
        else if (ENUM_ATTR(profession, military, dwarfs[dwarf]->profession))
            dwarf_info[dwarf].state = MILITARY;
        // Account for incoming migrants
        else if (is_migrant)
        {
            dwarf_info[dwarf].state = OTHER;
        }
        else if (dwarfs[dwarf]->job.current_job == NULL)
        {
            dwarf_info[dwarf].state = IDLE;
        }
        else
        {
            int job = dwarfs[dwarf]->job.current_job->job_type;
            if (job >= 0 && size_t(job) < ARRAY_COUNT(dwarf_states))
                dwarf_info[dwarf].state = dwarf_states[job];
            else
            {
                out.print("Dwarf %i \"%s\" has unknown job %i\n", dwarf, dwarfs[dwarf]->name.first_name.c_str(), job);
                dwarf_info[dwarf].state = BUSY;
            }
        }

        // Debug: Output dwarf job and state data
        if(print_debug)
            out.print("Dwarf %i %s State: %i\n", dwarf, dwarfs[dwarf]->name.first_name.c_str(),
                      dwarf_info[dwarf].state);

        // Increment corresponding labor in default_labor_infos struct
        state_count[dwarf_info[dwarf].state]++;

    }

    // At this point the debug if present has been completed
    print_debug = false;

    // This is a vector of all the labors
    std::vector<df::unit_labor> labors;

    // For every labor...
    FOR_ENUM_ITEMS(unit_labor, labor)
    {
        // Ignore all nonexistent labors
        if (labor == unit_labor::NONE)
            continue;

        // Set number of active dwarves for this job to zero
        labor_infos[labor].active_dwarfs = 0;

        // And add the labor to the aforementioned vector of labors
        labors.push_back(labor);
    }

    // This is a different algorithm than Autolabor. Instead, the intent is to
    // have "real" jobs filled first, then if nothing is available the dwarf
    // instead resorts to hauling.

    // IDLE     - Enable hauling
    // BUSY     - Disable hauling
    // OTHER    - Enable hauling
    // MILITARY - Enable hauling

    // There was no reason to put potential haulers in an array. All of them are
    // covered in the following for loop.

    FOR_ENUM_ITEMS(unit_labor, labor)
    {
        if (labor == unit_labor::NONE)
            continue;
        if (labor_infos[labor].mode() != HAULERS)
            continue;

        for(size_t dwarf = 0; dwarf < dwarfs.size(); dwarf++)
        {
            if (!Units::isValidLabor(dwarfs[dwarf], labor))
                continue;

            // Set hauling labors based on employment states
            if(dwarf_info[dwarf].state == IDLE) {
                dwarfs[dwarf]->status.labors[labor] = true;
            }
            else if(dwarf_info[dwarf].state == MILITARY) {
                dwarfs[dwarf]->status.labors[labor] = true;
            }
            else if(dwarf_info[dwarf].state == OTHER) {
                dwarfs[dwarf]->status.labors[labor] = true;
            }
            else if(dwarf_info[dwarf].state == BUSY) {
                dwarfs[dwarf]->status.labors[labor] = false;
            }
            // If at the end of this the dwarf has the hauling labor, increment the
            // counter
            if(dwarfs[dwarf]->status.labors[labor])
            {
                labor_infos[labor].active_dwarfs++;
            }
            // CHILD ignored
        }
    }
    return CR_OK;
}

void print_labor (df::unit_labor labor, color_ostream &out)
{
    std::string labor_name = ENUM_KEY_STR(unit_labor, labor);
    out << labor_name << ": ";
    for (int i = 0; i < 20 - (int)labor_name.length(); i++)
        out << ' ';
    if (labor_infos[labor].mode() == ALLOW) out << "allow" << std::endl;
    else if(labor_infos[labor].mode() == FORBID) out << "forbid" << std::endl;
    else if(labor_infos[labor].mode() == HAULERS)
    {
        out << "haulers, currently " << labor_infos[labor].active_dwarfs << " dwarfs" << std::endl;
    }
    else
    {
        out << "Warning: Invalid labor mode!" << std::endl;
    }
}

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable )
{
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a game first.\n");
        return CR_FAILURE;
    }

    if (enable && !enable_autohauler)
    {
        enable_plugin(out);
    }
    else if(!enable && enable_autohauler)
    {
        enable_autohauler = false;
        setOptionEnabled(CF_ENABLED, false);

        out << "Autohauler is disabled." << std::endl;
    }

    return CR_OK;
}

command_result autohauler (color_ostream &out, std::vector <std::string> & parameters)
{
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a game first.\n");
        return CR_FAILURE;
    }

    if (parameters.size() == 1 &&
        (parameters[0] == "0" || parameters[0] == "enable" ||
         parameters[0] == "1" || parameters[0] == "disable"))
    {
        bool enable = (parameters[0] == "1" || parameters[0] == "enable");

        return plugin_enable(out, enable);
    }
    else if (parameters.size() == 2 && parameters[0] == "frameskip")
    {
        auto cfg_frameskip = World::GetPersistentData("autohauler/frameskip");
        if(cfg_frameskip.isValid())
        {
            int newValue = atoi(parameters[1].c_str());
            cfg_frameskip.ival(0) = newValue;
            out << "Setting frame skip to " << newValue << std::endl;
            frame_skip = cfg_frameskip.ival(0);
            return CR_OK;
        }
        else
        {
            out << "Warning! No persistent data for frame skip!" << std::endl;
            return CR_OK;
        }
    }
    else if (parameters.size() >= 2 && parameters.size() <= 4)
    {
        if (!enable_autohauler)
        {
            out << "Error: The plugin is not enabled." << std::endl;
            return CR_FAILURE;
        }

        df::unit_labor labor = unit_labor::NONE;

        FOR_ENUM_ITEMS(unit_labor, test_labor)
        {
            if (parameters[0] == ENUM_KEY_STR(unit_labor, test_labor))
                labor = test_labor;
        }

        if (labor == unit_labor::NONE)
        {
            out.printerr("Could not find labor %s.\n", parameters[0].c_str());
            return CR_WRONG_USAGE;
        }

        if (parameters[1] == "haulers")
        {
            labor_infos[labor].set_mode(HAULERS);
            print_labor(labor, out);
            return CR_OK;
        }
        if (parameters[1] == "allow")
        {
            labor_infos[labor].set_mode(ALLOW);
            print_labor(labor, out);
            return CR_OK;
        }
        if (parameters[1] == "forbid")
        {
            labor_infos[labor].set_mode(FORBID);
            print_labor(labor, out);
            return CR_OK;
        }
        if (parameters[1] == "reset")
        {
            reset_labor(labor);
            print_labor(labor, out);
            return CR_OK;
        }

        print_labor(labor, out);

        return CR_OK;
    }
    else if (parameters.size() == 1 && parameters[0] == "reset-all")
    {
        if (!enable_autohauler)
        {
            out << "Error: The plugin is not enabled." << std::endl;
            return CR_FAILURE;
        }

        for (size_t i = 0; i < labor_infos.size(); i++)
        {
            reset_labor((df::unit_labor) i);
        }
        out << "All labors reset." << std::endl;
        return CR_OK;
    }
    else if (parameters.size() == 1 && (parameters[0] == "list" || parameters[0] == "status"))
    {
        if (!enable_autohauler)
        {
            out << "Error: The plugin is not enabled." << std::endl;
            return CR_FAILURE;
        }

        bool need_comma = false;
        for (int i = 0; i < NUM_STATE; i++)
        {
            if (state_count[i] == 0)
                continue;
            if (need_comma)
                out << ", ";
            out << state_count[i] << ' ' << state_names[i];
            need_comma = true;
        }
        out << std::endl;

        out << "Autohauler is running every " << frame_skip << " frames." << std::endl;

        if (parameters[0] == "list")
        {
            FOR_ENUM_ITEMS(unit_labor, labor)
            {
                if (labor == unit_labor::NONE)
                    continue;

                print_labor(labor, out);
            }
        }

        return CR_OK;
    }
    else if (parameters.size() == 1 && parameters[0] == "debug")
    {
        if (!enable_autohauler)
        {
            out << "Error: The plugin is not enabled." << std::endl;
            return CR_FAILURE;
        }

        print_debug = true;

        return CR_OK;
    }
    else
    {
        out.print("Automatically assigns hauling labors to dwarves.\n"
            "Activate with 'enable autohauler', deactivate with 'disable autohauler'.\n"
            "Current state: %d.\n", enable_autohauler);

        return CR_OK;
    }
}
