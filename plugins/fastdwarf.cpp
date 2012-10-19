#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/MapCache.h"

#include "DataDefs.h"
#include "df/ui.h"
#include "df/world.h"
#include "df/unit.h"

using std::string;
using std::vector;
using namespace DFHack;

using df::global::world;
using df::global::ui;

// dfhack interface
DFHACK_PLUGIN("fastdwarf");

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

static bool enable_fastdwarf = false;
static bool enable_teledwarf = false;

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    // check run conditions
    if(!world || !world->map.block_index)
    {
        enable_fastdwarf = enable_teledwarf = false;
        return CR_OK;
    }
    int32_t race = ui->race_id;
    int32_t civ = ui->civ_id;
    
    if ( enable_fastdwarf ) {
        for (size_t i = 0; i < world->units.all.size(); i++)
        {
            df::unit *unit = world->units.all[i];
            
            if (unit->race == race && unit->civ_id == civ && unit->counters.job_counter > 0)
                unit->counters.job_counter = 0;
            // could also patch the unit->job.current_job->completion_timer
        }
    }
    if ( enable_teledwarf ) {
        MapExtras::MapCache *MCache = new MapExtras::MapCache();
        for (size_t i = 0; i < world->units.all.size(); i++)
        {
            df::unit *unit = world->units.all[i];
            
            if (unit->race != race || unit->civ_id != civ || unit->path.dest.x == -30000)
                continue;
            if (unit->relations.draggee_id != -1 || unit->relations.dragger_id != -1)
                continue;
            if (unit->relations.following != 0)
                continue;
            
            MapExtras::Block* block = MCache->BlockAtTile(unit->pos);
            df::coord2d pos(unit->pos.x % 16, unit->pos.y % 16);
            df::tile_occupancy occ = block->OccupancyAt(pos);
            occ.bits.unit = 0;
            occ.bits.unit_grounded = 0;
            block->setOccupancyAt(pos, occ);
            
            //move immediately to destination
            unit->pos.x = unit->path.dest.x;
            unit->pos.y = unit->path.dest.y;
            unit->pos.z = unit->path.dest.z;
        }
        MCache->WriteAll();
        delete MCache;
    }
    return CR_OK;
}

static command_result fastdwarf (color_ostream &out, vector <string> & parameters)
{
    if (parameters.size() == 1) {
        if ( parameters[0] == "0" ) {
            enable_fastdwarf = false;
            enable_teledwarf = false;
        } else if ( parameters[0] == "1" ) {
            enable_fastdwarf = true;
            enable_teledwarf = false;
        } else {
            out.print("Incorrect usage.\n");
            return CR_OK;
        }
    } else if (parameters.size() == 2) {
        if ( parameters[0] == "0" ) {
            enable_fastdwarf = false;
        } else if ( parameters[0] == "1" ) {
            enable_fastdwarf = true;
        } else {
            out.print("Incorrect usage.\n");
            return CR_OK;
        }
        if ( parameters[1] == "0" ) {
            enable_teledwarf = false;
        } else if ( parameters[1] == "1" ) {
            enable_teledwarf = true;
        } else {
            out.print("Incorrect usage.\n");
            return CR_OK;
        }
    } else if (parameters.size() == 0) {
        //print status
        out.print("Current state: fast = %d, teleport = %d.\n", enable_fastdwarf, enable_teledwarf);
    } else {
        out.print("Incorrect usage.\n");
        return CR_OK;
    }
    /*if (parameters.size() == 1 && (parameters[0] == "0" || parameters[0] == "1"))
    {
        if (parameters[0] == "0")
            enable_fastdwarf = 0;
        else
            enable_fastdwarf = 1;
        out.print("fastdwarf %sactivated.\n", (enable_fastdwarf ? "" : "de"));
    }
    else
    {
        out.print("Makes your minions move at ludicrous speeds.\n"
            "Activate with 'fastdwarf 1', deactivate with 'fastdwarf 0'.\n"
            "Current state: %d.\n", enable_fastdwarf);
    }*/

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("fastdwarf",
        "enable/disable fastdwarf and teledwarf (parameters=0/1)",
        fastdwarf, false,
        "fastdwarf: controls speedydwarf and teledwarf. Speedydwarf makes dwarves move quickly and perform tasks quickly. Teledwarf makes dwarves move instantaneously, but do jobs at the same speed.\n"
        "Usage:\n"
        "  fastdwarf 0 0: disable both speedydwarf and teledwarf\n"
        "  fastdwarf 0 1: disable speedydwarf, enable teledwarf\n"
        "  fastdwarf 1 0: enable speedydwarf, disable teledwarf\n"
        "  fastdwarf 1 1: enable speedydwarf, enable teledwarf\n"
        "  fastdwarf 0: disable speedydwarf, disable teledwarf\n"
        "  fastdwarf 1: enable speedydwarf, disable teledwarf\n"
        ));
    
    return CR_OK;
}
