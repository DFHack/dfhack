/*
* Labor manager (formerly Autolabor 2) module for dfhack
*
* */


#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <vector>
#include <algorithm>
#include <queue>
#include <map>
#include <iterator>

#include "modules/Units.h"
#include "modules/World.h"
#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/Items.h"

// DF data structure definition headers
#include "DataDefs.h"
#include <MiscUtils.h>

#include <df/ui.h>
#include <df/world.h>
#include <df/unit.h>
#include <df/unit_relationship_type.h>
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
#include <df/tile_dig_designation.h>
#include <df/item_weaponst.h>
#include <df/itemdef_weaponst.h>
#include <df/general_ref_unit_workerst.h>
#include <df/general_ref_building_holderst.h>
#include <df/building_workshopst.h>
#include <df/building_furnacest.h>
#include <df/building_def.h>
#include <df/reaction.h>
#include <df/job_item.h>
#include <df/job_item_ref.h>
#include <df/unit_health_info.h>
#include <df/unit_health_flags.h>
#include <df/building_design.h>
#include <df/vehicle.h>
#include <df/units_other_id.h>
#include <df/ui.h>
#include <df/training_assignment.h>
#include <df/general_ref_contains_itemst.h>
#include <df/personality_facet_type.h>
#include <df/cultural_identity.h>
#include <df/ethic_type.h>
#include <df/value_type.h>

#include "labormanager.h"
#include "joblabormapper.h"
#include "LaborManagerClassic.h"
#include "AutoLaborManager.h"

using namespace std;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;
using df::global::ui;
using df::global::world;

DFHACK_PLUGIN_IS_ENABLED(enable_labormanager);


// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result labormanager(color_ostream& out, std::vector <std::string>& parameters);

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename - labormanager.plug.so or labormanager.plug.dll in this case
DFHACK_PLUGIN("labormanager");

static void generate_labor_to_skill_map();

static std::unique_ptr<AutoLaborManager> currentManager;

static bool initialized = false;

static void cleanup_state()
{
    if (currentManager)
    {
        currentManager->cleanup_state();
        currentManager.release();
    }

    enable_labormanager = false;
    initialized = false;
}

static void init_state()
{
    PersistentDataItem config = World::GetPersistentData("labormanager/unified/manager");

    if (config.isValid())
    {
        if (config.val() == "labormanager")
            currentManager = std::move(dts::make_unique<LaborManagerClassic>());
    }

    if (currentManager)
    {
        currentManager->init_state();
    }

    enable_labormanager = (bool)currentManager;
    initialized = true;
}


static void enable_manager(color_ostream& out, std::string manager_name)
{
    PersistentDataItem config = World::GetPersistentData("labormanager/unified/manager");

    if (!config.isValid())
    {
        config = World::AddPersistentData("labormanager/unified/manager");
        config.val() = "";
    }

    if (config.val() != manager_name)
    {
        if (currentManager)
        {
            out << "Disabling << " << config.val() << " plugin." << endl;
            currentManager->cleanup_state();
            currentManager.release();
        }
    }

    config.val() = manager_name;

    out << "Enabling " << manager_name << " manager." << endl;

    init_state();
}

static void disable_manager(color_ostream& out, std::string manager_name)
{
    PersistentDataItem config = World::GetPersistentData("labormanager/unified/manager");

    if (!config.isValid() || config.val() != manager_name) 
    {
        out << manager_name << " is not currently enabled." << endl;
        return;
    }

    out << "Disabling << " << config.val() << " manager." << endl;
    currentManager->cleanup_state();
    currentManager.release();
    config.val() = "";
}

static std::string current_manager_name()
{
    PersistentDataItem config = World::GetPersistentData("labormanager/unified/manager");

    if (config.isValid())
        return config.val();
    else
        return "";
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands)
{

    commands.push_back(PluginCommand(
        "labormanager", "Automatically manage dwarf labors.",
        labormanager, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  labormanager enable\n"
        "  labormanager disable\n"
        "    Enables or disables the plugin.\n"
        "  labormanager max <labor> <maximum>\n"
        "    Set max number of dwarves assigned to a labor.\n"
        "  labormanager max <labor> unmanaged\n"
        "  labormanager max <labor> disable\n"
        "    Don't attempt to manage this labor.\n"
        "    Any dwarves with unmanaged labors assigned will be less\n"
        "    likely to have managed labors assigned to them.\n"
        "  labormanager max <labor> none\n"
        "    Unrestrict the number of dwarves assigned to a labor.\n"
        "  labormanager priority <labor> <priority>\n"
        "    Change the assignment priority of a labor (default is 100)\n"
        "  labormanager reset <labor>\n"
        "    Return a labor to the default handling.\n"
        "  labormanager reset-all\n"
        "    Return all labors to the default handling.\n"
        "  labormanager list\n"
        "    List current status of all labors.\n"
        "  labormanager status\n"
        "    Show basic status information.\n"
        "Function:\n"
        "  When enabled, labormanager periodically checks your dwarves and enables or\n"
        "  disables labors.  Generally, each dwarf will be assigned exactly one labor.\n"
        "  Warning: labormanager will override any manual changes you make to labors\n"
        "  while it is enabled, except where the labor is marked as unmanaged.\n"
        "  Do not try to run both autolabor and labormanager at the same time.\n"
    ));

    // TODO: capture legacy enable states

    init_state();
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    cleanup_state();
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        cleanup_state();
        init_state();
        break;
    case SC_MAP_UNLOADED:
        cleanup_state();
        break;
    default:
        break;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    //    static int step_count = 0;
    // check run conditions
    if (!initialized || !world || !world->map.block_index || !enable_labormanager || !currentManager)
    {
        // give up if we shouldn't be running'
        return CR_OK;
    }

    //    if (++step_count < 60)
    //        return CR_OK;

    if (*df::global::process_jobs)
        return CR_OK;

    //    step_count = 0;

    currentManager->process(&out);

    return CR_OK;
}

DFhackCExport command_result labormanager_plugin_enable(color_ostream &out, bool enable)
{
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a fort first.\n");
        return CR_FAILURE;
    }

    if (enable)
    {
        enable_manager(out, "labormanager");
    }
    else if (!enable)
    {
        disable_manager(out, "labormanager");
    }

    return CR_OK;
}

command_result labormanager(color_ostream& out, std::vector <std::string>& parameters)
{
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a game first.\n");
        return CR_FAILURE;
    }

    if (parameters.size() == 1 &&
        (parameters[0] == "enable" || parameters[0] == "disable"))
    {
        bool enable = (parameters[0] == "enable");
        return labormanager_plugin_enable(out, enable);
    }

    if (parameters.size() == 0) {
        out.print("Automatically assigns labors to dwarves.\n"
            "Activate with 'labormanager enable', deactivate with 'labormanager disable'.\n"
            "Current state: %s.\n", enable_labormanager ? "enabled" : "disabled");

        return CR_OK;
    }

    if (currentManager && current_manager_name() == "labormanager" )
    {
        auto result = currentManager->do_command(out, parameters);
        return result;
    }
    else
    { 
        out.print("labormanager is not the current labor manager or is disabled, command aborted.\n");
        return CR_FAILURE;
    }
}

