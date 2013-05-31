

// some headers required for a plugin. Nothing special, just the basics.
#include "Core.h"
#include "Export.h"
#include "DataDefs.h"
#include "PluginManager.h"

#include "modules/EventManager.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"

#include "df/coord.h"
#include "df/global_objects.h"
#include "df/job.h"
#include "df/map_block.h"
#include "df/tile_dig_designation.h"
#include "df/world.h"

#include <map>
#include <vector>

using namespace DFHack;
using namespace std;

command_result digSmart (color_ostream &out, std::vector <std::string> & parameters);

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename - skeleton.plug.so or skeleton.plug.dll in this case
DFHACK_PLUGIN("digSmart");

void onDig(color_ostream& out, void* ptr);
void maybeExplore(color_ostream& out, MapExtras::MapCache& cache, df::coord pt, set<df::coord>& jobLocations);
EventManager::EventHandler digHandler(onDig, 0);
vector<df::coord> queue;
map<df::coord, int32_t> visitCount;

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


void onTick(color_ostream& out, void* ptr) {
/*
    MapExtras::MapCache cache;
    for ( size_t a = 0; a < queue.size(); a++ ) {
        df::coord pos = queue[a];
        for ( int16_t a = -1; a <= 1; a++ ) {
            for ( int16_t b = -1; b <= 1; b++ ) {
                maybeExplore(out, cache, df::coord(pos.x+a,pos.y+b,pos.z));
            }
        }
    }
    cache.trash();
    queue.clear();
*/
    
    map<df::coord,int> toRemove;
    for ( auto a = visitCount.begin(); a != visitCount.end(); a++ ) {
        if ( (*a).second <= 0 )
            continue;
        df::coord pt = (*a).first;
        df::map_block* block = Maps::getTileBlock(pt);
        if ( block->designation[pt.x&0xF][pt.y&0xF].bits.dig != df::enums::tile_dig_designation::Default ) {
            out.print("%d: %d,%d,%d: Default -> %d\n", __LINE__, pt.x,pt.y,pt.z, (int32_t)block->designation[pt.x&0xF][pt.y&0xF].bits.dig);
            toRemove[pt]++;
        }
    }
    for ( auto a = toRemove.begin(); a != toRemove.end(); a++ )
        visitCount.erase((*a).first);
    
    EventManager::EventHandler handler(onTick, 1);
    EventManager::registerTick(handler, 1, plugin_self);
}

void onDig(color_ostream& out, void* ptr) {
    CoreSuspender bob;
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
out.print("%d\n", __LINE__);
    
    set<df::coord> jobLocations;
    for ( df::job_list_link* link = &df::global::world->job_list; link != NULL; link = link->next ) {
        if ( link->item == NULL )
            continue;
        
        if ( link->item->job_type != df::enums::job_type::Dig &&
             link->item->job_type != df::enums::job_type::CarveUpwardStaircase && 
             link->item->job_type != df::enums::job_type::CarveDownwardStaircase && 
             link->item->job_type != df::enums::job_type::CarveUpDownStaircase && 
             link->item->job_type != df::enums::job_type::CarveRamp && 
             link->item->job_type != df::enums::job_type::DigChannel )
            continue;
        
        jobLocations.insert(link->item->pos);
    }
    
    //queue.push_back(job->pos);
    //EventManager::EventHandler handler(onTick, 1);
    //EventManager::registerTick(handler, 5, plugin_self);
    MapExtras::MapCache cache;
    df::coord pos = job->pos;
    for ( int16_t a = -1; a <= 1; a++ ) {
        for ( int16_t b = -1; b <= 1; b++ ) {
            maybeExplore(out, cache, df::coord(pos.x+a,pos.y+b,pos.z), jobLocations);
        }
    }
    cache.trash();
}

void maybeExplore(color_ostream& out, MapExtras::MapCache& cache, df::coord pt, set<df::coord>& jobLocations) {
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

//    if ( block->designation[pt.x&0xF][pt.y&0xF].bits.dig == df::enums::tile_dig_designation::Default )
//        return;
    if ( block->designation[pt.x&0xF][pt.y&0xF].bits.dig != df::enums::tile_dig_designation::No )
        return;
    
    uint32_t xMax,yMax,zMax;
    Maps::getSize(xMax,yMax,zMax);
    if ( pt.x == 0 || pt.y == 0 || pt.x+1 == xMax*16 || pt.y+1 == yMax*16 )
        return;
    if ( jobLocations.find(pt) != jobLocations.end() ) {
        return;
    }
    
    df::enums::tile_dig_designation::tile_dig_designation dig1,dig2;
    dig1 = block->designation[pt.x&0xF][pt.y&0xF].bits.dig;
    block->designation[pt.x&0xF][pt.y&0xF].bits.dig = df::enums::tile_dig_designation::Default;
    dig2 = block->designation[pt.x&0xF][pt.y&0xF].bits.dig;
    block->flags.bits.designated = true;
//    *df::global::process_dig  = true;
//    *df::global::process_jobs = true;
    
out.print("%d: %d,%d,%d, %d. %d -> %d\n", __LINE__, pt.x,pt.y,pt.z, visitCount[pt]++, dig1, dig2);
//out.print("%d: unk9 %d, unk13 %d\n", __LINE__, (int32_t)block->unk9[pt.x&0xF][pt.y&0xF], (int32_t)block->unk13[pt.x&0xF][pt.y&0xF]);
}

command_result digSmart (color_ostream &out, std::vector <std::string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    EventManager::EventHandler handler(onTick, 1);
    EventManager::registerTick(handler, 1, plugin_self);
    return CR_OK;
}
