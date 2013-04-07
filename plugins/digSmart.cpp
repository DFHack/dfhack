

// some headers required for a plugin. Nothing special, just the basics.
#include "Core.h"
#include "Export.h"
#include "DataDefs.h"
#include "PluginManager.h"

#include "modules/EventManager.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"

#include "df/coord.h"
#include "df/job.h"
#include "df/map_block.h"
#include "df/tile_dig_designation.h"
#include "df/world.h"

using namespace DFHack;

command_result digSmart (color_ostream &out, std::vector <std::string> & parameters);

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename - skeleton.plug.so or skeleton.plug.dll in this case
DFHACK_PLUGIN("digSmart");

void onDig(color_ostream& out, void* ptr);

EventManager::EventHandler digHandler(onDig, 5);

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "digSmart", "Automatically dig out veins as you discover them.",
        digSmart, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  This command does nothing at all.\n"
        "Example:\n"
        "  digSmart\n"
        "    Does nothing.\n"
    ));
    //digSmartPlugin = Core::getInstance().getPluginManager()->getPluginByName("digSmart");
    EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, digHandler, plugin_self);
    return CR_OK;
}

void maybeExplore(color_ostream& out, MapExtras::MapCache& cache, df::coord pt);

void onDig(color_ostream& out, void* ptr) {
    df::job* job = (df::job*)ptr;
    if ( job->completion_timer > 0 )
        return;

    if ( job->job_type != df::enums::job_type::Dig &&
         job->job_type != df::enums::job_type::CarveUpwardStaircase && 
         job->job_type != df::enums::job_type::CarveDownwardStaircase && 
         job->job_type != df::enums::job_type::CarveUpDownStaircase && 
         job->job_type != df::enums::job_type::CarveRamp && 
         job->job_type != df::enums::job_type::DigChannel )
        return;

    MapExtras::MapCache cache;
    df::coord pos = job->pos;
    for ( int16_t a = -1; a <= 1; a++ ) {
        for ( int16_t b = -1; b <= 1; b++ ) {
            maybeExplore(out, cache, df::coord(pos.x+a,pos.y+b,pos.z));
        }
    }
}

void maybeExplore(color_ostream& out, MapExtras::MapCache& cache, df::coord pt) {
    if ( !Maps::isValidTilePos(pt) ) {
        return;
    }
    
    df::map_block* block = Maps::getTileBlock(pt);
    if (!block)
        return;

    if ( block->designation[pt.x&0xF][pt.y&0xF].bits.hidden )
        return;

    df::tiletype type = block->tiletype[pt.x&0xF][pt.y&0xF];
    if ( ENUM_ATTR(tiletype, material, type) != df::enums::tiletype_material::MINERAL )
        return;
    if ( ENUM_ATTR(tiletype, shape, type) != df::enums::tiletype_shape::WALL )
        return;
    
    int16_t mat = cache.veinMaterialAt(pt);
    if ( mat == -1 )
        return;

    if ( block->designation[pt.x&0xF][pt.y&0xF].bits.dig == df::enums::tile_dig_designation::Default )
        return;
    if ( block->designation[pt.x&0xF][pt.y&0xF].bits.dig != df::enums::tile_dig_designation::No )
        return;
    
    uint32_t xMax,yMax,zMax;
    Maps::getSize(xMax,yMax,zMax);
    if ( pt.x == 0 || pt.y == 0 || pt.x+1 == xMax || pt.y+1 == yMax )
        return;
    
    block->designation[pt.x&0xF][pt.y&0xF].bits.dig = df::enums::tile_dig_designation::Default;
    block->flags.bits.designated = true;
}

command_result digSmart (color_ostream &out, std::vector <std::string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    return CR_OK;
}
